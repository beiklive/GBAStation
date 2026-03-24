#include "Game/game_view.hpp"
#include "Audio/AudioManager.hpp"

#include <borealis.hpp>
#include <borealis/core/application.hpp>

// ---- NanoVG 主头文件（通过 borealis.hpp 间接引入，此处无需额外包含 GL 扩展头文件）----

// ---- stb_image_write（用于截图 PNG 写入） -----------------------------------
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <limits>
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
// 游戏线程常量
// ============================================================
static constexpr double MAX_REASONABLE_FPS = 240.0; ///< 核心上报 FPS 的安全上限
static constexpr size_t STEREO_CHANNELS    = 2;     ///< 每音频帧采样数（左+右声道）
static constexpr double FPS_UPDATE_INTERVAL = 1.0;  ///< FPS 计数器更新间隔（秒）
// 自旋等待保护（从睡眠时间中减去，防止超时）
static constexpr double SPIN_GUARD_SEC = 0.002;     ///< 每帧 2ms 自旋等待预算
/// 菜单关闭后忽略游戏输入的帧数，防止关闭菜单的按键被透传到游戏核心。
static constexpr int INPUT_IGNORE_FRAMES_ON_MENU_CLOSE = 13;
/// RGBA 像素格式的通道数（红、绿、蓝、透明度）。
static constexpr int RGBA_CHANNELS = 4;

// ============================================================
// 解析 mgba_libretro 共享库路径
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
    #undef ABSOLUTE
    setPositionType(brls::PositionType::ABSOLUTE);
    setPositionTop(0);
    setPositionLeft(0);
    setWidthPercentage(100);
    setHeightPercentage(100);
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
// initialize – 延迟到首次 draw 时执行，确保 GL 上下文已就绪
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

    // ---- 优先从 gamedataManager 加载每游戏专属的偏移/缩放设置 ----
    // 若 gamedataManager 中存在对应字段，则覆盖 SettingManager 中的全局配置
    if (!m_romFileName.empty() && gamedataManager) {
        float xOff = getGameDataFloat(m_romFileName, GAMEDATA_FIELD_X_OFFSET, std::numeric_limits<float>::quiet_NaN());
        float yOff = getGameDataFloat(m_romFileName, GAMEDATA_FIELD_Y_OFFSET, std::numeric_limits<float>::quiet_NaN());
        float cs   = getGameDataFloat(m_romFileName, GAMEDATA_FIELD_CUSTOM_SCALE, std::numeric_limits<float>::quiet_NaN());
        // 仅当 gamedataManager 中有记录时才覆盖（以 NaN 表示"未设置"）
        if (!std::isnan(xOff)) m_display.xOffset    = xOff;
        if (!std::isnan(yOff)) m_display.yOffset    = yOff;
        if (!std::isnan(cs))   m_display.customScale = cs;

        // 从 gamedataManager 读取 display.mode / filter / integer_scale_mult（若存在则覆盖全局配置）
        {
            std::string modeStr = getGamedataOrSettingStr(m_romFileName,
                GAMEDATA_FIELD_DISPLAY_MODE, "display.mode", "");
            if (!modeStr.empty())
                m_display.mode = beiklive::DisplayConfig::stringToMode(modeStr);
        }
        {
            std::string filtStr = getGamedataOrSettingStr(m_romFileName,
                GAMEDATA_FIELD_DISPLAY_FILTER, "display.filter", "");
            if (!filtStr.empty())
                m_display.filterMode = beiklive::DisplayConfig::stringToFilterMode(filtStr);
        }
        {
            int mult = getGamedataOrSettingInt(m_romFileName,
                GAMEDATA_FIELD_DISPLAY_INT_SCALE, "display.integer_scale_mult", -1);
            if (mult >= 0)
                m_display.integerScaleMult = mult;
        }
    }

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

        // ---- 存档/即时存档目录默认值 --------------------------------
        cfg->SetDefault("save.sramDir",         CV(std::string("")));
        cfg->SetDefault("save.stateDir",         CV(std::string("")));
        cfg->SetDefault("save.slotCount",        CV(9));
        cfg->SetDefault("save.autoLoadState0",   CV(std::string("false")));
        cfg->SetDefault("save.autoSaveInterval", CV(0));
        cfg->SetDefault("cheat.dir",             CV(std::string("")));

        // ---- FPS 显示默认值 ------------------------------------
        cfg->SetDefault("display.showFps",           CV(std::string("false")));
        cfg->SetDefault("display.showFfOverlay",     CV(std::string("false")));
        cfg->SetDefault("display.showRewindOverlay", CV(std::string("false")));
        cfg->SetDefault("display.showMuteOverlay",   CV(std::string("true")));
        cfg->SetDefault(KEY_DISPLAY_OVERLAY_ENABLED,  CV(std::string("false")));
        cfg->SetDefault(KEY_DISPLAY_OVERLAY_GBA_PATH, CV(std::string("")));
        cfg->SetDefault(KEY_DISPLAY_OVERLAY_GBC_PATH, CV(std::string("")));
        // ---- 着色器预设路径（空=直通，不使用着色器）与开关 --------
        cfg->SetDefault(KEY_DISPLAY_SHADER_PATH,        CV(std::string("")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_ENABLED,     CV(std::string("false")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_GBA_PATH,    CV(std::string("")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_GBA_ENABLED, CV(std::string("false")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_GBC_PATH,    CV(std::string("")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_GBC_ENABLED, CV(std::string("false")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_GB_PATH,     CV(std::string("")));
        cfg->SetDefault(KEY_DISPLAY_SHADER_GB_ENABLED,  CV(std::string("false")));

        // ---- 按键映射默认值（委托给 InputMappingConfig）----
        m_inputMap.setDefaults(*cfg);

        cfg->Save();
        m_core.setConfigManager(cfg);

        // ---- 读取显示覆盖层开关标志 ----------------------------------
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
        m_showMuteOverlay   = getBool("display.showMuteOverlay",   true);
        m_overlayEnabled    = getBool(KEY_DISPLAY_OVERLAY_ENABLED, false);

        // ---- 解析当前游戏的覆盖层路径 --------------------------
        // 1. 优先使用 gamedataManager 中的逐游戏覆盖层路径。
        // 2. 回退到全局 GBA / GBC 路径（根据 ROM 扩展名选择）。
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

    // ---- 加载 libretro 核心 -------------------------------------------
    std::string corePath = resolveCoreLibPath();
    bklog::info("Loading libretro core: {}", corePath);

    // ---- 配置存档位置core -----------------------
    {
        std::string sramCustomDir;
        if (gameRunner && gameRunner->settingConfig) {
            auto v = gameRunner->settingConfig->Get("save.sramDir");
            if (v) { if (auto s = v->AsString()) sramCustomDir = *s; }
        }
        // 读取用户自定义 SRAM 目录并确保其存在，然后传给核心。
        // 核心会在该目录下按游戏名创建子目录来存储 SRAM 文件。
        std::string sramDir = resolveSaveDir(m_romPath, sramCustomDir);
        if (!sramDir.empty()) {
            // 确保目录存在
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

        // ---- 将金手指列表传递给 GameMenu 并注册切换回调 --------
        if (m_gameMenu) {
            m_gameMenu->setCheats(m_cheats);
            m_gameMenu->setCheatToggleCallback([this](int idx, bool enabled) {
                // 由 GameMenu（UI 线程）在游戏暂停时调用，直接更新核心
                if (idx >= 0 && idx < static_cast<int>(m_cheats.size())) {
                    m_cheats[idx].enabled = enabled;
                    updateCheats();
                }
            });
        }

        m_core.reset(); // 加载存档/金手指后确保核心从干净状态启动

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

    // ---- 创建视频输出 GL 纹理 ----------------------------
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    // 根据显示配置应用过滤模式（最近邻或线性）
    m_activeFilter     = m_display.filterMode;
    GLenum glFilter    = (m_activeFilter == beiklive::FilterMode::Nearest)
                         ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 用游戏尺寸预分配纹理，使用 GL_RGBA（通用兼容格式）
    unsigned gw = m_core.gameWidth()  > 0 ? m_core.gameWidth()  : 256;
    unsigned gh = m_core.gameHeight() > 0 ? m_core.gameHeight() : 224;
    std::vector<uint32_t> blank(gw * gh, 0xFF000000u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(gw),
                 static_cast<GLsizei>(gh), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 blank.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_texWidth  = gw;
    m_texHeight = gh;

    // ---- 初始化渲染链（含可选着色器管线） --------------------------
    {
        // 优先从 gamedataManager 读取游戏专属着色器设置
        std::string enabledStr = getGamedataOrSettingStr(m_romFileName,
            GAMEDATA_FIELD_SHADER_ENABLED, KEY_DISPLAY_SHADER_ENABLED, "false");
        bool shaderEnabled = (enabledStr == "true" || enabledStr == "1");
        std::string shaderPath = getGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_PATH, "");

        // 若游戏无专属着色器路径，根据 ROM 扩展名选择平台对应的全局着色器
        if (shaderPath.empty() && !m_romFileName.empty()) {
            const std::string ext = beiklive::string::getFileSuffix(m_romFileName);
            if (beiklive::string::iequals(ext, "gba")) {
                shaderPath = beiklive::cfgGetStr(KEY_DISPLAY_SHADER_GBA_PATH, "");
            } else if (beiklive::string::iequals(ext, "gbc")) {
                shaderPath = beiklive::cfgGetStr(KEY_DISPLAY_SHADER_GBC_PATH, "");
            } else if (beiklive::string::iequals(ext, "gb")) {
                shaderPath = beiklive::cfgGetStr(KEY_DISPLAY_SHADER_GB_PATH, "");
            }
            // 将平台对应的着色器路径回写到 gamedata，使游戏拥有专属路径记录
            if (!shaderPath.empty()) {
                setGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_PATH, shaderPath);
            }
            // 若平台路径也为空，回退到全局通用着色器路径
            if (shaderPath.empty()) {
                shaderPath = beiklive::cfgGetStr(KEY_DISPLAY_SHADER_PATH, "");
            }
        }

        // 仅当开关开启且路径非空时加载着色器
        std::string effectivePath = (shaderEnabled && !shaderPath.empty()) ? shaderPath : "";
        bklog::info("GameView: 着色器开关={}, 路径='{}', 生效路径='{}'",
                    shaderEnabled, shaderPath, effectivePath);
        if (!m_renderChain.init(effectivePath)) {
            bklog::warning("RenderChain init failed; using direct texture rendering");
        }
        // 渲染链初始化完成后立即刷新菜单中的着色器参数列表（含游戏重启场景）
        if (m_gameMenu)
            m_gameMenu->updateShaderParams(m_renderChain.getShaderParams());
    }

    // ---- 启动音频管理器 ------------------------------------------
    if (!beiklive::AudioManager::instance().init(32768, 2)) {
        bklog::warning("AudioManager init failed; game will run without audio");
    }

    m_initialized = true;
    bklog::info("GameView initialized successfully");

    // ---- 启动独立的模拟器线程 ---------------------------
    startGameThread();
}

// ============================================================
// startGameThread – 在新线程中启动模拟循环
// ============================================================

void GameView::startGameThread()
{
    m_fpsLastTime    = std::chrono::steady_clock::now();
    m_fpsFrameCount  = 0;
    m_currentFps     = 0.0f;

    // 配置音频背压阈值：约 4 帧音频量。
    // 保持环形缓冲区轻量，防止延迟随时间累积。
    {
        double coreFps = m_core.fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS) coreFps = 60.0;
        size_t maxLatencyFrames = static_cast<size_t>(32768.0 / coreFps * 4.0 + 0.5);
        beiklive::AudioManager::instance().setMaxLatencyFrames(maxLatencyFrames);
    }

    m_running.store(true, std::memory_order_release);
    m_gameThread = std::thread([this]() {
#ifdef __SWITCH__
        // 将游戏模拟线程绑定到核心 1（专用核心）。
        // 核心 0 = UI/主线程，核心 1 = 游戏模拟（含音频）。
        svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
#endif
        double fps = m_core.fps();
        if (fps <= 0.0 || fps > MAX_REASONABLE_FPS) fps = 60.0;

        using Clock    = std::chrono::steady_clock;
        using Duration = std::chrono::duration<double>;
        const Duration frameDuration(1.0 / fps);
        // 保护时长：从粗粒度睡眠中减去，让自旋等待准时结束
        const Duration spinGuard(SPIN_GUARD_SEC);

#ifdef _WIN32
        // 请求 1ms Windows 定时器精度，确保 sleep_for() 准确
        timeBeginPeriod(1);
#endif

        // 线程内 FPS 计数器
        Clock::time_point fpsTimerStart = Clock::now();
        unsigned          fpsCounter    = 0;

        // 线程内游戏时长追踪：每 60 秒保存一次总游戏时长。
        Clock::time_point playtimeTimer = Clock::now();

        // RTC 同步计时器：每秒向核心 RTC 内存写入当前 Unix 时间。
        Clock::time_point rtcSyncTimer = Clock::now();

        // 自动存档计时器：定时保存到即时存档0（.ss0）
        Clock::time_point autoSaveTimer = Clock::now();

        // 累积理想帧结束时间，用于无漂移的帧率控制。
        // 每次迭代加一个 frameDuration，防止慢帧压缩下一帧预算。
        // 初始化为 Clock::now()，确保第一帧获得完整的帧时长预算。
        auto nextFrameTarget = Clock::now();

#ifdef __SWITCH__
        // 追踪快进状态，用于检测快进→正常的切换时机。
        bool prevFastForward = false;
#endif

        while (m_running.load(std::memory_order_acquire)) {
            // 轮询手柄输入并转发给核心
            pollInput();  // 处理输入事件

            bool ff      = m_fastForward.load(std::memory_order_relaxed);
            bool rew     = m_rewinding.load(std::memory_order_relaxed);
            bool paused  = m_paused.load(std::memory_order_relaxed);

            // 通知核心当前快进状态，使其能正确响应
            // RETRO_ENVIRONMENT_GET_FASTFORWARDING 查询。
            m_core.setFastForwarding(ff && !paused);

#ifdef __SWITCH__
            // 快进结束时，清空环形缓冲区中的残留音频样本，
            // 防止它们以正常速度回放（会产生噪音或突发的加速音频）。
            if (!paused && prevFastForward && !ff) {
                beiklive::AudioManager::instance().flushRingBuffer();
            }
            prevFastForward = paused ? false : ff;
#endif

            // framesThisIter：本次迭代渲染的逻辑帧数，用于 FPS 计数。
            unsigned framesThisIter = 1u;

            if (paused) {
                // ---- 暂停状态：清空所有游戏按键，跳过核心运行和音频推送 ----
                // 清空游戏按键状态，防止恢复后残留按键触发游戏操作。
                for (unsigned id = 0; id <= RETRO_DEVICE_ID_JOYPAD_R3; ++id)
                    m_core.setButtonState(id, false);
            } else if (rew && m_inputMap.rewindEnabled) {
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
                    m_core.run();  // 运行核心以更新视频输出，保证倒带流畅。
                }
                // 排空音频（根据配置决定静音或透传）
                {
                    std::vector<int16_t> dummy;
                    bool hasSamples = m_core.drainAudio(dummy) && !dummy.empty();
                    if (!m_inputMap.rewindMute && hasSamples) {
                        size_t frames = dummy.size() / STEREO_CHANNELS;
                        beiklive::AudioManager::instance().pushSamples(dummy.data(), frames);
                    }
                }
            } else {
                // ---- 正常或快进：运行核心 --------------------
                // 计算本次迭代运行的帧数。
                // 快进倍率 N 时，每个正常帧周期运行 N 帧，实现精确 N 倍速。
                // 倍率 < 1 时，仍运行 1 帧但拉长睡眠时间。
                if (ff) {
                    framesThisIter = (m_inputMap.ffMultiplier >= 1.0f)
                        ? static_cast<unsigned>(std::round(m_inputMap.ffMultiplier))
                        : 1u;
                }

                for (unsigned i = 0; i < framesThisIter; ++i) {
                    // 每帧前保存状态到倒带缓冲区（快进时同样保存）
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

                // 排空音频样本并直接推送到 AudioManager。
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
                        // 快进（倍率 > 1）且不静音时，限制推送的音频量为一正常帧的量。
                        // 每次循环运行 N 帧会产生 N 倍音频，否则会撑满环形缓冲区，
                        // 导致 pushSamples() 永久阻塞，冻结游戏线程。
                        if (ff && m_inputMap.ffMultiplier > 1.0f) {
                            // 先用浮点计算再取整，保证精度。
                            size_t limit = static_cast<size_t>(
                                std::round(static_cast<double>(frames) / m_inputMap.ffMultiplier));
                            // 若取整为零说明总量极小，直接全部推送。
                            if (limit > 0)
                                frames = limit;
                        }
                        beiklive::AudioManager::instance().pushSamples(samples.data(), frames);
                    }
                }
            }

            // ---- FPS 计数器（游戏线程侧）-------------------------
            // 暂停时不计帧数，保持 FPS 覆盖层显示上一次的有效值。
            auto nowPost = Clock::now(); // 捕获一次，供后续睡眠复用
            if (!paused) {
                // 统计所有渲染帧数（含快进倍增的帧）。
                fpsCounter += framesThisIter;
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

                // 更新游戏运行时长，每 1 分钟保存一次
                if (gamedataManager && !m_romFileName.empty()) {
                    auto playtimeElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        nowPost - playtimeTimer).count();
                    if (playtimeElapsed >= 60) {
                        // 精确推进 60 秒，防止线程挂起或检查延迟导致多计时间。
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

                // RTC 实时同步：保持 GB MBC3 RTC 存档缓冲区中的 unixTime 字段最新。
                // GBMBCRTCSaveBuffer::unixTime 位于字节偏移 40（10 × uint32_t），
                // GBMBCRTCRead 在下次游戏加载时会将其读入 rtcLastLatch。
                // 保持此字段更新可确保存档重载后经过时间计算正确。
                {
                    auto rtcElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        nowPost - rtcSyncTimer).count();
                    if (rtcElapsed >= 1) {
                        rtcSyncTimer += std::chrono::seconds(1);
                        // GBMBCRTCSaveBuffer::unixTime 偏移量为 40 字节。
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
                // 同时把原子变量重置为 -1（表示"无待处理请求"）。
                {
                    int slot = m_pendingQuickSave.exchange(-1, std::memory_order_relaxed);
                    if (slot >= 0) doQuickSave(slot);
                }
                {
                    int slot = m_pendingQuickLoad.exchange(-1, std::memory_order_relaxed);
                    if (slot >= 0) doQuickLoad(slot);
                }

                // ---- 重置游戏核心 --------
                if (m_pendingReset.exchange(false, std::memory_order_relaxed)) {
                    bklog::info("GameView: 执行游戏核心重置");
                    m_core.reset();
                }

                // ---- 自动定时存档（定期保存到即时存档0 .ss0） --------
                {
                    int autoSaveIntervalSec = beiklive::cfgGetInt("save.autoSaveInterval", 0);
                    if (autoSaveIntervalSec > 0) {
                        auto autoSaveElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            nowPost - autoSaveTimer).count();
                        if (autoSaveElapsed >= autoSaveIntervalSec) {
                            // 重置计时器（以自动存档间隔为单位推进，避免漂移）
                            autoSaveTimer += std::chrono::seconds(autoSaveIntervalSec);
                            // 静默模式：自动存档不在游戏界面显示保存 overlay
                            doQuickSave(0, /*silent=*/true);
                        }
                    }
                }
            } else {
                // 暂停时推进各计时器基准点，避免恢复后触发积压的时间操作
                playtimeTimer = nowPost;
                rtcSyncTimer  = nowPost;
                autoSaveTimer = nowPost;
                fpsTimerStart = nowPost;
            }

            // 帧率限制器
            {
                Duration targetDur = frameDuration;
                if (!paused && ff && m_inputMap.ffMultiplier < 1.0f) {
                    targetDur = Duration(1.0 / (fps * static_cast<double>(m_inputMap.ffMultiplier)));
                }

                // 向前推进一帧的目标时间。
                nextFrameTarget += std::chrono::duration_cast<Clock::duration>(targetDur);

                // 防漂移：若帧超时，将目标重置为 now，
                // 使下一帧获得全新的完整预算，而非调度到过去的时刻。
                if (nextFrameTarget < nowPost) {
                    nextFrameTarget = nowPost;
                }

                if (nowPost < nextFrameTarget) {
                    // 粗粒度睡眠（预留自旋等待时间）
                    auto coarseDur = (nextFrameTarget - nowPost) - spinGuard;
                    if (coarseDur.count() > 0)
                        std::this_thread::sleep_for(coarseDur);
                    // 精确等待截止时间
                    while (Clock::now() < nextFrameTarget) { /* 忙等待 */ }
                }
            }
        }

#ifdef _WIN32
        timeEndPeriod(1);
#endif
    });
}

// ============================================================
// stopGameThread – 通知并等待模拟线程退出
// ============================================================

void GameView::stopGameThread()
{
    m_running.store(false, std::memory_order_release);
    if (m_gameThread.joinable()) {
        m_gameThread.join();
    }
}

// ============================================================
// cleanup – 释放所有资源
// ============================================================

void GameView::cleanup()
{
    // 若 UI 输入封锁仍持有（如析构时未经 onFocusLost），则释放。
    // 正常情况不应发生，此处防御异常析构顺序。
#ifndef __SWITCH__
    if (m_uiBlocked) {
        brls::Application::unblockInputs();
        m_uiBlocked = false;
    }
#endif

    // 在 join 游戏线程前先通知 AudioManager 停止，唤醒可能阻塞在
    // pushSamples() 的等待者，避免 stopGameThread() 死锁。
    beiklive::AudioManager::instance().deinit();

    // 停止并等待模拟线程退出（音频在线程内调用）
    stopGameThread();

    // 清空倒带缓冲区
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        m_rewindBuffer.clear();
    }

    // 释放覆盖层 NVG 图像句柄（NVG 上下文销毁时自动删除纹理，此处仅重置句柄）。
    if (m_overlayNvgImage >= 0) {
        m_overlayNvgImage = -1;
    }

    // 反初始化渲染链（释放 GL 资源）
    m_renderChain.deinit();

    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    if (m_core.isLoaded()) {
        // 若从菜单退出且开启了自动保存（save.autoSaveState），在卸载前保存即时存档0。
        // 使用 exchange 原子地读取并清除标志，防止重复执行（即使 cleanup 被多次调用）。
        if (m_autoSaveOnExit.exchange(false, std::memory_order_relaxed)) {
            bklog::info("GameView: cleanup 中执行退出自动保存 (slot 0)");
            doQuickSave(0, /*silent=*/true);
        }
        // 卸载游戏前保存 SRAM（电池存档）
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
// setGameMenu – 绑定游戏内菜单并设置关闭回调
//
// 当菜单的"返回游戏"按钮被触发时，关闭回调将恢复游戏输入并将焦点返回给 GameView。
// 必须在主（UI）线程调用（通常在 initialize() 之前）。
// ============================================================

void GameView::setGameMenu(GameMenu* menu)
{
    m_gameMenu = menu;
    if (m_gameMenu) {
        m_gameMenu->setCloseCallback([this]() {
            // 菜单关闭：恢复游戏运行，启用游戏输入封锁（阻止 UI 事件），将焦点返回给 GameView
            // 设置若干帧的输入忽略计数，防止关闭菜单的按键（如 B 键）透传到游戏核心
            m_inputIgnoreFrames.store(INPUT_IGNORE_FRAMES_ON_MENU_CLOSE, std::memory_order_relaxed);
            setPaused(false);
            setGameInputEnabled(true);
            brls::Application::giveFocus(this);
        });
        // 退出游戏回调：若开启了 save.autoSaveState（每次退出游戏时自动保存即时存档0），
        // 则标记退出时自动保存（cleanup() 中在停止游戏线程后执行），再弹出 Activity
        m_gameMenu->setExitGameCallback([this]() {
            if (beiklive::cfgGetBool("save.autoSaveState", false)) {
                m_autoSaveOnExit.store(true, std::memory_order_relaxed);
                bklog::info("GameView: 退出游戏时将自动保存即时存档0");
            }
            brls::Application::popActivity();
        });
        // 重置游戏回调：标记游戏线程在下一帧执行 core.reset()
        m_gameMenu->setResetGameCallback([this]() {
            m_pendingReset.store(true, std::memory_order_relaxed);
        });
        // 遮罩路径变更时立即更新 m_overlayPath 并标记重新加载，
        // draw() 下一帧将在菜单仍打开期间预加载新纹理，恢复游戏无卡顿
        m_gameMenu->setOverlayChangedCallback([this](const std::string& newPath) {
            m_overlayPath = newPath;
            m_overlayReloadPending.store(true, std::memory_order_release);
        });
        // 遮罩开关变更时立即更新 m_overlayEnabled，确保设置立即生效无需重启游戏
        m_gameMenu->setOverlayEnabledChangedCallback([this](bool enabled) {
            m_overlayEnabled = enabled;
            if (enabled && m_overlayPath.empty() && !m_romFileName.empty()) {
                // 启用时若路径尚未加载，则尝试从 gamedataManager 读取
                std::string perGame = getGameDataStr(m_romFileName, GAMEDATA_FIELD_OVERLAY, "");
                if (!perGame.empty()) {
                    m_overlayPath = perGame;
                    m_overlayReloadPending.store(true, std::memory_order_release);
                }
            }
        });
        // 着色器开关变更时立即更新渲染链（即时生效）并同步参数滑条
        m_gameMenu->setShaderEnabledChangedCallback([this](bool enabled) {
            bklog::info("GameView: 着色器开关变更 → {}", enabled);
            // 优先使用游戏专属着色器路径，回退到全局配置，避免跨游戏路径污染
            std::string path = getGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_PATH, "");
            if (path.empty())
                path = beiklive::cfgGetStr(KEY_DISPLAY_SHADER_PATH, "");
            std::string effectivePath = (enabled && !path.empty()) ? path : "";
            m_renderChain.setShader(effectivePath);
            // 同步参数滑条（切换后管线重建，参数列表已变更）
            if (m_gameMenu)
                m_gameMenu->updateShaderParams(m_renderChain.getShaderParams());
        });
        // 着色器路径变更时立即更新渲染链（即时生效）并更新菜单中的参数滑条
        m_gameMenu->setShaderPathChangedCallback([this](const std::string& newPath) {
            bklog::info("GameView: 着色器路径变更 → '{}'", newPath);
            bool enabled = beiklive::cfgGetBool(KEY_DISPLAY_SHADER_ENABLED, false);
            std::string effectivePath = (enabled && !newPath.empty()) ? newPath : "";
            m_renderChain.setShader(effectivePath);
            // 更新菜单中的着色器参数滑条
            if (m_gameMenu)
                m_gameMenu->updateShaderParams(m_renderChain.getShaderParams());
        });
        // 显示模式变更时立即更新 m_display
        m_gameMenu->setDisplayModeChangedCallback([this](beiklive::ScreenMode mode) {
            m_display.mode = mode;
        });
        // 纹理过滤变更时立即更新 m_display（draw() 会在下一帧应用）
        m_gameMenu->setDisplayFilterChangedCallback([this](beiklive::FilterMode filter) {
            m_display.filterMode = filter;
        });
        // 整数缩放倍率变更时立即更新 m_display
        m_gameMenu->setDisplayIntScaleChangedCallback([this](int mult) {
            m_display.integerScaleMult = mult;
        });
        // X 坐标偏移实时更新 m_display（拖动滑条时连续调用）
        m_gameMenu->setDisplayXOffsetChangedCallback([this](float v) {
            m_display.xOffset = v;
        });
        // Y 坐标偏移实时更新 m_display
        m_gameMenu->setDisplayYOffsetChangedCallback([this](float v) {
            m_display.yOffset = v;
        });
        // 自定义缩放实时更新 m_display
        m_gameMenu->setDisplayCustomScaleChangedCallback([this](float v) {
            m_display.customScale = v;
        });
        // 着色器参数实时更新渲染链
        m_gameMenu->setShaderParamChangedCallback([this](const std::string& name, float value) {
            m_renderChain.setShaderParam(name, value);
        });
        // 保存状态回调：设置待处理槽号，游戏线程下一帧执行实际存档
        m_gameMenu->setSaveStateCallback([this](int slot) {
            m_pendingQuickSave.store(slot, std::memory_order_relaxed);
        });
        // 读取状态回调：设置待处理槽号，游戏线程下一帧执行实际读档
        m_gameMenu->setLoadStateCallback([this](int slot) {
            m_pendingQuickLoad.store(slot, std::memory_order_relaxed);
        });
        // 状态槽位信息查询回调：检查状态文件是否存在，返回缩略图路径
        // 说明：libretro 接口（retro_serialize）保存的状态为二进制格式，不包含截图。
        // 因此我们在 doQuickSave 时额外保存一个 .ss*.png 缩略图文件。
        // 若状态文件本身是 PNG 格式（mGBA 原生 GUI 以 SAVESTATE_SCREENSHOT 保存的格式），
        // 则直接将该状态文件用作缩略图，无需独立的 .png 文件。
        m_gameMenu->setStateInfoCallback([this](int slot) -> GameMenu::StateSlotInfo {
            GameMenu::StateSlotInfo info;
            std::string statePath = quickSaveStatePath(slot);
            if (!std::filesystem::exists(statePath)) {
                return info;
            }
            info.exists = true;
            // 优先检查状态文件本身是否为 PNG（mGBA 原生格式，状态即截图）
            static constexpr uint8_t k_pngSig[8] = {0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A};
            {
                std::ifstream sf(statePath, std::ios::binary);
                uint8_t hdr[8] = {};
                sf.read(reinterpret_cast<char*>(hdr), 8);
                if (sf && sf.gcount() == 8) {
                    if (std::memcmp(hdr, k_pngSig, 8) == 0) {
                        // 状态文件本身就是包含截图的 PNG（mGBA 原生格式）
                        info.thumbPath = statePath;
                        return info;
                    }
                }
            }
            // 二进制格式（libretro 路径）：查找配套缩略图文件
            std::string thumbPath = statePath + ".png";
            if (std::filesystem::exists(thumbPath))
                info.thumbPath = thumbPath;
            return info;
        });
        // 初始化着色器参数滑条（使用当前已加载的着色器参数）
        m_gameMenu->updateShaderParams(m_renderChain.getShaderParams());
        // 初始化 X/Y 偏移和自定义缩放滑条（使用 m_display 当前值）
        m_gameMenu->updateDisplayOffsetSliders(
            m_display.xOffset, m_display.yOffset, m_display.customScale);
    }
}


// ============================================================
// setGameInputEnabled – 一键切换 borealis 输入封锁
// ============================================================
// enabled = true：封锁 borealis UI 事件分发，防止导航/提示/动画干扰游戏；
//   GameInputController 启用，可分发热键事件。
// enabled = false：解除封锁，GameInputController 暂停（重置按键状态，防止误触）。
// 传入与当前状态相同的值时为幂等操作。
//
// 线程安全：必须在主（UI）线程调用。
// ============================================================

void GameView::setGameInputEnabled(bool enabled)
{
#ifndef __SWITCH__
    if (enabled && !m_uiBlocked)
    {
        brls::Application::blockInputs(true); // true = 静音 UI 点击错误音效
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
// setPaused – 暂停或恢复游戏运行
// ============================================================
// 暂停时游戏线程跳过核心执行和音频推送，所有游戏按键状态清零。
// 主要供打开/关闭 GameMenu 时调用，防止菜单导航输入影响游戏核心。
//
// 线程安全：必须在主（UI）线程调用。
// ============================================================

void GameView::setPaused(bool paused)
{
    m_paused.store(paused, std::memory_order_relaxed);
}


// ============================================================
// registerGamepadHotkeys – 将 m_inputCtrl 绑定到模拟器热键
//
// 在 initialize() 加载 m_inputMap 后调用。
// 每个热键绑定注册为 GameInputController 动作，
// 自动追踪 Press/ShortPress/LongPress/Release 事件，无需 borealis 动作处理器。
// ============================================================

void GameView::registerGamepadHotkeys()
{
    m_inputCtrl.clear();

    using Hotkey = beiklive::InputMappingConfig::Hotkey;
    using KeyEvent = beiklive::GameInputController::KeyEvent;

    // 辅助函数：若热键已绑定则注册手柄按键组合。
    auto reg = [&](Hotkey h, beiklive::GameInputController::Callback cb)
    {
        const auto& hk = m_inputMap.hotkeyBinding(h);
        if (hk.isPadBound())
            m_inputCtrl.registerAction(hk.padButtons, std::move(cb));
    };
    // -- 打开菜单
    reg(Hotkey::OpenMenu, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress) {
            // 菜单操作须在主线程执行（涉及 UI 输入封锁和焦点切换），
            // 通过原子标志将请求传递给 draw() 处理。
            m_requestOpenMenu.store(true, std::memory_order_relaxed);
        }
    });

    // ── 快进──────────────────────────────────────────────────────
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

    // ── 倒带────────────────────────────────────────────────────────
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

    // ── 退出游戏 ────────────────────────────────────────────────────────────
    reg(Hotkey::ExitGame, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress && !m_requestExit.load(std::memory_order_relaxed))
            m_requestExit.store(true, std::memory_order_relaxed);
    });

    // ── 快速保存即时存档 ─────────────────────────────────────────────────────
    reg(Hotkey::QuickSave, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress)
            m_pendingQuickSave.store(m_saveSlot, std::memory_order_relaxed);
    });

    // ── 快速读取即时存档 ─────────────────────────────────────────────────────
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

    // ── 截图 ─────────────────────────────────────────────────────────────────
    reg(Hotkey::Screenshot, [this](KeyEvent evt)
    {
        if (evt == KeyEvent::ShortPress)
            m_pendingScreenshot.store(true, std::memory_order_relaxed);
    });
}

// ============================================================
// resolveSaveDir – 确定存档文件目录
// ============================================================

std::string GameView::resolveSaveDir(const std::string& romPath,
                                      const std::string& customDir)
{
    if (!customDir.empty()) {
        // 指定了模拟器目录时，在其下创建以游戏文件名命名的子目录，防止存档文件堆积
        if (!romPath.empty()) {
            std::filesystem::path p(romPath);
            return (std::filesystem::path(customDir) / p.stem()).string();
        }
        return customDir;
    }
    // 默认：与 ROM 同目录
    if (!romPath.empty()) {
        return beiklive::file::getParentPath(romPath);
    }
    return BK_APP_ROOT_DIR + std::string("saves");
}

// ============================================================
// quickSaveStatePath – 计算即时存档文件路径
// ============================================================

std::string GameView::quickSaveStatePath(int slot) const
{
    std::string stateCustomDir;
    if (gameRunner && gameRunner->settingConfig) {
        auto v = gameRunner->settingConfig->Get("save.stateDir");
        if (v) { if (auto s = v->AsString()) stateCustomDir = *s; }
    }
    std::string dir = resolveSaveDir(m_romPath, stateCustomDir);

    // 提取不含扩展名的 ROM 文件名
    std::string base;
    if (!m_romPath.empty()) {
        std::filesystem::path p(m_romPath);
        base = p.stem().string();
    } else {
        base = "game";
    }

    // 确保目录存在
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
// sramSavePath – 计算电池存档（.sav）文件路径
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

    // 确保目录存在
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
// rtcSavePath – 计算 GB MBC3 RTC（.rtc）文件路径
// ============================================================
// RTC 文件与 SRAM 文件存放在同一目录，方便集中管理。

std::string GameView::rtcSavePath() const
{
    std::string customDir;
    if (gameRunner && gameRunner->settingConfig) {
        // 复用 save.sramDir：.rtc 文件与 .sav 文件同目录，保持存档集中。
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
// cheatFilePath – 计算金手指（.cht）文件路径
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
// loadSram – 从磁盘加载 SRAM 到核心
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
// saveSram – 将核心 SRAM 保存到磁盘
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
// loadRtc – 从磁盘加载 GB MBC3 RTC 数据到核心
// ============================================================

void GameView::loadRtc()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_RTC);
    if (sz == 0) {
        return; // 核心无 RTC 区域（非 GB MBC3 游戏）
    }

    // GBMBCRTCSaveBuffer 内存布局（mGBA）：
    //   偏移  0: sec         (uint32)
    //   偏移  4: min         (uint32)
    //   偏移  8: hour        (uint32)
    //   偏移 12: days        (uint32)
    //   偏移 16: daysHi      (uint32)
    //   偏移 20: latchedSec  (uint32)
    //   偏移 24: latchedMin  (uint32)
    //   偏移 28: latchedHour (uint32)
    //   偏移 32: latchedDays (uint32)
    //   偏移 36: latchedDaysHi (uint32)
    //   偏移 40: unixTime    (uint64)  ← GBMBCRTCRead 将其读入 rtcLastLatch
    static constexpr size_t k_rtcUnixTimeOffset = 10 * sizeof(uint32_t); // = 40

    std::string path = rtcSavePath();
    if (!std::filesystem::exists(path)) {
        // 无存档文件时，以当前墙钟时间初始化 unixTime，
        // 确保 GBMBCRTCRead 正确初始化 rtcLastLatch，
        // 避免从 0xFF 填充缓冲区计算出巨大的错误经过时间。
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
// saveRtc – 将核心 GB MBC3 RTC 数据保存到磁盘
// ============================================================

void GameView::saveRtc()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_RTC);
    if (sz == 0) {
        return; // 核心无 RTC 区域（非 GB MBC3 游戏）
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
// doQuickSave – 将核心状态序列化到指定槽位文件
//
// 状态文件格式说明：
//   mGBA 有两种状态保存路径：
//   1. 原生 GUI 路径（gui-runner.c）：调用 mCoreSaveState() 时传入 SAVESTATE_SCREENSHOT
//      标志，状态文件保存为 PNG 格式。PNG 像素数据即为游戏截图，
//      游戏核心状态以 zlib 压缩后存储于自定义 PNG 块 "gbAs" 中，
//      扩展数据（savedata、RTC 等）存储于 "gbAx" 块中。
//      此格式状态文件本身就是截图，无需额外缩略图文件。
//   2. libretro 路径（libretro.c：retro_serialize）：调用 mCoreSaveStateNamed()
//      时仅传入 SAVESTATE_SAVEDATA | SAVESTATE_RTC，不含截图标志。
//      状态文件为二进制格式，不包含截图信息。
//
//   本项目通过 libretro 接口调用，状态文件为二进制格式，因此在保存状态后
//   额外保存一个同名 .png 缩略图文件（路径 = 状态文件路径 + ".png"），
//   供 GameMenu 状态槽位面板显示。
// ============================================================

void GameView::doQuickSave(int slot, bool silent)
{
    size_t sz = m_core.serializeSize();
    if (sz == 0) {
        bklog::warning("GameView: core does not support serialize (slot {})", slot);
        if (!silent) {
            std::lock_guard<std::mutex> lk(m_saveMsgMutex);
            m_saveMsg     = "Save failed (unsupported)";
            m_saveMsgTime = std::chrono::steady_clock::now();
        }
        return;
    }

    std::vector<uint8_t> buf(sz);
    if (!m_core.serialize(buf.data(), sz)) {
        bklog::warning("GameView: serialize failed (slot {})", slot);
        if (!silent) {
            std::lock_guard<std::mutex> lk(m_saveMsgMutex);
            m_saveMsg     = "Save failed";
            m_saveMsgTime = std::chrono::steady_clock::now();
        }
        return;
    }

    std::string path = quickSaveStatePath(slot);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        bklog::warning("GameView: cannot open state file for writing: {}", path);
        if (!silent) {
            std::lock_guard<std::mutex> lk(m_saveMsgMutex);
            m_saveMsg     = "Save failed (I/O)";
            m_saveMsgTime = std::chrono::steady_clock::now();
        }
        return;
    }

    f.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(sz));
    if (!f) {
        bklog::warning("GameView: write error on state file: {}", path);
        if (!silent) {
            std::lock_guard<std::mutex> lk(m_saveMsgMutex);
            m_saveMsg     = "Save failed (I/O)";
            m_saveMsgTime = std::chrono::steady_clock::now();
        }
        return;
    }

    bklog::info("GameView: state saved to {} ({} bytes)", path, sz);

    // 保存状态缩略图（与状态文件同目录，扩展名 .png）
    {
        auto frame = m_core.getVideoFrame();
        if (!frame.pixels.empty() && frame.width > 0 && frame.height > 0) {
            std::string thumbPath = path + ".png";
            int ret = stbi_write_png(
                thumbPath.c_str(),
                static_cast<int>(frame.width),
                static_cast<int>(frame.height),
                RGBA_CHANNELS,
                frame.pixels.data(),
                static_cast<int>(frame.width * RGBA_CHANNELS));
            if (ret)
                bklog::debug("GameView: 状态缩略图已保存: {}", thumbPath);
            else
                bklog::warning("GameView: 状态缩略图保存失败: {}", thumbPath);
        }
    }

    if (!silent) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Saved (slot %d)", slot);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = msg;
        m_saveMsgTime = std::chrono::steady_clock::now();
    }
}

// ============================================================
// doQuickLoad – 从指定槽位文件反序列化核心状态
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

    // 状态恢复后清空倒带缓冲区，避免混乱
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
// doScreenshot – 截图并保存为 PNG
//
// 从 OpenGL 帧缓冲读取当前渲染帧（包含游戏画面+遮罩），
// 并保存为 PNG 文件。必须在主线程（draw 函数）中调用。
//
// 截图保存位置由 screenshot.dir 配置决定：
//   0（默认）= 与游戏 ROM 同目录
//   1        = BK_APP_ROOT_DIR/albums/{游戏文件名（无后缀）}/
//
// 文件名格式：{游戏名}_{时间戳}.png
// ============================================================

void GameView::doScreenshot()
{
    // 使用实际窗口像素尺寸进行截图，确保与渲染输出完全一致
    unsigned captW = brls::Application::windowWidth;
    unsigned captH = brls::Application::windowHeight;

    if (captW == 0 || captH == 0) {
        // 回退到内容逻辑尺寸
        captW = static_cast<unsigned>(brls::Application::contentWidth);
        captH = static_cast<unsigned>(brls::Application::contentHeight);
    }

    if (captW == 0 || captH == 0) {
        bklog::warning("GameView: 截图失败，窗口尺寸为零");
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Screenshot failed (no frame)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    // 从 GL 帧缓冲读取像素（RGBA8888）
    // OpenGL 坐标系原点在左下角，y 轴与屏幕坐标系相反
    std::vector<uint8_t> pixels(captW * captH * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0,
                 static_cast<GLsizei>(captW),
                 static_cast<GLsizei>(captH),
                 GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels.data());

    // 垂直翻转（OpenGL 行序与 PNG 行序相反）
    for (unsigned row = 0; row < captH / 2; ++row) {
        uint8_t* top = pixels.data() + row * captW * 4;
        uint8_t* bot = pixels.data() + (captH - 1u - row) * captW * 4;
        std::swap_ranges(top, top + captW * 4, bot);
    }

    // 决定保存目录
    int screenshotDirMode = beiklive::cfgGetInt("screenshot.dir", 0);
    std::string saveDir;
    if (screenshotDirMode == 1) {
        // 模拟器相册目录：BK_APP_ROOT_DIR/albums/{游戏文件名（无后缀）}/
        std::string gameStem;
        if (!m_romFileName.empty()) {
            std::filesystem::path p(m_romFileName);
            gameStem = p.stem().string();
        } else {
            gameStem = "unknown";
        }
        saveDir = std::string(BK_APP_ROOT_DIR) + "albums/" + gameStem;
    } else {
        // 与游戏 ROM 同目录
        saveDir = beiklive::file::getParentPath(m_romPath);
        if (saveDir.empty()) {
            saveDir = std::string(BK_APP_ROOT_DIR) + "albums/screenshots";
        }
    }

    // 确保目录存在
    {
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);
        if (ec) {
            bklog::warning("GameView: 截图目录创建失败 '{}': {}", saveDir, ec.message());
            std::lock_guard<std::mutex> lk(m_saveMsgMutex);
            m_saveMsg     = "Screenshot failed (mkdir)";
            m_saveMsgTime = std::chrono::steady_clock::now();
            return;
        }
    }

    // 生成文件名：{游戏名}_{时间戳}.png
    std::string gameStem;
    if (!m_romFileName.empty()) {
        std::filesystem::path p(m_romFileName);
        gameStem = p.stem().string();
    } else {
        gameStem = "screenshot";
    }
    std::time_t t = std::time(nullptr);
    char timeBuf[32];
    // 使用线程安全的时间格式化接口
#if defined(_WIN32)
    struct tm tmBuf{};
    localtime_s(&tmBuf, &t);
    std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &tmBuf);
#else
    struct tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &tmBuf);
#endif
    std::string sep = (!saveDir.empty() && saveDir.back() != '/' && saveDir.back() != '\\')
                      ? "/" : "";
    std::string filePath = saveDir + sep + gameStem + "_" + timeBuf + ".png";

    // 将 RGBA8888 像素数据写入 PNG（stbi_write_png）
    int result = stbi_write_png(
        filePath.c_str(),
        static_cast<int>(captW),
        static_cast<int>(captH),
        RGBA_CHANNELS,
        pixels.data(),
        static_cast<int>(captW * RGBA_CHANNELS)
    );

    if (result == 0) {
        bklog::warning("GameView: 截图写入失败: {}", filePath);
        std::lock_guard<std::mutex> lk(m_saveMsgMutex);
        m_saveMsg     = "Screenshot failed (write)";
        m_saveMsgTime = std::chrono::steady_clock::now();
        return;
    }

    bklog::info("GameView: 截图已保存: {}", filePath);
    std::lock_guard<std::mutex> lk(m_saveMsgMutex);
    m_saveMsg     = "Screenshot saved";
    m_saveMsgTime = std::chrono::steady_clock::now();
}

//
// 支持的格式：
//
// 1. RetroArch .cht 格式：
//    cheats = N
//    cheat0_desc = "Name"
//    cheat0_enable = true
//    cheat0_code = XXXXXXXX+YYYYYYYY
//
// 2. 简单逐行格式（默认启用）：
//    # 注释
//    +XXXXXXXX YYYYYYYY   （'+' 前缀 = 启用）
//    -XXXXXXXX YYYYYYYY   （'-' 前缀 = 禁用）
//    XXXXXXXX YYYYYYYY    （无前缀  = 启用）
// ============================================================

/// 解析 .cht 金手指文件，返回金手指条目列表。
/// 若文件不存在或解析失败，返回空列表。
static std::vector<CheatEntry> parseChtFile(const std::string& path)
{
    std::vector<CheatEntry> result;

    if (!std::filesystem::exists(path)) {
        bklog::info("GameView: no cheat file found at {}", path);
        return result;
    }

    std::ifstream f(path);
    if (!f) {
        bklog::warning("GameView: failed to open cheat file: {}", path);
        return result;
    }

    std::string content;
    {
        std::ostringstream oss;
        oss << f.rdbuf();
        content = oss.str();
    }

    if (content.find("cheats = ") != std::string::npos ||
        content.find("cheats=")   != std::string::npos)
    {
        // ---- RetroArch .cht 格式 ----
        std::unordered_map<std::string, std::string> kv;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key   = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            auto trim = [](std::string s) -> std::string {
                size_t b = s.find_first_not_of(" \t\"");
                size_t e = s.find_last_not_of(" \t\"");
                if (b == std::string::npos) return "";
                return s.substr(b, e - b + 1);
            };
            kv[trim(key)] = trim(value);
        }

        unsigned total = 0;
        {
            auto it = kv.find("cheats");
            if (it != kv.end()) {
                try { total = static_cast<unsigned>(std::stoi(it->second)); } catch (...) {}
            }
        }

        for (unsigned i = 0; i < total; ++i) {
            std::string iStr      = std::to_string(i);
            std::string descKey   = "cheat" + iStr + "_desc";
            std::string enableKey = "cheat" + iStr + "_enable";
            std::string codeKey   = "cheat" + iStr + "_code";

            auto cit = kv.find(codeKey);
            if (cit == kv.end()) continue;

            CheatEntry entry;
            entry.code    = cit->second;
            entry.enabled = true;
            entry.desc    = "cheat" + iStr;

            auto eit = kv.find(enableKey);
            if (eit != kv.end()) {
                std::string ev = eit->second;
                for (char& c : ev) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                entry.enabled = (ev == "true" || ev == "1" || ev == "yes");
            }

            auto dit = kv.find(descKey);
            if (dit != kv.end()) entry.desc = dit->second;

            result.push_back(std::move(entry));
        }
    }
    else
    {
        // ---- 简单逐行格式 ----
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            size_t b = line.find_first_not_of(" \t");
            if (b == std::string::npos) continue;
            line = line.substr(b);
            if (line.empty()) continue;

            CheatEntry entry;
            entry.enabled = true;
            if (line[0] == '+') {
                entry.enabled = true;
                line = line.substr(1);
            } else if (line[0] == '-') {
                entry.enabled = false;
                line = line.substr(1);
            }
            b = line.find_first_not_of(" \t");
            if (b == std::string::npos) continue;
            line = line.substr(b);
            if (line.empty()) continue;

            entry.code = line;
            entry.desc = line; // 简单格式无名称，使用代码作为描述
            result.push_back(std::move(entry));
        }
    }

    return result;
}

/// 将金手指列表以 RetroArch .cht 格式写入文件。
/// 返回 true 表示成功。
static bool saveChtFile(const std::string& path,
                        const std::vector<CheatEntry>& entries)
{
    std::ofstream f(path);
    if (!f) {
        bklog::warning("GameView: 无法写入金手指文件: {}", path);
        return false;
    }

    f << "cheats = " << entries.size() << "\n\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        f << "cheat" << i << "_desc = \"" << e.desc << "\"\n";
        f << "cheat" << i << "_enable = " << (e.enabled ? "true" : "false") << "\n";
        f << "cheat" << i << "_code = " << e.code << "\n\n";
    }

    return true;
}

void GameView::loadCheats()
{
    // 优先读取 gamedata 中保存的金手指路径，回退到配置目录路径
    m_cheatPath = getGameDataStr(m_romFileName, GAMEDATA_FIELD_CHEATPATH, "");
    if (m_cheatPath.empty()) {
        m_cheatPath = cheatFilePath();
        // 首次启动时将默认路径写入 gamedata，方便用户查看和自定义
        if (!m_romFileName.empty()) {
            setGameDataStr(m_romFileName, GAMEDATA_FIELD_CHEATPATH, m_cheatPath);
        }
    }

    m_cheats = parseChtFile(m_cheatPath);

    // 应用金手指到核心
    // 注意：mGBA 的 retro_cheat_set 会忽略 enabled 参数（UNUSED），
    // 因此只对已启用的金手指调用 cheatSet，禁用的金手指不传入核心。
    m_core.cheatReset();
    for (size_t i = 0; i < m_cheats.size(); ++i) {
        bklog::info("GameView: cheat[{}] \"{}\" {} code={}", i,
                    m_cheats[i].desc,
                    m_cheats[i].enabled ? "enabled" : "disabled",
                    m_cheats[i].code);
        if (m_cheats[i].enabled) {
            m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
        }
    }

    if (!m_cheats.empty())
        bklog::info("GameView: {} cheat(s) loaded from {}", m_cheats.size(), m_cheatPath);
    else
        bklog::info("GameView: cheat file {} contained no valid cheats", m_cheatPath);
}

// ============================================================
// updateCheats – 将当前 m_cheats 状态重新应用到核心并保存到文件
// ============================================================

void GameView::updateCheats()
{
    // 重置核心金手指状态，再逐条重新设置
    // 注意：mGBA 的 retro_cheat_set 会忽略 enabled 参数（UNUSED），
    // 因此只对已启用的金手指调用 cheatSet，禁用的金手指不传入核心。
    m_core.cheatReset();
    for (size_t i = 0; i < m_cheats.size(); ++i) {
        if (m_cheats[i].enabled) {
            m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
        }
    }
    // 将启用状态写回磁盘上的 .cht 文件
    if (!m_cheatPath.empty()) {
        saveChtFile(m_cheatPath, m_cheats);
        brls::Logger::info("GameView: cheat file updated: {}", m_cheatPath);
    }
}



void GameView::uploadFrame(NVGcontext* vg,
                            const beiklive::LibretroLoader::VideoFrame& frame)
{
    if (!frame.width || !frame.height || frame.pixels.empty()) return;

    glBindTexture(GL_TEXTURE_2D, m_texture);

    bool sizeChanged = (frame.width  != m_texWidth ||
                        frame.height != m_texHeight);

    if (sizeChanged) {
        // 调整纹理大小（像素格式为 RGBA8888）
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(frame.width),
                     static_cast<GLsizei>(frame.height),
                     0, GL_RGBA, GL_UNSIGNED_BYTE,
                     frame.pixels.data());
        m_texWidth  = frame.width;
        m_texHeight = frame.height;
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
// loadOverlayImage – 将覆盖层 PNG 加载为 NVG 图像。
// 首次需要时从 draw() 懒加载，m_overlayPath 变化时重新加载。
// ============================================================

void GameView::loadOverlayImage(NVGcontext* vg)
{
    // 释放已有的覆盖层图像
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
// pollInput – 将手柄状态映射到 libretro 按键，
//             并通过 GameInputController 触发模拟器热键。
//
// 本函数是手柄控制系统唯一的输入处理路径，运行于游戏线程，
// 仅读取主线程通过 refreshInputSnapshot() 填充的线程安全快照，
// 确保不在游戏线程直接调用平台输入管理器接口。
//
// 模拟器热键逻辑通过 GameInputController 流转，触发
// registerGamepadHotkeys() 中注册的 Press/ShortPress/LongPress/Release 回调。
// ============================================================

void GameView::pollInput()
{
    // ---- 从主线程快照获取输入状态 ---------------
    InputSnapshot snap;
    {
        std::lock_guard<std::mutex> lk(m_inputSnapMutex);
        snap = m_inputSnap;
    }
    const brls::ControllerState& state = snap.ctrlState;

    // ── GameInputController：处理所有注册的手柄热键组合 ──
    // 处理快进按住/切换、倒带按住/切换、退出游戏、
    // 即时存读档等 Press/ShortPress/LongPress/Release 事件。
    // 回调写入原子变量和游戏线程布尔值。
    m_inputCtrl.update(state);

    // 倒带时禁用快进
    if (m_rewinding.load(std::memory_order_relaxed))
        m_fastForward.store(false, std::memory_order_relaxed);

    // ── 输入忽略帧：菜单关闭后若干帧内不传递游戏按键 ──────────────────────
    // 防止关闭菜单时使用的按键（如 B 键）被透传到游戏核心。
    // 仅在进入忽略期的第一帧清空按键状态，后续帧直接跳过输入处理即可
    // （libretro 核心按键状态持久，清空一次后不再重复设置就保持为零）。
    int ignoreFrames = m_inputIgnoreFrames.load(std::memory_order_relaxed);
    if (ignoreFrames > 0) {
        m_inputIgnoreFrames.store(ignoreFrames - 1, std::memory_order_relaxed);
        if (ignoreFrames == INPUT_IGNORE_FRAMES_ON_MENU_CLOSE) {
            // 仅首帧清空所有游戏按键状态
            for (const auto& entry : m_inputMap.gameButtonMap())
                m_core.setButtonState(entry.retroId, false);
        }
        return;
    }

    // ── 游戏按键映射 ───────────────────────────────────────────────────────
    // 将每个已配置的手柄按键映射到对应的 libretro 手柄 ID。
    const auto& btnMap = m_inputMap.gameButtonMap();
    for (const auto& entry : btnMap)
    {
        bool pressed = false;
        // 手柄按键
        if (entry.padButton >= 0 && entry.padButton < static_cast<int>(brls::_BUTTON_MAX))
            pressed = state.buttons[entry.padButton];
        m_core.setButtonState(entry.retroId, pressed);
    }

    // ── 调试日志 ─────────────────────────────────────────────────────────
    if (brls::Logger::getLogLevel() <= brls::LogLevel::LOG_DEBUG)
    {
        for (const auto& entry : btnMap)
        {
            bool padPressed = (entry.padButton >= 0 &&
                               entry.padButton < static_cast<int>(brls::_BUTTON_MAX) &&
                               state.buttons[entry.padButton]);
            if (padPressed)
                bklog::debug("pollInput: retroId={} pressed (pad)",
                             entry.retroId);
        }
    }
}

// ============================================================
// refreshInputSnapshot – 在主线程捕获手柄状态快照
// ============================================================
// 平台输入管理器接口（updateUnifiedControllerState 等）只能在主线程调用。
// 游戏模拟线程通过 pollInput() 读取此线程安全快照，
// 彻底避免在非主线程调用输入管理器。
//
void GameView::refreshInputSnapshot()
{
    auto* platform = brls::Application::getPlatform();
    auto* im       = platform ? platform->getInputManager() : nullptr;
    if (!im) return;

    InputSnapshot snap;

    // ── 手柄/控制器状态 ───────────────────────────────────────────────────
    // updateUnifiedControllerState() 通过平台输入管理器读取当前硬件状态
    // （桌面平台用 GLFW，Switch 用 HID），必须在主线程调用。
    im->updateUnifiedControllerState(&snap.ctrlState);

    // 发布快照
    std::lock_guard<std::mutex> lk(m_inputSnapMutex);
    m_inputSnap = std::move(snap);
}

// ============================================================
// draw – 每帧渲染入口（GL 上传 + NVG 渲染）
// ============================================================

void GameView::draw(NVGcontext* vg, float x, float y, float width, float height,
                    brls::Style style, brls::FrameContext* ctx)
{
    // 延迟初始化（此时 GL 上下文已就绪）
    if (!m_initialized && !m_coreFailed) {
        initialize();
    }

    // ── 在主线程刷新输入快照（GLFW 线程安全要求）──
    // 每帧必须调用，确保 pollInput()（游戏线程）始终获得最新的硬件输入状态，
    // 而无需直接访问 GLFW。
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

    // ---- OpenMenu 热键：游戏线程设置此标志，在主线程处理 -----
    if (m_requestOpenMenu.exchange(false, std::memory_order_relaxed)) {
        if (!m_gameMenu) {
            bklog::warning("GameView: 收到打开菜单请求，但 m_gameMenu 未设置");
        } else if (m_gameMenu->getVisibility() == brls::Visibility::GONE) {
            m_gameMenu->setVisibility(brls::Visibility::VISIBLE);
            // 暂停游戏：防止菜单导航输入被传递到游戏核心
            setPaused(true);
            // 解除 borealis 输入封锁，使菜单能接收手柄导航和点击事件
            setGameInputEnabled(false);
            // 将焦点转移到菜单（borealis 会找到第一个可聚焦子控件）
            brls::Application::giveFocus(m_gameMenu);
        }
    }

    if (!m_initialized) {
        // ROM 加载失败：首次检测到时弹出对话框，确定后自动关闭 GameView
        if (m_coreFailed && !m_failDialogShown) {
            m_failDialogShown = true;
            auto* dlg = new brls::Dialog("beiklive/hints/rom_load_failed"_i18n);
            dlg->setCancelable(false);
            dlg->addButton("hints/ok"_i18n, []() {
                // 对话框关闭后，其 Activity 已弹出，此时弹出 GameView 所在的 Activity
                brls::Application::popActivity();
            });
            dlg->open();
        }
        return;
    }

    // ---- 将最新视频帧上传到 GL 纹理 -------------
    // （模拟在独立线程运行，此处仅消费结果。）
    {
        auto frame = m_core.getVideoFrame();
        if (!frame.pixels.empty()) {
            uploadFrame(vg, frame);
        }
    }

    // ---- 预先按显示模式计算目标矩形 ----------
    beiklive::DisplayRect preRect = { x, y, width, height };
    if (m_texWidth > 0 && m_texHeight > 0) {
        preRect = m_display.computeRect(x, y, width, height, m_texWidth, m_texHeight);
    }

    // ---- 计算传入渲染链的视口尺寸（物理像素）----
    // 对比 RetroArch example/gfx 渲染链（gl2/gl3）：viewport 缩放使用完整输出视口尺寸
    // （gl->video_width × gl->video_height = 整个游戏渲染区域的物理像素）。
    // 使用 preRect（已应用显示模式的宽高比校正矩形）×windowScale 作为着色器视口，
    // 确保 FBO 尺寸与最终显示矩形物理尺寸一致，避免宽高比不匹配导致画面压扁。
    // 在 FullScreen/Fill 模式下 preRect == 完整视图矩形，行为与原来相同。
    float windowScale = brls::Application::windowScale;
    unsigned passViewW = std::max(1u, static_cast<unsigned>(std::lround(preRect.w * windowScale)));
    unsigned passViewH = std::max(1u, static_cast<unsigned>(std::lround(preRect.h * windowScale)));

    // ---- 运行渲染链并确定显示纹理 ----------
    // 若已加载着色器管线，将游戏纹理经过多通道 Shader 处理后作为显示纹理；
    // 否则直通（pass-through）返回原始游戏纹理。
    GLuint displayTex = m_texture;
    int    displayW   = static_cast<int>(m_texWidth);
    int    displayH   = static_cast<int>(m_texHeight);

    if (m_texWidth > 0 && m_texHeight > 0) {
        GLuint chainOut = m_renderChain.run(m_texture, m_texWidth, m_texHeight,
                                            passViewW, passViewH);
        if (chainOut != 0) {
            displayTex = chainOut;
            // 使用着色器管线实际输出尺寸（可能因缩放不同于输入）
            if (m_renderChain.outputW() > 0) displayW = static_cast<int>(m_renderChain.outputW());
            if (m_renderChain.outputH() > 0) displayH = static_cast<int>(m_renderChain.outputH());
        }
    }

    // ---- 处理运行时过滤模式变更（在渲染链处理之后执行）----
    // 使用着色器时跳过：过滤由各 pass 的 FBO 纹理参数控制，不作用于原始游戏纹理
    if (!m_renderChain.hasShader() && m_display.filterMode != m_activeFilter && m_texture) {
        m_activeFilter  = m_display.filterMode;
        GLenum glFilter = (m_activeFilter == beiklive::FilterMode::Nearest)
                          ? GL_NEAREST : GL_LINEAR;
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // ---- 直接以 OpenGL 渲染游戏帧到屏幕（不经过 NanoVG 批量渲染）----
    // 确定最终显示矩形：
    // - 视口缩放着色器（FBO 输出尺寸 == passViewW×passViewH）：
    //   使用 preRect（已根据显示模式计算），确保显示模式设置仍然有效。
    // - source/absolute 缩放着色器（FBO 输出尺寸 ≠ passViewW×passViewH，如 scalefx 3×）：
    //   以着色器实际输出尺寸重新调用 computeRect()，保持宽高比缩放正确。
    // - 无着色器：直接使用预计算的 preRect。
    if (displayW > 0 && displayH > 0) {
        beiklive::DisplayRect rect = preRect;
        if (m_renderChain.hasShader()) {
            unsigned shOutW = m_renderChain.outputW();
            unsigned shOutH = m_renderChain.outputH();
            if (shOutW > 0 && shOutH > 0 && (shOutW != passViewW || shOutH != passViewH)) {
                // source/absolute 缩放着色器：输出尺寸与传入视口不同，重新计算显示矩形
                rect = m_display.computeRect(x, y, width, height, shOutW, shOutH);
            } else {
                // viewport 缩放着色器（或尺寸未知）：使用 preRect（已应用显示模式）
                // 确保显示模式（Fit/Fill/Original等）在着色器激活时仍生效
                rect = preRect;
            }
        }

        int   screenW = static_cast<int>(brls::Application::windowWidth);
        int   screenH = static_cast<int>(brls::Application::windowHeight);
        // 使用直接 GL 路径绘制游戏帧；NanoVG UI 叠加层继续在 nvgEndFrame 时渲染于其上
        m_renderChain.drawToScreen(displayTex,
                                   rect.x, rect.y, rect.w, rect.h,
                                   windowScale, screenW, screenH);
    }

    // ---- 遮罩绘制 – 全屏，叠加在游戏画面之上
    // 若遮罩路径已变更（菜单中用户重新选择），立即重新加载纹理（菜单打开时即完成，恢复游戏无卡顿）
    if (m_overlayReloadPending.exchange(false, std::memory_order_acquire)) {
        loadOverlayImage(vg);
    }
    if (m_overlayEnabled && !m_overlayPath.empty()) {
        // 懒加载：首次使用时创建 NVG 图像句柄
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

    // ---- FPS 覆盖层 -------------------------------------------------
    if (m_showFps) {
        float currentFps = 0.0f;
        {
            std::lock_guard<std::mutex> lk(m_fpsMutex);
            currentFps = m_currentFps;
        }
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", currentFps);

        // 半透明背景
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

    // ---- 快进覆盖层（可配置）-----------------------------------------
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

    // ---- 倒带状态覆盖层（可配置）------------------------------------
    if (m_showRewindOverlay && m_inputMap.rewindEnabled && m_rewinding.load(std::memory_order_relaxed)) {
        char rewBuf[32];
        // rewindStep = 每次倒带弹出的帧数（倒带速度）
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

    // ---- 存读档状态覆盖层 ---------------------------------
    // 快速存读档操作后短暂显示提示消息，2 秒后淡出。
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
                    m_saveMsg.clear(); // 已过期
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
    // 游戏静音时在屏幕右下角显示静音提示（由 display.showMuteOverlay 控制）。
    if (m_showMuteOverlay && m_muted.load(std::memory_order_relaxed)) {
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

    // ---- 暂停状态覆盖层 --------------------------------------
    // 游戏暂停（打开菜单）时在屏幕顶部中央显示暂停提示。
    if (m_paused.load(std::memory_order_relaxed)) {
        const char* pauseText = "PAUSED";
        float pw = 90.0f, ph = 22.0f;
        float px = x + width  * 0.5f - pw * 0.5f;
        float py = y + 4.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, px, py, pw, ph, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 180));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(255, 220, 60, 230));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, px + pw * 0.5f, py + ph * 0.5f, pauseText, nullptr);
    }

    // ---- 截图处理（主线程，在所有渲染完成后从 GL 帧缓冲抓取）--------
    // 截图包含游戏画面+遮罩（整个屏幕）。
    // 调用 nvgEndFrame 将本帧全部 NanoVG 绘制命令提交到 GL 后缓冲区，
    // 随后 glReadPixels 可读取完整的游戏渲染帧（游戏画面 + 遮罩 PNG）。
    // NanoVG 的 nvgEndFrame 仅刷新并清空待提交命令队列，不影响上下文状态，
    // 因此框架后续的焦点高亮、通知等 NVG 绘制仍会被框架自身的 nvgEndFrame 正确提交。
    if (m_pendingScreenshot.exchange(false, std::memory_order_relaxed)) {
        nvgEndFrame(vg);
        doScreenshot();
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

