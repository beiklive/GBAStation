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
#include "Utils/ConfigManager.hpp"

namespace beiklive {

/// Encapsulates a single loaded libretro core.
/// Handles dynamic library loading, callback registration and lifetime.
class LibretroLoader {
public:
    LibretroLoader()  = default;
    ~LibretroLoader() = default;

    LibretroLoader(const LibretroLoader&)            = delete;
    LibretroLoader& operator=(const LibretroLoader&) = delete;

    // ---- Lifecycle --------------------------------------------------

    /// Load the shared library at @a path and resolve all required symbols.
    /// @return true on success.
    bool load(const std::string& libPath);

    /// Unload the shared library.  Safe to call even if load() was not called.
    void unload();

    bool isLoaded() const { return m_handle != nullptr; }

    // ---- Libretro API forwarding ------------------------------------

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

    // ---- Memory (SRAM / battery save) ---------------------------------------

    /// Returns a pointer to the core's memory region (e.g. RETRO_MEMORY_SAVE_RAM).
    /// Valid only while a game is loaded; nullptr if unsupported.
    void*  getMemoryData(unsigned id) const;

    /// Returns the size in bytes of a core memory region.
    size_t getMemorySize(unsigned id) const;

    // ---- Cheats -------------------------------------------------------------

    /// Reset / clear all cheats registered with the core.
    void cheatReset();

    /// Set (add or update) a single cheat entry.
    /// @param index   Position in the cheat list (0-based).
    /// @param enabled Whether the cheat is active.
    /// @param code    Cheat code string (format depends on core, e.g. GameShark).
    void cheatSet(unsigned index, bool enabled, const std::string& code);

    // ---- Save directory -----------------------------------------------------

    /// Set the directory that the core should use for saves.
    /// Called before loadGame() so that RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY
    /// returns this path.
    void setSaveDirectory(const std::string& dir) { m_saveDirectory = dir; }

    /// Set the system/BIOS directory for the core.
    void setSystemDirectory(const std::string& dir) { m_systemDirectory = dir; }

    // ---- Video frame ------------------------------------------------

    struct VideoFrame {
        std::vector<uint32_t> pixels;  ///< RGBA8888 pixels (converted from core output)
        unsigned width  = 0;
        unsigned height = 0;
    };

    /// Returns a snapshot of the most-recently delivered video frame.
    /// Thread-safe.
    VideoFrame getVideoFrame() const;

    // ---- Audio sample ring buffer -----------------------------------
    // Consumed by AudioManager; samples are int16_t stereo interleaved.
    bool drainAudio(std::vector<int16_t>& out);

    // ---- Input ------------------------------------------------------

    /// Per-button pressed state.  Index = RETRO_DEVICE_ID_JOYPAD_*
    void setButtonState(unsigned id, bool pressed);
    bool getButtonState(unsigned id) const;

    // ---- Geometry ---------------------------------------------------

    unsigned gameWidth()  const { return m_avInfo.geometry.base_width; }
    unsigned gameHeight() const { return m_avInfo.geometry.base_height; }
    double   fps()        const { return m_avInfo.timing.fps; }

    // ---- Settings (core variables via libretro environment) ---------

    /// Provide a ConfigManager from which core variable values are read.
    /// Call before load() so that retro_set_environment() picks them up.
    void setConfigManager(beiklive::ConfigManager* cfg) { m_configManager = cfg; }

    // ---- Fast-forward state -----------------------------------------

    /// Notify the core whether the host is currently fast-forwarding.
    /// Used to answer RETRO_ENVIRONMENT_GET_FASTFORWARDING queries from cores.
    void setFastForwarding(bool ff) { m_fastForwarding.store(ff, std::memory_order_relaxed); }

private:
    // ---- Dynamic library handle -------------------------------------
    void* m_handle = nullptr;

    // ---- Function pointers ------------------------------------------
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
    // ---- Cheat / memory API -------------------------------------------------
    void (*fn_cheat_reset)()                                 = nullptr;
    void (*fn_cheat_set)(unsigned, bool, const char*)        = nullptr;
    void* (*fn_get_memory_data)(unsigned)                    = nullptr;
    size_t (*fn_get_memory_size)(unsigned)                   = nullptr;

    // ---- State ------------------------------------------------------
    retro_system_av_info m_avInfo{};
    retro_pixel_format   m_pixelFormat = RETRO_PIXEL_FORMAT_0RGB1555;
    bool m_coreReady  = false;
    bool m_gameLoaded = false;

    // ---- Video frame storage ----------------------------------------
    mutable std::mutex   m_videoMutex;
    VideoFrame           m_videoFrame;

    // ---- Audio ring buffer ------------------------------------------
    mutable std::mutex       m_audioMutex;
    std::vector<int16_t>     m_audioBuffer;

    // ---- Input state ------------------------------------------------
    bool m_buttons[RETRO_DEVICE_ID_JOYPAD_R3 + 1] = {};

    // ---- Core variable / settings storage ---------------------------
    // ConfigManager provides user-saved values; m_coreVarStorage holds
    // c_str() pointers that remain valid for the lifetime of the loader.
    beiklive::ConfigManager*                        m_configManager = nullptr;
    std::unordered_map<std::string, std::string> m_coreVarStorage;

    // ---- Save / system directory -------------------------------------
    std::string m_saveDirectory;    ///< Returned to core via GET_SAVE_DIRECTORY
    std::string m_systemDirectory;  ///< Returned to core via GET_SYSTEM_DIRECTORY

    // ---- Host state communicated to core ----------------------------
    std::atomic<bool> m_fastForwarding{false};  ///< Tracks host fast-forward state

    // ---- Static singleton for callbacks -----------------------------
    // libretro callbacks are plain C function pointers, so we route them
    // through a static instance pointer.
    static LibretroLoader* s_current;

    // ---- Static C callbacks -----------------------------------------
    static bool  s_environmentCallback(unsigned cmd, void* data);
    static void  s_videoRefreshCallback(const void* data, unsigned width,
                                        unsigned height, size_t pitch);
    static void  s_audioSampleCallback(int16_t left, int16_t right);
    static size_t s_audioSampleBatchCallback(const int16_t* data, size_t frames);
    static void  s_inputPollCallback();
    static int16_t s_inputStateCallback(unsigned port, unsigned device,
                                         unsigned index, unsigned id);

    // ---- Helper -----------------------------------------------------
    template<typename T>
    bool resolveSymbol(T& fnPtr, const char* name);
};

} // namespace beiklive
