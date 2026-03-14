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
#include "Control/GameInputController.hpp"
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
    std::string  m_romFileName;  ///< File name (with extension) extracted from m_romPath
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

    // ---- Quick save / load state ------------------------------------
    /// Slot number to save to (game thread reads, main thread also writes).
    std::atomic<int>  m_pendingQuickSave{-1};
    /// Slot number to load from (game thread reads, main thread also writes).
    std::atomic<int>  m_pendingQuickLoad{-1};
    /// Current active quick-save slot (0-based).
    int  m_saveSlot = 0;
    /// Previous keyboard quick-save key state (game thread only, edge detection).
    bool m_kbdQsSavePrev = false;
    /// Previous keyboard quick-load key state (game thread only, edge detection).
    bool m_kbdQlLoadPrev = false;
    /// Status message for save/load overlay (set from game thread, read in draw).
    mutable std::mutex  m_saveMsgMutex;
    std::string         m_saveMsg;
    std::chrono::steady_clock::time_point m_saveMsgTime;

    // ---- Input mapping (key bindings + FF/rewind settings) ----------
    beiklive::InputMappingConfig m_inputMap;

    // ---- Low-level gamepad input controller -------------------------
    /// Registered gamepad combo actions; updated from the game thread via pollInput().
    /// Actions are registered once in initialize() before the game thread starts.
    beiklive::GameInputController m_inputCtrl;

    // ---- Fast-forward runtime state ---------------------------------
    /// Whether the gamepad hold-key is currently held (hold mode, game thread only).
    bool m_ffPadHeld       = false;
    /// Whether the keyboard hold-key is currently held (hold mode, game thread only).
    bool m_ffKbdHeld       = false;
    /// Toggle state set by dedicated toggle keys (game thread only).
    bool m_ffToggled       = false;
    /// Previous keyboard hold-key state for edge detection (game thread only).
    bool m_ffKbdHoldPrev   = false;
    /// Previous keyboard toggle-key state for edge detection (game thread only).
    bool m_ffKbdTogglePrev = false;

    // ---- Rewind runtime state ---------------------------------------
    std::atomic<bool>            m_rewinding{false};
    /// Whether the gamepad rewind hold-key is currently held (game thread only).
    bool m_rewPadHeld       = false;
    /// Whether the keyboard rewind hold-key is currently held (game thread only).
    bool m_rewKbdHeld       = false;
    /// Rewind toggle state (game thread only).
    bool m_rewindToggled    = false;
    /// Previous keyboard rewind hold-key state for edge detection (game thread only).
    bool m_rewKbdHoldPrev   = false;
    /// Previous keyboard rewind toggle-key state for edge detection (game thread only).
    bool m_rewKbdTogglePrev = false;
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
    bool m_uiBlocked = false;

    // ---- Main-thread input snapshot (thread-safe GLFW access) ------
    struct InputSnapshot {
        brls::ControllerState ctrlState{};
        /// Keyboard key states: scancode → pressed (for all mapped scancodes)
        std::unordered_map<int, bool> kbdState;
    };
    mutable std::mutex m_inputSnapMutex;
    InputSnapshot      m_inputSnap;

    // ---- Overlay (遮罩) -----------------------------------------------
    bool        m_overlayEnabled   = false; ///< display.overlay.enabled
    std::string m_overlayPath;              ///< resolved overlay PNG path for this game
    int         m_overlayNvgImage  = -1;    ///< NVG image handle for overlay texture

    /// Load (or reload) the overlay texture into m_overlayNvgImage.
    /// Frees the previous image if any. Clears m_overlayNvgImage on failure.
    void loadOverlayImage(NVGcontext* vg);

    // ---- Helper methods ---------------------------------------------
    void initialize();
    void cleanup();

    /// Refresh m_inputSnap from the main thread (called inside draw()).
    void refreshInputSnapshot();

    /// Register all gamepad hotkey actions with m_inputCtrl.
    /// Called from initialize() after m_inputMap is loaded.
    void registerGamepadHotkeys();

    /// Resolve the directory where save files (SRAM / state / cheat) are stored.
    /// Returns @a customDir if non-empty, otherwise the directory of @a romPath.
    static std::string resolveSaveDir(const std::string& romPath,
                                       const std::string& customDir);

    /// Compute the full path for a quick-save state file.
    /// Slot -1 → base name without slot suffix (used for per-game state).
    std::string quickSaveStatePath(int slot) const;

    /// Compute the full path for the SRAM (.sav) file.
    std::string sramSavePath() const;

    /// Compute the full path for the GB MBC3 RTC (.rtc) file.
    std::string rtcSavePath() const;

    /// Compute the full path for the cheat (.cht) file.
    std::string cheatFilePath() const;

    /// Load SRAM data from disk into the core's save-RAM region.
    void loadSram();

    /// Save SRAM data from the core's save-RAM region to disk.
    void saveSram();

    /// Load GB MBC3 RTC data from disk into the core's RTC memory region.
    void loadRtc();

    /// Save GB MBC3 RTC data from the core's RTC memory region to disk.
    void saveRtc();

    /// Save quick-save state to @a slot.
    void doQuickSave(int slot);

    /// Load quick-save state from @a slot.
    void doQuickLoad(int slot);

    /// Load cheats from the .cht file associated with the current ROM.
    void loadCheats();

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