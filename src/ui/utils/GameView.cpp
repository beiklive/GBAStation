#include "GameView.hpp"
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

    GameView::GameView(beiklive::GameEntry gameData) : m_gameEntry(std::move(gameData))
    {
        _brls_inputLocked = false;
        GameInputManager::instance().sayHello();
        HIDE_BRLS_HIGHLIGHT(this);

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

        // 初始化渲染器（首帧时，GL 上下文已就绪）
        if (!m_rendererReady && m_gba_core && m_gba_core->IsReady()) {
            unsigned gw = m_gba_core->GameWidth()  > 0 ? m_gba_core->GameWidth()  : 240;
            unsigned gh = m_gba_core->GameHeight() > 0 ? m_gba_core->GameHeight() : 160;
            if (m_renderer.init(gw, gh, false)) {
                m_rendererReady = true;
                brls::Logger::info("GameView: 渲染器初始化完成 ({}x{})", gw, gh);
            }
        }

        // 上传待渲染帧（游戏线程已写入 m_pendingFrame）
        _uploadPendingFrame();

        // 将游戏帧绘制到当前视图区域
        if (m_rendererReady) {
            float windowScale = brls::Application::windowScale;
            int   windowW     = brls::Application::windowWidth;
            int   windowH     = brls::Application::windowHeight;
            m_renderer.drawToScreen(x, y, width, height, windowScale, windowW, windowH);
        }
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
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_REWIND,
            {{brls::BUTTON_RSB}},
            []()
            {
                // 使用切换模式：每次按下切换倒带开关，避免 HOLD 松开后信号无法自动清除的问题
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

        auto frameStart = Clock::now();

        while (m_running.load(std::memory_order_acquire))
        {
            auto& sig = GameSignal::instance();

            // ---- 暂停处理 ----
            if (sig.isPaused()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                frameStart = Clock::now();
                continue;
            }

            // ---- 重置请求 ----
            if (sig.consumeReset()) {
                m_gba_core->Reset();
            }

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
