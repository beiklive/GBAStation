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

    /**
     * Enable or disable the borealis input system for this view.
     *
     * When @a enabled is true, borealis UI event dispatching is blocked so
     * that no navigation, hint, or click-animation events reach the UI while
     * the game is running.  The low-level GameInputController is also enabled
     * so gamepad hotkeys are processed.
     *
     * When @a enabled is false, borealis input is unblocked and the
     * GameInputController is suspended.  Use this to hand control back to the
     * UI when a menu is pushed on top of the game.
     *
     * This is a one-click toggle: calling it multiple times with the same
     * value is idempotent.
     *
     * @note Must be called from the main (UI) thread.
     *       `brls::Application::blockInputs` / `unblockInputs` are
     *       main-thread-only APIs, and `m_uiBlocked` is only touched on
     *       the main thread.
     */
    void setGameInputEnabled(bool enabled);

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

    // ---- Exit request -----------------------------------------------
    std::atomic<bool> m_requestExit{false}; ///< Set by game thread; consumed by draw()

    // ---- Quick save / load state ------------------------------------
    /// Slot number to save to (game thread reads, main thread also writes).
    std::atomic<int>  m_pendingQuickSave{-1};
    /// Slot number to load from (game thread reads, main thread also writes).
    std::atomic<int>  m_pendingQuickLoad{-1};
    /// Current active quick-save slot (默认1，对应 .ss1).
    int  m_saveSlot = 1;

    // ---- 截图请求 ---------------------------------------------------
    /// 截图请求标志（游戏线程检测后截图）
    std::atomic<bool> m_pendingScreenshot{false};
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

    // ---- 静音状态 ---------------------------------------------------
    /// 用户通过热键触发的静音开关（游戏线程读，主线程绘制覆盖层）
    std::atomic<bool> m_muted{false};

    // ---- Fast-forward runtime state ---------------------------------
    /// Whether the gamepad hold-key is currently held (hold mode, game thread only).
    bool m_ffPadHeld       = false;
    /// Toggle state set by dedicated toggle keys (game thread only).
    bool m_ffToggled       = false;

    // ---- Rewind runtime state ---------------------------------------
    std::atomic<bool>            m_rewinding{false};
    /// Whether the gamepad rewind hold-key is currently held (game thread only).
    bool m_rewPadHeld       = false;
    /// Rewind toggle state (game thread only).
    bool m_rewindToggled    = false;
    std::deque<std::vector<uint8_t>> m_rewindBuffer; ///< Circular save-state buffer
    mutable std::mutex           m_rewindMutex;

    // ---- FPS display ------------------------------------------------
    bool     m_showFps           = false; ///< display.showFps
    bool     m_showFfOverlay     = true;  ///< display.showFfOverlay
    bool     m_showRewindOverlay = true;  ///< display.showRewindOverlay
    bool     m_showMuteOverlay   = true;  ///< display.showMuteOverlay
    mutable std::mutex m_fpsMutex;
    unsigned m_fpsFrameCount = 0;
    float    m_currentFps    = 0.0f;
    std::chrono::steady_clock::time_point m_fpsLastTime;

    // ---- UI input block tracking ----------------------------------
    /// true when we have an outstanding blockInputs() token (desktop only).
    bool m_uiBlocked = false;

    // ---- Main-thread input snapshot (thread-safe access) -----------
    struct InputSnapshot {
        brls::ControllerState           ctrlState{};
        std::unordered_map<int, bool>   kbState;   ///< BrlsKeyboardScancode → pressed
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
    /// If @a silent is true, do not show the save/load status overlay message.
    void doQuickSave(int slot, bool silent = false);

    /// Load quick-save state from @a slot.
    void doQuickLoad(int slot);

    /// Capture the current GL framebuffer (game frame + overlay) and save as PNG.
    /// Must be called from the main (draw) thread after all rendering is complete.
    /// Uses the full window pixel dimensions (brls::Application::windowWidth/Height).
    void doScreenshot();

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