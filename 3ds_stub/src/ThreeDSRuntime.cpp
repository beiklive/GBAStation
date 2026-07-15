#include "three_ds_stub/ThreeDSRuntime.hpp"

#include <array>
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
            renderer_.Present(software.Screen(VideoCore::ScreenId::TopLeft),
                              software.Screen(VideoCore::ScreenId::Bottom));
        }
        frame_ready_ = true;
    }

    void BeginFrame() {
        frame_ready_ = false;
    }

    [[nodiscard]] bool FrameReady() const {
        return frame_ready_;
    }

private:
    ThreeDSRenderer& renderer_;
    ThreeDSInput& input_;
    bool frame_ready_ = false;
    bool touch_active_ = false;
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
};

ThreeDSRuntime::ThreeDSRuntime() : impl_(std::make_unique<Impl>()) {}

ThreeDSRuntime::~ThreeDSRuntime() {
    Shutdown();
}

bool ThreeDSRuntime::Init() {
    if (impl_->initialized) {
        return true;
    }

    CreateDataDirectories();
    FileUtil::SetUserPath("sdmc:/GBAStation/3ds/");
    Common::Log::Initialize("sdmc:/GBAStation/3ds/log/azahar.log");
    Common::Log::Start();
    Common::Log::SetGlobalFilter(Common::Log::Filter{Common::Log::Level::Info});

    for (const auto& service_module : Service::service_module_map) {
        Settings::values.lle_modules.emplace(service_module.name, false);
    }

    auto& system = Core::System::GetInstance();
    Frontend::RegisterDefaultApplets(system);
    system.RegisterImageInterface(std::make_shared<Frontend::ImageInterface>());

    if (!impl_->input.Init() || !impl_->renderer.Init()) {
        impl_->last_error = "Switch input or framebuffer initialization failed";
        return false;
    }

    impl_->window = std::make_unique<ThreeDSEmuWindow>(impl_->renderer, impl_->input);
    impl_->initialized = true;
    return true;
}

bool ThreeDSRuntime::LoadGame(const std::string& path) {
    if ((!impl_->initialized && !Init()) || path.empty()) {
        impl_->last_error = path.empty() ? "No 3DS ROM path was provided" : impl_->last_error;
        return false;
    }

    Settings::values.graphics_api = Settings::GraphicsAPI::Software;
    Settings::values.use_cpu_jit = true;
    Settings::values.use_shader_jit = true;
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

    auto& system = Core::System::GetInstance();
    const Core::System::ResultStatus result = system.Load(*impl_->window, path);
    if (result != Core::System::ResultStatus::Success) {
        impl_->last_error = ResultMessage(result, system.GetStatusDetails());
        appendLog("GBAStation3DSStub: load failed status=%u detail=%s",
                  static_cast<unsigned>(result), impl_->last_error.c_str());
        impl_->renderer.PresentStatus("Unable to load game", impl_->last_error.c_str());
        return false;
    }

    std::uint64_t program_id = 0;
    system.GetAppLoader().ReadProgramId(program_id);
    system.GPU().ApplyPerProgramSettings(program_id);
    impl_->loaded = true;
    appendLog("GBAStation3DSStub: game loaded path=%s title_id=%016llx", path.c_str(),
              static_cast<unsigned long long>(program_id));
    return true;
}

bool ThreeDSRuntime::RunFrame() {
    if (!impl_->loaded || !impl_->window) {
        return false;
    }

    impl_->window->BeginFrame();
    auto& system = Core::System::GetInstance();
    while (!impl_->window->FrameReady() && !impl_->input.ExitRequested()) {
        const Core::System::ResultStatus result = system.RunLoop();
        if (result != Core::System::ResultStatus::Success) {
            if (result == Core::System::ResultStatus::ShutdownRequested) {
                return false;
            }
            impl_->last_error = ResultMessage(result, system.GetStatusDetails());
            appendLog("GBAStation3DSStub: run failed status=%u detail=%s",
                      static_cast<unsigned>(result), impl_->last_error.c_str());
            impl_->renderer.PresentStatus("Azahar stopped", impl_->last_error.c_str());
            return false;
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

    auto& system = Core::System::GetInstance();
    if (system.IsPoweredOn()) {
        system.Shutdown();
    }
    impl_->loaded = false;
    impl_->window.reset();
    Input::UnregisterFactory<Input::TouchDevice>("emu_window");
    impl_->input.Shutdown();
    impl_->renderer.Shutdown();
    if (impl_->initialized) {
        Common::Log::Stop();
    }
    impl_->initialized = false;
}

bool ThreeDSRuntime::ExitRequested() const {
    return impl_->input.ExitRequested();
}

const std::string& ThreeDSRuntime::LastError() const {
    return impl_->last_error;
}

} // namespace beiklive::three_ds_stub
