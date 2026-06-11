#include "GameView.hpp"
#include "GameMenuView.hpp"
#include "RewindSelectorView.hpp"
#include "game/audio/AudioManager.hpp"
#include "ui/audio/BKAudioPlayer.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "core/Tools.hpp"

#include <algorithm>
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
        brls::Logger::debug("[GameView] constructor: platform={}, path={}",
            m_gameEntry.platform, m_gameEntry.path);
        _brls_inputLocked = false;
        GameInputManager::instance().sayHello();
        HIDE_BRLS_HIGHLIGHT(this);

        // 从 GameEntry 加载画面模式（默认 Fit）
        m_screenMode = static_cast<beiklive::ScreenMode>(m_gameEntry.displayMode);

        // 从配置读取倒带相关设置
        m_rewindSaveInterval = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, 1);
        // 确保间隔值在合法范围内（与设置页面的选项匹配：1/2/4/8/16/60/120）
        if (m_rewindSaveInterval < 1)   m_rewindSaveInterval = 1;
        if (m_rewindSaveInterval > 120) m_rewindSaveInterval = 120;
        m_rewindBufferSize = static_cast<unsigned>(
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_BUFFER_SIZE, 600));
        if (m_rewindBufferSize < 10)   m_rewindBufferSize = 10;
        if (m_rewindBufferSize > 1800) m_rewindBufferSize = 1800;
        m_rewindEnabled = GET_SETTING_KEY_INT("rewind.enabled", 0) != 0;
        m_rewindShowUI = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SHOW_UI, 0) != 0;

        // 缓存缩略图压缩模式（避免每帧读取配置）
        m_cachedThumbCompression = GET_SETTING_KEY_INT(
            beiklive::SettingKey::KEY_REWIND_THUMB_COMPRESSION, 0);

        // 读取连发速率（Hz）
        {
            float turboHz = GET_SETTING_KEY_FLOAT("turbo.rate", 10.0f);
            if (turboHz < 1.0f) turboHz = 1.0f;
            if (turboHz > 30.0f) turboHz = 30.0f;
            m_turboToggleInterval = std::max(1, static_cast<int>(30.0f / turboHz));
        }

        _registerGameInput();
        _registerGameRuntime();
    }

    GameView::~GameView()
    {
        brls::Logger::debug("[GameView] destructor: platform={}, path={}",
            m_gameEntry.platform, m_gameEntry.path);
        _stopGameThread();

        if (m_core) {
            delete m_core;
            m_core = nullptr;
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
                    // 菜单从底部滑入，入场动画（120ms）
                    AnimationHelper::slideInFromBottom(m_gameMenuView, 60.f, 120);
                    brls::Application::giveFocus(m_gameMenuView);
                    m_gameMenuView->onShow();
                });
            }
            // 不提前返回：继续渲染当前游戏帧，防止菜单弹出时出现黑帧闪烁
        }

        // 消费打开倒带UI信号：暂停游戏并弹出可视化倒带选择界面
        if (GameSignal::instance().consumeOpenRewindUI()) {
            if (m_rewindSelectorView) {
                GameSignal::instance().requestPause(true);
                // 取出缩略图快照（游戏已暂停，可安全读取缓冲区）
                auto thumbs = snapshotRewindThumbs();
                brls::sync([this, thumbs = std::move(thumbs)]() mutable {
                    m_rewindSelectorView->openWithFrames(std::move(thumbs));
                    AnimationHelper::slideInFromBottom(m_rewindSelectorView, 80.f, 220);
                    brls::Application::giveFocus(m_rewindSelectorView);
                });
            }
        }

        // 初始化渲染器（首帧时，GL 上下文已就绪）
        if (!m_rendererReady && m_core && m_core->IsReady()) {
            unsigned gw = m_core->GameWidth()  > 0 ? m_core->GameWidth()  : beiklive::GetGamePixelWidth(m_gameEntry.platform);
            unsigned gh = m_core->GameHeight() > 0 ? m_core->GameHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);
            // 若游戏条目启用了着色器且路径有效，则传入着色器路径初始化渲染链
            std::string shaderPath;
            if (m_gameEntry.shaderEnabled && !m_gameEntry.shaderPath.empty()) {
                shaderPath = m_gameEntry.shaderPath;
                // _onShaderToggle(true); // 同步着色器开关状态，确保启用着色器
            }
            if (m_renderer.init(gw, gh, false, shaderPath)) {
                m_rendererReady = true;
                brls::Logger::info("GameView: 渲染器初始化完成 ({}x{} shader={})",
                                   gw, gh,
                                   shaderPath.empty() ? "无" : shaderPath);
                // 应用保存的着色器参数
                if (!shaderPath.empty() && !m_gameEntry.shaderParaNames.empty()) {
                    for (size_t i = 0; i < m_gameEntry.shaderParaNames.size() &&
                                       i < m_gameEntry.shaderParaValues.size(); ++i) {
                        m_renderer.setShaderParam(m_gameEntry.shaderParaNames[i],
                                                  m_gameEntry.shaderParaValues[i]);
                    }
                    brls::Logger::info("GameView: 已应用 {} 个保存的着色器参数",
                                       std::min(m_gameEntry.shaderParaNames.size(),
                                                m_gameEntry.shaderParaValues.size()));
                }
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

            int intScale = static_cast<int>(m_gameEntry.integerAspectRatio);
            beiklive::DisplayRect rect = beiklive::computeDisplayRect(
                m_screenMode, x, y, width, height, gw, gh,
                m_gameEntry.customScale, m_gameEntry.customOffsetX, m_gameEntry.customOffsetY,
                intScale);

            m_renderer.drawToScreen(rect.x, rect.y, rect.w, rect.h, windowScale, windowW, windowH);

            // 绘制遮罩层（覆盖整个 GameView 区域）
            if (m_gameEntry.overlayEnabled && !m_gameEntry.overlayPath.empty())
            {
                if (!m_overlayImage)
                {
                    m_overlayImage = new brls::Image();
                    m_overlayImage->setScalingType(brls::ImageScalingType::STRETCH);
                    m_overlayImage->setWidth(1280.f);
                    m_overlayImage->setHeight(720.f);
                }
                if (!m_overlayImage->getTexture() && std::filesystem::exists(m_gameEntry.overlayPath))
                    m_overlayImage->setImageFromFile(m_gameEntry.overlayPath);
                if (m_overlayImage->getTexture())
                    m_overlayImage->draw(vg, x, y, width, height, style, ctx);
            }
            else if (m_overlayImage && m_overlayImage->getTexture())
            {
                m_overlayImage->clear();
            }
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
        if (GET_SETTING_KEY_INT("display.showFps", 0))
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
        if (sig.isFastForward() && GET_SETTING_KEY_INT("display.showFfOverlay", 1))
            GameOverlayRenderer::drawFastForward(vg, x, y, w);

        // 倒带覆盖层（顶部居中）
        if (sig.isRewinding() && GET_SETTING_KEY_INT("display.showRewindOverlay", 1))
            GameOverlayRenderer::drawRewind(vg, x, y, w);

        // 暂停覆盖层（顶部居中，快进/倒带时不另外显示）
        if (sig.isPaused() && !sig.isFastForward() && !sig.isRewinding())
            GameOverlayRenderer::drawPaused(vg, x, y, w);

        // 静音覆盖层（右下角）
        if (sig.isMuted() && GET_SETTING_KEY_INT("display.showMuteOverlay", 1))
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
            if (val == "none" || val.empty()) continue;
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

        // 快进（支持按住/切换两种模式）
        {
            std::string val = GET_SETTING_KEY_STR("handle.fastforward", "LSB");
            std::string mode = GET_SETTING_KEY_STR("fastforward.mode", "hold");
            auto combos = beiklive::tools::parseMultiCombo(val);
            if (mode == "hold") {
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_FAST_FORWARD, {combo},
                        []() { GameSignal::instance().requestFastForward(true); },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_FAST_FORWARD, {combo},
                        []() { GameSignal::instance().requestFastForward(false); },
                        TriggerType::RELEASE);
                }
            } else {
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
        }

        // 倒带切换：若启用可视化倒带界面则打开UI，否则执行传统倒带
        {
            std::string val  = GET_SETTING_KEY_STR("handle.rewind", "RSB");
            std::string mode = GET_SETTING_KEY_STR("rewind.mode", "hold");
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
                } else if (mode == "hold") {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        []() { GameSignal::instance().requestRewind(true); },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        []() { GameSignal::instance().requestRewind(false); },
                        TriggerType::RELEASE);
                } else {
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

        // 连发 A（Turbo A）
        {
            std::string val = GET_SETTING_KEY_STR("handle.a_turbo", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_A_TURBO, {combo},
                    [this]() { m_turboAheld.store(true, std::memory_order_release); },
                    TriggerType::HOLD);
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_A_TURBO, {combo},
                    [this]() { m_turboAheld.store(false, std::memory_order_release); },
                    TriggerType::RELEASE);
            }
        }

        // 连发 B（Turbo B）
        {
            std::string val = GET_SETTING_KEY_STR("handle.b_turbo", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_B_TURBO, {combo},
                    [this]() { m_turboBheld.store(true, std::memory_order_release); },
                    TriggerType::HOLD);
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_B_TURBO, {combo},
                    [this]() { m_turboBheld.store(false, std::memory_order_release); },
                    TriggerType::RELEASE);
            }
        }
    }

    // ============================================================
    // _registerGameRuntime – 创建并初始化核心，启动游戏线程
    // ============================================================
    void GameView::_registerGameRuntime()
    {
        brls::Logger::debug("[GameView] _registerGameRuntime: platform={}", m_gameEntry.platform);
        m_core = CreateEmulatorCore(m_gameEntry.platform);
        if (!m_core) {
            brls::Logger::warning("[GameView] _registerGameRuntime: unsupported platform={}", m_gameEntry.platform);
            return;
        }

        if (m_core->SetupGame(m_gameEntry))
        {
            brls::Logger::debug("核心已初始化，平台={}, 路径={}",
                                beiklive::tools::platformName(m_gameEntry.platform),
                                m_gameEntry.path);
            if (auto* player = dynamic_cast<beiklive::BKAudioPlayer*>(
                    brls::Application::getAudioPlayer()))
            {
                constexpr std::chrono::milliseconds kAudioPlayerWaitTimeout{500};
                auto deadline = std::chrono::steady_clock::now() + kAudioPlayerWaitTimeout;
                while (player->isPlaying()
                       && std::chrono::steady_clock::now() < deadline)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            double fps = m_core->Fps();
            if (fps <= 0.0) fps = 59.7;
            double srate = m_core->SampleRate();
            if (srate <= 0.0) srate = 32768.0;
            brls::Logger::debug("[GameView] audio init: fps={:.2f} sampleRate={:.0f}", fps, srate);
            AudioManager::instance().init(static_cast<int>(srate), 2);
            {
                std::vector<int16_t> initAudioDiscard;
                m_core->DrainAudio(initAudioDiscard);
            }
            GameSignal::instance().resetAll();
            _initPlayTimeTracking();
            brls::Logger::debug("[GameView] starting game thread...");
            _startGameThread();
        }
        else
        {
            brls::Logger::error("核心初始化失败，平台={}, 路径={}",
                                beiklive::tools::platformName(m_gameEntry.platform),
                                m_gameEntry.path);
            delete m_core;
            m_core = nullptr;
        }
    }

    // ============================================================
    // _startGameThread / _stopGameThread
    // ============================================================
    void GameView::_startGameThread()
    {
        brls::Logger::debug("[GameView] _startGameThread");
        m_running.store(true, std::memory_order_release);
        m_gameThread = std::thread(&GameView::_gameLoop, this);
    }

    void GameView::_stopGameThread()
    {
        brls::Logger::debug("[GameView] _stopGameThread begin");
        m_running.store(false, std::memory_order_release);
        if (m_gameThread.joinable())
            m_gameThread.join();
        brls::Logger::debug("[GameView] _stopGameThread end");
    }

    // ============================================================
    // _saveRewindState – 序列化当前核心状态并存入倒带缓冲区
    // 支持间隔保存（每 m_rewindSaveInterval 帧保存一次）
    // 若 m_rewindShowUI 开启则同时捕获 RGB565 缩略图
    // ============================================================
    void GameView::_saveRewindState()
    {
        if (!m_rewindEnabled)
            return;

        // 间隔控制：每 m_rewindSaveInterval 帧才保存一次
        ++m_rewindFrameCounter;
        if (m_rewindFrameCounter < static_cast<unsigned>(m_rewindSaveInterval))
            return;
        m_rewindFrameCounter = 0;

        RewindFrame frame;
        if (!m_core->Serialize(frame.state) || frame.state.empty())
            return;

        // 若启用可视化倒带界面，则同时捕获并压缩缩略图
        if (m_rewindShowUI) {
            auto videoFrame = m_core->GetVideoFrame();
            if (!videoFrame.pixels.empty() && videoFrame.width > 0 && videoFrame.height > 0) {
                frame.thumb = _downsampleToRGB565(
                    videoFrame.pixels, videoFrame.width, videoFrame.height,
                    RewindFrame::THUMB_W, RewindFrame::THUMB_H);
            }
        }

        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            m_rewindBuffer.push_front(std::move(frame));
            // 根据保存间隔计算实际最大条目数：
            // m_rewindBufferSize 表示"最多缓存多少帧游戏时间"（如 1800 = 60fps × 30s = 30秒）。
            // 每个条目覆盖 m_rewindSaveInterval 帧，因此最大条目数 = bufferSize / saveInterval。
            // 这样无论 saveInterval 取何值，实际缓冲时长始终等于 bufferSize/60 秒。
            // 使用 std::max(1u, ...) 避免 saveInterval 意外为 0 时的除零错误
            unsigned saveInterval = static_cast<unsigned>(std::max(1, m_rewindSaveInterval));
            unsigned maxEntries = std::max(1u, m_rewindBufferSize / saveInterval);
            while (m_rewindBuffer.size() > maxEntries)
                m_rewindBuffer.pop_back();
        }
    }

    // ============================================================
    // _stepRewind – 从倒带缓冲区弹出状态并恢复，返回是否成功
    // 优化：Unserialize（可能较慢）在锁外执行，减少临界区
    // ============================================================
    bool GameView::_stepRewind()
    {
        // 在锁内取出待恢复的状态副本，锁外执行反序列化
        std::vector<uint8_t> stateToRestore;
        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            if (m_rewindBuffer.empty()) return false;

            for (unsigned step = 0; step < REWIND_STEP && !m_rewindBuffer.empty(); ++step) {
                stateToRestore = std::move(m_rewindBuffer.front().state);
                m_rewindBuffer.pop_front();
                // 仅保留最后弹出的帧用于恢复（快退效果：弹出多帧后恢复最后一帧 = 快退 REWIND_STEP 帧）
            }
        }

        if (stateToRestore.empty()) return false;

        // 锁外执行反序列化（可能涉及内存分配、状态重建，较耗时）
        if (!m_core->Unserialize(stateToRestore)) {
            brls::Logger::warning("GameView: 倒带状态反序列化失败，丢弃该帧");
            return false;
        }

        // 运行一帧以刷新视频输出，保证倒带画面流畅
        m_core->RunFrame();
        return true;
    }

    // ============================================================
    // _stepFrame – 执行正常/快进帧，返回本次运行的帧数
    // ============================================================
    unsigned GameView::_stepFrame(bool ff)
    {
        if (!ff) {
            _saveRewindState();
            m_core->RunFrame();
            return 1u;
        }

        if (m_ffMultiplier >= 1.0f) {
            unsigned integerPart = static_cast<unsigned>(m_ffMultiplier);
            float fracPart = m_ffMultiplier - static_cast<float>(integerPart);

            unsigned frames = integerPart;

            // 小数部分累加：累加器满 1.0 时多跑一帧
            m_ffSlowAccum += fracPart;
            if (m_ffSlowAccum >= 1.0f) {
                m_ffSlowAccum -= 1.0f;
                ++frames;
            }

            if (frames == 0) frames = 1u;
            for (unsigned i = 0; i < frames; ++i) {
                if (i == 0) _saveRewindState();
                m_core->RunFrame();
            }
            return frames;
        }

        // 慢动作：使用累加器，仅在累积满 1 帧时才执行一帧
        m_ffSlowAccum += m_ffMultiplier;
        if (m_ffSlowAccum >= 1.0f) {
            m_ffSlowAccum -= 1.0f;
            _saveRewindState();
            m_core->RunFrame();
            return 1u;
        }
        return 0u;
    }

    // ============================================================
    // _captureVideoFrame – 取出最新视频帧并暂存，等待 UI 线程上传
    // ============================================================
    void GameView::_captureVideoFrame()
    {
        auto frame = m_core->GetVideoFrame();
        if (!frame.pixels.empty()) {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            m_pendingFrame = std::move(frame);
            m_frameReady   = true;
        }
    }

    // ============================================================
    // _pushFrameAudio – 推送音频数据（快进时限制推送量）
    // ============================================================
    void GameView::_pushFrameAudio(bool ff)
    {
        if (GameSignal::instance().isMuted()) {
            m_core->DrainAudio(m_audioDrainBuf);
            return;
        }

        if (!m_core->DrainAudio(m_audioDrainBuf) || m_audioDrainBuf.empty()) return;

        size_t frames = m_audioDrainBuf.size() / 2;

        if (ff) {
            if (m_ffMute)
                return;
        AudioManager::instance().pushSamples(m_audioDrainBuf.data(), frames);
            return;
        }
        if (GameSignal::instance().isRewinding() && GET_SETTING_KEY_INT("rewind.mute", 0)) {
            return;
        }

        AudioManager::instance().pushSamplesNoBlocking(m_audioDrainBuf.data(), frames);
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
        if (elap >= FPS_UPDATE_INTERVAL && elap > 0.0) {
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
    // _savePlayTimeCheckpoint – 计时累加到 playTime 并写入临时文件
    // ============================================================
    void GameView::_savePlayTimeCheckpoint()
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - m_playStartTime).count();
        if (elapsed < 0.5) return; // 忽略极短间隔

        m_gameEntry.playTime += static_cast<int>(elapsed);
        m_playStartTime = now;

        if (!m_playTimeTempPath.empty()) {
            std::ofstream f(m_playTimeTempPath, std::ios::trunc);
            if (f) { f << m_gameEntry.playTime; f.close(); }
        }
    }

    // ============================================================
    // _saveAndCommitPlayTime – 累加剩余时长并提交到 GameDB
    // ============================================================
    void GameView::_saveAndCommitPlayTime()
    {
        if (m_playTimeTempPath.empty()) return;

        _savePlayTimeCheckpoint();

        if (beiklive::GameDB && m_gameEntry.playTime > 0) {
            // beiklive::GameDB->upsert(m_gameEntry);
            beiklive::GameDB->set(m_gameEntry.path, "playTime", nlohmann::json(m_gameEntry.playTime));
            // beiklive::GameDB->flush();
        }
        std::error_code ec;
        std::filesystem::remove(m_playTimeTempPath, ec);
    }

    // ============================================================
    // _initPlayTimeTracking – 启动时检查遗留临时文件并合并到 GameDB
    // ============================================================
    void GameView::_initPlayTimeTracking()
    {
        namespace fs = std::filesystem;

        // 确定存档目录（与 getStatePath 逻辑一致）
        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();

        // 提取 ROM 文件名（不含扩展名）
        std::string stem;
        if (!m_gameEntry.path.empty())
            stem = fs::path(m_gameEntry.path).stem().string();
        else
            stem = "game";

        std::error_code ec;
        fs::create_directories(dir, ec);

        m_playTimeTempPath = dir + "/" + stem + ".playtime";

        // 检查是否存在遗留的临时文件（上次异常退出或未正常终止）
        if (fs::exists(m_playTimeTempPath, ec) && !ec) {
            try {
                std::ifstream f(m_playTimeTempPath);
                if (f) {
                    long long legacySeconds = 0;
                    f >> legacySeconds;
                    f.close();
                    if (legacySeconds > 0) {
                        // 取遗留值和当前 GameEntry 中保存值的最大值，避免数据倒退
                        m_gameEntry.playTime = std::max(m_gameEntry.playTime, static_cast<int>(legacySeconds));
                        if (beiklive::GameDB) {
                            beiklive::GameDB->upsertByPath(m_gameEntry);
                            beiklive::GameDB->flush();
                            brls::Logger::info("GameView: 已合并遗留时长 {} 秒到 GameDB，清理临时文件",
                                               legacySeconds);
                        }
                        // 合并后删除临时文件，由热路径重新创建
                        fs::remove(m_playTimeTempPath, ec);
                    }
                }
            } catch (...) {
                brls::Logger::warning("GameView: 读取遗留时长临时文件失败");
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

        nextTarget += frameDurNs;

        auto now = Clock::now();
        if (nextTarget < now) {
            nextTarget = now;
            if (ff)
                std::this_thread::yield();
            return;
        }

        auto coarse = nextTarget - now - spinGuardNs;
        if (coarse.count() > 0)
            std::this_thread::sleep_for(coarse);

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

        if (!m_core || !m_core->IsReady()) return;

        brls::Logger::debug("[GameView] _gameLoop enter");

#ifdef _WIN32
        timeBeginPeriod(1); // 提升 Windows 定时器精度至 1ms
#endif

        // 从核心获取目标帧率
        double coreFps = m_core->Fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS)
            coreFps = 59.7275;

        // 预计算帧时长（nanoseconds 避免浮点精度损失）
        const auto baseFrameDurNs = std::chrono::nanoseconds(
            static_cast<long long>(1e9 / coreFps));
        auto frameDurNs         = baseFrameDurNs;
        const auto spinGuardNs  = std::chrono::nanoseconds(
            static_cast<long long>(std::min(SPIN_GUARD_SEC, 1.0 / coreFps * 0.1) * 1e9));

        // 帧率限制：nextFrameTarget 累加目标时间点
        auto nextFrameTarget = Clock::now();

        // FPS 统计
        auto     fpsLastTime  = Clock::now();
        unsigned fpsCount     = 0u;

        GameTimer::instance().start();

        brls::Logger::info("GameView: 游戏循环开始 playTime={} coreFps={:.2f}",
                           m_gameEntry.playTime, coreFps);

        m_playStartTime = Clock::now();
        bool wasPaused  = false;

        // 初始化 SRAM 检测时间
        m_sramLastCheck = Clock::now();

        // ── 读取自动存档配置 ──
        int autoLoadSlot = GET_SETTING_KEY_INT("save.autoLoadState0", 0);
        int autoSaveSlot = GET_SETTING_KEY_INT("save.autoSaveState", 0);
        int autoSaveSecs = GET_SETTING_KEY_INT("save.autoSaveInterval", 0);
        m_autoSaveTimer = Clock::now();
        bool autoLoadDone = false;

        // 读取快进倍率配置
        m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
        if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;
        m_ffSlowAccum = 0.0f;
        m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

        // 读取连发速率
        {
            float turboHz = GET_SETTING_KEY_FLOAT("turbo.rate", 10.0f);
            if (turboHz < 1.0f) turboHz = 1.0f;
            if (turboHz > 30.0f) turboHz = 30.0f;
            m_turboToggleInterval = std::max(1, static_cast<int>(coreFps / (turboHz * 2.0f)));
        }


        // 读取shader开关配置
        // _onShaderToggle(GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, 0));


        while (m_running.load(std::memory_order_acquire))
        {
            auto& sig = GameSignal::instance();

            // ---- 自动加载即时存档 ----
            if (!sig.isPaused() && !autoLoadDone && autoLoadSlot > 0 && m_core && m_core->IsReady()) {
                if (stateExists(autoLoadSlot - 1)) {
                    _doLoadState(autoLoadSlot - 1);
                    brls::Logger::info("GameView: 自动加载存档槽 {}", autoLoadSlot - 1);
                }
                autoLoadDone = true;
            }

            // ---- 自动保存即时存档 ----
            if (!sig.isPaused() && !wasPaused && autoSaveSlot > 0 && autoSaveSecs > 0) {
                auto now = Clock::now();
                double sinceSave = std::chrono::duration<double>(now - m_autoSaveTimer).count();
                if (sinceSave >= static_cast<double>(autoSaveSecs)) {
                    _doSaveState(autoSaveSlot - 1);
                    m_autoSaveTimer = now;
                    brls::Logger::debug("GameView: 自动保存存档槽 {} (间隔 {}s)", autoSaveSlot - 1, autoSaveSecs);
                }
            }

            // ---- 暂停处理 ----
            if (sig.isPaused()) {
                if (!wasPaused) {
                    _savePlayTimeCheckpoint();
                    if (m_sramDirty && m_core && m_core->IsReady())
                        m_core->saveSram();
                    AudioManager::instance().flushRingBuffer(); // 立即停止音频
                    wasPaused = true;
                }
                // 暂停时仍可消费金手指重载信号（来自菜单关闭时的批量同步）
                if (sig.consumeReloadCheats() && m_core)
                    m_core->ReloadCheats();
                // 暂停时仍可消费退出自动存档信号
                {
                    int exitSaveSlot = sig.consumeAutoSave();
                    if (exitSaveSlot >= 0)
                        _doSaveState(exitSaveSlot);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                auto now     = Clock::now();
                nextFrameTarget = now;
                fpsLastTime     = now;
                continue;
            }
            if (wasPaused) {
                m_playStartTime = Clock::now();
                m_autoSaveTimer = Clock::now();
                wasPaused = false;
            }

            // ---- 重置请求 ----
            if (sig.consumeReset()) {
                m_core->Reset();
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
                    if (!m_core->Unserialize(m_rewindBuffer[restoreIdx].state)) {
                        brls::Logger::warning("GameView: 倒带帧恢复失败 idx={}", restoreIdx);
                    } else {
                        for (int i = 0; i < restoreIdx && !m_rewindBuffer.empty(); ++i)
                            m_rewindBuffer.pop_front();
                        m_core->RunFrame();
                    }
                }
            }

            // ---- 金手指切换 ----
            auto cheatReq = sig.consumeCheatToggle();
            if (cheatReq.pending && m_core)
                m_core->ToggleCheat(cheatReq.idx, cheatReq.enabled);

            // ---- 金手指重载 ----
            if (sig.consumeReloadCheats() && m_core)
                m_core->ReloadCheats();

            // ---- 退出自动存档 ----
            {
                int exitSaveSlot = sig.consumeAutoSave();
                if (exitSaveSlot >= 0)
                    _doSaveState(exitSaveSlot);
            }

            // ---- 从信号更新游戏按键状态 ----
            // 先处理连发（Turbo）状态
            m_turboFrameCount++;
            if (m_turboFrameCount >= m_turboToggleInterval) {
                m_turboFrameCount = 0;
                if (m_turboAheld.load(std::memory_order_acquire)) {
                    m_turboAon = !m_turboAon;
                    if (m_turboAon)
                        GameSignal::instance().pressGameButton(8); // RETRO_DEVICE_ID_JOYPAD_A
                    else
                        GameSignal::instance().releaseGameButton(8);
                }
                if (m_turboBheld.load(std::memory_order_acquire)) {
                    m_turboBon = !m_turboBon;
                    if (m_turboBon)
                        GameSignal::instance().pressGameButton(0); // RETRO_DEVICE_ID_JOYPAD_B
                    else
                        GameSignal::instance().releaseGameButton(0);
                }
            }
            // 连发键释放时确保对应按键松开
            if (!m_turboAheld.load(std::memory_order_acquire) && m_turboAon) {
                m_turboAon = false;
                GameSignal::instance().releaseGameButton(8);
            }
            if (!m_turboBheld.load(std::memory_order_acquire) && m_turboBon) {
                m_turboBon = false;
                GameSignal::instance().releaseGameButton(0);
            }
            m_core->SetButtonsFromSignal();

            // ---- 决定本帧行为 ----
            bool ff      = sig.isFastForward();
            bool rew     = sig.isRewinding();

            // 倒带时禁用快进，防止逻辑冲突
            if (rew) ff = false;

            // 通知核心当前快进状态（供 RETRO_ENVIRONMENT_GET_FASTFORWARDING 查询）
            m_core->SetFastForwarding(ff);

            // 每帧读取快进倍率（支持菜单中实时调整）
            m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
            if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;
            m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

            // 同步倍速到音频重采样器
            {
                static float s_lastSpeed = 1.0f;
                float curSpeed = ff ? m_ffMultiplier : 1.0f;
                if (curSpeed != s_lastSpeed) {
                    AudioManager::instance().setSpeed(curSpeed);
                    s_lastSpeed = curSpeed;
                }
            }

            unsigned framesRan = 1u;

            if (rew) {
                // 倒带：从历史缓冲区恢复状态
                _stepRewind();
            } else {
                // 正常 / 快进：运行核心并保存倒带状态
                framesRan = _stepFrame(ff);
            }

            // ---- 取出视频帧暂存（慢动作跳过帧时不捕获）----
            if (framesRan > 0)
                _captureVideoFrame();

            // ---- 推送音频（慢动作跳过帧时不推送）----
            if (framesRan > 0)
                _pushFrameAudio(ff);

            // ---- FPS 统计（慢动作跳过帧时仍计入时间）----
            _updateFpsStats(framesRan, fpsLastTime, fpsCount);

            // ---- SRAM 自动落盘检测 ----
            _checkAndAutoSaveSram();

            // ── PLL：仅减速不加速 ──
            if (framesRan > 0 && !ff) {
                size_t ringFill = AudioManager::instance().available();
                double targetFill = 8000.0;
                if (targetFill > 0.0) {
                    double errorRatio = (static_cast<double>(ringFill) - targetFill) / targetFill;
                    if (errorRatio > 0.0) {
                        double gain = 0.01;
                        double correction = 1.0 + errorRatio * gain;
                        if (correction > 1.02) correction = 1.02;
                        frameDurNs = std::chrono::nanoseconds(
                            static_cast<long long>(baseFrameDurNs.count() * correction));
                    } else {
                        frameDurNs = baseFrameDurNs;
                    }
                }
            } else {
                frameDurNs = baseFrameDurNs;
            }

            // ---- 帧率限制（快进时不限速）----
            _throttleFrameRate(ff, nextFrameTarget, frameDurNs, spinGuardNs);
        }

        // ---- 提交时长记录 ----
        _saveAndCommitPlayTime();

        // ---- 强制保存 SRAM ----
        if (m_core && m_core->IsReady()) {
            m_core->saveSram();
            brls::Logger::info("GameView: SRAM saved on exit");
        }
        brls::Logger::info("GameView: 游戏循环结束 playTime={}",
                           m_gameEntry.playTime);

        // ---- 音频清理 ----
        AudioManager::instance().deinit();

#ifdef _WIN32
        timeEndPeriod(1);
#endif
        brls::Logger::debug("[GameView] _gameLoop exit");
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
        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return beiklive::tools::getStatePath(dir, m_gameEntry.path, slot);
    }

    std::string GameView::getStateThumbPath(int slot) const
    {
        return beiklive::tools::getStateThumbPath(
            m_gameEntry.savePath.empty() ? beiklive::path::savePath() : m_gameEntry.savePath,
            m_gameEntry.path, slot);
    }

    bool GameView::stateExists(int slot) const
    {
        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();
        return beiklive::tools::stateExists(dir, m_gameEntry.path, slot);
    }

    // ============================================================
    // _doSaveState – 序列化核心状态并保存截图（游戏线程调用）
    // ============================================================

    void GameView::_doSaveState(int slot)
    {
        if (!m_core || !m_core->IsReady()) return;

        std::vector<uint8_t> buf;
        if (!m_core->Serialize(buf) || buf.empty()) {
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
        auto frame = m_core->GetVideoFrame();
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
        if (!m_core || !m_core->IsReady()) return;

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

        // 先用当前核心状态大小做一次轻量兼容性探测，避免把旧核心或损坏的状态
        // 直接喂给新核心，导致反序列化失败后内部状态被部分污染。
        {
            std::vector<uint8_t> probe;
            if (m_core->Serialize(probe) && !probe.empty() &&
                probe.size() != static_cast<size_t>(got)) {
                brls::Logger::warning(
                    "GameView: 存档大小与当前核心不匹配 (slot {} file={} expected={})",
                    slot, static_cast<size_t>(got), probe.size());
                m_core->Reset();
                AudioManager::instance().flushRingBuffer();
                brls::sync([slot](){
                    std::string msg = (slot == 0)
                        ? "读取失败：自动存档与当前核心不兼容，已重置游戏"
                        : "读取失败：槽位 " + std::to_string(slot) + " 与当前核心不兼容";
                    brls::Application::notify(msg);
                });
                return;
            }
        }

        if (!m_core->Unserialize(buf)) {
            brls::Logger::warning("GameView: 存档反序列化失败 (slot {})", slot);
            m_core->Reset();
            AudioManager::instance().flushRingBuffer();
            brls::sync([slot](){
                std::string msg = (slot == 0)
                    ? "读取失败：自动存档无效，已重置游戏"
                    : "读取失败 (slot " + std::to_string(slot) + ")";
                brls::Application::notify(msg);
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
    // 游戏已暂停时调用，自动根据保存间隔计算 item 数量（每秒 1 个 item）
    // ============================================================
    std::vector<RewindThumbSnapshot>
    GameView::snapshotRewindThumbs() const
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        std::vector<RewindThumbSnapshot> result;

        int total = static_cast<int>(m_rewindBuffer.size());
        if (total == 0) return result;

        // 根据保存间隔自动计算每秒对应 1 个 item 时所需 item 数量（上限 120）
        // 公式：每条目代表 saveInterval 帧，60帧约1秒；缓冲总时长(秒) = total*saveInterval/60
        // 当缓冲时长小于 1 秒时，clamp 为 1（至少显示最新帧）
        int maxItems = std::max(1, std::min(120,
            total * m_rewindSaveInterval / 60));

        if (maxItems <= 0 || total <= maxItems) {
            // 条目数不超过限制，全部返回
            result.reserve(total);
            for (int i = 0; i < total; ++i) {
                RewindThumbSnapshot snap;
                snap.bufferIdx  = i;
                snap.secondsAgo = i * m_rewindSaveInterval / 60;
                snap.thumb      = m_rewindBuffer[i].thumb;
                result.push_back(std::move(snap));
            }
        } else if (maxItems == 1) {
            // 只取最新帧
            RewindThumbSnapshot snap;
            snap.bufferIdx  = 0;
            snap.secondsAgo = 0;
            snap.thumb      = m_rewindBuffer[0].thumb;
            result.push_back(std::move(snap));
        } else {
            // 均匀采样：在 [0, total-1] 范围内选取 maxItems 个索引（maxItems >= 2，不会除零）
            result.reserve(maxItems);
            for (int k = 0; k < maxItems; ++k) {
                int idx = k * (total - 1) / (maxItems - 1);
                RewindThumbSnapshot snap;
                snap.bufferIdx  = idx;
                snap.secondsAgo = idx * m_rewindSaveInterval / 60;
                snap.thumb      = m_rewindBuffer[idx].thumb;
                result.push_back(std::move(snap));
            }
        }

        // 反转顺序：使最旧的帧排在最前，最新的帧排在最后
        // 显示时最旧帧在左侧，最新帧在右侧，焦点默认放在最右边（最新帧）
        std::reverse(result.begin(), result.end());
        return result;
    }

    // ============================================================
    // requestRestoreRewindFrame – 通过 GameSignal 请求恢复指定帧
    // ============================================================
    void GameView::requestRestoreRewindFrame(int frameIndex)
    {
        GameSignal::instance().requestRewindRestore(frameIndex);
    }

    void GameView::requestCheatPathUpdate(const std::string& path)
    {
        if (m_core)
        {
            m_gameEntry.cheatPath = path;
            m_core->SetCheatPath(path);
            m_core->ReloadCheats();
        }
    }

    void GameView::_onShaderToggle(bool on)
    {
        if (!m_rendererReady) return;

        m_gameEntry.shaderEnabled = on;
        // 持久化到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "shaderEnabled",
                nlohmann::json(m_gameEntry.shaderEnabled));
        }
        brls::Logger::debug("GameView: Shader {} (enabled={})", m_gameEntry.shaderPath, m_gameEntry.shaderEnabled);
        if (on) {
            std::string path = m_gameEntry.shaderPath;
            if (!path.empty())
                m_renderer.setShader(path);
        } else {
            m_renderer.setShader("");
        }
    }

    void GameView::_onShaderPathChange(const std::string& path)
    {
        if (!m_rendererReady) return;
        bool shaderOn = m_gameEntry.shaderEnabled;
        m_gameEntry.shaderPath = path;
        brls::Logger::debug("GameView: Shader path changed to {} (enabled={})", m_gameEntry.shaderPath, shaderOn);
        // 持久化到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "shaderPath",
                nlohmann::json(m_gameEntry.shaderPath));
        }
        if (shaderOn && !path.empty())
            m_renderer.setShader(path);
        else if (!shaderOn)
            m_renderer.setShader("");
    }

    void GameView::_onDisplayModeChange(const std::string& mode)
    {
        // 画面模式: fit/fill/original/integer/custom
        if (mode == "fit")      m_screenMode = beiklive::ScreenMode::Fit;
        else if (mode == "fill") m_screenMode = beiklive::ScreenMode::Fill;
        else if (mode == "integer") m_screenMode = beiklive::ScreenMode::IntegerScale;
        else if (mode == "custom")  m_screenMode = beiklive::ScreenMode::FreeScale;
        else m_screenMode = beiklive::ScreenMode::Fit;
        m_gameEntry.displayMode = static_cast<int>(m_screenMode);

        // 持久化画面模式到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "displayMode",
                nlohmann::json(m_gameEntry.displayMode));
            beiklive::GameDB->flush();
        }
    }

    void GameView::_onIntegerScaleChange(float scale)
    {
        m_gameEntry.integerAspectRatio = scale;

        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "integerAspectRatio",
                nlohmann::json(static_cast<float>(scale)));
            beiklive::GameDB->flush();
        }
    }

    void GameView::_onCustomValuesChanged(float x, float y, float scale)
    {
        m_gameEntry.customOffsetX = x;
        m_gameEntry.customOffsetY = y;
        m_gameEntry.customScale  = scale;

        // 持久化自定义值到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "customOffsetX",
                nlohmann::json(static_cast<double>(x)));
            beiklive::GameDB->set(m_gameEntry.path, "customOffsetY",
                nlohmann::json(static_cast<double>(y)));
            beiklive::GameDB->set(m_gameEntry.path, "customScale",
                nlohmann::json(static_cast<double>(scale)));
            beiklive::GameDB->flush();
        }
    }

    void GameView::_onOverlayToggle(bool enabled)
    {
        m_gameEntry.overlayEnabled = enabled;
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "overlayEnabled",
                nlohmann::json(enabled));
        }
    }

    void GameView::_onOverlayPathChange(const std::string& path)
    {
        m_gameEntry.overlayPath = path;
        // 清除已缓存的遮罩纹理，使其重新加载
        if (m_overlayImage) m_overlayImage->clear();
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "overlayPath",
                nlohmann::json(path));
        }
    }

    void GameView::_onFilterChange(const std::string& filter)
    {
        if (!m_rendererReady) return;
        m_renderer.setFilter(filter == "linear");
    }

    void GameView::_onConfigUpdated()
    {
        if (m_core)
            m_core->NotifyConfigUpdated();
    }

    // ============================================================
    // _downsampleToRGB565 – RGBA8888 降采样并转换为 RGB565
    // 支持最近邻（NearestNeighbor）和双线性（Bilinear）两种压缩策略
    // ============================================================
    std::vector<uint16_t> GameView::_downsampleToRGB565(
        const std::vector<uint32_t>& src,
        unsigned srcW, unsigned srcH,
        unsigned dstW, unsigned dstH)
    {
        std::vector<uint16_t> dst(dstW * dstH, 0);
        if (src.empty() || srcW == 0 || srcH == 0) return dst;

        // 使用缓存的压缩策略设置
        bool useBilinear = (m_cachedThumbCompression == static_cast<int>(
            beiklive::RewindThumbCompression::Bilinear));

        for (unsigned y = 0; y < dstH; ++y) {
            for (unsigned x = 0; x < dstW; ++x) {
                uint8_t r, g, b;

                if (useBilinear) {
                    // 双线性插值：计算源坐标（浮点）并对四邻域加权
                    float sx = (x + 0.5f) * static_cast<float>(srcW) / static_cast<float>(dstW) - 0.5f;
                    float sy = (y + 0.5f) * static_cast<float>(srcH) / static_cast<float>(dstH) - 0.5f;
                    int x0 = static_cast<int>(sx);
                    int y0 = static_cast<int>(sy);
                    int x1 = x0 + 1;
                    int y1 = y0 + 1;
                    // 钳位到边界
                    if (x0 < 0) x0 = 0;
                    if (y0 < 0) y0 = 0;
                    if (x1 >= static_cast<int>(srcW)) x1 = static_cast<int>(srcW) - 1;
                    if (y1 >= static_cast<int>(srcH)) y1 = static_cast<int>(srcH) - 1;
                    float fx = sx - static_cast<float>(x0);
                    float fy = sy - static_cast<float>(y0);
                    // 双线性加权混合四个源像素
                    // makeRGBA8888 存储格式（字节序）：字节0=R，字节1=G，字节2=B，字节3=A
                    // 对应 uint32 移位：R=(px>>0)&0xFF, G=(px>>8)&0xFF, B=(px>>16)&0xFF
                    auto getPixelAt = [&](int px, int py) -> uint32_t {
                        return src[static_cast<unsigned>(py) * srcW + static_cast<unsigned>(px)];
                    };
                    uint32_t p00 = getPixelAt(x0, y0), p10 = getPixelAt(x1, y0);
                    uint32_t p01 = getPixelAt(x0, y1), p11 = getPixelAt(x1, y1);
                    auto extractChannel = [](uint32_t p, int shift) -> float {
                        return static_cast<float>((p >> shift) & 0xFF);
                    };
                    r = static_cast<uint8_t>(
                        extractChannel(p00, 0)*(1-fx)*(1-fy) + extractChannel(p10, 0)*fx*(1-fy) +
                        extractChannel(p01, 0)*(1-fx)*fy     + extractChannel(p11, 0)*fx*fy);
                    g = static_cast<uint8_t>(
                        extractChannel(p00, 8)*(1-fx)*(1-fy) + extractChannel(p10, 8)*fx*(1-fy) +
                        extractChannel(p01, 8)*(1-fx)*fy     + extractChannel(p11, 8)*fx*fy);
                    b = static_cast<uint8_t>(
                        extractChannel(p00,16)*(1-fx)*(1-fy) + extractChannel(p10,16)*fx*(1-fy) +
                        extractChannel(p01,16)*(1-fx)*fy     + extractChannel(p11,16)*fx*fy);
                } else {
                    // 最近邻采样（默认）
                    unsigned sx = x * srcW / dstW;
                    unsigned sy = y * srcH / dstH;
                    // makeRGBA8888 存储格式（字节序）：字节0=R，字节1=G，字节2=B，字节3=A
                    // 对应 uint32 移位：R=(px>>0)&0xFF, G=(px>>8)&0xFF, B=(px>>16)&0xFF
                    uint32_t px = src[sy * srcW + sx];
                    r = static_cast<uint8_t>( px        & 0xFF);  // R：字节偏移 0
                    g = static_cast<uint8_t>((px >> 8)  & 0xFF);  // G：字节偏移 1
                    b = static_cast<uint8_t>((px >> 16) & 0xFF);  // B：字节偏移 2
                }

                // 打包为 RGB565：R(5) | G(6) | B(5)
                dst[y * dstW + x] = static_cast<uint16_t>(
                    ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
            }
        }
        return dst;
    }

    // ============================================================
    // _crc32Sram – 简单 CRC32 计算（缓冲区版本）
    // ============================================================
    uint32_t GameView::_crc32Sram(const void* data, size_t size)
    {
        if (!data || size == 0) return 0;
        uint32_t crc = 0xFFFFFFFF;
        const uint8_t* p = static_cast<const uint8_t*>(data);
        static const uint32_t table[256] = {
            0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
            0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
            0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
            0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
            0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
            0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
            0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
            0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C7B,0x58684C11,0xC1611DAB,0xB6662D3D,
            0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
            0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
            0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
            0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
            0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
            0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
            0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
            0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
            0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
            0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
            0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
            0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
            0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
            0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
            0xCB61B38C,0xBC66831A,0x256FD2A0,0x527F2236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
            0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB30A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
            0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
            0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
            0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
            0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
            0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
            0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
            0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
            0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
        };
        for (size_t i = 0; i < size; ++i)
            crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFF;
    }

    // ============================================================
    // _checkAndAutoSaveSram – CRC 检测 + 延迟自动落盘
    // 每秒检查一次 SRAM CRC，变化后等待 2 秒写盘
    // ============================================================
    void GameView::_checkAndAutoSaveSram()
    {
        if (!m_core || !m_core->IsReady()) return;

        auto now = std::chrono::steady_clock::now();
        double sinceCheck = std::chrono::duration<double>(now - m_sramLastCheck).count();
        if (sinceCheck < SRAM_CHECK_INTERVAL) return;
        m_sramLastCheck = now;

        size_t sz = m_core->getSramSize();
        const void* ptr = m_core->getSramData();
        if (!ptr || sz == 0) return;

        uint32_t crc = _crc32Sram(ptr, sz);
        if (crc != m_sramLastCRC)
        {
            m_sramLastCRC  = crc;
            m_sramDirty    = true;
            m_sramDirtyTime = now;
        }
        else if (m_sramDirty)
        {
            double sinceDirty = std::chrono::duration<double>(now - m_sramDirtyTime).count();
            if (sinceDirty >= SRAM_FLUSH_DELAY)
            {
                m_core->saveSram();
                m_sramDirty = false;
                brls::Logger::debug("GameView: SRAM auto-saved");
            }
        }
    }
}
