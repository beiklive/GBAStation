#include "GameView.hpp"
#include "GameMenuView.hpp"
#include "game/audio/AudioManager.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h> // timeBeginPeriod / timeEndPeriod
#endif

namespace beiklive
{
    // 游戏线程常量
    static constexpr double MAX_REASONABLE_FPS = 240.0; ///< 核心上报 FPS 的安全上限
    static constexpr double SPIN_GUARD_SEC     = 0.002; ///< 每帧 2ms 自旋等待预算
    static constexpr double FPS_UPDATE_INTERVAL = 1.0;  ///< FPS 计数器更新间隔（秒）
    static constexpr int    PLAY_TIME_SAVE_INTERVAL = 180; ///< 游戏时长保存间隔（秒，3分钟）

    GameView::GameView(beiklive::GameEntry gameData) : m_gameEntry(std::move(gameData))
    {
        _brls_inputLocked = false;
        GameInputManager::instance().sayHello();
        HIDE_BRLS_HIGHLIGHT(this);

        // 从 GameEntry 加载画面模式（默认 Fit）
        m_screenMode = static_cast<beiklive::ScreenMode>(m_gameEntry.displayMode);

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

        GameInputManager::instance().handleInput();

        // 消费退出信号
        if (GameSignal::instance().consumeExit()) {
            brls::sync([](){ brls::Application::popActivity(); });
            return;
        }

        // 消费打开菜单信号
        if (GameSignal::instance().consumeOpenMenu()) {
            if (m_gameMenuView) {
                brls::sync([this](){
                    m_gameMenuView->setVisibility(brls::Visibility::VISIBLE);
                    brls::Application::giveFocus(m_gameMenuView);
                });
            }
            return;
        }

        // 初始化渲染器（首帧时，GL 上下文已就绪）
        if (!m_rendererReady && m_gba_core && m_gba_core->IsReady()) {
            unsigned gw = m_gba_core->GameWidth()  > 0 ? m_gba_core->GameWidth()  : 240;
            unsigned gh = m_gba_core->GameHeight() > 0 ? m_gba_core->GameHeight() : 160;
            if (m_renderer.init(gw, gh, false)) {
                m_rendererReady = true;
                brls::Logger::info("GameView: 渲染器初始化完成 ({}x{})", gw, gh);
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

            unsigned gw = m_renderer.texWidth()  > 0 ? m_renderer.texWidth()  : 240;
            unsigned gh = m_renderer.texHeight() > 0 ? m_renderer.texHeight() : 160;

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
        // ---- 游戏按键绑定（EMU_A ~ EMU_SELECT 直接映射到 brls 按键）-----------
        // 按住时持续置位，松开时清除，使用 GameSignal 按键位掩码传入游戏帧。
        struct GameBtnMap {
            EmuFunctionKey emuKey;
            int            brlsBtn;
            unsigned       retroId;
        };
        static const GameBtnMap gameBtnMaps[] = {
            { EMU_A,      brls::BUTTON_A,     8  }, // RETRO_DEVICE_ID_JOYPAD_A
            { EMU_B,      brls::BUTTON_B,     0  }, // RETRO_DEVICE_ID_JOYPAD_B
            { EMU_X,      brls::BUTTON_X,     9  }, // RETRO_DEVICE_ID_JOYPAD_X
            { EMU_Y,      brls::BUTTON_Y,     1  }, // RETRO_DEVICE_ID_JOYPAD_Y
            { EMU_UP,     brls::BUTTON_UP,    4  }, // RETRO_DEVICE_ID_JOYPAD_UP
            { EMU_DOWN,   brls::BUTTON_DOWN,  5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
            { EMU_LEFT,   brls::BUTTON_LEFT,  6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
            { EMU_RIGHT,  brls::BUTTON_RIGHT, 7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
            { EMU_L,      brls::BUTTON_LB,    10 }, // RETRO_DEVICE_ID_JOYPAD_L
            { EMU_R,      brls::BUTTON_RB,    11 }, // RETRO_DEVICE_ID_JOYPAD_R
            { EMU_L2,     brls::BUTTON_LT,    12 }, // RETRO_DEVICE_ID_JOYPAD_L2
            { EMU_R2,     brls::BUTTON_RT,    13 }, // RETRO_DEVICE_ID_JOYPAD_R2
            { EMU_L3,     brls::BUTTON_LSB,   14 }, // RETRO_DEVICE_ID_JOYPAD_L3
            { EMU_R3,     brls::BUTTON_RSB,   15 }, // RETRO_DEVICE_ID_JOYPAD_R3
            { EMU_START,  brls::BUTTON_START, 3  }, // RETRO_DEVICE_ID_JOYPAD_START
            { EMU_SELECT, brls::BUTTON_BACK,  2  }, // RETRO_DEVICE_ID_JOYPAD_SELECT
        };
        for (const auto& m : gameBtnMaps) {
            unsigned rid = m.retroId;
            // 按住持续置位
            GameInputManager::instance().registerEmuFunctionKey(
                m.emuKey, {{m.brlsBtn}},
                [rid]() { GameSignal::instance().pressGameButton(rid); },
                TriggerType::HOLD);
            // 松开时清除
            GameInputManager::instance().registerEmuFunctionKey(
                m.emuKey, {{m.brlsBtn}},
                [rid]() { GameSignal::instance().releaseGameButton(rid); },
                TriggerType::RELEASE);
        }

        // ---- 功能热键绑定 ------------------------------------------------------

        // 打开菜单：LB+START 长按 2.5s 或 RT+LT 长按
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_OPEN_MENU,
            {{brls::BUTTON_LB, brls::BUTTON_START}, {brls::BUTTON_RT, brls::BUTTON_LT}},
            []()
            {
                brls::Logger::debug("打开菜单热键触发！");
                GameSignal::instance().requestOpenMenu();
            },
            TriggerType::LONG_PRESS,
            2.5f
        );

        // 快进：LSB 切换
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_FAST_FORWARD,
            {{brls::BUTTON_LSB}},
            []()
            {
                bool cur = GameSignal::instance().isFastForward();
                GameSignal::instance().requestFastForward(!cur);
                brls::Logger::debug("快进切换：{}", !cur);
            }
        );

        // 倒带：RSB 切换
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_REWIND,
            {{brls::BUTTON_RSB}},
            []()
            {
                bool cur = GameSignal::instance().isRewinding();
                GameSignal::instance().requestRewind(!cur);
                brls::Logger::debug("倒带切换：{}", !cur);
            });
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
    // _gameLoop – 游戏主循环（独立线程）
    //
    // 按核心目标帧率执行 RunFrame()，并将音频数据推送给 AudioManager。
    // 支持通过 GameSignal 控制暂停、快进、倒带等功能。
    // 每 3 分钟将游戏时长累加到 GameEntry 并保存到数据库。
    // ============================================================
    void GameView::_gameLoop()
    {
        using Clock = std::chrono::steady_clock;

        if (!m_gba_core || !m_gba_core->IsReady()) return;

#ifdef _WIN32
        // 提升 Windows 定时器精度至 1ms
        timeBeginPeriod(1);
#endif

        // 从核心获取目标帧率
        double coreFps = m_gba_core->Fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS)
            coreFps = 59.7275;

        const double frameDuration = 1.0 / coreFps;
        // 自旋等待预算：取帧时长的 10%（最多 2ms），避免在高帧率下 CPU 空转过多
        const double spinGuard = std::min(SPIN_GUARD_SEC, frameDuration * 0.1);

        // 计时器：线程开始时启动
        GameTimer::instance().start();

        auto frameStart   = Clock::now();
        // FPS 统计
        auto fpsLastTime  = Clock::now();
        unsigned fpsCount = 0;
        // 游戏时长记录（仅统计非暂停时间）
        auto playTimeLast = Clock::now();
        int  pendingPlaySecs = 0; ///< 本次未保存的游戏秒数

        while (m_running.load(std::memory_order_acquire))
        {
            auto& sig = GameSignal::instance();

            // ---- 暂停处理 ----
            if (sig.isPaused()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                frameStart   = Clock::now();
                playTimeLast = Clock::now(); // 暂停期间不计时
                continue;
            }

            // ---- 重置请求 ----
            if (sig.consumeReset()) {
                m_gba_core->Reset();
            }

            // ---- 从信号更新游戏按键状态 ----
            m_gba_core->SetButtonsFromSignal();

            // ---- 快进状态 ----
            bool fastForward = sig.isFastForward();
            m_gba_core->SetFastForwarding(fastForward);

            // ---- 执行一帧游戏逻辑 ----
            m_gba_core->RunFrame();

            // ---- 取出视频帧并暂存（由 UI 线程在 draw() 中上传 GPU）----
            {
                auto frame = m_gba_core->GetVideoFrame();
                if (!frame.pixels.empty()) {
                    std::lock_guard<std::mutex> lk(m_frameMutex);
                    m_pendingFrame = std::move(frame);
                    m_frameReady   = true;
                }
            }

            // ---- 推送音频数据 ----
            if (!sig.isMuted()) {
                std::vector<int16_t> audioBuf;
                if (m_gba_core->DrainAudio(audioBuf) && !audioBuf.empty()) {
                    AudioManager::instance().pushSamples(
                        audioBuf.data(), audioBuf.size() / 2);
                }
            }

            // ---- FPS 统计 ----
            ++fpsCount;
            {
                auto now    = Clock::now();
                double elap = std::chrono::duration<double>(now - fpsLastTime).count();
                if (elap >= FPS_UPDATE_INTERVAL) {
                    float fps = static_cast<float>(fpsCount / elap);
                    {
                        std::lock_guard<std::mutex> lk(m_fpsMutex);
                        m_currentFps = fps;
                    }
                    fpsCount  = 0;
                    fpsLastTime = now;
                }
            }

            // ---- 游戏时长记录（每 3 分钟精确累加并保存一次）----
            {
                auto now = Clock::now();
                double elapsed = std::chrono::duration<double>(now - playTimeLast).count();
                // 每满 PLAY_TIME_SAVE_INTERVAL 秒精确累加一次，避免误差积累
                while (elapsed >= static_cast<double>(PLAY_TIME_SAVE_INTERVAL)) {
                    elapsed -= static_cast<double>(PLAY_TIME_SAVE_INTERVAL);
                    playTimeLast += std::chrono::duration_cast<Clock::duration>(
                        std::chrono::duration<double>(PLAY_TIME_SAVE_INTERVAL));
                    m_gameEntry.playTime += PLAY_TIME_SAVE_INTERVAL;
                    if (beiklive::GameDB) {
                        beiklive::GameDB->upsert(m_gameEntry);
                        beiklive::GameDB->flush();
                        brls::Logger::debug("游戏时长已保存：{} 秒", m_gameEntry.playTime);
                    }
                }
            }

            // ---- 帧率控制（快进时不限速）----
            if (!fastForward) {
                auto frameEnd = Clock::now();
                double elapsed = std::chrono::duration<double>(frameEnd - frameStart).count();
                double remaining = frameDuration - elapsed - spinGuard;

                if (remaining > 0.0) {
                    auto sleepDur = std::chrono::duration<double>(remaining);
                    std::this_thread::sleep_for(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(sleepDur));
                }

                // 自旋等待剩余精度时间
                while (true) {
                    auto now = Clock::now();
                    double total = std::chrono::duration<double>(now - frameStart).count();
                    if (total >= frameDuration) break;
                    std::this_thread::yield();
                }
            }

            frameStart = Clock::now();
        }

        // ---- 音频清理 ----
        AudioManager::instance().deinit();

#ifdef _WIN32
        timeEndPeriod(1);
#endif
    }

} // namespace beiklive

