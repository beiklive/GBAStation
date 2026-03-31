#include "LibretroLoader.hpp"

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <algorithm>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
// PSVita / Nintendo Switch 不支持 POSIX 动态链接器。
// dynXxx 函数均返回 nullptr 或空操作，load() 会在运行时优雅失败。
#else
#  include <dlfcn.h>
#endif

// ---- 像素格式辅助函数 -------------------------------------------

/// 构造 RGBA8888 像素（小端 uint32：字节序 [R,G,B,A]）。
static inline uint32_t makeRGBA8888(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint32_t>(r)
         | (static_cast<uint32_t>(g) << 8)
         | (static_cast<uint32_t>(b) << 16)
         | 0xFF000000u;
}

// ---- Libretro 日志接口回调 --------------------------------

/// 将核心日志输出到 stderr，便于查看时钟/RTC 错误。
static void RETRO_CALLCONV s_coreLogCallback(enum retro_log_level level,
                                              const char* fmt, ...)
{
    static const char* const levelStr[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    const char* tag = (level >= RETRO_LOG_DEBUG && level <= RETRO_LOG_ERROR)
                      ? levelStr[level] : "?";
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[Core/%s] ", tag);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

// ---- Libretro 性能接口回调 -----------------------
// 通过 RETRO_ENVIRONMENT_GET_PERF_INTERFACE 提供给核心。
// get_time_usec 是核心用于 RTC 和计时的主时钟；
// counter/register/start/stop 回调支持可选的性能分析。

/// 返回自 Unix 纪元以来的当前墙钟时间（微秒）。
static retro_time_t RETRO_CALLCONV s_perfGetTimeUsec(void)
{
#if defined(_WIN32)
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    // FILETIME 以 100 纳秒为单位，起点为 1601-01-01。
    // 换算为自 Unix 纪元（1970-01-01）起的微秒数。
    // 两个纪元相差 134774 天 × 86400 秒 = 11644473600 秒。
    static const uint64_t k_fileTimeToUnixEpochSeconds = 11644473600ULL;
    uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return (retro_time_t)(t / 10LL - (int64_t)(k_fileTimeToUnixEpochSeconds * 1000000ULL));
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (retro_time_t)ts.tv_sec * 1000000LL + (retro_time_t)(ts.tv_nsec / 1000LL);
#endif
}

/// 返回高精度单调计数器的当前 tick 值。
static retro_perf_tick_t RETRO_CALLCONV s_perfGetCounter(void)
{
#if defined(_WIN32)
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (retro_perf_tick_t)li.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (retro_perf_tick_t)ts.tv_sec * 1000000000ULL
         + (retro_perf_tick_t)ts.tv_nsec;
#endif
}

/// 将性能计数器标记为已注册。
static void RETRO_CALLCONV s_perfRegister(struct retro_perf_counter* counter)
{
    if (counter) counter->registered = true;
}

/// 记录起始 tick 并递增调用次数。
static void RETRO_CALLCONV s_perfStart(struct retro_perf_counter* counter)
{
    if (counter) {
        counter->start = s_perfGetCounter();
        ++counter->call_cnt;
    }
}

/// 将已用 tick 累加到计数器总量。
static void RETRO_CALLCONV s_perfStop(struct retro_perf_counter* counter)
{
    if (counter) {
        counter->total += s_perfGetCounter() - counter->start;
    }
}

/// 空操作日志：宿主不打印核心性能数据。
static void RETRO_CALLCONV s_perfLog(void) {}

/// 返回 0——宿主不向核心通告任何 CPU 特性。
static uint64_t RETRO_CALLCONV s_perfGetCpuFeatures(void) { return 0; }

namespace beiklive {

// ---- 静态实例指针 ----------------------------------------
LibretroLoader* LibretroLoader::s_current = nullptr;

// ============================================================
// 动态库辅助函数
// ============================================================

static void* dynOpen(const std::string& path)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(LoadLibraryA(path.c_str()));
#elif defined(__PSV__) || defined(__psp2__) || defined(__SWITCH__)
    (void)path;
    return nullptr; // PSVita / Switch 不支持动态加载
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
    (void)handle; // PSVita / Switch 上为空操作
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
    return nullptr; // PSVita / Switch 上为空操作
#else
    return dlsym(handle, name);
#endif
}

// ============================================================
// 符号解析辅助函数
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
// 加载 / 卸载
// ============================================================

bool LibretroLoader::load(const std::string& libPath)
{
    unload();

#ifdef __SWITCH__
    // Switch 上 mgba_libretro.a 静态链接到二进制文件中，
    // retro_* 符号在链接期解析，无需 dlopen。
    // libretro.h（通过 LibretroLoader.hpp 包含）已声明所有入口点。
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
    fn_cheat_reset            = retro_cheat_reset;
    fn_cheat_set              = retro_cheat_set;
    fn_get_memory_data        = retro_get_memory_data;
    fn_get_memory_size        = retro_get_memory_size;
    m_handle = reinterpret_cast<void*>(1); // 哨兵值：符号已绑定
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
    // 可选符号：缺失时不报错
    resolveSymbol(fn_cheat_reset,               "retro_cheat_reset");
    resolveSymbol(fn_cheat_set,                 "retro_cheat_set");
    resolveSymbol(fn_get_memory_data,           "retro_get_memory_data");
    resolveSymbol(fn_get_memory_size,           "retro_get_memory_size");

    if (!ok) {
        dynClose(m_handle);
        m_handle = nullptr;
        return false;
    }
#endif

    // 注册静态回调（须在 retro_init 前调用）
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
    // 清空函数指针
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
    fn_cheat_reset             = nullptr;
    fn_cheat_set               = nullptr;
    fn_get_memory_data         = nullptr;
    fn_get_memory_size         = nullptr;
}

// ============================================================
// 核心生命周期
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
// 视频 / 音频访问器
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
// 输入
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
// 内存（SRAM）
// ============================================================

void* LibretroLoader::getMemoryData(unsigned id) const
{
    if (!m_gameLoaded || !fn_get_memory_data) return nullptr;
    return fn_get_memory_data(id);
}

size_t LibretroLoader::getMemorySize(unsigned id) const
{
    if (!m_gameLoaded || !fn_get_memory_size) return 0;
    return fn_get_memory_size(id);
}

// ============================================================
// 金手指
// ============================================================

void LibretroLoader::cheatReset()
{
    if (m_gameLoaded && fn_cheat_reset) fn_cheat_reset();
}

void LibretroLoader::cheatSet(unsigned index, bool enabled, const std::string& code)
{
    if (m_gameLoaded && fn_cheat_set) fn_cheat_set(index, enabled, code.c_str());
}

// ============================================================
// 静态回调（libretro C 接口）
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
            if (*fmt == RETRO_PIXEL_FORMAT_XRGB8888 ||
                *fmt == RETRO_PIXEL_FORMAT_RGB565) {
                s_current->m_pixelFormat = *fmt;
                return true;
            }
            return false;
        }
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
            const char** dir = static_cast<const char**>(data);
            if (dir) {
                *dir = s_current->m_systemDirectory.empty()
                     ? "." : s_current->m_systemDirectory.c_str();
            }
            return true;
        }
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
            const char** dir = static_cast<const char**>(data);
            if (dir) {
                *dir = s_current->m_saveDirectory.empty()
                     ? "." : s_current->m_saveDirectory.c_str();
            }
            return true;
        }
        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH: {
            // 返回当前目录
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
        // ---- 核心选项版本：返回 0 以使用旧版 SET_VARIABLES ----
        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
            unsigned* ver = static_cast<unsigned*>(data);
            if (ver) *ver = 0;
            return true;
        }
        // ---- 核心声明变量及默认值 ----------------------
        case RETRO_ENVIRONMENT_SET_VARIABLES: {
            const retro_variable* vars = static_cast<const retro_variable*>(data);
            if (!vars || !s_current->m_configManager) return false;
            beiklive::ConfigManager* cfg = s_current->m_configManager;
            for (const retro_variable* v = vars; v->key; ++v) {
                if (!v->value) continue;
                // 格式："描述文本; 默认值|选项2|选项3..."
                // "; " 后的第一个选项为默认值。
                std::string valStr(v->value);
                size_t semiPos = valStr.find("; ");
                if (semiPos == std::string::npos) continue;
                std::string optsPart = valStr.substr(semiPos + 2);
                size_t pipePos = optsPart.find('|');
                std::string defaultVal = (pipePos != std::string::npos)
                    ? optsPart.substr(0, pipePos) : optsPart;
                if (defaultVal.empty()) continue;
                std::string cfgKey = std::string("core.") + v->key;
                cfg->SetDefault(cfgKey, defaultVal);
            }
            cfg->Save();
            return true;
        }
        // ---- 核心查询变量值 ---------------------------------
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            retro_variable* var = static_cast<retro_variable*>(data);
            if (!var || !var->key) return false;
            var->value = nullptr;
            if (!s_current->m_configManager) return false;
            std::string cfgKey = std::string("core.") + var->key;
            auto entry = s_current->m_configManager->Get(cfgKey);
            if (!entry) return false;
            auto str = entry->AsString();
            if (!str) return false;
            // 存入持久 map，确保 c_str() 指针始终有效。
            auto& stored = s_current->m_coreVarStorage[var->key];
            stored = *str;
            var->value = stored.c_str();
            return true;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
            bool* b = static_cast<bool*>(data);
            if (b) *b = false;
            return true;
        }
        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
        case RETRO_ENVIRONMENT_SET_ROTATION:
            return false;
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
            bool* ff = static_cast<bool*>(data);
            if (ff) *ff = s_current->m_fastForwarding.load(std::memory_order_relaxed);
            return true;
        }
        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
            retro_perf_callback* cb = static_cast<retro_perf_callback*>(data);
            if (cb) {
                cb->get_time_usec    = s_perfGetTimeUsec;
                cb->get_cpu_features = s_perfGetCpuFeatures;
                cb->get_perf_counter = s_perfGetCounter;
                cb->perf_register    = s_perfRegister;
                cb->perf_start       = s_perfStart;
                cb->perf_stop        = s_perfStop;
                cb->perf_log         = s_perfLog;
            }
            return true;
        }
        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE:
        case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE:
        case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
            return false;
        // case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
        //     retro_log_callback* log = static_cast<retro_log_callback*>(data);
        //     if (log) log->log = s_coreLogCallback;
        //     return true;
        // }
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
    vf.pixels.resize(width * height);

    const uint8_t* src = static_cast<const uint8_t*>(data);

    if (s_current->m_pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888) {
        // 源每行字节数为 pitch（可能宽于 width*4）
        // 将 XRGB8888 [B,G,R,X] 转换为 RGBA8888 [R,G,B,0xFF]
        for (unsigned row = 0; row < height; ++row) {
            const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(src + row * pitch);
            uint32_t*       dstRow = vf.pixels.data() + row * width;
            for (unsigned col = 0; col < width; ++col) {
                uint32_t px = srcRow[col]; // 小端：byte0=B, byte1=G, byte2=R, byte3=X
                dstRow[col] = makeRGBA8888(
                    static_cast<uint8_t>((px >> 16) & 0xFF),
                    static_cast<uint8_t>((px >>  8) & 0xFF),
                    static_cast<uint8_t>( px        & 0xFF));
            }
        }
    } else if (s_current->m_pixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
        // RGB565：16 位像素，用位移近似扩展为 RGBA8888
        // 位布局：RRRRR_GGGGGG_BBBBB（位 15-11=R，10-5=G，4-0=B）
        for (unsigned row = 0; row < height; ++row) {
            const uint16_t* srcRow = reinterpret_cast<const uint16_t*>(src + row * pitch);
            uint32_t*       dstRow = vf.pixels.data() + row * width;
            for (unsigned col = 0; col < width; ++col) {
                uint16_t px = srcRow[col];
                uint8_t r5 = (px >> 11) & 0x1F;
                uint8_t g6 = (px >>  5) & 0x3F;
                uint8_t b5 =  px        & 0x1F;
                // 5 位扩展为 8 位：(v << 3) | (v >> 2)
                // 6 位扩展为 8 位：(v << 2) | (v >> 4)
                dstRow[col] = makeRGBA8888(
                    static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
                    static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
                    static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
            }
        }
    } else {
        // RETRO_PIXEL_FORMAT_0RGB1555（libretro 默认格式）：
        // 16 位像素，位布局：0_RRRRR_GGGGG_BBBBB（位15=0，14-10=R，9-5=G，4-0=B）
        for (unsigned row = 0; row < height; ++row) {
            const uint16_t* srcRow = reinterpret_cast<const uint16_t*>(src + row * pitch);
            uint32_t*       dstRow = vf.pixels.data() + row * width;
            for (unsigned col = 0; col < width; ++col) {
                uint16_t px = srcRow[col];
                uint8_t r5 = (px >> 10) & 0x1F;  // 位 14-10
                uint8_t g5 = (px >>  5) & 0x1F;  // 位 9-5
                uint8_t b5 =  px        & 0x1F;  // 位 4-0
                // 5 位扩展为 8 位：(v << 3) | (v >> 2)
                dstRow[col] = makeRGBA8888(
                    static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
                    static_cast<uint8_t>((g5 << 3) | (g5 >> 2)),
                    static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
            }
        }
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
    const size_t samples = frames * 2; // 立体声
    buf.insert(buf.end(), data, data + samples);
    return frames;
}

void LibretroLoader::s_inputPollCallback()
{
    // 输入状态由主线程在 retro_run() 前更新，此处无需处理。
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
