#include "GameView.hpp"
#include "GameMenuView.hpp"
#include "RewindSelectorView.hpp"
#include "game/audio/AudioManager.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "core/Tools.hpp"

#include <filesystem>
#include <fstream>

// stb_image_write 用于保存存档缩略图（PNG 格式）
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h> // timeBeginPeriod / timeEndPeriod
#endif

namespace beiklive
{

    GameView::GameView(beiklive::GameEntry gameData) : m_gameEntry(std::move(gameData))
    {
        _brls_inputLocked = false;
        GameInputManager::instance().sayHello();
        HIDE_BRLS_HIGHLIGHT(this);

        // 从 GameEntry 加载画面模式（默认 Fit）
        m_screenMode = static_cast<beiklive::ScreenMode>(m_gameEntry.displayMode);

        // 从配置读取倒带相关设置
        m_rewindSaveInterval = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, 1);
        // 确保间隔值在合法范围内（与设置页面的选项匹配：1/2/4/8/16）
        if (m_rewindSaveInterval < 1)  m_rewindSaveInterval = 1;
        if (m_rewindSaveInterval > 16) m_rewindSaveInterval = 16;
        m_rewindShowUI = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SHOW_UI, 0) != 0;
        // 从配置读取缩略图降采样质量（0=最近邻，1=区域平均，2=双线性）
        int thumbSampleVal = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_THUMB_SAMPLE, 0);
        if (thumbSampleVal < 0 || thumbSampleVal > 2) thumbSampleVal = 0;
        m_thumbSampleMode = static_cast<ThumbSampleMode>(thumbSampleVal);

        _registerGameInput();
        _registerGameRuntime();
    }

    GameView::~GameView()
    {
        _stopGameThread();

        if (m_gba_core) {
            delete m_gba_core;
            m_gba_core = nullptr;
        }

        GameInputManager::instance().clearEmuFunctionKeys();
        GameInputManager::instance().dropInput();
    }

    void GameView::onFocusGained()
    {
        Box::onFocusGained();
        brls::Logger::debug("GameView gained focus");

        // 获得焦点时恢复游戏运行
        GameSignal::instance().requestPause(false);
        GameInputManager::instance().setInputEnabled(true);

        if (!_brls_inputLocked)
        {
            _brls_inputLocked = true;
            brls::Application::blockInputs(true);
        }
    }

    void GameView::onFocusLost()
    {
        Box::onFocusLost();
        brls::Logger::debug("GameView lost focus");

        // 失去焦点时暂停游戏
        GameSignal::instance().requestPause(true);
        GameInputManager::instance().setInputEnabled(false);
        GameInputManager::instance().dropInput();

        if (_brls_inputLocked)
        {
            _brls_inputLocked = false;
            brls::Application::unblockInputs();
        }
    }

    void GameView::draw(NVGcontext *vg, float x, float y, float width, float height,
                        brls::Style style, brls::FrameContext *ctx)
    {
        Box::draw(vg, x, y, width, height, style, ctx);

        GameInputManager::instance().handleInput(); // 每帧获取输入

        // 消费退出信号：异步弹出活动，本帧仍继续渲染避免闪烁
        if (GameSignal::instance().consumeExit()) {
            brls::sync([this](){ 
                brls::Application::popActivity(); });
            // 不提前返回：继续渲染最后一帧，防止画面出现黑帧闪烁
        }

        // 消费打开菜单信号：异步触发菜单入场，本帧仍继续渲染避免闪烁
        if (GameSignal::instance().consumeOpenMenu()) {
            if (m_gameMenuView) {
                brls::sync([this](){
                    // 菜单从底部滑入，入场动画（220ms）
                    AnimationHelper::slideInFromBottom(m_gameMenuView, 60.f, 220);
                    brls::Application::giveFocus(m_gameMenuView);
                });
            }
            // 不提前返回：继续渲染当前游戏帧，防止菜单弹出时出现黑帧闪烁
        }

        // 消费打开倒带UI信号：暂停游戏并弹出可视化倒带选择界面
        if (GameSignal::instance().consumeOpenRewindUI()) {
            if (m_rewindSelectorView) {
                GameSignal::instance().requestPause(true);
                // 取出缩略图快照（游戏已暂停，可安全读取缓冲区）
                int maxItems = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_UI_ITEM_COUNT, 10);
                auto thumbs = snapshotRewindThumbs(maxItems);
                brls::sync([this, thumbs = std::move(thumbs)]() mutable {
                    m_rewindSelectorView->openWithFrames(std::move(thumbs));
                    AnimationHelper::slideInFromBottom(m_rewindSelectorView, 80.f, 220);
                    // 将焦点设置到最右侧卡片（最新帧），修复焦点留在 GameView 的问题
                    m_rewindSelectorView->focusNewest();
                });
            }
        }

        // 初始化渲染器（首帧时，GL 上下文已就绪）
        if (!m_rendererReady && m_gba_core && m_gba_core->IsReady()) {
            unsigned gw = m_gba_core->GameWidth()  > 0 ? m_gba_core->GameWidth()  : beiklive::GetGamePixelWidth(m_gameEntry.platform);
            unsigned gh = m_gba_core->GameHeight() > 0 ? m_gba_core->GameHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);
            // 若游戏条目启用了着色器且路径有效，则传入着色器路径初始化渲染链
            std::string shaderPath;
            if (m_gameEntry.shaderEnabled && !m_gameEntry.shaderPath.empty()) {
                shaderPath = m_gameEntry.shaderPath;
            }
            if (m_renderer.init(gw, gh, false, shaderPath)) {
                m_rendererReady = true;
                brls::Logger::info("GameView: 渲染器初始化完成 ({}x{} shader={})",
                                   gw, gh,
                                   shaderPath.empty() ? "无" : shaderPath);
                // 初始化 FPS 计时
                m_fpsLastTime = std::chrono::steady_clock::now();
            }
        }

        // 上传待渲染帧（游戏线程已写入 m_pendingFrame）
        _uploadPendingFrame();

        // 根据画面模式计算绘制矩形，将游戏帧绘制到视图区域
        if (m_rendererReady) {
            float windowScale = brls::Application::windowScale;
            int   windowW     = brls::Application::windowWidth;
            int   windowH     = brls::Application::windowHeight;

            unsigned gw = m_renderer.texWidth()  > 0 ? m_renderer.texWidth()  : beiklive::GetGamePixelWidth(m_gameEntry.platform);
            unsigned gh = m_renderer.texHeight() > 0 ? m_renderer.texHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);

            beiklive::DisplayRect rect = beiklive::computeDisplayRect(
                m_screenMode, x, y, width, height, gw, gh,
                m_gameEntry.customScale, m_gameEntry.customOffsetX, m_gameEntry.customOffsetY);

            m_renderer.drawToScreen(rect.x, rect.y, rect.w, rect.h, windowScale, windowW, windowH);
        }

        // 绘制状态覆盖层
        _drawOverlays(vg, x, y, width, height);
    }

    // ============================================================
    // _drawOverlays – 绘制 FPS/快进/倒带/暂停/静音覆盖层
    // ============================================================
    void GameView::_drawOverlays(NVGcontext* vg, float x, float y, float w, float h)
    {
        auto& sig = GameSignal::instance();

        // FPS 覆盖层（左上角）
        {
            float fps = 0.f;
            {
                std::lock_guard<std::mutex> lk(m_fpsMutex);
                fps = m_currentFps;
            }
            if (fps > 0.f)
                GameOverlayRenderer::drawFps(vg, x, y, fps);
        }

        // 快进覆盖层（右上角）
        if (sig.isFastForward())
            GameOverlayRenderer::drawFastForward(vg, x, y, w);

        // 倒带覆盖层（顶部居中）
        if (sig.isRewinding())
            GameOverlayRenderer::drawRewind(vg, x, y, w);

        // 暂停覆盖层（顶部居中，快进/倒带时不另外显示）
        if (sig.isPaused() && !sig.isFastForward() && !sig.isRewinding())
            GameOverlayRenderer::drawPaused(vg, x, y, w);

        // 静音覆盖层（右下角）
        if (sig.isMuted())
            GameOverlayRenderer::drawMute(vg, x, y, w, h);
    }

    // ============================================================
    // _uploadPendingFrame – 将游戏线程产出的最新帧上传到 GPU
    // ============================================================
    void GameView::_uploadPendingFrame()
    {
        if (!m_rendererReady) return;

        LibretroLoader::VideoFrame frame;
        bool hasFrame = false;
        {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            if (m_frameReady) {
                frame       = std::move(m_pendingFrame);
                m_frameReady = false;
                hasFrame     = true;
            }
        }

        if (hasFrame)
            m_renderer.uploadFrame(frame);
    }

    // ============================================================
    // _registerGameInput – 注册游戏热键
    // ============================================================
    void GameView::_registerGameInput()
    {
        // ---- 读取摇杆模式配置 -------------------------------------------
        bool joystickEnabled  = GET_SETTING_KEY_INT("input.joystick.enabled",  1) != 0;
        bool joystickDiagonal = GET_SETTING_KEY_INT("input.joystick.diagonal", 1) != 0;
        GameInputManager::instance().setDiagonalMode(joystickDiagonal);

        // ---- 游戏按键绑定（从配置读取多 combo 按键映射）--------------------
        // 按住时持续置位，松开时清除，使用 GameSignal 按键位掩码传入游戏帧。
        // GameBtnInfo：游戏按键配置项，存储模拟器功能键、配置后缀和 libretro 手柄 ID 的映射关系。
        struct GameBtnInfo {
            EmuFunctionKey emuKey;      ///< 模拟器功能键枚举值
            const char*    cfgSuffix;   ///< 配置键后缀（"handle.<suffix>" 为完整键）
            unsigned       retroId;     ///< libretro 手柄 ID（RETRO_DEVICE_ID_JOYPAD_*）
        };
        static const GameBtnInfo gameBtnInfos[] = {
            { EMU_A,      "a",      8  }, // RETRO_DEVICE_ID_JOYPAD_A
            { EMU_B,      "b",      0  }, // RETRO_DEVICE_ID_JOYPAD_B
            { EMU_X,      "x",      9  }, // RETRO_DEVICE_ID_JOYPAD_X
            { EMU_Y,      "y",      1  }, // RETRO_DEVICE_ID_JOYPAD_Y
            { EMU_UP,     "up",     4  }, // RETRO_DEVICE_ID_JOYPAD_UP
            { EMU_DOWN,   "down",   5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
            { EMU_LEFT,   "left",   6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
            { EMU_RIGHT,  "right",  7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
            { EMU_L,      "l",      10 }, // RETRO_DEVICE_ID_JOYPAD_L
            { EMU_R,      "r",      11 }, // RETRO_DEVICE_ID_JOYPAD_R
            { EMU_L2,     "l2",     12 }, // RETRO_DEVICE_ID_JOYPAD_L2
            { EMU_R2,     "r2",     13 }, // RETRO_DEVICE_ID_JOYPAD_R2
            { EMU_L3,     "l3",     14 }, // RETRO_DEVICE_ID_JOYPAD_L3
            { EMU_R3,     "r3",     15 }, // RETRO_DEVICE_ID_JOYPAD_R3
            { EMU_START,  "start",  3  }, // RETRO_DEVICE_ID_JOYPAD_START
            { EMU_SELECT, "select", 2  }, // RETRO_DEVICE_ID_JOYPAD_SELECT
        };
        for (const auto& info : gameBtnInfos) {
            std::string val = GET_SETTING_KEY_STR(std::string("handle.") + info.cfgSuffix, "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            if (combos.empty()) continue;
            unsigned rid = info.retroId;
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    info.emuKey, {combo},
                    [rid]() { GameSignal::instance().pressGameButton(rid); },
                    TriggerType::HOLD);
                GameInputManager::instance().registerEmuFunctionKey(
                    info.emuKey, {combo},
                    [rid]() { GameSignal::instance().releaseGameButton(rid); },
                    TriggerType::RELEASE);
            }
        }

        // ---- 摇杆方向键映射（从配置读取，受 joystickEnabled 控制）---------
        // retroId 对应 RETRO_DEVICE_ID_JOYPAD：UP=4, DOWN=5, LEFT=6, RIGHT=7
        if (joystickEnabled) {
            // StickBtnInfo：摇杆方向键配置项，存储功能键枚举、配置后缀和 libretro ID 的映射关系。
            struct StickBtnInfo {
                EmuFunctionKey emuKey;      ///< 模拟器功能键枚举值
                const char*    cfgSuffix;   ///< 配置键后缀（"handle.<suffix>"）
                unsigned       retroId;     ///< libretro 手柄 ID
            };
            static const StickBtnInfo stickBtnInfos[] = {
                { EMU_LEFT_STICK_UP,     "lstick_up",    4  }, // RETRO_DEVICE_ID_JOYPAD_UP
                { EMU_LEFT_STICK_DOWN,   "lstick_down",  5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
                { EMU_LEFT_STICK_LEFT,   "lstick_left",  6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
                { EMU_LEFT_STICK_RIGHT,  "lstick_right", 7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
                { EMU_RIGHT_STICK_UP,    "rstick_up",    4  }, // RETRO_DEVICE_ID_JOYPAD_UP
                { EMU_RIGHT_STICK_DOWN,  "rstick_down",  5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
                { EMU_RIGHT_STICK_LEFT,  "rstick_left",  6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
                { EMU_RIGHT_STICK_RIGHT, "rstick_right", 7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
            };
            for (const auto& info : stickBtnInfos) {
                std::string val = GET_SETTING_KEY_STR(std::string("handle.") + info.cfgSuffix, "none");
                auto combos = beiklive::tools::parseMultiCombo(val);
                if (combos.empty()) continue;
                unsigned rid = info.retroId;
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        info.emuKey, {combo},
                        [rid]() { GameSignal::instance().pressGameButton(rid); },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        info.emuKey, {combo},
                        [rid]() { GameSignal::instance().releaseGameButton(rid); },
                        TriggerType::RELEASE);
                }
            }
        }

        // ---- 功能热键绑定（从配置读取多 combo）----------------------------

        // 打开菜单
        {
            std::string val = GET_SETTING_KEY_STR("hotkey.menu.pad", "LT+RT");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_OPEN_MENU, {combo},
                    [this]() {
                        brls::Logger::debug("打开菜单热键触发！");
                        GameSignal::instance().requestOpenMenu();
                        this->setFocusable(false);
                    });
            }
        }

        // 快进切换
        {
            std::string val = GET_SETTING_KEY_STR("handle.fastforward", "LSB");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_FAST_FORWARD, {combo},
                    []() {
                        bool cur = GameSignal::instance().isFastForward();
                        GameSignal::instance().requestFastForward(!cur);
                        brls::Logger::debug("快进切换：{}", !cur);
                    });
            }
        }

        // 倒带切换：若启用可视化倒带界面则打开UI，否则执行传统倒带
        {
            std::string val = GET_SETTING_KEY_STR("handle.rewind", "RSB");
            auto combos = beiklive::tools::parseMultiCombo(val);
            bool showUI = m_rewindShowUI;
            for (const auto& combo : combos) {
                if (showUI) {
                    // 可视化倒带模式：按键触发时打开倒带选择界面
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        [this]() {
                            brls::Logger::debug("倒带UI触发！");
                            GameSignal::instance().requestOpenRewindUI();
                            this->setFocusable(false);
                        });
                } else {
                    // 传统倒带模式：按键切换倒带状态
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        []() {
                            bool cur = GameSignal::instance().isRewinding();
                            GameSignal::instance().requestRewind(!cur);
                            brls::Logger::debug("倒带切换：{}", !cur);
                        });
                }
            }
        }

        // 快速保存（默认槽位 1）
        {
            std::string val = GET_SETTING_KEY_STR("hotkey.quicksave.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_QUICK_SAVE, {combo},
                    []() { GameSignal::instance().requestQuickSave(1); });
            }
        }

        // 快速读取（默认槽位 1）
        {
            std::string val = GET_SETTING_KEY_STR("hotkey.quickload.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_QUICK_LOAD, {combo},
                    []() { GameSignal::instance().requestQuickLoad(1); });
            }
        }

        // 静音切换
        {
            std::string val = GET_SETTING_KEY_STR("hotkey.mute.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_MUTE, {combo},
                    []() {
                        bool cur = GameSignal::instance().isMuted();
                        GameSignal::instance().requestMute(!cur);
                    });
            }
        }
    }

    // ============================================================
    // _registerGameRuntime – 创建并初始化核心，启动游戏线程
    // ============================================================
    void GameView::_registerGameRuntime()
    {
        if (m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuGBA ||
            m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuGB  ||
            m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuGBC)
        {
            m_gba_core = new beiklive::gba::CoreMgba();
            if (m_gba_core->SetupGame(m_gameEntry))
            {
                brls::Logger::debug("GBA 核心已初始化，游戏路径：{}", m_gameEntry.path);
                // 初始化音频系统
                double fps = m_gba_core->Fps();
                if (fps <= 0.0) fps = 59.7;
                AudioManager::instance().init(32768, 2);
                // 重置信号状态，准备开始游戏
                GameSignal::instance().resetAll();
                // 启动游戏线程
                _startGameThread();
            }
            else
            {
                brls::Logger::error("GBA 核心初始化失败，游戏路径：{}", m_gameEntry.path);
                delete m_gba_core;
                m_gba_core = nullptr;
            }
        }
        else
        {
            brls::Logger::warning("不支持的平台：{}", m_gameEntry.platform);
        }
    }

    // ============================================================
    // _startGameThread / _stopGameThread
    // ============================================================
    void GameView::_startGameThread()
    {
        m_running.store(true, std::memory_order_release);
        m_gameThread = std::thread(&GameView::_gameLoop, this);
    }

    void GameView::_stopGameThread()
    {
        m_running.store(false, std::memory_order_release);
        if (m_gameThread.joinable())
            m_gameThread.join();
    }

    // ============================================================
    // _saveRewindState – 序列化当前核心状态并存入倒带缓冲区
    // 支持间隔保存（每 m_rewindSaveInterval 帧保存一次）
    // 若 m_rewindShowUI 开启则同时捕获 RGB565 缩略图
    // ============================================================
    void GameView::_saveRewindState()
    {
        // 间隔控制：每 m_rewindSaveInterval 帧才保存一次
        ++m_rewindFrameCounter;
        if (m_rewindFrameCounter < static_cast<unsigned>(m_rewindSaveInterval))
            return;
        m_rewindFrameCounter = 0;

        RewindFrame frame;
        if (!m_gba_core->Serialize(frame.state) || frame.state.empty())
            return;

        // 若启用可视化倒带界面，则同时捕获并压缩缩略图
        if (m_rewindShowUI) {
            auto videoFrame = m_gba_core->GetVideoFrame();
            if (!videoFrame.pixels.empty() && videoFrame.width > 0 && videoFrame.height > 0) {
                frame.thumb = _downsampleToRGB565(
                    videoFrame.pixels, videoFrame.width, videoFrame.height,
                    RewindFrame::THUMB_W, RewindFrame::THUMB_H,
                    m_thumbSampleMode);
            }
        }

        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            m_rewindBuffer.push_front(std::move(frame));
            // 超出最大缓冲帧数时淘汰最旧帧
            while (m_rewindBuffer.size() > REWIND_BUFFER_SIZE)
                m_rewindBuffer.pop_back();
        }
    }

    // ============================================================
    // _stepRewind – 从倒带缓冲区弹出状态并恢复，返回是否成功
    // ============================================================
    bool GameView::_stepRewind()
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        if (m_rewindBuffer.empty()) return false;

        bool didRestore = false;
        // 每次弹出 REWIND_STEP 帧，实现比正常速度快的倒带
        for (unsigned step = 0; step < REWIND_STEP && !m_rewindBuffer.empty(); ++step) {
            if (m_gba_core->Unserialize(m_rewindBuffer.front().state)) {
                m_rewindBuffer.pop_front();
                didRestore = true;
            } else {
                // 状态数据异常，丢弃该帧并停止本次倒带
                brls::Logger::warning("GameView: 倒带状态反序列化失败，丢弃该帧");
                m_rewindBuffer.pop_front();
                break;
            }
        }
        if (didRestore) {
            // 运行一帧以刷新视频输出，保证倒带画面流畅
            m_gba_core->RunFrame();
        }
        return didRestore;
    }

    // ============================================================
    // _stepFrame – 执行正常/快进帧，返回本次运行的帧数
    // ============================================================
    unsigned GameView::_stepFrame(bool ff)
    {
        // 快进时每次迭代运行 FF_MULTIPLIER 帧
        unsigned frames = ff ? FF_MULTIPLIER : 1u;

        for (unsigned i = 0; i < frames; ++i) {
            // 第一帧前保存倒带状态（快进时也保存，保证倒带缓冲区持续更新）
            if (i == 0) _saveRewindState();  
            m_gba_core->RunFrame();
        }
        return frames;
    }

    // ============================================================
    // _captureVideoFrame – 取出最新视频帧并暂存，等待 UI 线程上传
    // ============================================================
    void GameView::_captureVideoFrame()
    {
        auto frame = m_gba_core->GetVideoFrame();
        if (!frame.pixels.empty()) {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            m_pendingFrame = std::move(frame);
            m_frameReady   = true;
        }
    }

    // ============================================================
    // _pushFrameAudio – 推送音频数据（快进时限制推送量）
    // ============================================================
    void GameView::_pushFrameAudio(bool ff, unsigned framesRan)
    {
        if (GameSignal::instance().isMuted()) {
            // 静音时仍需排空缓冲区，防止积压
            std::vector<int16_t> dummy;
            m_gba_core->DrainAudio(dummy);
            return;
        }

        std::vector<int16_t> samples;
        if (!m_gba_core->DrainAudio(samples) || samples.empty()) return;

        size_t frames = samples.size() / 2; // 立体声

        // 快进时（framesRan > 1）限制推送量为单帧音频量，避免撑满音频缓冲区
        if (ff && framesRan > 1) {
            size_t limit = frames / framesRan;
            if (limit > 0) frames = limit;
        }

        AudioManager::instance().pushSamples(samples.data(), frames);
    }

    // ============================================================
    // _updateFpsStats – 更新 FPS 统计（游戏线程侧）
    // ============================================================
    void GameView::_updateFpsStats(unsigned framesRan,
                                   std::chrono::steady_clock::time_point& lastTime,
                                   unsigned& counter)
    {
        using Clock = std::chrono::steady_clock;

        counter += framesRan;
        auto now    = Clock::now();
        double elap = std::chrono::duration<double>(now - lastTime).count();
        if (elap >= FPS_UPDATE_INTERVAL) {
            float fps = static_cast<float>(counter / elap);
            {
                std::lock_guard<std::mutex> lk(m_fpsMutex);
                m_currentFps = fps;
            }
            counter  = 0;
            lastTime = now;
        }
    }

    // ============================================================
    // _updatePlayTime – 更新游戏时长（每 PLAY_TIME_SAVE_INTERVAL 秒保存一次）
    // ============================================================
    void GameView::_updatePlayTime(std::chrono::steady_clock::time_point& lastTime)
    {
        using Clock = std::chrono::steady_clock;

        auto now     = Clock::now();
        double elapsed = std::chrono::duration<double>(now - lastTime).count();
        while (elapsed >= static_cast<double>(PLAY_TIME_SAVE_INTERVAL)) {
            elapsed -= static_cast<double>(PLAY_TIME_SAVE_INTERVAL);
            lastTime += std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(PLAY_TIME_SAVE_INTERVAL));
            m_gameEntry.playTime += PLAY_TIME_SAVE_INTERVAL;
            if (beiklive::GameDB) {
                beiklive::GameDB->upsert(m_gameEntry);
                beiklive::GameDB->flush();
                brls::Logger::debug("游戏时长已保存：{} 秒", m_gameEntry.playTime);
            }
        }
    }

    // ============================================================
    // _throttleFrameRate – 帧率限制器
    //
    // 使用 nextTarget 累加模式（而非每帧重新取 Clock::now()）：
    //   - nextTarget 每帧递增一个 frameDurNs，避免睡眠超时引发的帧率漂移；
    //   - 若某帧耗时超过目标时间（nextTarget 落在过去），直接重置到 now，不补偿；
    //   - 快进状态下不做限速，全速运行。
    // ============================================================
    void GameView::_throttleFrameRate(bool ff,
                                      std::chrono::steady_clock::time_point& nextTarget,
                                      std::chrono::nanoseconds frameDurNs,
                                      std::chrono::nanoseconds spinGuardNs)
    {
        using Clock = std::chrono::steady_clock;

        if (ff) return; // 快进：不限速

        nextTarget += frameDurNs;

        auto now = Clock::now();
        // 漂移防护：若目标时间已落后于当前时间，说明此帧超时，重置目标
        if (nextTarget < now) {
            nextTarget = now;
            return;
        }

        // 粗粒度睡眠（预留自旋等待时间）
        auto coarse = nextTarget - now - spinGuardNs;
        if (coarse.count() > 0)
            std::this_thread::sleep_for(coarse);

        // 精确自旋等待至目标时间
        while (Clock::now() < nextTarget)
            std::this_thread::yield();
    }

    // ============================================================
    // _gameLoop – 游戏主循环（独立线程）
    //
    // 按核心目标帧率执行游戏逻辑，支持：
    //   - 暂停：跳过帧执行
    //   - 倒带：从缓冲区恢复历史状态
    //   - 快进：每迭代运行 FF_MULTIPLIER 帧
    //   - 帧率控制：nextFrameTarget 累加模式，严格对齐目标帧率
    // ============================================================
    void GameView::_gameLoop()
    {
        using Clock = std::chrono::steady_clock;

        if (!m_gba_core || !m_gba_core->IsReady()) return;

#ifdef _WIN32
        timeBeginPeriod(1); // 提升 Windows 定时器精度至 1ms
#endif

        // 从核心获取目标帧率
        double coreFps = m_gba_core->Fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS)
            coreFps = 59.7275;

        // 预计算帧时长（nanoseconds 避免浮点精度损失）
        const auto frameDurNs   = std::chrono::nanoseconds(
            static_cast<long long>(1e9 / coreFps));
        const auto spinGuardNs  = std::chrono::nanoseconds(
            static_cast<long long>(std::min(SPIN_GUARD_SEC, 1.0 / coreFps * 0.1) * 1e9));

        // 帧率限制：nextFrameTarget 累加目标时间点
        auto nextFrameTarget = Clock::now();

        // FPS 统计
        auto     fpsLastTime  = Clock::now();
        unsigned fpsCount     = 0u;

        // 游戏时长记录
        auto playTimeLast = Clock::now();

        GameTimer::instance().start();

        while (m_running.load(std::memory_order_acquire))
        {
            auto& sig = GameSignal::instance();

            // ---- 暂停处理 ----
            if (sig.isPaused()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                // 暂停期间统一推进计时基准，避免恢复后触发积压操作
                auto now     = Clock::now();
                nextFrameTarget = now;
                playTimeLast    = now;
                fpsLastTime     = now;
                continue;
            }

            // ---- 重置请求 ----
            if (sig.consumeReset()) {
                m_gba_core->Reset();
                std::lock_guard<std::mutex> lk(m_rewindMutex);
                m_rewindBuffer.clear(); // 重置后清空倒带缓冲区
            }

            // ---- 快速存档 ----
            int saveSlot = sig.consumeQuickSave();
            if (saveSlot >= 0)
                _doSaveState(saveSlot);

            // ---- 快速读档 ----
            int loadSlot = sig.consumeQuickLoad();
            if (loadSlot >= 0)
                _doLoadState(loadSlot);

            // ---- 倒带帧恢复（由可视化倒带UI触发）----
            int restoreIdx = sig.consumeRewindRestore();
            if (restoreIdx >= 0) {
                std::lock_guard<std::mutex> lk(m_rewindMutex);
                if (restoreIdx < static_cast<int>(m_rewindBuffer.size())) {
                    if (!m_gba_core->Unserialize(m_rewindBuffer[restoreIdx].state)) {
                        brls::Logger::warning("GameView: 倒带帧恢复失败 idx={}", restoreIdx);
                    } else {
                        // 恢复后丢弃该帧之前的所有帧（比该帧更新的帧）
                        while (static_cast<int>(m_rewindBuffer.size()) > restoreIdx)
                            m_rewindBuffer.pop_front();
                        // 运行一帧以刷新渲染缓冲区，确保 UI 线程能立即看到恢复后的画面
                        m_gba_core->RunFrame();
                    }
                }
            }

            // ---- 从信号更新游戏按键状态 ----
            m_gba_core->SetButtonsFromSignal();

            // ---- 决定本帧行为 ----
            bool ff      = sig.isFastForward();
            bool rew     = sig.isRewinding();

            // 倒带时禁用快进，防止逻辑冲突
            if (rew) ff = false;

            // 通知核心当前快进状态（供 RETRO_ENVIRONMENT_GET_FASTFORWARDING 查询）
            m_gba_core->SetFastForwarding(ff);

            // ---- 执行核心帧逻辑 ----
            unsigned framesRan = 1u;

            if (rew) {
                // 倒带：从历史缓冲区恢复状态
                _stepRewind();
            } else {
                // 正常 / 快进：运行核心并保存倒带状态
                framesRan = _stepFrame(ff);
            }

            // ---- 取出视频帧暂存 ----
            _captureVideoFrame();

            // ---- 推送音频 ----
            _pushFrameAudio(ff, framesRan);

            // ---- FPS 统计 ----
            _updateFpsStats(framesRan, fpsLastTime, fpsCount);

            // ---- 游戏时长记录 ----
            _updatePlayTime(playTimeLast);

            // ---- 帧率限制（快进时不限速）----
            _throttleFrameRate(ff, nextFrameTarget, frameDurNs, spinGuardNs);
        }

        // ---- 音频清理 ----
        AudioManager::instance().deinit();

#ifdef _WIN32
        timeEndPeriod(1);
#endif
    }

    // ============================================================
    // 即时存档路径计算
    //
    // 存档文件命名：{romStem}.ss{slot}  (slot=0 为自动存档)
    // 缩略图：       {statePath}.png
    // 存档目录优先级：GameEntry.savePath → 全局 saves 目录
    // ============================================================

    std::string GameView::getStatePath(int slot) const
    {
        namespace fs = std::filesystem;

        // 确定存档目录
        std::string dir = m_gameEntry.savePath.empty()
                          ? beiklive::path::savePath()
                          : m_gameEntry.savePath;

        // 提取 ROM 文件名（不含扩展名）
        std::string stem;
        if (!m_gameEntry.path.empty())
            stem = fs::path(m_gameEntry.path).stem().string();
        else
            stem = "game";

        // 确保目录存在
        if (!dir.empty()) {
            std::error_code ec;
            fs::create_directories(dir, ec);
        }

        // 路径分隔符
        std::string sep;
        if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
            sep = "/";

        return dir + sep + stem + ".ss" + std::to_string(slot);
    }

    std::string GameView::getStateThumbPath(int slot) const
    {
        return getStatePath(slot) + ".png";
    }

    bool GameView::stateExists(int slot) const
    {
        std::error_code ec;
        return std::filesystem::exists(getStatePath(slot), ec);
    }

    // ============================================================
    // _doSaveState – 序列化核心状态并保存截图（游戏线程调用）
    // ============================================================

    void GameView::_doSaveState(int slot)
    {
        if (!m_gba_core || !m_gba_core->IsReady()) return;

        std::vector<uint8_t> buf;
        if (!m_gba_core->Serialize(buf) || buf.empty()) {
            brls::Logger::warning("GameView: 存档序列化失败 (slot {})", slot);
            brls::sync([slot](){
                brls::Application::notify("存档失败 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        std::string path = getStatePath(slot);
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) {
            brls::Logger::warning("GameView: 无法打开存档文件写入: {}", path);
            brls::sync([slot](){
                brls::Application::notify("存档失败：无法写入文件 (slot " + std::to_string(slot) + ")");
            });
            return;
        }
        f.write(reinterpret_cast<const char*>(buf.data()),
                static_cast<std::streamsize>(buf.size()));
        f.close();

        brls::Logger::info("GameView: 已保存到 {} ({} bytes)", path, buf.size());

        // 保存缩略图
        auto frame = m_gba_core->GetVideoFrame();
        if (!frame.pixels.empty() && frame.width > 0 && frame.height > 0) {
            std::string thumbPath = getStateThumbPath(slot);
            stbi_write_png(thumbPath.c_str(),
                           static_cast<int>(frame.width),
                           static_cast<int>(frame.height),
                           4,   // RGBA
                           frame.pixels.data(),
                           static_cast<int>(frame.width * 4));
        }

        // UI 线程通知
        brls::sync([slot](){
            std::string msg = (slot == 0) ? "已保存到自动存档" : "已保存到槽位 " + std::to_string(slot);
            brls::Application::notify(msg);
        });
    }

    // ============================================================
    // _doLoadState – 从文件反序列化核心状态（游戏线程调用）
    // ============================================================

    void GameView::_doLoadState(int slot)
    {
        if (!m_gba_core || !m_gba_core->IsReady()) return;

        std::string path = getStatePath(slot);
        if (!std::filesystem::exists(path)) {
            brls::Logger::warning("GameView: 存档文件不存在: {}", path);
            brls::sync([slot](){
                brls::Application::notify("读取失败：槽位 " + std::to_string(slot) + " 无存档");
            });
            return;
        }

        std::ifstream f(path, std::ios::binary);
        if (!f) {
            brls::Logger::warning("GameView: 无法打开存档文件读取: {}", path);
            brls::sync([slot](){
                brls::Application::notify("读取失败：无法读取文件 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        f.seekg(0, std::ios::end);
        std::streampos fileSize = f.tellg();
        f.seekg(0, std::ios::beg);
        if (fileSize <= 0) {
            brls::sync([slot](){
                brls::Application::notify("读取失败：存档文件为空 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        std::vector<uint8_t> buf(static_cast<size_t>(fileSize));
        f.read(reinterpret_cast<char*>(buf.data()), fileSize);
        std::streamsize got = f.gcount();
        f.close();

        if (!m_gba_core->Unserialize(buf)) {
            brls::Logger::warning("GameView: 存档反序列化失败 (slot {})", slot);
            brls::sync([slot](){
                brls::Application::notify("读取失败 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        // 读档后清空倒带缓冲区，避免时序混乱
        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            m_rewindBuffer.clear();
        }

        brls::Logger::info("GameView: 已从 {} 读取状态 ({} bytes)", path, got);
        brls::sync([slot](){
            std::string msg = (slot == 0) ? "已从自动存档读取" : "已从槽位 " + std::to_string(slot) + " 读取";
            brls::Application::notify(msg);
        });
    }

    // ============================================================
    // snapshotRewindThumbs – 获取倒带缓冲区缩略图快照（UI 线程调用）
    // 游戏已暂停时调用，均匀采样至多 maxItems 条
    // ============================================================
    std::vector<std::pair<int, std::vector<uint16_t>>>
    GameView::snapshotRewindThumbs(int maxItems) const
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        std::vector<std::pair<int, std::vector<uint16_t>>> result;

        int total = static_cast<int>(m_rewindBuffer.size());
        if (total == 0) return result;

        // 若 maxItems <= 0 或缓冲帧数不超过限制，则全部返回
        if (maxItems <= 0 || total <= maxItems) {
            result.reserve(total);
            for (int i = 0; i < total; ++i)
                result.emplace_back(i, m_rewindBuffer[i].thumb);
        } else if (maxItems == 1) {
            // 只取最新帧
            result.emplace_back(0, m_rewindBuffer[0].thumb);
        } else {
            // 均匀采样：在 [0, total-1] 范围内选取 maxItems 个索引（maxItems >= 2，不会除零）
            result.reserve(maxItems);
            for (int k = 0; k < maxItems; ++k) {
                int idx = k * (total - 1) / (maxItems - 1);
                result.emplace_back(idx, m_rewindBuffer[idx].thumb);
            }
        }
        return result;
    }

    // ============================================================
    // requestRestoreRewindFrame – 通过 GameSignal 请求恢复指定帧
    // ============================================================
    void GameView::requestRestoreRewindFrame(int frameIndex)
    {
        GameSignal::instance().requestRewindRestore(frameIndex);
    }

    // ============================================================
    // _downsampleToRGB565 – RGBA8888 降采样并转换为 RGB565
    // 支持三种质量模式：最近邻、区域平均（盒式滤波）、双线性插值
    // ============================================================
    std::vector<uint16_t> GameView::_downsampleToRGB565(
        const std::vector<uint32_t>& src,
        unsigned srcW, unsigned srcH,
        unsigned dstW, unsigned dstH,
        ThumbSampleMode mode)
    {
        std::vector<uint16_t> dst(dstW * dstH, 0);
        if (src.empty() || srcW == 0 || srcH == 0) return dst;

        // 辅助 lambda：将 r/g/b 分量打包为 RGB565
        auto packRgb565 = [](uint32_t r, uint32_t g, uint32_t b) -> uint16_t {
            return static_cast<uint16_t>(
                ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        };

        if (mode == ThumbSampleMode::AreaAverage) {
            // ── 区域平均（盒式滤波）──────────────────────────────────────────
            // 对每个目标像素，计算其对应源区域内所有像素的平均值，降采样质量最好
            for (unsigned dy = 0; dy < dstH; ++dy) {
                unsigned sy0 = dy * srcH / dstH;
                unsigned sy1 = (dy + 1) * srcH / dstH;
                if (sy1 <= sy0) sy1 = sy0 + 1;
                if (sy1 > srcH) sy1 = srcH;

                for (unsigned dx = 0; dx < dstW; ++dx) {
                    unsigned sx0 = dx * srcW / dstW;
                    unsigned sx1 = (dx + 1) * srcW / dstW;
                    if (sx1 <= sx0) sx1 = sx0 + 1;
                    if (sx1 > srcW) sx1 = srcW;

                    uint32_t r = 0, g = 0, b = 0, count = 0;
                    for (unsigned sy = sy0; sy < sy1; ++sy) {
                        for (unsigned sx = sx0; sx < sx1; ++sx) {
                            uint32_t px = src[sy * srcW + sx];
                            r += (px >> 16) & 0xFF;
                            g += (px >>  8) & 0xFF;
                            b +=  px        & 0xFF;
                            ++count;
                        }
                    }
                    if (count > 0) {
                        dst[dy * dstW + dx] = packRgb565(r / count, g / count, b / count);
                    }
                }
            }
        } else if (mode == ThumbSampleMode::Bilinear) {
            // ── 双线性插值 ───────────────────────────────────────────────────
            // 对每个目标像素，采用 2×2 双线性加权插值，质量均衡
            for (unsigned dy = 0; dy < dstH; ++dy) {
                float fy = (dy + 0.5f) * static_cast<float>(srcH) / static_cast<float>(dstH) - 0.5f;
                unsigned y0 = (fy > 0.f) ? static_cast<unsigned>(fy) : 0u;
                unsigned y1 = (y0 + 1 < srcH) ? y0 + 1 : y0;
                float wy = fy - static_cast<float>(y0);
                if (wy < 0.f) wy = 0.f;

                for (unsigned dx = 0; dx < dstW; ++dx) {
                    float fx = (dx + 0.5f) * static_cast<float>(srcW) / static_cast<float>(dstW) - 0.5f;
                    unsigned x0 = (fx > 0.f) ? static_cast<unsigned>(fx) : 0u;
                    unsigned x1 = (x0 + 1 < srcW) ? x0 + 1 : x0;
                    float wx = fx - static_cast<float>(x0);
                    if (wx < 0.f) wx = 0.f;

                    // 双线性权重：(1-wx)(1-wy), wx(1-wy), (1-wx)wy, wx*wy
                    auto getComp = [&](unsigned sx, unsigned sy, int shift) -> float {
                        return static_cast<float>((src[sy * srcW + sx] >> shift) & 0xFF);
                    };
                    float r = getComp(x0,y0,16)*(1.f-wx)*(1.f-wy) + getComp(x1,y0,16)*wx*(1.f-wy)
                            + getComp(x0,y1,16)*(1.f-wx)*wy       + getComp(x1,y1,16)*wx*wy;
                    float g = getComp(x0,y0, 8)*(1.f-wx)*(1.f-wy) + getComp(x1,y0, 8)*wx*(1.f-wy)
                            + getComp(x0,y1, 8)*(1.f-wx)*wy       + getComp(x1,y1, 8)*wx*wy;
                    float b = getComp(x0,y0, 0)*(1.f-wx)*(1.f-wy) + getComp(x1,y0, 0)*wx*(1.f-wy)
                            + getComp(x0,y1, 0)*(1.f-wx)*wy       + getComp(x1,y1, 0)*wx*wy;

                    dst[dy * dstW + dx] = packRgb565(
                        static_cast<uint32_t>(r), static_cast<uint32_t>(g), static_cast<uint32_t>(b));
                }
            }
        } else {
            // ── 最近邻（默认）──────────────────────────────────────────────
            for (unsigned dy = 0; dy < dstH; ++dy) {
                for (unsigned dx = 0; dx < dstW; ++dx) {
                    unsigned sx = dx * srcW / dstW;
                    unsigned sy = dy * srcH / dstH;
                    uint32_t px = src[sy * srcW + sx]; // RGBA8888
                    dst[dy * dstW + dx] = packRgb565(
                        (px >> 16) & 0xFF, (px >> 8) & 0xFF, px & 0xFF);
                }
            }
        }
        return dst;
    }

} // namespace beiklive
