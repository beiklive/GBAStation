#include "three_ds_stub/ThreeDSRuntime.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "audio_core/sink_details.h"
#include "common/file_util.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/frontend/applets/default_applets.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/image_interface.h"
#include "core/frontend/input.h"
#include "core/hle/service/service.h"
#include "core/loader/loader.h"
#include "three_ds_stub/ThreeDSInput.hpp"
#include "three_ds_stub/ThreeDSLog.hpp"
#include "three_ds_stub/ThreeDSRenderer.hpp"
#include "video_core/gpu.h"
#include "video_core/renderer_software/renderer_software.h"

namespace beiklive::three_ds_stub {
namespace {

constexpr unsigned kOutputWidth = 1280;
constexpr unsigned kOutputHeight = 720;
constexpr auto kSlowRunLoopThreshold = std::chrono::milliseconds{500};

Common::Log::Level AzaharLogLevel(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return Common::Log::Level::Trace;
    case LogLevel::Debug:
        return Common::Log::Level::Debug;
    case LogLevel::Info:
        return Common::Log::Level::Info;
    case LogLevel::Warning:
        return Common::Log::Level::Warning;
    case LogLevel::Error:
        return Common::Log::Level::Error;
    case LogLevel::Critical:
        return Common::Log::Level::Critical;
    case LogLevel::Off:
        return Common::Log::Level::Count;
    }
    return Common::Log::Level::Info;
}

std::string ResultMessage(Core::System::ResultStatus status, const std::string& details) {
    std::string message;
    switch (status) {
    case Core::System::ResultStatus::Success:
        return {};
    case Core::System::ResultStatus::ErrorGetLoader:
        message = "No loader for this ROM";
        break;
    case Core::System::ResultStatus::ErrorLoader:
        message = "ROM loading failed";
        break;
    case Core::System::ResultStatus::ErrorLoader_ErrorEncrypted:
        message = "Encrypted 3DS ROMs are not supported";
        break;
    case Core::System::ResultStatus::ErrorLoader_ErrorInvalidFormat:
        message = "Unsupported 3DS ROM format";
        break;
    case Core::System::ResultStatus::ErrorSystemFiles:
        message = "Required 3DS system files are missing";
        break;
    case Core::System::ResultStatus::ShutdownRequested:
        message = "The emulated system requested shutdown";
        break;
    default:
        message = "Azahar error " + std::to_string(static_cast<unsigned>(status));
        break;
    }
    if (!details.empty()) {
        message += ": " + details;
    }
    return message;
}

void CreateDataDirectories() {
    const std::array paths = {
        "sdmc:/GBAStation/3ds",
        "sdmc:/GBAStation/3ds/games",
        "sdmc:/GBAStation/3ds/saves",
        "sdmc:/GBAStation/3ds/states",
        "sdmc:/GBAStation/3ds/cache",
        "sdmc:/GBAStation/3ds/log",
    };
    for (const char* path : paths) {
        std::error_code error;
        std::filesystem::create_directories(path, error);
        if (error) {
            logMessage(LogLevel::Warning,
                       "GBAStation3DSStub: create directory failed path=%s error=%d message=%s",
                       path, error.value(), error.message().c_str());
        }
    }
}

class ThreeDSEmuWindow final : public Frontend::EmuWindow {
public:
    ThreeDSEmuWindow(ThreeDSRenderer& renderer, ThreeDSInput& input)
        : renderer_(renderer), input_(input) {
        window_info.type = Frontend::WindowSystemType::Headless;
        Layout::FramebufferLayout layout{};
        layout.width = kOutputWidth;
        layout.height = kOutputHeight;
        layout.top_screen_enabled = true;
        layout.bottom_screen_enabled = true;
        layout.top_screen = {240, 0, 1040, 480};
        layout.bottom_screen = {480, 480, 800, 720};
        layout.is_rotated = false;
        NotifyFramebufferLayoutChanged(layout);
    }

    void PollEvents() override {
        ++poll_count_;
        if (poll_count_ == 1) {
            logMessage(LogLevel::Debug, "GBAStation3DSStub: first PollEvents entry");
        }
        input_.Poll();
        const ThreeDSTouchState touch = input_.TouchState();
        if (touch.pressed) {
            if (touch_active_) {
                TouchMoved(touch.x, touch.y);
            } else {
                touch_active_ = TouchPressed(touch.x, touch.y);
            }
        } else if (touch_active_) {
            TouchReleased();
            touch_active_ = false;
        }

        auto& system = Core::System::GetInstance();
        if (system.IsPoweredOn()) {
            auto& software =
                static_cast<SwRenderer::RendererSoftware&>(system.GPU().Renderer());
            const auto& top = software.Screen(VideoCore::ScreenId::TopLeft);
            const auto& bottom = software.Screen(VideoCore::ScreenId::Bottom);
            if (!screen_info_logged_) {
                logMessage(LogLevel::Info,
                           "GBAStation3DSStub: first screen buffers top=%ux%u bytes=%zu bottom=%ux%u bytes=%zu",
                           top.width, top.height, top.pixels.size(), bottom.width, bottom.height,
                           bottom.pixels.size());
                screen_info_logged_ = true;
            }
            renderer_.Present(top, bottom);
        }
        frame_ready_ = true;
    }

    void BeginFrame() {
        frame_ready_ = false;
    }

    [[nodiscard]] bool FrameReady() const {
        return frame_ready_;
    }

    [[nodiscard]] std::uint64_t PollCount() const {
        return poll_count_;
    }

private:
    ThreeDSRenderer& renderer_;
    ThreeDSInput& input_;
    bool frame_ready_ = false;
    bool touch_active_ = false;
    bool screen_info_logged_ = false;
    std::uint64_t poll_count_ = 0;
};

} // namespace

class ThreeDSRuntime::Impl {
public:
    ThreeDSRenderer renderer;
    ThreeDSInput input;
    std::unique_ptr<ThreeDSEmuWindow> window;
    std::string last_error;
    bool initialized = false;
    bool loaded = false;
    bool azahar_logging_started = false;
    bool first_run_loop_logged = false;
    std::uint64_t run_loop_count = 0;
    std::uint64_t frame_count = 0;
};

ThreeDSRuntime::ThreeDSRuntime() : impl_(std::make_unique<Impl>()) {}

ThreeDSRuntime::~ThreeDSRuntime() {
    Shutdown();
}

bool ThreeDSRuntime::Init() {
    if (impl_->initialized) {
        return true;
    }

    logMessage(LogLevel::Info, "GBAStation3DSStub: runtime init begin");
    logMessage(LogLevel::Debug, "GBAStation3DSStub: creating data directories");
    CreateDataDirectories();
    logMessage(LogLevel::Debug, "GBAStation3DSStub: configuring Azahar user path");
    FileUtil::SetUserPath("sdmc:/GBAStation/3ds/");
    Common::Log::Initialize("azahar.log");
    Common::Log::Start();
    impl_->azahar_logging_started = true;
    const auto azahar_log_level = AzaharLogLevel(currentLogLevel());
    Common::Log::SetGlobalFilter(Common::Log::Filter{azahar_log_level});
    logMessage(LogLevel::Info, "GBAStation3DSStub: Azahar logging started file=azahar.log level=%s",
               logLevelName(currentLogLevel()));

    logMessage(LogLevel::Debug, "GBAStation3DSStub: configuring service modules");
    std::size_t service_count = 0;
    for (const auto& service_module : Service::service_module_map) {
        Settings::values.lle_modules.emplace(service_module.name, false);
        ++service_count;
    }
    logMessage(LogLevel::Debug, "GBAStation3DSStub: service modules configured count=%zu",
               service_count);

    auto& system = Core::System::GetInstance();
    logMessage(LogLevel::Debug, "GBAStation3DSStub: registering default applets");
    Frontend::RegisterDefaultApplets(system);
    logMessage(LogLevel::Debug, "GBAStation3DSStub: registering image interface");
    system.RegisterImageInterface(std::make_shared<Frontend::ImageInterface>());

    logMessage(LogLevel::Debug, "GBAStation3DSStub: input init begin");
    if (!impl_->input.Init()) {
        impl_->last_error = "Switch input initialization failed";
        logMessage(LogLevel::Error, "GBAStation3DSStub: %s", impl_->last_error.c_str());
        return false;
    }
    logMessage(LogLevel::Info, "GBAStation3DSStub: input init complete");

    logMessage(LogLevel::Debug, "GBAStation3DSStub: framebuffer init begin");
    if (!impl_->renderer.Init()) {
        impl_->last_error = "Switch framebuffer initialization failed";
        logMessage(LogLevel::Error, "GBAStation3DSStub: %s", impl_->last_error.c_str());
        return false;
    }
    logMessage(LogLevel::Info, "GBAStation3DSStub: framebuffer init complete");

    impl_->window = std::make_unique<ThreeDSEmuWindow>(impl_->renderer, impl_->input);
    impl_->initialized = true;
    logMessage(LogLevel::Info, "GBAStation3DSStub: runtime init complete output=%ux%u",
               kOutputWidth, kOutputHeight);
    return true;
}

bool ThreeDSRuntime::LoadGame(const std::string& path) {
    if ((!impl_->initialized && !Init()) || path.empty()) {
        impl_->last_error = path.empty() ? "No 3DS ROM path was provided" : impl_->last_error;
        return false;
    }

    Settings::values.graphics_api = Settings::GraphicsAPI::Software;
    Settings::values.use_cpu_jit = true;
    // The ARM64 Pica shader JIT corrupts host state on Switch after entering real 3D draws.
    // Keep the CPU Dynarmic JIT enabled, but use the safe shader interpreter for now.
    Settings::values.use_shader_jit = false;
    Settings::values.use_hw_shader = false;
    Settings::values.use_disk_shader_cache = false;
    Settings::values.async_shader_compilation = false;
    Settings::values.async_presentation = false;
    Settings::values.resolution_factor = 1;
    Settings::values.frame_limit = 100.0;
    Settings::values.audio_emulation = Settings::AudioEmulation::HLE;
    Settings::values.enable_audio_stretching = false;
    Settings::values.output_type = AudioCore::SinkType::Null;
    Settings::values.output_device = "Nintendo Switch audout";
    Settings::values.input_type = AudioCore::InputType::Null;
    Settings::values.use_virtual_sd = true;
    Settings::values.plugin_loader_enabled = false;

    logMessage(LogLevel::Info,
               "GBAStation3DSStub: settings graphics=software cpu_jit=true shader_jit=false hw_shader=false disk_shader_cache=false async_shader=false async_present=false resolution=1 frame_limit=100 audio=HLE output=null virtual_sd=true plugins=false");

    auto& system = Core::System::GetInstance();
    logMessage(LogLevel::Info, "GBAStation3DSStub: system.Load begin path=%s", path.c_str());
    const auto load_start = std::chrono::steady_clock::now();
    const Core::System::ResultStatus result = system.Load(*impl_->window, path);
    const auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - load_start)
                             .count();
    logMessage(result == Core::System::ResultStatus::Success ? LogLevel::Info : LogLevel::Error,
               "GBAStation3DSStub: system.Load end status=%u elapsed_ms=%lld",
               static_cast<unsigned>(result), static_cast<long long>(load_ms));
    if (result != Core::System::ResultStatus::Success) {
        impl_->last_error = ResultMessage(result, system.GetStatusDetails());
        logMessage(LogLevel::Error, "GBAStation3DSStub: load failed status=%u detail=%s",
                   static_cast<unsigned>(result), impl_->last_error.c_str());
        impl_->renderer.PresentStatus("Unable to load game", impl_->last_error.c_str());
        return false;
    }

    std::uint64_t program_id = 0;
    system.GetAppLoader().ReadProgramId(program_id);
    system.GPU().ApplyPerProgramSettings(program_id);
    impl_->loaded = true;
    logMessage(LogLevel::Info, "GBAStation3DSStub: game loaded path=%s title_id=%016llx",
               path.c_str(), static_cast<unsigned long long>(program_id));
    return true;
}

bool ThreeDSRuntime::RunFrame() {
    if (!impl_->loaded || !impl_->window) {
        return false;
    }

    impl_->window->BeginFrame();
    auto& system = Core::System::GetInstance();
    std::uint64_t loops_without_poll = 0;
    while (!impl_->window->FrameReady() && !impl_->input.ExitRequested()) {
        const std::uint64_t poll_count_before = impl_->window->PollCount();
        if (!impl_->first_run_loop_logged) {
            logMessage(LogLevel::Info, "GBAStation3DSStub: first system.RunLoop begin");
        }
        const auto run_start = std::chrono::steady_clock::now();
        const Core::System::ResultStatus result = system.RunLoop();
        const auto run_elapsed = std::chrono::steady_clock::now() - run_start;
        ++impl_->run_loop_count;
        if (!impl_->first_run_loop_logged) {
            const auto elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(run_elapsed).count();
            logMessage(LogLevel::Info,
                       "GBAStation3DSStub: first system.RunLoop end status=%u elapsed_ms=%lld polls=%llu",
                       static_cast<unsigned>(result), static_cast<long long>(elapsed_ms),
                       static_cast<unsigned long long>(impl_->window->PollCount()));
            impl_->first_run_loop_logged = true;
        }
        if (run_elapsed >= kSlowRunLoopThreshold) {
            const auto elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(run_elapsed).count();
            logMessage(LogLevel::Warning,
                       "GBAStation3DSStub: slow system.RunLoop call=%llu elapsed_ms=%lld status=%u",
                       static_cast<unsigned long long>(impl_->run_loop_count),
                       static_cast<long long>(elapsed_ms), static_cast<unsigned>(result));
        }
        if (result != Core::System::ResultStatus::Success) {
            if (result == Core::System::ResultStatus::ShutdownRequested) {
                logMessage(LogLevel::Info,
                           "GBAStation3DSStub: emulated system requested shutdown");
                return false;
            }
            impl_->last_error = ResultMessage(result, system.GetStatusDetails());
            logMessage(LogLevel::Error, "GBAStation3DSStub: run failed status=%u detail=%s",
                       static_cast<unsigned>(result), impl_->last_error.c_str());
            impl_->renderer.PresentStatus("Azahar stopped", impl_->last_error.c_str());
            return false;
        }

        if (impl_->window->PollCount() == poll_count_before) {
            ++loops_without_poll;
            if (loops_without_poll == 1000 || loops_without_poll % 10000 == 0) {
                logMessage(LogLevel::Warning,
                           "GBAStation3DSStub: RunLoop progress without PollEvents count=%llu total_calls=%llu",
                           static_cast<unsigned long long>(loops_without_poll),
                           static_cast<unsigned long long>(impl_->run_loop_count));
            }
        } else {
            loops_without_poll = 0;
        }
    }

    if (impl_->window->FrameReady()) {
        ++impl_->frame_count;
        if (impl_->frame_count == 1 || impl_->frame_count % 60 == 0) {
            logMessage(LogLevel::Debug,
                       "GBAStation3DSStub: frame heartbeat frame=%llu run_loop_calls=%llu polls=%llu",
                       static_cast<unsigned long long>(impl_->frame_count),
                       static_cast<unsigned long long>(impl_->run_loop_count),
                       static_cast<unsigned long long>(impl_->window->PollCount()));
        }
    }
    return !impl_->input.ExitRequested();
}

void ThreeDSRuntime::Reset() {
    if (impl_->loaded) {
        Core::System::GetInstance().Reset();
    }
}

void ThreeDSRuntime::Shutdown() {
    if (!impl_) {
        return;
    }

    logMessage(LogLevel::Info,
               "GBAStation3DSStub: runtime shutdown begin initialized=%s loaded=%s frames=%llu run_loop_calls=%llu",
               impl_->initialized ? "true" : "false", impl_->loaded ? "true" : "false",
               static_cast<unsigned long long>(impl_->frame_count),
               static_cast<unsigned long long>(impl_->run_loop_count));
    auto& system = Core::System::GetInstance();
    if (system.IsPoweredOn()) {
        logMessage(LogLevel::Debug, "GBAStation3DSStub: system shutdown begin");
        system.Shutdown();
        logMessage(LogLevel::Debug, "GBAStation3DSStub: system shutdown complete");
    }
    impl_->loaded = false;
    impl_->window.reset();
    Input::UnregisterFactory<Input::TouchDevice>("emu_window");
    impl_->input.Shutdown();
    impl_->renderer.Shutdown();
    if (impl_->azahar_logging_started) {
        Common::Log::Stop();
        impl_->azahar_logging_started = false;
    }
    impl_->initialized = false;
    logMessage(LogLevel::Info, "GBAStation3DSStub: runtime shutdown complete");
}

bool ThreeDSRuntime::ExitRequested() const {
    return impl_->input.ExitRequested();
}

const std::string& ThreeDSRuntime::LastError() const {
    return impl_->last_error;
}

} // namespace beiklive::three_ds_stub
