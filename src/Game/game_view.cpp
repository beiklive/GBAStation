#include "Game/game_view.hpp"
#include "Audio/AudioManager.hpp"

#include <borealis.hpp>
#include <borealis/core/application.hpp>

// ---- NanoVG GL backend include (must match the active borealis backend) ----
#ifdef BOREALIS_USE_OPENGL
#  ifdef USE_GLES3
#    define NANOVG_GLES3
#  elif defined(USE_GLES2)
#    define NANOVG_GLES2
#  elif defined(USE_GL2)
#    define NANOVG_GL2
#  else
#    define NANOVG_GL3
#  endif
#  include <nanovg/nanovg_gl.h>
#endif

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <filesystem>

#ifdef __SWITCH__
#  include <switch.h>
#endif

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>   // timeBeginPeriod / timeEndPeriod
#endif

// ============================================================
// Game thread constants
// ============================================================
static constexpr double MAX_REASONABLE_FPS = 240.0; ///< Safety cap for core-reported FPS
static constexpr size_t STEREO_CHANNELS    = 2;     ///< Samples per audio frame (L + R)
static constexpr double FPS_UPDATE_INTERVAL = 1.0;  ///< Seconds between FPS counter updates
// Spin-wait guard (subtract from sleep to ensure we don't overshoot)
static constexpr double SPIN_GUARD_SEC = 0.002;     ///< 2 ms spin-wait budget per frame

// ============================================================
// NanoVG helper: create NVG image from an existing GL texture.
// Selects the correct function based on the active GL backend.
// ============================================================
static int nvgImageFromGLTexture(NVGcontext* vg, GLuint tex,
                                  int w, int h,
                                  beiklive::FilterMode filter = beiklive::FilterMode::Nearest)
{
    // NVG_IMAGE_NODELETE ensures NanoVG won't delete our texture.
    // NVG_IMAGE_NEAREST selects nearest-neighbor sampling in NanoVG.
    int flags = NVG_IMAGE_NODELETE;
    if (filter == beiklive::FilterMode::Nearest)
        flags |= NVG_IMAGE_NEAREST;
#if defined(USE_GLES2)
    return nvglCreateImageFromHandleGLES2(vg, tex, w, h, flags);
#elif defined(USE_GLES3)
    return nvglCreateImageFromHandleGLES3(vg, tex, w, h, flags);
#elif defined(USE_GL2)
    return nvglCreateImageFromHandleGL2(vg, tex, w, h, flags);
#else
    return nvglCreateImageFromHandleGL3(vg, tex, w, h, flags);
#endif
}



// ============================================================
// Resolve the mgba_libretro shared library path
// ============================================================
std::string GameView::resolveCoreLibPath()
{
#if defined(__SWITCH__)
    // On Switch, mgba_libretro.a is statically linked into the binary.
    // LibretroLoader::load() ignores the path in this case.
    return "";
#elif defined(_WIN32)
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.dll");
#elif defined(__APPLE__)
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.dylib");
#else
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.so");
#endif
}

// ============================================================
// Constructors / Destructor
// ============================================================

GameView::GameView(std::string romPath) : GameView()
{
    m_romPath = std::move(romPath);
    SettingManager->Set("last_game_path", beiklive::file::getParentPath(m_romPath));
}

GameView::GameView()
{
    setFocusable(true);
    setHideHighlight(true);
	beiklive::CheckGLSupport();

    // ── Disable all borealis key handling while the game is running ───────────
    // Swallow every controller button so borealis navigation, hints, and click
    // animations never interfere with in-game input.  All actual input
    // processing is handled by the low-level GameInputController (gamepad) and
    // the keyboard snapshot in pollInput() (keyboard).
    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_B);
    beiklive::swallow(this, brls::BUTTON_X);
    beiklive::swallow(this, brls::BUTTON_Y);
    beiklive::swallow(this, brls::BUTTON_UP);
    beiklive::swallow(this, brls::BUTTON_DOWN);
    beiklive::swallow(this, brls::BUTTON_LEFT);
    beiklive::swallow(this, brls::BUTTON_RIGHT);
    beiklive::swallow(this, brls::BUTTON_NAV_UP);
    beiklive::swallow(this, brls::BUTTON_NAV_DOWN);
    beiklive::swallow(this, brls::BUTTON_NAV_LEFT);
    beiklive::swallow(this, brls::BUTTON_NAV_RIGHT);
    beiklive::swallow(this, brls::BUTTON_LB);
    beiklive::swallow(this, brls::BUTTON_RB);
    beiklive::swallow(this, brls::BUTTON_LT);
    beiklive::swallow(this, brls::BUTTON_RT);
    beiklive::swallow(this, brls::BUTTON_START);
    beiklive::swallow(this, brls::BUTTON_BACK);
}

GameView::~GameView()
{
    cleanup();
}

// ============================================================
// initialize – deferred to first draw so GL context is ready
// ============================================================

void GameView::initialize()
{
    if (m_initialized || m_coreFailed) return;

    setHideHighlight(true);
    setHideClickAnimation(true);
    setHideHighlightBackground(true);
    setHideHighlightBorder(true);

    // ---- Load display settings from config
    if (gameRunner && gameRunner->settingConfig)
        m_display.load(*gameRunner->settingConfig);

    // Register mgba core variable defaults and wire up SettingManager.
    // SetDefault() only writes the value when the key is not already present,
    // so user-saved values in the config file take precedence.
    if (gameRunner && gameRunner->settingConfig) {
        beiklive::ConfigManager* cfg = gameRunner->settingConfig;
        using CV = beiklive::ConfigValue;
        cfg->SetDefault("core.mgba_gb_model",                  CV(std::string("Autodetect")));
        cfg->SetDefault("core.mgba_use_bios",                   CV(std::string("ON")));
        cfg->SetDefault("core.mgba_skip_bios",                  CV(std::string("OFF")));
        cfg->SetDefault("core.mgba_gb_colors",                  CV(std::string("Grayscale")));
        cfg->SetDefault("core.mgba_gb_colors_preset",           CV(std::string("0")));
        cfg->SetDefault("core.mgba_sgb_borders",                CV(std::string("ON")));
        cfg->SetDefault("core.mgba_audio_low_pass_filter",      CV(std::string("disabled")));
        cfg->SetDefault("core.mgba_audio_low_pass_range",       CV(std::string("60")));
        cfg->SetDefault("core.mgba_allow_opposing_directions",  CV(std::string("no")));
        cfg->SetDefault("core.mgba_solar_sensor_level",         CV(std::string("0")));
        cfg->SetDefault("core.mgba_force_gbp",                  CV(std::string("OFF")));
        cfg->SetDefault("core.mgba_idle_optimization",          CV(std::string("Remove Known")));
        cfg->SetDefault("core.mgba_frameskip",                  CV(std::string("0")));

        // ---- FPS display defaults ------------------------------------
        cfg->SetDefault("display.showFps",           CV(std::string("false")));
        cfg->SetDefault("display.showFfOverlay",     CV(std::string("true")));
        cfg->SetDefault("display.showRewindOverlay", CV(std::string("true")));

        // ---- Input mapping defaults (delegated to InputMappingConfig) ----
        m_inputMap.setDefaults(*cfg);

        cfg->Save();
        m_core.setConfigManager(cfg);

        // ---- Load display overlay flags ----------------------------------
        auto getBool = [&](const std::string& key, bool def) -> bool {
            auto v = cfg->Get(key);
            if (v) {
                if (auto s = v->AsString()) return (*s == "true" || *s == "1" || *s == "yes");
                if (auto i = v->AsInt())    return (*i != 0);
            }
            return def;
        };
        m_showFps           = getBool("display.showFps",           false);
        m_showFfOverlay     = getBool("display.showFfOverlay",     true);
        m_showRewindOverlay = getBool("display.showRewindOverlay", true);

        // ---- Load all input bindings and FF/rewind settings --------------
        m_inputMap.load(*cfg);

        // ---- Register gamepad hotkey actions with GameInputController ----
        registerGamepadHotkeys();
    } // end if (gameRunner && gameRunner->settingConfig)

    // ---- Load libretro core -------------------------------------------
    std::string corePath = resolveCoreLibPath();
    bklog::info("Loading libretro core: {}", corePath);

    if (!m_core.load(corePath)) {
        bklog::error("Failed to load libretro core from: {}", corePath);
        m_coreFailed = true;
        return;
    }

    if (!m_core.initCore()) {
        bklog::error("retro_init() failed");
        m_core.unload();
        m_coreFailed = true;
        return;
    }

    // ---- Load ROM -------------------------------------------------------
    if (!m_romPath.empty()) {
        if (!std::filesystem::exists(m_romPath)) {
            bklog::error("ROM not found: {}", m_romPath);
            m_core.deinitCore();
            m_core.unload();
            m_coreFailed = true;
            return;
        }

        if (!m_core.loadGame(m_romPath)) {
            bklog::error("retro_load_game() failed for: {}", m_romPath);
            m_core.deinitCore();
            m_core.unload();
            m_coreFailed = true;
            return;
        }
        bklog::info("ROM loaded: {} ({}x{} @ {:.2f} fps)",
                    m_romPath,
                    m_core.gameWidth(), m_core.gameHeight(),
                    m_core.fps());
    }

    // ---- Create GL texture for video output ----------------------------
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    // Apply filter mode from display config (Nearest or Linear)
    m_activeFilter     = m_display.filterMode;
    GLenum glFilter    = (m_activeFilter == beiklive::FilterMode::Nearest)
                         ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Pre-allocate with game dimensions using GL_RGBA (universally supported)
    unsigned gw = m_core.gameWidth()  > 0 ? m_core.gameWidth()  : 256;
    unsigned gh = m_core.gameHeight() > 0 ? m_core.gameHeight() : 224;
    std::vector<uint32_t> blank(gw * gh, 0xFF000000u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(gw),
                 static_cast<GLsizei>(gh), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 blank.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_texWidth  = gw;
    m_texHeight = gh;

    // ---- Initialise render chain -------------------------------------
    if (!m_renderChain.init()) {
        bklog::warning("RenderChain init failed; using direct texture rendering");
    }

    // ---- Start audio manager ------------------------------------------
    if (!beiklive::AudioManager::instance().init(32768, 2)) {
        bklog::warning("AudioManager init failed; game will run without audio");
    }

    m_initialized = true;
    bklog::info("GameView initialized successfully");

    // ---- Start independent emulation thread ---------------------------
    startGameThread();
}

// ============================================================
// startGameThread – launches the emulation loop in a new thread
// ============================================================

void GameView::startGameThread()
{
    m_fpsLastTime    = std::chrono::steady_clock::now();
    m_fpsFrameCount  = 0;
    m_currentFps     = 0.0f;

    // Configure audio backpressure threshold: ~4 game-frames worth of audio.
    // This keeps the ring buffer lean so latency cannot build up over time.
    {
        double coreFps = m_core.fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS) coreFps = 60.0;
        size_t maxLatencyFrames = static_cast<size_t>(32768.0 / coreFps * 4.0 + 0.5);
        beiklive::AudioManager::instance().setMaxLatencyFrames(maxLatencyFrames);
    }

    m_running.store(true, std::memory_order_release);
    m_gameThread = std::thread([this]() {
#ifdef __SWITCH__
        // Pin game-emulation thread to Core 1 (dedicated core).
        // Core 0 = UI/main thread.  Core 1 = game emulation (audio inline).
        svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
#endif
        double fps = m_core.fps();
        if (fps <= 0.0 || fps > MAX_REASONABLE_FPS) fps = 60.0;

        using Clock    = std::chrono::steady_clock;
        using Duration = std::chrono::duration<double>;
        const Duration frameDuration(1.0 / fps);
        // Guard duration: subtract from coarse sleep so spin-wait finishes on time
        const Duration spinGuard(SPIN_GUARD_SEC);

#ifdef _WIN32
        // Request 1 ms Windows timer resolution for accurate sleep_for()
        timeBeginPeriod(1);
#endif

        // Per-thread FPS counter
        Clock::time_point fpsTimerStart = Clock::now();
        unsigned          fpsCounter    = 0;

        // Accumulated ideal frame-end time for drift-free 60fps timing.
        // Advancing by frameDuration each iteration prevents timing errors from
        // compounding: one slow frame does not shrink the next frame's budget.
        // Initialised to Clock::now() so the very first frame gets a full
        // frameDuration budget starting from the current wall-clock instant.
        auto nextFrameTarget = Clock::now();

#ifdef __SWITCH__
        // Track fast-forward state to detect the fast-forward→normal transition.
        bool prevFastForward = false;
#endif

        while (m_running.load(std::memory_order_acquire)) {
            // Poll controller input and forward to the core
            pollInput();

            bool ff      = m_fastForward.load(std::memory_order_relaxed);
            bool rew     = m_rewinding.load(std::memory_order_relaxed);

#ifdef __SWITCH__
            // When fast-forward ends, flush any stale audio samples from the ring
            // buffer so they are not played back at normal speed (which would
            // sound like noise or a brief burst of sped-up audio).
            if (prevFastForward && !ff) {
                beiklive::AudioManager::instance().flushRingBuffer();
            }
            prevFastForward = ff;
#endif

            // framesThisIter tracks how many logical frames were rendered,
            // used by the FPS counter below.
            unsigned framesThisIter = 1u;

            if (rew && m_inputMap.rewindEnabled) {
                // ---- Rewind: restore from buffer then run to update video ----
                bool didRestore = false;
                {
                    std::lock_guard<std::mutex> lk(m_rewindMutex);
                    for (unsigned step = 0; step < m_inputMap.rewindStep && !m_rewindBuffer.empty(); ++step) {
                        const auto& state = m_rewindBuffer.front();
                        m_core.unserialize(state.data(), state.size());
                        m_rewindBuffer.pop_front();
                        didRestore = true;
                    }
                }
                // Run core after restoring state to produce a fresh video frame.
                if (didRestore) {
                    m_core.run();
                }
                // Drain audio (mute or pass through based on config)
                {
                    std::vector<int16_t> dummy;
                    bool hasSamples = m_core.drainAudio(dummy) && !dummy.empty();
                    if (!m_inputMap.rewindMute && hasSamples) {
                        size_t frames = dummy.size() / STEREO_CHANNELS;
                        beiklive::AudioManager::instance().pushSamples(dummy.data(), frames);
                    }
                }
            } else {
                // ---- Normal or fast-forward: run core --------------------
                // Compute how many frames to run this iteration.
                // For fast-forward with multiplier = N, run N frames per normal
                // frame-period so effective speed = N × fps (exact multiplier).
                // For sub-1x, run 1 frame but stretch the sleep duration.
                if (ff) {
                    framesThisIter = (m_inputMap.ffMultiplier >= 1.0f)
                        ? static_cast<unsigned>(std::round(m_inputMap.ffMultiplier))
                        : 1u;
                }

                for (unsigned i = 0; i < framesThisIter; ++i) {
                    // Save state for rewind buffer before each frame (including fast-forward)
                    if (m_inputMap.rewindEnabled) {
                        size_t sz = m_core.serializeSize();
                        if (sz > 0) {
                            std::vector<uint8_t> state(sz);
                            if (m_core.serialize(state.data(), sz)) {
                                std::lock_guard<std::mutex> lk(m_rewindMutex);
                                m_rewindBuffer.push_front(std::move(state));
                                while (m_rewindBuffer.size() > m_inputMap.rewindBufSize)
                                    m_rewindBuffer.pop_back();
                            }
                        }
                    }
                    m_core.run();
                }

                // Drain audio samples and push directly to AudioManager.
                {
                    std::vector<int16_t> samples;
                    bool hasSamples = m_core.drainAudio(samples) && !samples.empty();
                    // Mute conditions:
                    //   - fast-forward with mute enabled
                    //   - no samples available
                    bool mute = (ff && m_inputMap.ffMute) || !hasSamples;
                    if (!mute) {
                        size_t frames = samples.size() / STEREO_CHANNELS;
                        // During fast-forward (multiplier > 1) with audio not muted,
                        // limit the audio pushed to one normal frame's worth of samples.
                        // Running N frames per loop iteration generates N× the usual
                        // audio, which would saturate the ring buffer and cause
                        // pushSamples() to block indefinitely, freezing the game thread.
                        if (ff && m_inputMap.ffMultiplier > 1.0f) {
                            // Divide in floating-point first for precision, then round to
                            // the nearest integer sample-frame count.
                            size_t limit = static_cast<size_t>(
                                std::round(static_cast<double>(frames) / m_inputMap.ffMultiplier));
                            // If limit rounds to zero the total is already tiny; push all.
                            if (limit > 0)
                                frames = limit;
                        }
                        beiklive::AudioManager::instance().pushSamples(samples.data(), frames);
                    }
                }
            }

            // ---- FPS counter (game-thread side) -------------------------
            // Count all rendered frames (including fast-forward multiplied frames).
            fpsCounter += framesThisIter;
            auto nowPost = Clock::now(); // captured once, reused for sleep below
            double elapsed = std::chrono::duration<double>(nowPost - fpsTimerStart).count();
            if (elapsed >= FPS_UPDATE_INTERVAL) {
                float newFps = static_cast<float>(static_cast<double>(fpsCounter) / elapsed);
                {
                    std::lock_guard<std::mutex> lk(m_fpsMutex);
                    m_currentFps    = newFps;
                    m_fpsFrameCount = fpsCounter;
                }
                fpsCounter   = 0;
                fpsTimerStart = nowPost;
            }

            // ---- Frame-rate control (accumulated ideal-time approach) ----
            // nextFrameTarget advances by exactly one frame period each
            // iteration, independent of actual execution time.  This prevents
            // timing errors from accumulating across frames (e.g. one slow
            // frame no longer shrinks the next frame's sleep budget).
            //
            // fast-forward (multiplier >= 1x):
            //   Run N frames per period; sleep for same frameDuration.
            //   Effective speed = N / frameDuration = multiplier × fps. ✓
            //
            // fast-forward (sub-1x):
            //   Stretch period to 1/(fps*multiplier) per frame.
            //
            // normal / rewind:
            //   Sleep for frameDuration.
            //
            // Hybrid sleep: coarse sleep for (remaining − spinGuard), then
            // spin-wait for precise deadline.  Anti-drift guard resets the
            // accumulated target to now whenever a frame runs over budget,
            // giving the next frame a fresh full-period budget instead of
            // scheduling it in the past (which would cause a catch-up burst).
            {
                Duration targetDur = frameDuration;
                if (ff && m_inputMap.ffMultiplier < 1.0f) {
                    targetDur = Duration(1.0 / (fps * static_cast<double>(m_inputMap.ffMultiplier)));
                }

                // Advance accumulated target by one frame.
                nextFrameTarget += std::chrono::duration_cast<Clock::duration>(targetDur);

                // Anti-drift: if the frame ran over budget, sync nextFrameTarget
                // to now so the next frame gets a fresh full budget instead of
                // trying to schedule itself in the past.
                if (nextFrameTarget < nowPost) {
                    nextFrameTarget = nowPost;
                }

                if (nowPost < nextFrameTarget) {
                    // Coarse sleep (leave spinGuard for spin-wait)
                    auto coarseDur = (nextFrameTarget - nowPost) - spinGuard;
                    if (coarseDur.count() > 0)
                        std::this_thread::sleep_for(coarseDur);
                    // Spin-wait for precise deadline
                    while (Clock::now() < nextFrameTarget) { /* busy spin */ }
                }
            }
        }

#ifdef _WIN32
        timeEndPeriod(1);
#endif
    });
}

// ============================================================
// stopGameThread – signals and joins the emulation thread
// ============================================================

void GameView::stopGameThread()
{
    m_running.store(false, std::memory_order_release);
    if (m_gameThread.joinable()) {
        m_gameThread.join();
    }
}

// ============================================================
// cleanup
// ============================================================

void GameView::cleanup()
{
    // Release UI input block if it's still held (e.g. when the destructor
    // is invoked without a prior onFocusLost, which should not normally
    // happen but guards against unusual destruction order).
#ifndef __SWITCH__
    if (m_uiBlocked) {
        brls::Application::unblockInputs();
        m_uiBlocked = false;
    }
#endif

    // Signal AudioManager to stop and wake any pushSamples() waiter BEFORE
    // joining the game thread.
    beiklive::AudioManager::instance().deinit();

    // Stop and join the emulation thread (audio is called inline)
    stopGameThread();

    // Clear rewind buffer
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        m_rewindBuffer.clear();
    }

    if (m_nvgImage >= 0) {
        m_nvgImage = -1;
    }

    // Deinit render chain (releases any GL resources)
    m_renderChain.deinit();

    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    if (m_core.isLoaded()) {
        m_core.unloadGame();
        m_core.deinitCore();
        m_core.unload();
    }

    m_initialized = false;
}

// ============================================================
// registerGamepadHotkeys – wire m_inputCtrl to emulator hotkeys
//
// Called from initialize() after m_inputMap is loaded.
// Each gamepad hotkey binding is registered as a GameInputController
// action so that press/release/long-press events are automatically
// tracked without any borealis action handlers.
// ============================================================

void GameView::registerGamepadHotkeys()
{
    m_inputCtrl.clear();

    using Hotkey = beiklive::InputMappingConfig::Hotkey;
    using KeyEvent = beiklive::GameInputController::KeyEvent;

    // Helper: register one gamepad button combo if it is bound.
    auto reg = [&](Hotkey h, beiklive::GameInputController::Callback cb)
    {
        const auto& hk = m_inputMap.hotkeyBinding(h);
        if (hk.isPadBound())
            m_inputCtrl.registerAction({hk.padButton}, std::move(cb));
    };

    // ── Fast-forward (hold key) ──────────────────────────────────────────────
    reg(Hotkey::FastForwardHold, [this](KeyEvent evt)
    {
        if (!m_inputMap.ffToggleMode)
        {
            // Hold mode: FF active while key is held.
            m_ffPadHeld = (evt == KeyEvent::Press);
        }
        else
        {
            // Toggle mode: each press flips the toggle state.
            if (evt == KeyEvent::Press)
                m_ffToggled = !m_ffToggled;
        }
        m_fastForward.store(m_ffPadHeld || m_ffKbdHeld || m_ffToggled,
                            std::memory_order_relaxed);
    });

    // ── Fast-forward (dedicated toggle key) ─────────────────────────────────
    reg(Hotkey::FastForwardToggle, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::Press)
        {
            m_ffToggled = !m_ffToggled;
            m_fastForward.store(m_ffPadHeld || m_ffKbdHeld || m_ffToggled,
                                std::memory_order_relaxed);
        }
    });

    // ── Rewind (hold key) ────────────────────────────────────────────────────
    reg(Hotkey::RewindHold, [this](KeyEvent evt)
    {
        if (!m_inputMap.rewindToggleMode)
        {
            m_rewPadHeld = (evt == KeyEvent::Press);
        }
        else
        {
            if (evt == KeyEvent::Press)
                m_rewindToggled = !m_rewindToggled;
        }
        m_rewinding.store(m_rewPadHeld || m_rewKbdHeld || m_rewindToggled,
                          std::memory_order_relaxed);
    });

    // ── Rewind (dedicated toggle key) ───────────────────────────────────────
    reg(Hotkey::RewindToggle, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::Press)
        {
            m_rewindToggled = !m_rewindToggled;
            m_rewinding.store(m_rewPadHeld || m_rewKbdHeld || m_rewindToggled,
                              std::memory_order_relaxed);
        }
    });

    // ── Exit game ────────────────────────────────────────────────────────────
    reg(Hotkey::ExitGame, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::Press && !m_requestExit.load(std::memory_order_relaxed))
            m_requestExit.store(true, std::memory_order_relaxed);
    });
}

// ============================================================
// uploadFrame – copy RGBA8888 pixels from libretro into the GL texture
// ============================================================

void GameView::uploadFrame(NVGcontext* vg,
                            const beiklive::LibretroLoader::VideoFrame& frame)
{
    if (!frame.width || !frame.height || frame.pixels.empty()) return;

    glBindTexture(GL_TEXTURE_2D, m_texture);

    bool sizeChanged = (frame.width  != m_texWidth ||
                        frame.height != m_texHeight);

    if (sizeChanged) {
        // Resize the texture (pixels are RGBA8888)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(frame.width),
                     static_cast<GLsizei>(frame.height),
                     0, GL_RGBA, GL_UNSIGNED_BYTE,
                     frame.pixels.data());
        m_texWidth  = frame.width;
        m_texHeight = frame.height;

        // Recreate the NanoVG image handle with new dimensions
        if (m_nvgImage >= 0) {
            nvgDeleteImage(vg, m_nvgImage);
        }
        m_nvgImage = nvgImageFromGLTexture(vg, m_texture,
                                           static_cast<int>(m_texWidth),
                                           static_cast<int>(m_texHeight),
                                           m_activeFilter);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        static_cast<GLsizei>(frame.width),
                        static_cast<GLsizei>(frame.height),
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        frame.pixels.data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
// pollInput – map controller/keyboard state to libretro buttons
// and fire emulator hotkeys via GameInputController.
//
// Gamepad hotkeys: handled by m_inputCtrl (registered in initialize())
// Keyboard hotkeys: handled inline using the keyboard snapshot
// Game buttons: mapped to libretro joypad IDs via m_inputMap
//
// NOTE: All actual GLFW/input-manager calls happen in refreshInputSnapshot()
// on the main thread.  pollInput() only reads from the thread-safe snapshot.
// ============================================================

void GameView::pollInput()
{
    // ---- Obtain input state from the main-thread snapshot ---------------
    InputSnapshot snap;
    {
        std::lock_guard<std::mutex> lk(m_inputSnapMutex);
        snap = m_inputSnap;
    }
    const brls::ControllerState& state = snap.ctrlState;

    // Helper: look up a keyboard key state from the snapshot.
    auto kbdState = [&](brls::BrlsKeyboardScancode sc) -> bool {
        auto it = snap.kbdState.find(static_cast<int>(sc));
        return (it != snap.kbdState.end()) && it->second;
    };

    // ── GameInputController: process all registered gamepad hotkey combos ──
    // This handles FF hold/toggle, rewind hold/toggle, and exit-game actions
    // for the gamepad side. Callbacks write to atomics and game-thread booleans.
    m_inputCtrl.update(state);

    // ── Keyboard detection and mode switch ────────────────────────────────
    const auto& btnMap = m_inputMap.gameButtonMap();
#ifndef __SWITCH__
    bool keyboardActive = false;
    for (const auto& entry : btnMap) {
        if (entry.kbdScancode >= 0 &&
            kbdState(static_cast<brls::BrlsKeyboardScancode>(entry.kbdScancode))) {
            keyboardActive = true;
            break;
        }
    }
    if (!keyboardActive) {
        using Hotkey = beiklive::InputMappingConfig::Hotkey;
        for (int h = 0; h < static_cast<int>(Hotkey::_Count); ++h) {
            const auto& hk = m_inputMap.hotkeyBinding(static_cast<Hotkey>(h));
            if (hk.kbdCombo.isBound()) {
                if (kbdState(static_cast<brls::BrlsKeyboardScancode>(hk.kbdCombo.scancode))) {
                    keyboardActive = true;
                    break;
                }
            }
        }
    }
    if (keyboardActive) {
        m_useKeyboard = true;
    } else {
        const bool enterPressed =
            kbdState(brls::BRLS_KBD_KEY_ENTER) ||
            kbdState(brls::BRLS_KBD_KEY_KP_ENTER);
        const bool escPressed = kbdState(brls::BRLS_KBD_KEY_ESCAPE);
        for (int i = 0; i < static_cast<int>(brls::_BUTTON_MAX); ++i) {
            if (!state.buttons[i]) continue;
            if (i == static_cast<int>(brls::BUTTON_A) && enterPressed) continue;
            if (i == static_cast<int>(brls::BUTTON_B) && escPressed)   continue;
            m_useKeyboard = false;
            break;
        }
    }
#endif

    // ── Keyboard-only hotkey helper ───────────────────────────────────────
    // Returns true when the keyboard side of a hotkey is currently active.
    // Modifier keys are checked as required by the binding.
    using Hotkey = beiklive::InputMappingConfig::Hotkey;
    auto isKbdHotkeyPressed = [&](Hotkey h) -> bool {
#ifndef __SWITCH__
        const auto& hk = m_inputMap.hotkeyBinding(h);
        if (hk.kbdCombo.isBound()) {
            auto sc = static_cast<brls::BrlsKeyboardScancode>(hk.kbdCombo.scancode);
            if (kbdState(sc)) {
                bool ctrlOk  = !hk.kbdCombo.ctrl  ||
                    kbdState(brls::BRLS_KBD_KEY_LEFT_CONTROL) ||
                    kbdState(brls::BRLS_KBD_KEY_RIGHT_CONTROL);
                bool shiftOk = !hk.kbdCombo.shift ||
                    kbdState(brls::BRLS_KBD_KEY_LEFT_SHIFT) ||
                    kbdState(brls::BRLS_KBD_KEY_RIGHT_SHIFT);
                bool altOk   = !hk.kbdCombo.alt   ||
                    kbdState(brls::BRLS_KBD_KEY_LEFT_ALT) ||
                    kbdState(brls::BRLS_KBD_KEY_RIGHT_ALT);
                if (ctrlOk && shiftOk && altOk) return true;
            }
        }
#endif
        return false;
    };

    // ── Keyboard fast-forward ─────────────────────────────────────────────
#ifndef __SWITCH__
    {
        bool kbdFfHold   = isKbdHotkeyPressed(Hotkey::FastForwardHold);
        bool kbdFfToggle = isKbdHotkeyPressed(Hotkey::FastForwardToggle);

        // Dedicated keyboard toggle: edge-detected
        if (kbdFfToggle && !m_ffKbdTogglePrev)
            m_ffToggled = !m_ffToggled;
        m_ffKbdTogglePrev = kbdFfToggle;

        if (m_inputMap.ffToggleMode) {
            if (kbdFfHold && !m_ffKbdHoldPrev)
                m_ffToggled = !m_ffToggled;
            m_ffKbdHoldPrev = kbdFfHold;
        } else {
            m_ffKbdHeld     = kbdFfHold;
            m_ffKbdHoldPrev = kbdFfHold;
        }
        m_fastForward.store(m_ffPadHeld || m_ffKbdHeld || m_ffToggled,
                            std::memory_order_relaxed);
    }

    // ── Keyboard rewind ───────────────────────────────────────────────────
    if (m_inputMap.rewindEnabled) {
        bool kbdRewHold   = isKbdHotkeyPressed(Hotkey::RewindHold);
        bool kbdRewToggle = isKbdHotkeyPressed(Hotkey::RewindToggle);

        if (kbdRewToggle && !m_rewKbdTogglePrev)
            m_rewindToggled = !m_rewindToggled;
        m_rewKbdTogglePrev = kbdRewToggle;

        if (m_inputMap.rewindToggleMode) {
            if (kbdRewHold && !m_rewKbdHoldPrev)
                m_rewindToggled = !m_rewindToggled;
            m_rewKbdHoldPrev = kbdRewHold;
        } else {
            m_rewKbdHeld     = kbdRewHold;
            m_rewKbdHoldPrev = kbdRewHold;
        }
        m_rewinding.store(m_rewPadHeld || m_rewKbdHeld || m_rewindToggled,
                          std::memory_order_relaxed);
    }
#endif

    // Disable fast-forward while rewinding
    if (m_rewinding.load(std::memory_order_relaxed))
        m_fastForward.store(false, std::memory_order_relaxed);

    // ── Keyboard exit ─────────────────────────────────────────────────────
#ifndef __SWITCH__
    if (!m_requestExit.load(std::memory_order_relaxed)) {
        if (isKbdHotkeyPressed(Hotkey::ExitGame))
            m_requestExit.store(true, std::memory_order_relaxed);
    }
#endif

    // ── Game button mapping ───────────────────────────────────────────────
    if (m_useKeyboard) {
        for (const auto& entry : btnMap) {
            if (entry.kbdScancode < 0) continue;
            bool pressed = false;
#ifndef __SWITCH__
            pressed = kbdState(static_cast<brls::BrlsKeyboardScancode>(entry.kbdScancode));
#endif
            m_core.setButtonState(entry.retroId, pressed);
        }
    } else {
        for (const auto& entry : btnMap) {
            bool pressed = false;
            if (entry.padButton >= 0 && entry.padButton < static_cast<int>(brls::_BUTTON_MAX))
                pressed = state.buttons[entry.padButton];
            m_core.setButtonState(entry.retroId, pressed);
        }
    }

    // ── Debug logging ─────────────────────────────────────────────────────
#ifndef __SWITCH__
    if (brls::Logger::getLogLevel() <= brls::LogLevel::LOG_DEBUG) {
        for (const auto& entry : btnMap) {
            bool pressed = false;
            if (m_useKeyboard && entry.kbdScancode >= 0)
                pressed = kbdState(static_cast<brls::BrlsKeyboardScancode>(entry.kbdScancode));
            else if (!m_useKeyboard && entry.padButton >= 0)
                pressed = state.buttons[entry.padButton];
            if (pressed)
                bklog::debug("pollInput: retroId={} pressed ({})",
                             entry.retroId, m_useKeyboard ? "kbd" : "pad");
        }
    }
#endif
}

// ============================================================
// refreshInputSnapshot – capture input state on the main thread
// ============================================================
// GLFW functions (glfwGetKey, glfwGetWindowAttrib, glfwGetGamepadState, ...)
// must only be called from the main thread.  The game emulation thread calls
// pollInput() which would otherwise invoke these functions from a non-main
// thread, leading to undefined behaviour (GLFW is NOT thread-safe for window
// and input queries).
//
// This function is called once per rendered frame from draw() (main thread).
// It writes a fresh snapshot of all relevant input states into m_inputSnap
// under m_inputSnapMutex, and pollInput() on the game thread reads from that
// snapshot, thereby completely avoiding any off-thread GLFW calls.
//
void GameView::refreshInputSnapshot()
{
    auto* platform = brls::Application::getPlatform();
    auto* im       = platform ? platform->getInputManager() : nullptr;
    if (!im) return;

    InputSnapshot snap;

    // ── Gamepad / controller state ───────────────────────────────────────────
    // updateUnifiedControllerState() reads the current hardware controller state.
    // On desktop this calls GLFW functions that must run on the main thread.
    // On Switch this reads hidGetNpadStates which is safe on any thread but we
    // still call it from draw() (main thread) to keep a consistent pattern.
    im->updateUnifiedControllerState(&snap.ctrlState);

#ifndef __SWITCH__
    // ── Keyboard state ───────────────────────────────────────────────────────
    // Collect all scancodes we need to track: game button map + hotkeys +
    // modifier keys.  Keyboard input only exists on non-Switch platforms.
    const auto& btnMap = m_inputMap.gameButtonMap();
    for (const auto& entry : btnMap) {
        if (entry.kbdScancode >= 0)
            snap.kbdState[entry.kbdScancode] =
                im->getKeyboardKeyState(
                    static_cast<brls::BrlsKeyboardScancode>(entry.kbdScancode));
    }

    // Modifier keys (used for hotkey combo detection)
    static const brls::BrlsKeyboardScancode k_mods[] = {
        brls::BRLS_KBD_KEY_LEFT_CONTROL,  brls::BRLS_KBD_KEY_RIGHT_CONTROL,
        brls::BRLS_KBD_KEY_LEFT_SHIFT,    brls::BRLS_KBD_KEY_RIGHT_SHIFT,
        brls::BRLS_KBD_KEY_LEFT_ALT,      brls::BRLS_KBD_KEY_RIGHT_ALT,
    };
    for (auto sc : k_mods)
        snap.kbdState[static_cast<int>(sc)] = im->getKeyboardKeyState(sc);

    // Always capture borealis keyboard-navigation keys (ENTER/KP_ENTER/ESC).
    // Borealis maps these to BUTTON_A / BUTTON_B in updateUnifiedControllerState,
    // so we need their raw keyboard states to distinguish between "keyboard key
    // pressed" and "physical gamepad button pressed" in pollInput().
    static const brls::BrlsKeyboardScancode k_navKeys[] = {
        brls::BRLS_KBD_KEY_ENTER,
        brls::BRLS_KBD_KEY_KP_ENTER,
        brls::BRLS_KBD_KEY_ESCAPE,
    };
    for (auto sc : k_navKeys)
        snap.kbdState[static_cast<int>(sc)] = im->getKeyboardKeyState(sc);

    // Hotkey keyboard bindings
    using Hotkey = beiklive::InputMappingConfig::Hotkey;
    for (int h = 0; h < static_cast<int>(Hotkey::_Count); ++h) {
        const auto& hk = m_inputMap.hotkeyBinding(static_cast<Hotkey>(h));
        if (hk.kbdCombo.isBound() && hk.kbdCombo.scancode >= 0) {
            int sc = hk.kbdCombo.scancode;
            snap.kbdState[sc] =
                im->getKeyboardKeyState(
                    static_cast<brls::BrlsKeyboardScancode>(sc));
        }
    }
#endif

    // Publish the snapshot
    std::lock_guard<std::mutex> lk(m_inputSnapMutex);
    m_inputSnap = std::move(snap);
}

// ============================================================
// draw – per-frame render entry point (GL upload + NVG render only)
// ============================================================

void GameView::draw(NVGcontext* vg, float x, float y, float width, float height,
                    brls::Style style, brls::FrameContext* ctx)
{
    // Deferred initialization (GL context guaranteed to exist at this point)
    if (!m_initialized && !m_coreFailed) {
        initialize();
    }

    // ── Refresh input snapshot from the main thread (GLFW thread safety) ──
    // Must be called every frame so pollInput() (game thread) always has a
    // fresh view of the hardware input state without touching GLFW directly.
    if (m_initialized) {
        refreshInputSnapshot();
    }

    // ---- ExitGame hotkey: game thread sets this flag; handle on main thread -----
#ifndef __SWITCH__
    if (m_requestExit.exchange(false, std::memory_order_relaxed)) {
        bklog::info("GameView: exit requested via hotkey");
        stopGameThread();
        brls::Application::popActivity();
        return;
    }
#endif

    if (!m_initialized) {
        // Draw error/placeholder rectangle
        nvgBeginPath(vg);
        nvgRect(vg, x, y, width, height);
        nvgFillColor(vg, nvgRGBA(30, 30, 30, 255));
        nvgFill(vg);

        nvgFontSize(vg, 20.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(200, 60, 60, 255));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + width * 0.5f, y + height * 0.5f - 15.0f,
                "Failed to load emulator core", nullptr);

        // Hint: press A to dismiss the screen
        nvgFontSize(vg, 16.0f);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 200));
        nvgText(vg, x + width * 0.5f, y + height * 0.5f + 15.0f,
                "beiklive/hints/close_on_a"_i18n.c_str(), nullptr);

        // Handle BUTTON_A press to dismiss core-failure screen.
        // Since all borealis actions are disabled we check the raw snapshot.
        // Edge detection via m_requestExit prevents repeated calls when held.
        if (m_coreFailed) {
            InputSnapshot snap;
            {
                std::lock_guard<std::mutex> lk(m_inputSnapMutex);
                snap = m_inputSnap;
            }
            bool btnA = snap.ctrlState.buttons[static_cast<int>(brls::BUTTON_A)];
            if (btnA && !m_requestExit.exchange(true, std::memory_order_relaxed)) {
                brls::Application::popActivity();
                return;
            }
        }
        return;
    }

    // ---- Upload the latest video frame to the GL texture -------------
    // (The emulation runs in a separate thread; we only consume the result.)
    {
        auto frame = m_core.getVideoFrame();
        if (!frame.pixels.empty()) {
            uploadFrame(vg, frame);
        }
    }

    // ---- Handle run-time filter mode changes -------------------------
    if (m_display.filterMode != m_activeFilter && m_texture) {
        m_activeFilter  = m_display.filterMode;
        GLenum glFilter = (m_activeFilter == beiklive::FilterMode::Nearest)
                          ? GL_NEAREST : GL_LINEAR;
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (m_nvgImage >= 0) {
            nvgDeleteImage(vg, m_nvgImage);
            m_nvgImage = -1;
        }
    }

    // ---- Run render chain and determine the display texture ----------
    // RenderChain::run() currently acts as a pass-through and returns srcTex.
    // Future custom rendering implementations replace this pipeline.
    GLuint displayTex = m_texture;
    int    displayW   = static_cast<int>(m_texWidth);
    int    displayH   = static_cast<int>(m_texHeight);

    if (m_texWidth > 0 && m_texHeight > 0) {
        GLuint chainOut = m_renderChain.run(m_texture, m_texWidth, m_texHeight);
        if (chainOut != 0) {
            displayTex = chainOut;
        }
    }

    // ---- Manage NVG image handle -------------------------------------
    if (m_nvgImage < 0 && m_texWidth > 0 && m_texHeight > 0) {
        m_nvgImage = nvgImageFromGLTexture(vg, displayTex,
                                           displayW, displayH,
                                           m_activeFilter);
    }

    // ---- Render the game texture using NanoVG ------------------------
    if (m_nvgImage >= 0) {
        beiklive::DisplayRect rect = m_display.computeRect(x, y, width, height,
                                                            static_cast<unsigned>(displayW),
                                                            static_cast<unsigned>(displayH));

        NVGpaint imgPaint = nvgImagePattern(vg,
                                            rect.x, rect.y,
                                            rect.w, rect.h,
                                            0.0f,
                                            m_nvgImage,
                                            1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, rect.x, rect.y, rect.w, rect.h);
        nvgFillPaint(vg, imgPaint);
        nvgFill(vg);
    }

    // ---- FPS overlay -------------------------------------------------
    if (m_showFps) {
        float currentFps = 0.0f;
        {
            std::lock_guard<std::mutex> lk(m_fpsMutex);
            currentFps = m_currentFps;
        }
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", currentFps);

        // Semi-transparent background
        float fw = 90.0f, fh = 22.0f;
        float fx = x + 4.0f, fy = y + 4.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, fx, fy, fw, fh, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(0, 255, 80, 230));
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, fx + 6.0f, fy + fh * 0.5f, fpsBuf, nullptr);
    }

    // ---- Fast-forward overlay (configurable) -------------------------
    if (m_showFfOverlay && m_fastForward.load(std::memory_order_relaxed)) {
        char ffBuf[32];
        snprintf(ffBuf, sizeof(ffBuf), ">> %.4gx", static_cast<double>(m_inputMap.ffMultiplier));
        float fw = 80.0f, fh = 22.0f;
        float fx = x + width - fw - 4.0f;
        float fy = y + 4.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, fx, fy, fw, fh, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(100, 220, 255, 230));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, fx + fw * 0.5f, fy + fh * 0.5f, ffBuf, nullptr);
    }

    // ---- Rewind status overlay (configurable) ------------------------
    if (m_showRewindOverlay && m_inputMap.rewindEnabled && m_rewinding.load(std::memory_order_relaxed)) {
        char rewBuf[32];
        // rewindStep = frames popped from buffer per rewind trigger (rewind speed)
        snprintf(rewBuf, sizeof(rewBuf), "<<< %u", m_inputMap.rewindStep);
        float rw = 90.0f, rh = 22.0f;
        float rx = x + width * 0.5f - rw * 0.5f;
        float ry = y + 4.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(255, 200, 0, 230));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, rx + rw * 0.5f, ry + rh * 0.5f, rewBuf, nullptr);
    }

    this->invalidate();
}

// ============================================================
// Focus / Layout callbacks
// ============================================================

void GameView::onFocusGained()
{
    Box::onFocusGained();
#ifndef __SWITCH__
    // Block all borealis UI input processing while the game has focus.
    // This prevents arrow-key navigation, button hints, and other UI
    // interactions from interfering with in-game keyboard/gamepad input.
    if (!m_uiBlocked) {
        brls::Application::blockInputs(true); // true = mute UI click-error sounds
        m_uiBlocked = true;
    }
#endif
}

void GameView::onFocusLost()
{
    Box::onFocusLost();
#ifndef __SWITCH__
    // Restore UI input processing when the game loses focus (e.g. a menu
    // activity is pushed on top, or the user exits the game).
    if (m_uiBlocked) {
        brls::Application::unblockInputs();
        m_uiBlocked = false;
    }
#endif
}

void GameView::onLayout()
{
    Box::onLayout();
}

