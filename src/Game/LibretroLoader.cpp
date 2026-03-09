#include "Game/LibretroLoader.hpp"

#include <cstring>
#include <cstdio>
#include <algorithm>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
// PSVita / Nintendo Switch have no POSIX dynamic linker.
// All dynXxx helpers return nullptr / no-op so LibretroLoader::load() fails
// gracefully at runtime.
#else
#  include <dlfcn.h>
#endif

namespace beiklive {

// ---- Static instance pointer ----------------------------------------
LibretroLoader* LibretroLoader::s_current = nullptr;

// ============================================================
// Dynamic library helpers
// ============================================================

static void* dynOpen(const std::string& path)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(LoadLibraryA(path.c_str()));
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
    (void)path;
    return nullptr; // dynamic loading not supported on PSVita / Switch
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

static void dynLoadError()
{
#if defined(_WIN32)
    DWORD err = GetLastError();
    char msg[256] = {};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, 0, msg, sizeof(msg) - 1, nullptr);
    fprintf(stderr, "[LibretroLoader] LoadLibrary failed (%lu): %s\n", err, msg);
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
    fprintf(stderr, "[LibretroLoader] dynamic loading not supported on this platform\n");
#else
    fprintf(stderr, "[LibretroLoader] dlopen failed: %s\n", dlerror());
#endif
}

static void dynClose(void* handle)
{
    if (!handle) return;
#if defined(_WIN32)
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
    (void)handle; // no-op on PSVita / Switch
#else
    dlclose(handle);
#endif
}

static void* dynSym(void* handle, const char* name)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(
        GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
    (void)handle; (void)name;
    return nullptr; // no-op on PSVita / Switch
#else
    return dlsym(handle, name);
#endif
}

// ============================================================
// Symbol resolution helper
// ============================================================

template<typename T>
bool LibretroLoader::resolveSymbol(T& fnPtr, const char* name)
{
    fnPtr = reinterpret_cast<T>(dynSym(m_handle, name));
    if (!fnPtr) {
        return false;
    }
    return true;
}

// ============================================================
// load / unload
// ============================================================

bool LibretroLoader::load(const std::string& libPath)
{
    unload();

#ifdef __SWITCH__
    // On Switch, mgba_libretro.a is statically linked into the binary.
    // The retro_* symbols are resolved at link time – no dlopen needed.
    // libretro.h (included via LibretroLoader.hpp) already provides the
    // extern "C" declarations for all entry points.
    fn_set_environment        = retro_set_environment;
    fn_set_video_refresh      = retro_set_video_refresh;
    fn_set_audio_sample       = retro_set_audio_sample;
    fn_set_audio_sample_batch = retro_set_audio_sample_batch;
    fn_set_input_poll         = retro_set_input_poll;
    fn_set_input_state        = retro_set_input_state;
    fn_init                   = retro_init;
    fn_deinit                 = retro_deinit;
    fn_api_version            = retro_api_version;
    fn_get_system_info        = retro_get_system_info;
    fn_get_system_av_info     = retro_get_system_av_info;
    fn_set_controller_port_device = retro_set_controller_port_device;
    fn_reset                  = retro_reset;
    fn_run                    = retro_run;
    fn_serialize_size         = retro_serialize_size;
    fn_serialize              = retro_serialize;
    fn_unserialize            = retro_unserialize;
    fn_load_game              = retro_load_game;
    fn_unload_game            = retro_unload_game;
    m_handle = reinterpret_cast<void*>(1); // sentinel: symbols are bound
#else
    m_handle = dynOpen(libPath);
    if (!m_handle) {
        dynLoadError();
        return false;
    }

    bool ok = true;
    ok &= resolveSymbol(fn_set_environment,         "retro_set_environment");
    ok &= resolveSymbol(fn_set_video_refresh,       "retro_set_video_refresh");
    ok &= resolveSymbol(fn_set_audio_sample,        "retro_set_audio_sample");
    ok &= resolveSymbol(fn_set_audio_sample_batch,  "retro_set_audio_sample_batch");
    ok &= resolveSymbol(fn_set_input_poll,          "retro_set_input_poll");
    ok &= resolveSymbol(fn_set_input_state,         "retro_set_input_state");
    ok &= resolveSymbol(fn_init,                    "retro_init");
    ok &= resolveSymbol(fn_deinit,                  "retro_deinit");
    ok &= resolveSymbol(fn_api_version,             "retro_api_version");
    ok &= resolveSymbol(fn_get_system_info,         "retro_get_system_info");
    ok &= resolveSymbol(fn_get_system_av_info,      "retro_get_system_av_info");
    ok &= resolveSymbol(fn_set_controller_port_device, "retro_set_controller_port_device");
    ok &= resolveSymbol(fn_reset,                   "retro_reset");
    ok &= resolveSymbol(fn_run,                     "retro_run");
    ok &= resolveSymbol(fn_serialize_size,          "retro_serialize_size");
    ok &= resolveSymbol(fn_serialize,               "retro_serialize");
    ok &= resolveSymbol(fn_unserialize,             "retro_unserialize");
    ok &= resolveSymbol(fn_load_game,               "retro_load_game");
    ok &= resolveSymbol(fn_unload_game,             "retro_unload_game");

    if (!ok) {
        dynClose(m_handle);
        m_handle = nullptr;
        return false;
    }
#endif

    // Register static callbacks (must happen before retro_init)
    s_current = this;
    fn_set_environment        (s_environmentCallback);
    fn_set_video_refresh      (s_videoRefreshCallback);
    fn_set_audio_sample       (s_audioSampleCallback);
    fn_set_audio_sample_batch (s_audioSampleBatchCallback);
    fn_set_input_poll         (s_inputPollCallback);
    fn_set_input_state        (s_inputStateCallback);

    return true;
}

void LibretroLoader::unload()
{
    if (m_gameLoaded && fn_unload_game) {
        fn_unload_game();
        m_gameLoaded = false;
    }
    if (m_coreReady && fn_deinit) {
        fn_deinit();
        m_coreReady = false;
    }
    if (m_handle) {
        dynClose(m_handle);
        m_handle = nullptr;
    }
    if (s_current == this) {
        s_current = nullptr;
    }
    // Reset function pointers
    fn_set_environment         = nullptr;
    fn_set_video_refresh       = nullptr;
    fn_set_audio_sample        = nullptr;
    fn_set_audio_sample_batch  = nullptr;
    fn_set_input_poll          = nullptr;
    fn_set_input_state         = nullptr;
    fn_init                    = nullptr;
    fn_deinit                  = nullptr;
    fn_api_version             = nullptr;
    fn_get_system_info         = nullptr;
    fn_get_system_av_info      = nullptr;
    fn_set_controller_port_device = nullptr;
    fn_reset                   = nullptr;
    fn_run                     = nullptr;
    fn_serialize_size          = nullptr;
    fn_serialize               = nullptr;
    fn_unserialize             = nullptr;
    fn_load_game               = nullptr;
    fn_unload_game             = nullptr;
}

// ============================================================
// Core lifecycle
// ============================================================

bool LibretroLoader::initCore()
{
    if (!m_handle || m_coreReady) return m_coreReady;
    fn_init();
    m_coreReady = true;
    return true;
}

void LibretroLoader::deinitCore()
{
    if (!m_coreReady) return;
    fn_deinit();
    m_coreReady = false;
}

unsigned LibretroLoader::apiVersion() const
{
    return m_handle ? fn_api_version() : 0;
}

void LibretroLoader::getSystemInfo(retro_system_info* info) const
{
    if (m_handle) fn_get_system_info(info);
}

void LibretroLoader::getSystemAvInfo(retro_system_av_info* info) const
{
    if (m_handle) fn_get_system_av_info(info);
}

bool LibretroLoader::loadGame(const std::string& romPath)
{
    if (!m_coreReady) return false;

    retro_game_info info{};
    info.path = romPath.c_str();
    info.data = nullptr;
    info.size = 0;
    info.meta = nullptr;

    if (!fn_load_game(&info)) return false;

    fn_get_system_av_info(&m_avInfo);
    m_gameLoaded = true;
    return true;
}

void LibretroLoader::unloadGame()
{
    if (!m_gameLoaded) return;
    fn_unload_game();
    m_gameLoaded = false;
}

void LibretroLoader::run()
{
    if (m_gameLoaded) fn_run();
}

void LibretroLoader::reset()
{
    if (m_gameLoaded && fn_reset) fn_reset();
}

size_t LibretroLoader::serializeSize() const
{
    return (m_gameLoaded && fn_serialize_size) ? fn_serialize_size() : 0;
}

bool LibretroLoader::serialize(void* data, size_t size) const
{
    return (m_gameLoaded && fn_serialize) ? fn_serialize(data, size) : false;
}

bool LibretroLoader::unserialize(const void* data, size_t size)
{
    return (m_gameLoaded && fn_unserialize) ? fn_unserialize(data, size) : false;
}

// ============================================================
// Video / Audio accessors
// ============================================================

LibretroLoader::VideoFrame LibretroLoader::getVideoFrame() const
{
    std::lock_guard<std::mutex> lk(m_videoMutex);
    return m_videoFrame;
}

bool LibretroLoader::drainAudio(std::vector<int16_t>& out)
{
    std::lock_guard<std::mutex> lk(m_audioMutex);
    if (m_audioBuffer.empty()) return false;
    out.swap(m_audioBuffer);
    m_audioBuffer.clear();
    return true;
}

// ============================================================
// Input
// ============================================================

void LibretroLoader::setButtonState(unsigned id, bool pressed)
{
    if (id <= RETRO_DEVICE_ID_JOYPAD_R3)
        m_buttons[id] = pressed;
}

bool LibretroLoader::getButtonState(unsigned id) const
{
    return (id <= RETRO_DEVICE_ID_JOYPAD_R3) ? m_buttons[id] : false;
}

// ============================================================
// Static callbacks (libretro C interface)
// ============================================================

bool LibretroLoader::s_environmentCallback(unsigned cmd, void* data)
{
    if (!s_current) return false;

    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
            bool* b = static_cast<bool*>(data);
            if (b) *b = true;
            return true;
        }
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            const retro_pixel_format* fmt = static_cast<const retro_pixel_format*>(data);
            // Accept XRGB8888 and RGB565; reject others
            return (*fmt == RETRO_PIXEL_FORMAT_XRGB8888 ||
                    *fmt == RETRO_PIXEL_FORMAT_RGB565);
        }
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH: {
            // Return current directory
            const char** dir = static_cast<const char**>(data);
            if (dir) *dir = ".";
            return true;
        }
        case RETRO_ENVIRONMENT_SET_MESSAGE: {
            const retro_message* msg = static_cast<const retro_message*>(data);
            if (msg && msg->msg) {
                fprintf(stdout, "[Core] %s\n", msg->msg);
            }
            return true;
        }
        case RETRO_ENVIRONMENT_SHUTDOWN:
            return true;
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            retro_variable* var = static_cast<retro_variable*>(data);
            if (var) var->value = nullptr;
            return false;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
            bool* b = static_cast<bool*>(data);
            if (b) *b = false;
            return true;
        }
        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        case RETRO_ENVIRONMENT_SET_VARIABLES:
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
        case RETRO_ENVIRONMENT_SET_ROTATION:
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE:
        case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE:
        case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
            return false;
        default:
            return false;
    }
}

void LibretroLoader::s_videoRefreshCallback(const void* data,
                                             unsigned width, unsigned height,
                                             size_t pitch)
{
    if (!s_current || !data) return;

    std::lock_guard<std::mutex> lk(s_current->m_videoMutex);
    auto& vf       = s_current->m_videoFrame;
    vf.width       = width;
    vf.height      = height;
    vf.pitch       = pitch;
    vf.pixels.resize(width * height);

    const uint8_t* src = static_cast<const uint8_t*>(data);
    for (unsigned row = 0; row < height; ++row) {
        const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(src + row * pitch);
        uint32_t*       dstRow = vf.pixels.data() + row * width;
        std::copy(srcRow, srcRow + width, dstRow);
    }
}

void LibretroLoader::s_audioSampleCallback(int16_t left, int16_t right)
{
    if (!s_current) return;
    std::lock_guard<std::mutex> lk(s_current->m_audioMutex);
    s_current->m_audioBuffer.push_back(left);
    s_current->m_audioBuffer.push_back(right);
}

size_t LibretroLoader::s_audioSampleBatchCallback(const int16_t* data, size_t frames)
{
    if (!s_current || !data) return frames;
    std::lock_guard<std::mutex> lk(s_current->m_audioMutex);
    auto& buf = s_current->m_audioBuffer;
    const size_t samples = frames * 2; // stereo
    buf.insert(buf.end(), data, data + samples);
    return frames;
}

void LibretroLoader::s_inputPollCallback()
{
    // Input state is updated from the main thread before retro_run();
    // nothing to do here.
}

int16_t LibretroLoader::s_inputStateCallback(unsigned port, unsigned device,
                                               unsigned /*index*/, unsigned id)
{
    if (!s_current || port != 0) return 0;
    if (device != RETRO_DEVICE_JOYPAD && device != RETRO_DEVICE_ANALOG) return 0;
    if (id > RETRO_DEVICE_ID_JOYPAD_R3) return 0;
    return s_current->m_buttons[id] ? 1 : 0;
}

} // namespace beiklive
