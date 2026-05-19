#include "NdsGameView.hpp"
#include "NdsGameMenuView.hpp"
#include "game/audio/AudioManager.hpp"
#include "ui/audio/BKAudioPlayer.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "core/Tools.hpp"

#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace beiklive
{

    NdsGameView::NdsGameView(beiklive::GameEntry gameData) : m_gameEntry(std::move(gameData))
    {
        _brls_inputLocked = false;
        GameInputManager::instance().sayHello();
        HIDE_BRLS_HIGHLIGHT(this);

        m_screenMode = static_cast<beiklive::ScreenMode>(m_gameEntry.displayMode);

        m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
        if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;

        _registerGameInput();
        _registerGameRuntime();
    }

    NdsGameView::~NdsGameView()
    {
        _stopGameThread();

        if (m_nds_core) {
            delete m_nds_core;
            m_nds_core = nullptr;
        }

        GameInputManager::instance().clearEmuFunctionKeys();
        GameInputManager::instance().dropInput();
    }

    void NdsGameView::onFocusGained()
    {
        Box::onFocusGained();
        GameSignal::instance().requestPause(false);
        GameInputManager::instance().setInputEnabled(true);

        if (!_brls_inputLocked)
        {
            _brls_inputLocked = true;
            brls::Application::blockInputs(true);
        }
    }

    void NdsGameView::onFocusLost()
    {
        Box::onFocusLost();
        GameSignal::instance().requestPause(true);
        GameInputManager::instance().setInputEnabled(false);
        GameInputManager::instance().dropInput();

        if (_brls_inputLocked)
        {
            _brls_inputLocked = false;
            brls::Application::unblockInputs();
        }
    }

    void NdsGameView::draw(NVGcontext *vg, float x, float y, float width, float height,
                           brls::Style style, brls::FrameContext *ctx)
    {
        Box::draw(vg, x, y, width, height, style, ctx);

        GameInputManager::instance().handleInput();

        if (GameSignal::instance().consumeExit()) {
            brls::sync([this](){ brls::Application::popActivity(); });
        }

        if (GameSignal::instance().consumeOpenMenu()) {
            if (m_gameMenuView) {
                brls::sync([this](){
                    AnimationHelper::slideInFromBottom(m_gameMenuView, 60.f, 120);
                    brls::Application::giveFocus(m_gameMenuView);
                    m_gameMenuView->onShow();
                });
            }
        }

        if (!m_rendererReady && m_nds_core && m_nds_core->IsReady()) {
            unsigned gw = m_nds_core->GameWidth();
            unsigned gh = m_nds_core->GameHeight();
            std::string shaderPath;
            if (m_gameEntry.shaderEnabled && !m_gameEntry.shaderPath.empty())
                shaderPath = m_gameEntry.shaderPath;
            if (m_renderer.init(gw, gh, false, shaderPath)) {
                m_rendererReady = true;
                brls::Logger::info("NdsGameView: renderer init {}x{} shader={}",
                                   gw, gh, shaderPath.empty() ? "none" : shaderPath);
                m_fpsLastTime = std::chrono::steady_clock::now();
            }
        }

        _uploadPendingFrame();

        if (m_rendererReady) {
            float windowScale = brls::Application::windowScale;
            int   windowW     = brls::Application::windowWidth;
            int   windowH     = brls::Application::windowHeight;

            unsigned gw = m_renderer.texWidth();
            unsigned gh = m_renderer.texHeight();

            int intScale = static_cast<int>(m_gameEntry.integerAspectRatio);
            beiklive::DisplayRect rect = beiklive::computeDisplayRect(
                m_screenMode, x, y, width, height, gw, gh,
                m_gameEntry.customScale, m_gameEntry.customOffsetX, m_gameEntry.customOffsetY,
                intScale);

            m_renderer.drawToScreen(rect.x, rect.y, rect.w, rect.h, windowScale, windowW, windowH);
        }

        _drawOverlays(vg, x, y, width, height);
    }

    void NdsGameView::_drawOverlays(NVGcontext* vg, float x, float y, float w, float h)
    {
        auto& sig = GameSignal::instance();

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

        if (sig.isFastForward() && GET_SETTING_KEY_INT("display.showFfOverlay", 1))
            GameOverlayRenderer::drawFastForward(vg, x, y, w);

        if (sig.isPaused() && !sig.isFastForward())
            GameOverlayRenderer::drawPaused(vg, x, y, w);

        if (sig.isMuted() && GET_SETTING_KEY_INT("display.showMuteOverlay", 1))
            GameOverlayRenderer::drawMute(vg, x, y, w, h);
    }

    void NdsGameView::_uploadPendingFrame()
    {
        if (!m_rendererReady) return;

        LibretroLoader::VideoFrame frame;
        bool hasFrame = false;
        {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            if (m_frameReady) {
                frame = std::move(m_pendingFrame);
                m_frameReady = false;
                hasFrame = true;
            }
        }

        if (hasFrame)
            m_renderer.uploadFrame(frame);
    }

    void NdsGameView::_registerGameInput()
    {
        bool joystickEnabled  = GET_SETTING_KEY_INT("input.joystick.enabled",  1) != 0;
        bool joystickDiagonal = GET_SETTING_KEY_INT("input.joystick.diagonal", 1) != 0;
        GameInputManager::instance().setDiagonalMode(joystickDiagonal);

        struct GameBtnInfo {
            EmuFunctionKey emuKey;
            const char*    cfgSuffix;
            unsigned       retroId;
        };
        static const GameBtnInfo gameBtnInfos[] = {
            { EMU_A,      "a",      8  },
            { EMU_B,      "b",      0  },
            { EMU_X,      "x",      9  },
            { EMU_Y,      "y",      1  },
            { EMU_UP,     "up",     4  },
            { EMU_DOWN,   "down",   5  },
            { EMU_LEFT,   "left",   6  },
            { EMU_RIGHT,  "right",  7  },
            { EMU_L,      "l",      10 },
            { EMU_R,      "r",      11 },
            { EMU_L2,     "l2",     12 },
            { EMU_R2,     "r2",     13 },
            { EMU_L3,     "l3",     14 },
            { EMU_R3,     "r3",     15 },
            { EMU_START,  "start",  3  },
            { EMU_SELECT, "select", 2  },
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

        if (joystickEnabled) {
            struct StickBtnInfo {
                EmuFunctionKey emuKey;
                const char*    cfgSuffix;
                unsigned       retroId;
            };
            static const StickBtnInfo stickBtnInfos[] = {
                { EMU_LEFT_STICK_UP,     "lstick_up",    4  },
                { EMU_LEFT_STICK_DOWN,   "lstick_down",  5  },
                { EMU_LEFT_STICK_LEFT,   "lstick_left",  6  },
                { EMU_LEFT_STICK_RIGHT,  "lstick_right", 7  },
                { EMU_RIGHT_STICK_UP,    "rstick_up",    4  },
                { EMU_RIGHT_STICK_DOWN,  "rstick_down",  5  },
                { EMU_RIGHT_STICK_LEFT,  "rstick_left",  6  },
                { EMU_RIGHT_STICK_RIGHT, "rstick_right", 7  },
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

        {
            std::string val = GET_SETTING_KEY_STR("hotkey.menu.pad", "LT+RT");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_OPEN_MENU, {combo},
                    [this]() {
                        GameSignal::instance().requestOpenMenu();
                        this->setFocusable(false);
                    });
            }
        }

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
                        });
                }
            }
        }

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

    void NdsGameView::_registerGameRuntime()
    {
        if (m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuDS)
        {
            m_nds_core = new beiklive::melonds::CoreMelonDS();
            if (m_nds_core->SetupGame(m_gameEntry))
            {
                brls::Logger::debug("NDS core initialized: {}", m_gameEntry.path);

                if (auto* player = dynamic_cast<beiklive::BKAudioPlayer*>(
                        brls::Application::getAudioPlayer()))
                {
                    constexpr std::chrono::milliseconds kTimeout{500};
                    auto deadline = std::chrono::steady_clock::now() + kTimeout;
                    while (player->isPlaying()
                           && std::chrono::steady_clock::now() < deadline)
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                AudioManager::instance().init(32768, 2);

                GameSignal::instance().resetAll();
                _startGameThread();
            }
            else
            {
                brls::Logger::error("NDS core init failed: {}", m_gameEntry.path);
                delete m_nds_core;
                m_nds_core = nullptr;
            }
        }
    }

    void NdsGameView::_startGameThread()
    {
        m_running.store(true, std::memory_order_release);
        m_gameThread = std::thread(&NdsGameView::_gameLoop, this);
    }

    void NdsGameView::_stopGameThread()
    {
        m_running.store(false, std::memory_order_release);
        if (m_gameThread.joinable())
            m_gameThread.join();
    }

    void NdsGameView::_captureVideoFrame()
    {
        if (!m_nds_core || !m_nds_core->IsReady()) return;

        const uint32_t* top = m_nds_core->GetTopFramebuffer();
        const uint32_t* bot = m_nds_core->GetBottomFramebuffer();
        if (!top || !bot) return;

        LibretroLoader::VideoFrame frame;
        frame.width = kScreenW;
        frame.height = kScreenH * 2;
        frame.pixels.resize(frame.width * frame.height);

        for (unsigned row = 0; row < kScreenH; ++row)
            std::memcpy(&frame.pixels[row * kScreenW],
                        &top[row * kScreenW], kScreenW * sizeof(uint32_t));
        for (unsigned row = 0; row < kScreenH; ++row)
            std::memcpy(&frame.pixels[(kScreenH + row) * kScreenW],
                        &bot[row * kScreenW], kScreenW * sizeof(uint32_t));

        std::lock_guard<std::mutex> lk(m_frameMutex);
        m_pendingFrame = std::move(frame);
        m_frameReady = true;
    }

    void NdsGameView::_pushFrameAudio(bool ff, unsigned framesRan)
    {
        if (!m_nds_core || !m_nds_core->IsReady()) return;

        m_audioDrainBuf.resize(4096);
        int got = m_nds_core->ReadAudio(m_audioDrainBuf.data(), 2048);
        if (got <= 0) return;

        if (GameSignal::instance().isMuted()) return;

        size_t sampleCount = static_cast<size_t>(got) * 2;
        if (ff && m_ffMute) return;

        AudioManager::instance().pushSamples(m_audioDrainBuf.data(), sampleCount);
    }

    void NdsGameView::_updateFpsStats(unsigned framesRan,
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

    void NdsGameView::_throttleFrameRate(bool ff,
                                          std::chrono::steady_clock::time_point& nextTarget,
                                          std::chrono::nanoseconds frameDurNs,
                                          std::chrono::nanoseconds spinGuardNs)
    {
        using Clock = std::chrono::steady_clock;
        nextTarget += frameDurNs;
        auto now = Clock::now();
        if (nextTarget < now) {
            nextTarget = now;
            if (ff) std::this_thread::yield();
            return;
        }
        auto coarse = nextTarget - now - spinGuardNs;
        if (coarse.count() > 0)
            std::this_thread::sleep_for(coarse);
        while (Clock::now() < nextTarget)
            std::this_thread::yield();
    }

    void NdsGameView::_gameLoop()
    {
        using Clock = std::chrono::steady_clock;

        if (!m_nds_core || !m_nds_core->IsReady()) return;

#ifdef _WIN32
        timeBeginPeriod(1);
#endif

        double coreFps = m_nds_core->Fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS)
            coreFps = 60.0;

        const auto frameDurNs  = std::chrono::nanoseconds(
            static_cast<long long>(1e9 / coreFps));
        const auto spinGuardNs = std::chrono::nanoseconds(
            static_cast<long long>(std::min(SPIN_GUARD_SEC, 1.0 / coreFps * 0.1) * 1e9));

        auto nextFrameTarget = Clock::now();

        auto     fpsLastTime  = Clock::now();
        unsigned fpsCount     = 0u;

        GameTimer::instance().start();

        brls::Logger::info("NdsGameView: gameloop start coreFps={:.2f}", coreFps);

        m_playStartTime = Clock::now();
        bool wasPaused  = false;

        while (m_running.load(std::memory_order_acquire))
        {
            auto& sig = GameSignal::instance();

            if (sig.isPaused()) {
                if (!wasPaused) {
                    if (m_nds_core && m_nds_core->IsReady())
                        m_nds_core->SaveNDSSave();
                    wasPaused = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                nextFrameTarget = Clock::now();
                fpsLastTime     = Clock::now();
                continue;
            }
            if (wasPaused) {
                m_playStartTime = Clock::now();
                wasPaused = false;
            }

            if (sig.consumeReset()) {
                if (m_nds_core) m_nds_core->Reset();
            }

            m_nds_core->SetButtonsFromSignal();

            bool ff = sig.isFastForward();

            unsigned framesRan = 0;

            m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
            if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;
            m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

            if (ff) {
                unsigned count = static_cast<unsigned>(m_ffMultiplier);
                float frac = m_ffMultiplier - static_cast<float>(count);
                m_ffSlowAccum += frac;
                if (m_ffSlowAccum >= 1.0f) { m_ffSlowAccum -= 1.0f; ++count; }
                if (count == 0) count = 1;

                for (unsigned i = 0; i < count; ++i) {
                    m_nds_core->RunFrame();
                }
                framesRan = count;
            } else {
                m_nds_core->RunFrame();
                framesRan = 1;
            }

            if (framesRan > 0)
                _captureVideoFrame();

            if (framesRan > 0)
                _pushFrameAudio(ff, framesRan);

            _updateFpsStats(framesRan, fpsLastTime, fpsCount);

            _throttleFrameRate(ff, nextFrameTarget, frameDurNs, spinGuardNs);
        }

        if (m_nds_core && m_nds_core->IsReady()) {
            m_nds_core->SaveNDSSave();
            brls::Logger::info("NdsGameView: NDS save on exit");
        }
        brls::Logger::info("NdsGameView: gameloop end");

        AudioManager::instance().deinit();

#ifdef _WIN32
        timeEndPeriod(1);
#endif
    }

} // namespace beiklive
