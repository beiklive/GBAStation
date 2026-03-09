#pragma once

#include <borealis.hpp>
#include <glad/glad.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "common.hpp"
#include "Game/LibretroLoader.hpp"
#include "Game/DisplayConfig.hpp"

class GameView : public brls::Box
{
  public:
    /// Construct with path to the ROM file that will be loaded automatically.
    explicit GameView(std::string romPath);
    GameView();
    ~GameView() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height,
              brls::Style style, brls::FrameContext* ctx) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onLayout() override;

  private:
    std::string  m_romPath;
    bool         m_initialized  = false;
    bool         m_coreFailed   = false;

    // ---- libretro core ----------------------------------------------
    beiklive::LibretroLoader m_core;

    // ---- OpenGL texture for game frame ------------------------------
    GLuint   m_texture   = 0;
    unsigned m_texWidth  = 0;
    unsigned m_texHeight = 0;

    // ---- NanoVG image handle wrapping the GL texture ----------------
    int  m_nvgImage = -1;

    // ---- Display configuration (scaling / filtering) ----------------
    beiklive::DisplayConfig  m_display;
    beiklive::FilterMode     m_activeFilter = beiklive::FilterMode::Nearest;

    // ---- Independent game thread ------------------------------------
    std::thread       m_gameThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_fastForward{false};

    // ---- Fast-forward config ----------------------------------------
    float    m_ffMultiplier   = 4.0f;   ///< Speed multiplier (fastforward.multiplier)
    bool     m_ffMute         = true;   ///< Mute audio during fast-forward (fastforward.mute)
    bool     m_ffToggleMode   = false;  ///< false=hold, true=toggle (fastforward.mode)
    bool     m_ffToggled      = false;  ///< Toggle-mode state
    bool     m_ffPrevKey      = false;  ///< Previous key state for edge detection
    int      m_ffButton       = static_cast<int>(brls::BUTTON_RT); ///< handle.fastforward

    // ---- Rewind config & state --------------------------------------
    bool     m_rewindEnabled    = false; ///< rewind.enabled
    unsigned m_rewindBufSize    = 3600;  ///< rewind.bufferSize
    unsigned m_rewindStep       = 2;     ///< rewind.step (frames to step back per trigger)
    bool     m_rewindMute       = false; ///< rewind.mute
    bool     m_rewindToggleMode = false; ///< false=hold, true=toggle (rewind.mode)
    int      m_rewindButton     = static_cast<int>(brls::BUTTON_LT); ///< handle.rewind
    bool     m_rewindToggled    = false;
    bool     m_rewindPrevKey    = false;
    std::atomic<bool>            m_rewinding{false};
    std::deque<std::vector<uint8_t>> m_rewindBuffer; ///< Circular save-state buffer
    mutable std::mutex           m_rewindMutex;

    // ---- FPS display ------------------------------------------------
    bool     m_showFps     = false; ///< display.showFps
    mutable std::mutex m_fpsMutex;
    unsigned m_fpsFrameCount = 0;
    float    m_currentFps    = 0.0f;
    std::chrono::steady_clock::time_point m_fpsLastTime;

    // ---- Button map (handle = gamepad, loaded from config) ----------
    struct BtnMap { int brlsBtn; unsigned retroId; };
    std::vector<BtnMap> m_buttonMap;   ///< Loaded gamepad mapping

    // ---- Keyboard button map (raw keyboard keys) --------------------
    struct KbMap { int scancode; unsigned retroId; };
    std::vector<KbMap> m_kbButtonMap;  ///< Loaded keyboard mapping
    bool m_useKeyboard = false;        ///< true when keyboard is active input source

    // ---- Helper methods ---------------------------------------------
    void initialize();
    void cleanup();

    /// Start the independent emulation thread (called from initialize()).
    void startGameThread();

    /// Stop and join the emulation thread (called from cleanup()).
    void stopGameThread();

    /// Load button maps from config. Called from initialize().
    void loadButtonMaps();

    /// Resolve path to mgba_libretro shared library (.dll/.so/.dylib).
    static std::string resolveCoreLibPath();

    /// Upload the latest video frame from the libretro core to m_texture.
    void uploadFrame(NVGcontext* vg, const beiklive::LibretroLoader::VideoFrame& frame);

    /// Map borealis controller state to libretro button states.
    void pollInput();
};