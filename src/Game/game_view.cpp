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
#include <ctime>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#ifdef __SWITCH__
#  include <switch.h>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
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
    return "";  // switch平台直接静态链接
#elif defined(_WIN32)
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.dll");
#endif
}

GameView::GameView(std::string romPath) : GameView()
{
    m_romPath     = std::move(romPath);
    m_romFileName = std::filesystem::path(m_romPath).filename().string();
    SettingManager->Set("last_game_path", beiklive::file::getParentPath(m_romPath));
}

GameView::GameView()
{
    setFocusable(true);
    setHideHighlight(true);
	beiklive::CheckGLSupport();

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

    // ---- 读取画面配置
    if (gameRunner && gameRunner->settingConfig)
        m_display.load(*gameRunner->settingConfig);

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

        // ---- Save / state directory defaults --------------------------------
        cfg->SetDefault("save.sramDir",         CV(std::string("")));
        cfg->SetDefault("save.stateDir",         CV(std::string("")));
        cfg->SetDefault("save.slotCount",        CV(9));
        cfg->SetDefault("save.autoLoadState0",   CV(std::string("false")));
        cfg->SetDefault("cheat.dir",             CV(std::string("")));

        // ---- FPS display defaults ------------------------------------
        cfg->SetDefault("display.showFps",           CV(std::string("false")));
        cfg->SetDefault("display.showFfOverlay",     CV(std::string("false")));
        cfg->SetDefault("display.showRewindOverlay", CV(std::string("false")));
        cfg->SetDefault(KEY_DISPLAY_OVERLAY_ENABLED,  CV(std::string("false")));
        cfg->SetDefault(KEY_DISPLAY_OVERLAY_GBA_PATH, CV(std::string("")));
        cfg->SetDefault(KEY_DISPLAY_OVERLAY_GBC_PATH, CV(std::string("")));

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
        m_showFfOverlay     = getBool("display.showFfOverlay",     false);
        m_showRewindOverlay = getBool("display.showRewindOverlay", false);
        m_overlayEnabled    = getBool(KEY_DISPLAY_OVERLAY_ENABLED, false);

        // ---- Resolve overlay path for this game --------------------------
        // 1. Try per-game overlay from gamedataManager.
        // 2. Fall back to global GBA / GBC path depending on ROM extension.
        if (m_overlayEnabled && !m_romFileName.empty()) {
            std::string perGame = getGameDataStr(m_romFileName, GAMEDATA_FIELD_OVERLAY, "");
            if (!perGame.empty()) { 
                // 每个游戏独立遮罩
                m_overlayPath = perGame;
            } else {
                // 使用通用遮罩
                std::string ext;
                auto dot = m_romFileName.rfind('.');
                if (dot != std::string::npos) {
                    ext = m_romFileName.substr(dot + 1);
                    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                std::string pathKey;
                if (ext == "gba")       pathKey = KEY_DISPLAY_OVERLAY_GBA_PATH;
                else if (ext == "gb" || ext == "gbc") pathKey = KEY_DISPLAY_OVERLAY_GBC_PATH;
                if (!pathKey.empty()) {
                    auto v = cfg->Get(pathKey);
                    if (v) { if (auto s = v->AsString()) m_overlayPath = *s; }
                }
            }
        }

        // ---- 读取按键映射 --------------
        m_inputMap.load(*cfg);

        // ---- 绑定功能键以及回调函数 ----
        registerGamepadHotkeys();
    } // end if (gameRunner && gameRunner->settingConfig)

    // ---- Load libretro core -------------------------------------------
    std::string corePath = resolveCoreLibPath();
    bklog::info("Loading libretro core: {}", corePath);

    // ---- 配置存档位置core -----------------------
    {
        std::string sramCustomDir;
        if (gameRunner && gameRunner->settingConfig) {
            auto v = gameRunner->settingConfig->Get("save.sramDir");
            if (v) { if (auto s = v->AsString()) sramCustomDir = *s; }
        }
        // 读取用户自定义SRAM目录（如果有），并确保它存在。然后将其路径传递给核心，核心会在该目录下为每个游戏创建子目录来存储SRAM文件。
        std::string sramDir = resolveSaveDir(m_romPath, sramCustomDir);
        if (!sramDir.empty()) {
            // Ensure the directory exists
            std::error_code ec;
            std::filesystem::create_directories(sramDir, ec);
            if (ec) {
                bklog::warning("GameView: failed to create save directory '{}': {}",
                               sramDir, ec.message());
            }
            m_core.setSaveDirectory(sramDir);
        }
    }
    // 初始化核心
    if (!m_core.load(corePath)) {
        bklog::error("Failed to load libretro core from: {}", corePath);
        m_coreFailed = true;
        return;
    }
    // 检查核心状态
    if (!m_core.initCore()) {
        bklog::error("retro_init() failed");
        m_core.unload();
        m_coreFailed = true;
        return;
    }

    // ---- 读取 ROM -------------------------------------------------------
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

        // ---- 读取 SRAM（电池保存）和金手指在 ROM 加载后 --------
        loadSram();
        loadCheats();

        m_core.reset(); // Ensure the core starts in a clean state after loading saves/cheats

        // ---- 自动读取即时存档0（如果开启） ----------------------
        // 使用 cfgGetBool 读取配置（gameRunner->settingConfig 与 SettingManager 指向同一对象）
        if (beiklive::cfgGetBool("save.autoLoadState0", false)) {
            std::string state0Path = quickSaveStatePath(0);
            if (std::filesystem::exists(state0Path)) {
                bklog::info("GameView: 自动读取即时存档0: {}", state0Path);
                doQuickLoad(0);
            } else {
                bklog::info("GameView: 自动读取即时存档0已开启, 但 slot0 存档文件不存在: {}", state0Path);
            }
        }
    }

    // ---- 创建用于视频输出的 GL 纹理 ----------------------------
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

        // Per-thread play-time tracker: save total play time every 60 seconds.
        Clock::time_point playtimeTimer = Clock::now();

        // RTC sync timer: write current Unix time to core's RTC memory once per second.
        Clock::time_point rtcSyncTimer = Clock::now();

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
            pollInput();  // 处理输入事件

            bool ff      = m_fastForward.load(std::memory_order_relaxed);
            bool rew     = m_rewinding.load(std::memory_order_relaxed);

            // Keep the core aware of the current fast-forward state so it can
            // answer RETRO_ENVIRONMENT_GET_FASTFORWARDING correctly.
            m_core.setFastForwarding(ff);

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
                // ---- 存储倒带状态
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

                if (didRestore) {
                    m_core.run();  // 核心运行以更新视频输出，保证倒带的流畅性。
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
                    // 静音条件：
                    //   - 快进时静音（fastforward.mute）
                    //   - 用户手动静音（m_muted）
                    //   - 没有可用的音频样本
                    bool mute = (ff && m_inputMap.ffMute)
                                || m_muted.load(std::memory_order_relaxed)
                                || !hasSamples;
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

            // 更新游戏运行时间长 1分钟一次
            if (gamedataManager && !m_romFileName.empty()) {
                auto playtimeElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    nowPost - playtimeTimer).count();
                if (playtimeElapsed >= 60) {
                    // Advance the timer by exactly one minute so each interval
                    // adds a fixed 60 seconds (prevents over-counting if the
                    // thread was suspended or the check was delayed).
                    playtimeTimer += std::chrono::seconds(60);
                    std::string prefix = gamedataKeyPrefix(m_romFileName);
                    std::string k = prefix + "." + GAMEDATA_FIELD_TOTALTIME;
                    int currentTotal = 0;
                    auto tv = gamedataManager->Get(k);
                    if (tv) {
                        if (auto iv = tv->AsInt()) currentTotal = *iv;
                    }
                    currentTotal += 60;
                    gamedataManager->Set(k, beiklive::ConfigValue(currentTotal));
                    gamedataManager->Save();
                }
            }

            // RTC real-time sync: keep the unixTime field in the GB MBC3
            // RTC save-buffer up to date.  GBMBCRTCSaveBuffer::unixTime is
            // at byte offset 40 (10 × uint32_t) – this is the field that
            // GBMBCRTCRead loads into rtcLastLatch on the next game load.
            // Keeping it current ensures that elapsed-time is calculated
            // correctly after a save-reload cycle.
            {
                auto rtcElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    nowPost - rtcSyncTimer).count();
                if (rtcElapsed >= 1) {
                    rtcSyncTimer += std::chrono::seconds(1);
                    // GBMBCRTCSaveBuffer::unixTime is at byte offset 40.
                    static constexpr size_t k_rtcUnixTimeOffset = 10 * sizeof(uint32_t);
                    size_t rtcSz = m_core.getMemorySize(RETRO_MEMORY_RTC);
                    if (rtcSz >= k_rtcUnixTimeOffset + sizeof(uint64_t)) {
                        void* rtcPtr = m_core.getMemoryData(RETRO_MEMORY_RTC);
                        if (rtcPtr) {
                            std::time_t t = std::time(nullptr);
                            if (t != static_cast<std::time_t>(-1)) {
                                uint64_t nowUnix = static_cast<uint64_t>(t);
                                std::memcpy(static_cast<uint8_t*>(rtcPtr) + k_rtcUnixTimeOffset,
                                            &nowUnix, sizeof(uint64_t));
                            }
                        }
                    }
                }
            }

            // ---- 存读即时存档 --------
            // exchange(-1, std::memory_order_relaxed) 的作用是：
            // 取出当前请求槽位（slot）；
            // 同时把原子变量重置为 -1（表示“无待处理请求”）。
            {
                int slot = m_pendingQuickSave.exchange(-1, std::memory_order_relaxed);
                if (slot >= 0) doQuickSave(slot);
            }
            {
                int slot = m_pendingQuickLoad.exchange(-1, std::memory_order_relaxed);
                if (slot >= 0) doQuickLoad(slot);
            }

            // 帧率限制器
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

    // Release overlay NVG image handle (actual deletion happens when the NVG
    // context is destroyed; we just reset our handle here).
    if (m_overlayNvgImage >= 0) {
        m_overlayNvgImage = -1;
    }

    // Deinit render chain (releases any GL resources)
    m_renderChain.deinit();

    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    if (m_core.isLoaded()) {
        // Save SRAM (battery save) before unloading the game
        if (!m_romPath.empty()) {
            saveSram();
        }
        m_core.unloadGame();
        m_core.deinitCore();
        m_core.unload();
    }

    m_initialized = false;
}

// ============================================================
// setGameInputEnabled – one-click toggle for borealis input block
// ============================================================
// When enabled = true:  borealis UI event dispatching is blocked so no
//   navigation, hint, or animation events interfere with the game.
//   GameInputController is enabled and ready to dispatch hotkey events.
// When enabled = false: borealis input is unblocked and GameInputController
//   is suspended (action states are reset to prevent spurious events).
// Calling with the same value as the current state is idempotent.
//
// THREADING: Must be called from the main (UI) thread.
//   brls::Application::blockInputs / unblockInputs are main-thread APIs.
//   m_uiBlocked is accessed exclusively on the main thread
//   (onFocusGained, onFocusLost, draw, and this method).
//   m_inputCtrl.setEnabled() is safe to call from the main thread before
//   the game thread starts, or at any time if setEnabled is called before
//   the game thread reads from the controller.
// ============================================================

void GameView::setGameInputEnabled(bool enabled)
{
#ifndef __SWITCH__
    if (enabled && !m_uiBlocked)
    {
        brls::Application::blockInputs(true); // true = mute UI click-error sounds
        m_uiBlocked = true;
    }
    else if (!enabled && m_uiBlocked)
    {
        brls::Application::unblockInputs();
        m_uiBlocked = false;
    }
#endif
    m_inputCtrl.setEnabled(enabled);
}


// ============================================================
// registerGamepadHotkeys – wire m_inputCtrl to emulator hotkeys
//
// Called from initialize() after m_inputMap is loaded.
// Each gamepad hotkey binding is registered as a GameInputController
// action so that Press / ShortPress / LongPress / Release events are
// automatically tracked without any borealis action handlers.
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
    reg(Hotkey::FastForward, [this](KeyEvent evt)
    {
        if(beiklive::cfgGetBool("fastforward.enabled", false))
        {
            if (!m_inputMap.ffToggleMode)
            {
                // 按住模式：Press 开始快进，Release 结束快进。
                if (evt == KeyEvent::Press)
                    m_ffPadHeld = true;
                else if (evt == KeyEvent::Release)
                    m_ffPadHeld = false;
            }
            else
            {
                // 切换模式：ShortPress 切换快进状态。
                if (evt == KeyEvent::ShortPress)
                    m_ffToggled = !m_ffToggled;
            }
            m_fastForward.store(m_ffPadHeld || m_ffToggled,
                                std::memory_order_relaxed);
        }
    });

    // ── Rewind (hold key) ────────────────────────────────────────────────────
    reg(Hotkey::Rewind, [this](KeyEvent evt)
    {
        if(beiklive::cfgGetBool("rewind.enabled", false)){
            if (!m_inputMap.rewindToggleMode)
            {
                if (evt == KeyEvent::Press)
                    m_rewPadHeld = true;
                else if (evt == KeyEvent::Release)
                    m_rewPadHeld = false;
            }
            else
            {
                if (evt == KeyEvent::ShortPress)
                    m_rewindToggled = !m_rewindToggled;
            }
            m_rewinding.store(m_rewPadHeld || m_rewindToggled,
                              std::memory_order_relaxed);

        }
    });

    // ── Exit game ────────────────────────────────────────────────────────────
    reg(Hotkey::ExitGame, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress && !m_requestExit.load(std::memory_order_relaxed))
            m_requestExit.store(true, std::memory_order_relaxed);
    });

    // ── Quick save state ─────────────────────────────────────────────────────
    reg(Hotkey::QuickSave, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress)
            m_pendingQuickSave.store(m_saveSlot, std::memory_order_relaxed);
    });

    // ── Quick load state ─────────────────────────────────────────────────────
    reg(Hotkey::QuickLoad, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress)
            m_pendingQuickLoad.store(m_saveSlot, std::memory_order_relaxed);
    });

    // ── 静音切换 ──────────────────────────────────────────────────────────────
    reg(Hotkey::Mute, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress)
            m_muted.store(!m_muted.load(std::memory_order_relaxed), std::memory_order_relaxed);
    });
}

// ============================================================
// resolveSaveDir – determine the directory for save files
// ============================================================

std::string GameView::resolveSaveDir(const std::string& romPath,
                                      const std::string& customDir)
{
    if (!customDir.empty()) return customDir;
    // Default: same directory as the ROM
    if (!romPath.empty()) {
        return beiklive::file::getParentPath(romPath);
    }
    return BK_APP_ROOT_DIR + std::string("saves");
}

// ============================================================
// quickSaveStatePath – compute path for a quick-save state file
// ============================================================

std::string GameView::quickSaveStatePath(int slot) const
{
    std::string stateCustomDir;
    if (gameRunner && gameRunner->settingConfig) {
        auto v = gameRunner->settingConfig->Get("save.stateDir");
        if (v) { if (auto s = v->AsString()) stateCustomDir = *s; }
    }
    std::string dir = resolveSaveDir(m_romPath, stateCustomDir);

    // Extract base ROM name without extension
    std::string base;
    if (!m_romPath.empty()) {
        std::filesystem::path p(m_romPath);
        base = p.stem().string();
    } else {
        base = "game";
    }

    // Ensure directory exists
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec)
            bklog::warning("GameView: failed to create state directory '{}': {}", dir, ec.message());
    }

    std::string sep = (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
                      ? "/" : "";
    if (slot < 0)
        return dir + sep + base + ".ss";
    return dir + sep + base + ".ss" + std::to_string(slot);
}

// ============================================================
// sramSavePath – compute path for the battery-save (.sav) file
// ============================================================

std::string GameView::sramSavePath() const
{
    std::string customDir;
    if (gameRunner && gameRunner->settingConfig) {
        auto v = gameRunner->settingConfig->Get("save.sramDir");
        if (v) { if (auto s = v->AsString()) customDir = *s; }
    }
    std::string dir = resolveSaveDir(m_romPath, customDir);

    std::string base;
    if (!m_romPath.empty()) {
        std::filesystem::path p(m_romPath);
        base = p.stem().string();
    } else {
        base = "game";
    }

    // Ensure directory exists
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec)
            bklog::warning("GameView: failed to create SRAM directory '{}': {}", dir, ec.message());
    }

    std::string sep = (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
                      ? "/" : "";
    return dir + sep + base + ".sav";
}

// ============================================================
// rtcSavePath – compute path for the GB MBC3 RTC (.rtc) file
// ============================================================
// RTC files are stored alongside SRAM files (same directory, .rtc extension).

std::string GameView::rtcSavePath() const
{
    std::string customDir;
    if (gameRunner && gameRunner->settingConfig) {
        // Intentionally reuses save.sramDir: .rtc files live in the same
        // directory as .sav files so all per-game save data stays together.
        auto v = gameRunner->settingConfig->Get("save.sramDir");
        if (v) { if (auto s = v->AsString()) customDir = *s; }
    }
    std::string dir = resolveSaveDir(m_romPath, customDir);

    std::string base;
    if (!m_romPath.empty()) {
        std::filesystem::path p(m_romPath);
        base = p.stem().string();
    } else {
        base = "game";
    }

    std::string sep = (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
                      ? "/" : "";
    return dir + sep + base + ".rtc";
}

// ============================================================
// cheatFilePath – compute path for the cheat (.cht) file
// ============================================================

std::string GameView::cheatFilePath() const
{
    std::string customDir;
    if (gameRunner && gameRunner->settingConfig) {
        auto v = gameRunner->settingConfig->Get("cheat.dir");
        if (v) { if (auto s = v->AsString()) customDir = *s; }
    }
    std::string dir = resolveSaveDir(m_romPath, customDir);

    std::string base;
    if (!m_romPath.empty()) {
        std::filesystem::path p(m_romPath);
        base = p.stem().string();
    } else {
        base = "game";
    }

    std::string sep = (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
                      ? "/" : "";
    return dir + sep + base + ".cht";
}

// ============================================================
// loadSram – load SRAM from disk into the core
// ============================================================

void GameView::loadSram()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0) {
        bklog::info("GameView: no SRAM region in core, skipping SRAM load");
        return;
    }

    std::string path = sramSavePath();

    if (!std::filesystem::exists(path)) {
        bklog::info("GameView: no SRAM file found at {}", path);
    } else {
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            bklog::warning("GameView: failed to open SRAM file: {}", path);
        } else {
            std::vector<uint8_t> buf(sz, 0);
            f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
            std::streamsize got = f.gcount();

            void* sramPtr = m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
            // 加载存档到核心的 SRAM 区域
            if (sramPtr) {
                std::memcpy(sramPtr, buf.data(), static_cast<size_t>(got));
                bklog::info("GameView: SRAM loaded from {} ({} bytes)", path, got);
            } else {
                bklog::warning("GameView: SRAM pointer is null, cannot load SRAM");
            }
        }
    }

    // 读取时钟信息
    loadRtc();
}

// ============================================================
// saveSram – save SRAM from the core to disk
// ============================================================

void GameView::saveSram()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0) return;

    const void* sramPtr = m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
    if (!sramPtr) {
        bklog::warning("GameView: SRAM pointer is null, cannot save SRAM");
        return;
    }

    std::string path = sramSavePath();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        bklog::warning("GameView: failed to open SRAM file for writing: {}", path);
        return;
    }

    f.write(reinterpret_cast<const char*>(sramPtr), static_cast<std::streamsize>(sz));
    if (!f) {
        bklog::warning("GameView: failed to write SRAM file: {}", path);
        return;
    }
    bklog::info("GameView: SRAM saved to {} ({} bytes)", path, sz);

    saveRtc();
}

// ============================================================
// loadRtc – load GB MBC3 RTC data from disk into the core
// ============================================================

void GameView::loadRtc()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_RTC);
    if (sz == 0) {
        return; // core has no RTC region (not a GB MBC3 game)
    }

    // GBMBCRTCSaveBuffer layout (mGBA):
    //   offset  0: sec         (uint32)
    //   offset  4: min         (uint32)
    //   offset  8: hour        (uint32)
    //   offset 12: days        (uint32)
    //   offset 16: daysHi      (uint32)
    //   offset 20: latchedSec  (uint32)
    //   offset 24: latchedMin  (uint32)
    //   offset 28: latchedHour (uint32)
    //   offset 32: latchedDays (uint32)
    //   offset 36: latchedDaysHi (uint32)
    //   offset 40: unixTime    (uint64)  ← GBMBCRTCRead loads this into rtcLastLatch
    static constexpr size_t k_rtcUnixTimeOffset = 10 * sizeof(uint32_t); // = 40

    std::string path = rtcSavePath();
    if (!std::filesystem::exists(path)) {
        // No save file yet.  Seed unixTime with the current wall-clock time so
        // GBMBCRTCRead (called from _doDeferredSetup on the first retro_run)
        // initialises rtcLastLatch correctly and _latchRtc computes 0 elapsed
        // time rather than a huge spurious value from the 0xFF-filled buffer.
        if (sz >= k_rtcUnixTimeOffset + sizeof(uint64_t)) {
            void* rtcPtr = m_core.getMemoryData(RETRO_MEMORY_RTC);
            if (rtcPtr) {
                std::time_t t = std::time(nullptr);
                if (t != static_cast<std::time_t>(-1)) {
                    uint64_t nowUnix = static_cast<uint64_t>(t);
                    std::memcpy(static_cast<uint8_t*>(rtcPtr) + k_rtcUnixTimeOffset,
                                &nowUnix, sizeof(uint64_t));
                    bklog::info("GameView: RTC – no save file, seeded unixTime={}", nowUnix);
                }
            }
        }
        return;
    }

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        bklog::warning("GameView: failed to open RTC file: {}", path);
        return;
    }

    std::vector<uint8_t> buf(sz, 0);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
    std::streamsize got = f.gcount();

    void* rtcPtr = m_core.getMemoryData(RETRO_MEMORY_RTC);
    if (rtcPtr) {
        std::memcpy(rtcPtr, buf.data(), static_cast<size_t>(got));
        bklog::info("GameView: RTC loaded from {} ({} bytes)", path, got);
    } else {
        bklog::warning("GameView: RTC pointer is null, cannot load RTC data");
    }
}

// ============================================================
// saveRtc – save GB MBC3 RTC data from the core to disk
// ============================================================

void GameView::saveRtc()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_RTC);
    if (sz == 0) {
        return; // core has no RTC region (not a GB MBC3 game)
    }

    const void* rtcPtr = m_core.getMemoryData(RETRO_MEMORY_RTC);
    if (!rtcPtr) {
        bklog::warning("GameView: RTC pointer is null, cannot save RTC data");
        return;
    }

    std::string path = rtcSavePath();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        bklog::warning("GameView: failed to open RTC file for writing: {}", path);
        return;
    }

    f.write(reinterpret_cast<const char*>(rtcPtr), static_cast<std::streamsize>(sz));
    if (!f) {
        bklog::warning("GameView: failed to write RTC file: {}", path);
        return;
    }
    bklog::info("GameView: RTC saved to {} ({} bytes)", path, sz);
}

// ============================================================
// doQuickSave – serialize core state to a slot file
// ============================================================

void GameView::doQuickSave(int slot)
{
    size_t sz = m_core.serializeSize();
    if (sz == 0) {
        bklog::warning("GameView: core does not support serialize (slot {})", slot);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Save failed (unsupported)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    std::vector<uint8_t> buf(sz);
    if (!m_core.serialize(buf.data(), sz)) {
        bklog::warning("GameView: serialize failed (slot {})", slot);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Save failed";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    std::string path = quickSaveStatePath(slot);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        bklog::warning("GameView: cannot open state file for writing: {}", path);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Save failed (I/O)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    f.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(sz));
    if (!f) {
        bklog::warning("GameView: write error on state file: {}", path);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Save failed (I/O)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    bklog::info("GameView: state saved to {} ({} bytes)", path, sz);
    char msg[64];
    snprintf(msg, sizeof(msg), "Saved (slot %d)", slot);
    std::lock_guard<std::mutex> lk(m_saveMsgMutex);
    m_saveMsg     = msg;
    m_saveMsgTime = std::chrono::steady_clock::now();
}

// ============================================================
// doQuickLoad – deserialize core state from a slot file
// ============================================================

void GameView::doQuickLoad(int slot)
{
    std::string path = quickSaveStatePath(slot);
    if (!std::filesystem::exists(path)) {
        bklog::warning("GameView: no state file at {} (slot {})", path, slot);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "No save in slot " + std::to_string(slot);
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        bklog::warning("GameView: cannot open state file: {}", path);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Load failed (I/O)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    f.seekg(0, std::ios::end);
    std::streampos fileSize = f.tellg();
    f.seekg(0, std::ios::beg);
    if (fileSize <= 0) {
        bklog::warning("GameView: state file is empty: {}", path);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Load failed (empty)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    std::vector<uint8_t> buf(static_cast<size_t>(fileSize));
    f.read(reinterpret_cast<char*>(buf.data()), fileSize);
    std::streamsize got = f.gcount();

    if (!m_core.unserialize(buf.data(), static_cast<size_t>(got))) {
        bklog::warning("GameView: unserialize failed (slot {})", slot);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Load failed";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    // Clear the rewind buffer to avoid confusion after a state restore
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        m_rewindBuffer.clear();
    }

    bklog::info("GameView: state loaded from {} ({} bytes)", path, got);
    char msg[64];
    snprintf(msg, sizeof(msg), "Loaded (slot %d)", slot);
    std::lock_guard<std::mutex> lk(m_saveMsgMutex);
    m_saveMsg     = msg;
    m_saveMsgTime = std::chrono::steady_clock::now();
}

// ============================================================
// loadCheats – parse and apply cheats from the .cht file
//
// Supported formats:
//
// 1. RetroArch .cht:
//    cheats = N
//    cheat0_desc = "Name"
//    cheat0_enable = true
//    cheat0_code = XXXXXXXX+YYYYYYYY
//
// 2. Simple one-code-per-line (enabled by default):
//    # comment
//    +XXXXXXXX YYYYYYYY   ('+' prefix = enabled)
//    -XXXXXXXX YYYYYYYY   ('-' prefix = disabled)
//    XXXXXXXX YYYYYYYY    (no prefix  = enabled)
// ============================================================

void GameView::loadCheats()
{
    std::string path = cheatFilePath();
    if (!std::filesystem::exists(path)) {
        bklog::info("GameView: no cheat file found at {}", path);
        return;
    }

    std::ifstream f(path);
    if (!f) {
        bklog::warning("GameView: failed to open cheat file: {}", path);
        return;
    }

    m_core.cheatReset();

    // Detect RetroArch format by checking for "cheats = " line
    std::string content;
    {
        std::ostringstream oss;
        oss << f.rdbuf();
        content = oss.str();
    }

    unsigned cheatCount = 0;

    if (content.find("cheats = ") != std::string::npos ||
        content.find("cheats=")   != std::string::npos)
    {
        // ---- RetroArch .cht format ----
        // Build a map of key → value
        std::unordered_map<std::string, std::string> kv;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            // Remove carriage return (Windows line endings)
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // Strip inline comments
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key   = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            // Trim whitespace
            auto trim = [](std::string s) -> std::string {
                size_t b = s.find_first_not_of(" \t\"");
                size_t e = s.find_last_not_of(" \t\"");
                if (b == std::string::npos) return "";
                return s.substr(b, e - b + 1);
            };
            kv[trim(key)] = trim(value);
        }

        // Parse cheat count
        unsigned total = 0;
        {
            auto it = kv.find("cheats");
            if (it != kv.end()) {
                try { total = static_cast<unsigned>(std::stoi(it->second)); } catch (...) {}
            }
        }

        for (unsigned i = 0; i < total; ++i) {
            std::string iStr = std::to_string(i);
            std::string descKey    = "cheat" + iStr + "_desc";
            std::string enableKey  = "cheat" + iStr + "_enable";
            std::string codeKey    = "cheat" + iStr + "_code";

            std::string code;
            bool        enabled = true;

            auto cit = kv.find(codeKey);
            if (cit == kv.end()) continue;
            code = cit->second;

            auto eit = kv.find(enableKey);
            if (eit != kv.end()) {
                std::string ev = eit->second;
                // lowercase
                for (char& c : ev) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                enabled = (ev == "true" || ev == "1" || ev == "yes");
            }

            std::string desc = "cheat" + iStr;
            auto dit = kv.find(descKey);
            if (dit != kv.end()) desc = dit->second;

            bklog::info("GameView: cheat[{}] \"{}\" {} code={}", i, desc,
                        enabled ? "enabled" : "disabled", code);
            m_core.cheatSet(i, enabled, code);
            ++cheatCount;
        }
    }
    else
    {
        // ---- Simple one-code-per-line format ----
        std::istringstream iss(content);
        std::string line;
        unsigned idx = 0;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // Strip inline comments
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            // Trim whitespace
            size_t b = line.find_first_not_of(" \t");
            if (b == std::string::npos) continue;
            line = line.substr(b);
            if (line.empty()) continue;

            bool enabled = true;
            if (line[0] == '+') {
                enabled = true;
                line = line.substr(1);
            } else if (line[0] == '-') {
                enabled = false;
                line = line.substr(1);
            }
            // Trim again after stripping prefix
            b = line.find_first_not_of(" \t");
            if (b == std::string::npos) continue;
            line = line.substr(b);
            if (line.empty()) continue;

            bklog::info("GameView: cheat[{}] {} code={}", idx,
                        enabled ? "enabled" : "disabled", line);
            m_core.cheatSet(idx, enabled, line);
            ++idx;
            ++cheatCount;
        }
    }

    if (cheatCount > 0)
        bklog::info("GameView: {} cheat(s) loaded from {}", cheatCount, path);
    else
        bklog::info("GameView: cheat file {} contained no valid cheats", path);
}

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
// loadOverlayImage – load the overlay PNG into an NVG image.
// Called lazily from draw() the first time the overlay is needed,
// and whenever m_overlayPath changes.
// ============================================================

void GameView::loadOverlayImage(NVGcontext* vg)
{
    // Release any existing overlay image
    if (m_overlayNvgImage >= 0) {
        nvgDeleteImage(vg, m_overlayNvgImage);
        m_overlayNvgImage = -1;
    }

    if (m_overlayPath.empty()) return;

    m_overlayNvgImage = nvgCreateImage(vg, m_overlayPath.c_str(), 0);
    if (m_overlayNvgImage < 0) {
        bklog::warning("GameView: failed to load overlay image: {}", m_overlayPath);
    } else {
        bklog::info("GameView: loaded overlay image: {}", m_overlayPath);
    }
}

// ============================================================
// pollInput – map gamepad state to libretro buttons and fire
//             emulator hotkeys via GameInputController.
//
// This function is the sole input processing path for the gamepad
// control system.  It runs on the game thread and reads exclusively
// from the thread-safe snapshot populated by refreshInputSnapshot()
// on the main thread, ensuring no platform input-manager calls are
// made from the game thread.
//
// Both gamepad and keyboard (keyboard.* config keys) game button bindings
// are processed here.  Emulator hotkey logic flows through
// GameInputController which fires Press / ShortPress / LongPress / Release
// callbacks registered in registerGamepadHotkeys().
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

    // ── GameInputController: process all registered gamepad hotkey combos ──
    // This handles FF hold/toggle, rewind hold/toggle, exit-game, and
    // quick-save/load actions via Press/ShortPress/LongPress/Release events.
    // Callbacks write to atomics and game-thread booleans.
    m_inputCtrl.update(state);

    // Disable fast-forward while rewinding
    if (m_rewinding.load(std::memory_order_relaxed))
        m_fastForward.store(false, std::memory_order_relaxed);

    // ── Game button mapping ───────────────────────────────────────────────
    // Map each configured gamepad/keyboard button to its libretro joypad ID.
    const auto& btnMap = m_inputMap.gameButtonMap();
    for (const auto& entry : btnMap)
    {
        bool pressed = false;
        // Gamepad
        if (entry.padButton >= 0 && entry.padButton < static_cast<int>(brls::_BUTTON_MAX))
            pressed = state.buttons[entry.padButton];
        // Keyboard (OR with gamepad so either input works)
        if (!pressed && entry.kbScancode >= 0) {
            auto it = snap.kbState.find(entry.kbScancode);
            if (it != snap.kbState.end())
                pressed = it->second;
        }
        m_core.setButtonState(entry.retroId, pressed);
    }

    // ── Debug logging ─────────────────────────────────────────────────────
    if (brls::Logger::getLogLevel() <= brls::LogLevel::LOG_DEBUG)
    {
        for (const auto& entry : btnMap)
        {
            bool padPressed = (entry.padButton >= 0 &&
                               entry.padButton < static_cast<int>(brls::_BUTTON_MAX) &&
                               state.buttons[entry.padButton]);
            bool kbPressed  = false;
            if (entry.kbScancode >= 0) {
                auto it = snap.kbState.find(entry.kbScancode);
                if (it != snap.kbState.end()) kbPressed = it->second;
            }
            if (padPressed || kbPressed)
                bklog::debug("pollInput: retroId={} pressed ({})",
                             entry.retroId, padPressed ? "pad" : "kbd");
        }
    }
}

// ============================================================
// refreshInputSnapshot – capture gamepad state on the main thread
// ============================================================
// Platform input-manager functions (updateUnifiedControllerState, etc.)
// must only be called from the main thread.  The game emulation thread
// calls pollInput() which reads from this thread-safe snapshot, thereby
// completely avoiding any off-thread input-manager calls.
//
// Only gamepad/controller state is captured here; keyboard is not part
// of the gamepad-only control system.
//
void GameView::refreshInputSnapshot()
{
    auto* platform = brls::Application::getPlatform();
    auto* im       = platform ? platform->getInputManager() : nullptr;
    if (!im) return;

    InputSnapshot snap;

    // ── Gamepad / controller state ───────────────────────────────────────────
    // updateUnifiedControllerState() reads the current hardware controller
    // state via the platform's input manager (GLFW on desktop, HID on Switch).
    // Must run on the main thread.
    im->updateUnifiedControllerState(&snap.ctrlState);

    // ── Keyboard state for game buttons ──────────────────────────────────────
    // Poll only the scancodes referenced by the current button map so we don't
    // hammer getKeyboardKeyState for every possible key every frame.
    for (const auto& entry : m_inputMap.gameButtonMap()) {
        if (entry.kbScancode >= 0) {
            snap.kbState[entry.kbScancode] =
                im->getKeyboardKeyState(
                    static_cast<brls::BrlsKeyboardScancode>(entry.kbScancode));
        }
    }

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

    // ---- ExitGame 热键：游戏线程设置此标志，在主线程处理 -----
    // 注意：所有平台均支持，Switch 平台不使用 blockInputs/unblockInputs
    if (m_requestExit.exchange(false, std::memory_order_relaxed)) {
        bklog::info("GameView: 收到退出请求，停止游戏线程...");
        // 必须在 join 游戏线程之前先反初始化音频，以解除可能阻塞在
        // pushSamples() 中的等待，否则 stopGameThread() 会死锁。
        beiklive::AudioManager::instance().deinit();
        stopGameThread();
#ifndef __SWITCH__
        // 立即解除 UI 输入封锁，使 popActivity() 后的视图可立即交互。
        if (m_uiBlocked) {
            brls::Application::unblockInputs();
            m_uiBlocked = false;
        }
#endif
        bklog::info("GameView: 游戏线程已停止，弹出 Activity");
        brls::Application::popActivity();
        return;
    }

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
                if (m_uiBlocked) {
                    brls::Application::unblockInputs();
                    m_uiBlocked = false;
                }
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

    // ---- Overlay (遮罩) drawing – full-screen, drawn on top of game frame
    if (m_overlayEnabled && !m_overlayPath.empty()) {
        // Lazy load: create NVG image handle on first use
        if (m_overlayNvgImage < 0) {
            loadOverlayImage(vg);
        }
        if (m_overlayNvgImage >= 0) {
            NVGpaint overlayPaint = nvgImagePattern(vg,
                                                    x, y,
                                                    width, height,
                                                    0.0f,
                                                    m_overlayNvgImage,
                                                    1.0f);
            nvgBeginPath(vg);
            nvgRect(vg, x, y, width, height);
            nvgFillPaint(vg, overlayPaint);
            nvgFill(vg);
        }
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

    // ---- Save / load status overlay ---------------------------------
    // Shows a brief message after quick-save or quick-load operations.
    // The message fades out after 2 seconds.
    {
        std::string msg;
        bool showMsg = false;
        {
            std::lock_guard<std::mutex> lk(m_saveMsgMutex);
            if (!m_saveMsg.empty()) {
                auto now = std::chrono::steady_clock::now();
                double age = std::chrono::duration<double>(now - m_saveMsgTime).count();
                if (age < 2.0) {
                    msg     = m_saveMsg;
                    showMsg = true;
                } else {
                    m_saveMsg.clear(); // expired
                }
            }
        }
        if (showMsg && !msg.empty()) {
            float mw = 200.0f, mh = 26.0f;
            float mx = x + width  * 0.5f - mw * 0.5f;
            float my = y + height - mh - 8.0f;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, mx, my, mw, mh, 5.0f);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 180));
            nvgFill(vg);

            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "regular");
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 230));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(vg, mx + mw * 0.5f, my + mh * 0.5f, msg.c_str(), nullptr);
        }
    }

    // ---- 静音状态覆盖层 --------------------------------------
    // 游戏静音时在屏幕右下角显示静音提示。
    if (m_muted.load(std::memory_order_relaxed)) {
        const char* muteText = "MUTE";
        float nw = 60.0f, nh = 22.0f;
        float nx = x + width  - nw - 4.0f;
        float ny = y + height - nh - 8.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, nx, ny, nw, nh, 4.0f);
        nvgFillColor(vg, nvgRGBA(180, 30, 30, 200));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 230));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, nx + nw * 0.5f, ny + nh * 0.5f, muteText, nullptr);
    }

    this->invalidate();
}

void GameView::onFocusGained()
{
    Box::onFocusGained();
    setGameInputEnabled(true);
}

void GameView::onFocusLost()
{
    Box::onFocusLost();
    setGameInputEnabled(false);
}

void GameView::onLayout()
{
    Box::onLayout();
}

