#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

// libretro public API types
#include "third_party/mgba/src/platform/libretro/libretro.h"
#include "core/ConfigManager.hpp"
namespace beiklive {

/// 封装单个已加载的libretro核心。
/// 处理动态库加载、回调注册和生命周期管理。
class LibretroLoader {
public:
    LibretroLoader()  = default;
    ~LibretroLoader() = default;

    LibretroLoader(const LibretroLoader&)            = delete;
    LibretroLoader& operator=(const LibretroLoader&) = delete;

    // ---- 生命周期 ---------------------------------------------------

    /// 加载 @a path 处的共享库并解析所有必需符号。
    /// @return 成功时返回true。
    bool load(const std::string& libPath);

    /// 卸载共享库。即使未调用load()也可安全调用。
    void unload();

    bool isLoaded() const { return m_handle != nullptr; }

    // ---- libretro API转发 ------------------------------------------

    bool        initCore();
    void        deinitCore();
    unsigned    apiVersion()  const;
    void        getSystemInfo(retro_system_info* info)   const;
    void        getSystemAvInfo(retro_system_av_info* info) const;
    bool        loadGame(const std::string& romPath);
    void        unloadGame();
    void        run();
    void        reset();
    size_t      serializeSize()               const;
    bool        serialize(void* data, size_t size)  const;
    bool        unserialize(const void* data, size_t size);

    // ---- 内存（SRAM/电池存档）--------------------------------------

    /// 返回核心内存区域指针（如RETRO_MEMORY_SAVE_RAM）。
    /// 仅在游戏加载期间有效；不支持时返回nullptr。
    void*  getMemoryData(unsigned id) const;

    /// 返回核心内存区域的字节大小。
    size_t getMemorySize(unsigned id) const;

    // ---- 金手指 -----------------------------------------------------

    /// 重置/清除所有已注册到核心的金手指。
    void cheatReset();

    /// 设置（添加或更新）单条金手指。
    /// @param index   金手指列表位置（从0开始）。
    /// @param enabled 金手指是否激活。
    /// @param code    金手指代码字符串（格式取决于核心，如GameShark）。
    void cheatSet(unsigned index, bool enabled, const std::string& code);

    // ---- 存档目录 ---------------------------------------------------

    /// 设置核心使用的存档目录。
    /// 在loadGame()之前调用，使RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY返回该路径。
    void setSaveDirectory(const std::string& dir) { m_saveDirectory = dir; }

    /// 设置核心的系统/BIOS目录。
    void setSystemDirectory(const std::string& dir) { m_systemDirectory = dir; }

    // ---- 视频帧 -----------------------------------------------------

    struct VideoFrame {
        std::vector<uint32_t> pixels;  ///< RGBA8888像素（从核心输出转换而来）
        unsigned width  = 0;
        unsigned height = 0;
    };

    /// 返回最近一帧视频快照，线程安全。
    VideoFrame getVideoFrame() const;

    // ---- 音频样本环形缓冲区 -----------------------------------------
    // 由AudioManager消费；样本为int16_t立体声交错格式。
    bool drainAudio(std::vector<int16_t>& out);

    // ---- 输入 -------------------------------------------------------

    /// 各按钮按下状态，索引 = RETRO_DEVICE_ID_JOYPAD_*
    void setButtonState(unsigned id, bool pressed);
    bool getButtonState(unsigned id) const;

    // ---- 几何信息 ---------------------------------------------------

    unsigned gameWidth()  const { return m_avInfo.geometry.base_width; }
    unsigned gameHeight() const { return m_avInfo.geometry.base_height; }
    double   fps()        const { return m_avInfo.timing.fps; }

    // ---- 设置（通过libretro环境的核心变量）-------------------------

    /// 提供ConfigManager以读取核心变量值。
    /// 在load()之前调用，使retro_set_environment()能获取到配置。
    void setConfigManager(beiklive::ConfigManager* cfg) { m_configManager = cfg; }

    // ---- 快进状态 ---------------------------------------------------

    /// 通知核心宿主是否正在快进，用于响应RETRO_ENVIRONMENT_GET_FASTFORWARDING查询。
    void setFastForwarding(bool ff) { m_fastForwarding.store(ff, std::memory_order_relaxed); }

private:
    // ---- 动态库句柄 -------------------------------------------------
    void* m_handle = nullptr;

    // ---- 函数指针 ---------------------------------------------------
    void (*fn_set_environment)(retro_environment_t)          = nullptr;
    void (*fn_set_video_refresh)(retro_video_refresh_t)      = nullptr;
    void (*fn_set_audio_sample)(retro_audio_sample_t)        = nullptr;
    void (*fn_set_audio_sample_batch)(retro_audio_sample_batch_t) = nullptr;
    void (*fn_set_input_poll)(retro_input_poll_t)            = nullptr;
    void (*fn_set_input_state)(retro_input_state_t)          = nullptr;
    void (*fn_init)()                                        = nullptr;
    void (*fn_deinit)()                                      = nullptr;
    unsigned (*fn_api_version)()                             = nullptr;
    void (*fn_get_system_info)(retro_system_info*)           = nullptr;
    void (*fn_get_system_av_info)(retro_system_av_info*)     = nullptr;
    void (*fn_set_controller_port_device)(unsigned, unsigned) = nullptr;
    void (*fn_reset)()                                       = nullptr;
    void (*fn_run)()                                         = nullptr;
    size_t (*fn_serialize_size)()                            = nullptr;
    bool (*fn_serialize)(void*, size_t)                      = nullptr;
    bool (*fn_unserialize)(const void*, size_t)              = nullptr;
    bool (*fn_load_game)(const retro_game_info*)             = nullptr;
    void (*fn_unload_game)()                                 = nullptr;
    void (*fn_get_region)()                                  = nullptr;
    // ---- 金手指/内存API ---------------------------------------------
    void (*fn_cheat_reset)()                                 = nullptr;
    void (*fn_cheat_set)(unsigned, bool, const char*)        = nullptr;
    void* (*fn_get_memory_data)(unsigned)                    = nullptr;
    size_t (*fn_get_memory_size)(unsigned)                   = nullptr;

    // ---- 运行状态 ---------------------------------------------------
    retro_system_av_info m_avInfo{};
    retro_pixel_format   m_pixelFormat = RETRO_PIXEL_FORMAT_0RGB1555;
    bool m_coreReady  = false;
    bool m_gameLoaded = false;

    // ---- 视频帧存储 -------------------------------------------------
    mutable std::mutex   m_videoMutex;
    VideoFrame           m_videoFrame;

    // ---- 音频环形缓冲区 ---------------------------------------------
    mutable std::mutex       m_audioMutex;
    std::vector<int16_t>     m_audioBuffer;

    // ---- 输入状态 ---------------------------------------------------
    bool m_buttons[RETRO_DEVICE_ID_JOYPAD_R3 + 1] = {};

    // ---- 核心变量/设置存储 ------------------------------------------
    // ConfigManager提供用户保存的值；m_coreVarStorage保存c_str()指针，
    // 在loader生命周期内保持有效。
    beiklive::ConfigManager*                        m_configManager = nullptr;
    std::unordered_map<std::string, std::string> m_coreVarStorage;

    // ---- 存档/系统目录 ----------------------------------------------
    std::string m_saveDirectory;    ///< 通过GET_SAVE_DIRECTORY返回给核心
    std::string m_systemDirectory;  ///< 通过GET_SYSTEM_DIRECTORY返回给核心

    // ---- 传递给核心的宿主状态 ----------------------------------------
    std::atomic<bool> m_fastForwarding{false};  ///< 跟踪宿主快进状态

    // ---- 回调用静态单例 ---------------------------------------------
    // libretro回调为纯C函数指针，通过静态实例指针路由。
    static LibretroLoader* s_current;

    // ---- 静态C回调 --------------------------------------------------
    static bool  s_environmentCallback(unsigned cmd, void* data);
    static void  s_videoRefreshCallback(const void* data, unsigned width,
                                        unsigned height, size_t pitch);
    static void  s_audioSampleCallback(int16_t left, int16_t right);
    static size_t s_audioSampleBatchCallback(const int16_t* data, size_t frames);
    static void  s_inputPollCallback();
    static int16_t s_inputStateCallback(unsigned port, unsigned device,
                                         unsigned index, unsigned id);

    // ---- 辅助函数 ---------------------------------------------------
    template<typename T>
    bool resolveSymbol(T& fnPtr, const char* name);
};

} // namespace beiklive
