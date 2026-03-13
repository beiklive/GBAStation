#pragma once

#include <borealis.hpp>
#include <glad/glad.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "Control/InputMapping.hpp"
#include "Retro/LibretroLoader.hpp"
#include "Video/DisplayConfig.hpp"
#include "Video/RenderChain.hpp"

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

    // ---- RenderChain (video post-processing pipeline) ---------------
    beiklive::RenderChain    m_renderChain;

    // ---- Independent game thread ------------------------------------
    std::thread       m_gameThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_fastForward{false};

    // ---- Keyboard exit ----------------------------------------------
    std::atomic<bool> m_requestExit{false}; ///< Set by game thread; consumed by draw()

    // ---- Input mapping (key bindings + FF/rewind settings) ----------
    beiklive::InputMappingConfig m_inputMap;

    // ---- Fast-forward runtime state ---------------------------------
    bool m_ffToggled       = false;  ///< Combined toggle state (hold-key toggle mode + toggle-key)
    bool m_ffPrevKey       = false;  ///< Previous hold-key state for edge detection
    bool m_ffTogglePrevKey = false;  ///< Previous toggle-key state for edge detection

    // ---- Rewind runtime state ---------------------------------------
    std::atomic<bool>            m_rewinding{false};
    bool                         m_rewindToggled       = false;
    bool                         m_rewindPrevKey       = false;
    bool                         m_rewindTogglePrevKey = false;
    std::deque<std::vector<uint8_t>> m_rewindBuffer; ///< Circular save-state buffer
    mutable std::mutex           m_rewindMutex;

    // ---- FPS display ------------------------------------------------
    bool     m_showFps           = false; ///< display.showFps
    bool     m_showFfOverlay     = true;  ///< display.showFfOverlay
    bool     m_showRewindOverlay = true;  ///< display.showRewindOverlay
    mutable std::mutex m_fpsMutex;
    unsigned m_fpsFrameCount = 0;
    float    m_currentFps    = 0.0f;
    std::chrono::steady_clock::time_point m_fpsLastTime;

    // ---- Input source tracking ------------------------------------
    bool m_useKeyboard = false; ///< true when keyboard is the active input source

    // ---- UI input block tracking ----------------------------------
    /// true when we have an outstanding blockInputs() token (desktop only).
    /// Prevents borealis UI navigation from stealing keyboard/gamepad inputs
    /// while a game is running.
    bool m_uiBlocked = false;

    // ---- Main-thread input snapshot (thread-safe GLFW access) ------
    // GLFW functions (glfwGetKey, glfwGetWindowAttrib, etc.) must only be
    // called from the main thread.  The game thread calls pollInput() which
    // would otherwise call those functions directly.  Instead, draw() (main
    // thread) captures fresh input state into m_inputSnap every frame; the
    // game thread reads from this snapshot under the mutex.
    struct InputSnapshot {
        brls::ControllerState ctrlState{};
        /// Keyboard key states: scancode → pressed (for all mapped scancodes)
        std::unordered_map<int, bool> kbdState;
    };
    mutable std::mutex m_inputSnapMutex;
    InputSnapshot      m_inputSnap;

    // ---- Helper methods ---------------------------------------------
    void initialize();
    void cleanup();

    /// Refresh m_inputSnap from the main thread (called inside draw()).
    /// Must be called from the main (render) thread so that GLFW functions
    /// are invoked safely.
    void refreshInputSnapshot();

    /// Start the independent emulation thread (called from initialize()).
    void startGameThread();

    /// Stop and join the emulation thread (called from cleanup()).
    void stopGameThread();

    /// Resolve path to mgba_libretro shared library (.dll/.so/.dylib).
    static std::string resolveCoreLibPath();

    /// Upload the latest video frame from the libretro core to m_texture.
    void uploadFrame(NVGcontext* vg, const beiklive::LibretroLoader::VideoFrame& frame);

    /// Map borealis controller state to libretro button states.
    void pollInput();
};