#include "GameView.hpp"
#include "GameMenuView.hpp"
#include "game/audio/AudioManager.hpp"
#include "ui/utils/AnimationHelper.hpp"

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
        // ---- 游戏按键绑定（EMU_A ~ EMU_SELECT 直接映射到 brls 按键）-----------
        // 按住时持续置位，松开时清除，使用 GameSignal 按键位掩码传入游戏帧。
        struct GameBtnMap {
            EmuFunctionKey emuKey;
            int            brlsBtn;
            unsigned       retroId;
        };
        // TODO: brlsBtn 后续改为使用接口获取设置的按键映射，而非固定死写死在这里
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

        // 打开菜单：
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_OPEN_MENU,
            {{brls::BUTTON_RT, brls::BUTTON_LT}},
            [this]()
            {
                brls::Logger::debug("打开菜单热键触发！");
                GameSignal::instance().requestOpenMenu();
                this->setFocusable(false); // 打开菜单时暂时取消 GameView 的焦点，避免输入冲突，菜单关闭后由 GamePage 恢复焦点

            }
            // TriggerType::LONG_PRESS,
            // 2.5f
        );

        // 快进：LSB 切换  TODO: hold状态需要专门写press/release来控制，不能直接用HOLD，否则会导致按住时反复触发开关
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

        // 倒带：RSB 切换  TODO: hold状态需要专门写press/release来控制，不能直接用HOLD，否则会导致按住时反复触发开关
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
    // _saveRewindState – 序列化当前核心状态并存入倒带缓冲区
    // ============================================================
    void GameView::_saveRewindState() // TODO: 后续改为使用setting的倒带开关控制是否记录倒带
    {
        std::vector<uint8_t> state;
        if (m_gba_core->Serialize(state) && !state.empty()) {
            m_rewindBuffer.push_front(std::move(state));
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
        if (m_rewindBuffer.empty()) return false;

        bool didRestore = false;
        // 每次弹出 REWIND_STEP 帧，实现比正常速度快的倒带
        for (unsigned step = 0; step < REWIND_STEP && !m_rewindBuffer.empty(); ++step) {
            if (m_gba_core->Unserialize(m_rewindBuffer.front())) {
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
                m_rewindBuffer.clear(); // 重置后清空倒带缓冲区
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

} // namespace beiklive
