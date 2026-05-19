#include "NdsGameView.hpp"
#include "NdsGameMenuView.hpp"
#include "game/audio/AudioManager.hpp"
#include "ui/audio/BKAudioPlayer.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "core/Tools.hpp"

#include <algorithm>
#include <filesystem>

#ifdef OGLRENDERER_ENABLED
#include "NDS.h"
#include "GPU3D_OpenGL.h"
#include "GPU_OpenGL.h"
#endif

#ifdef __SWITCH__
#include <switch.h>
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
        _joinLoadThread();

        if (m_nds_core) {
            if (m_nds_core->IsReady())
                m_nds_core->SaveNDSSave();
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

        if (m_nds_core && m_nds_core->IsReady())
        {
            unsigned gw, gh;

#ifdef OGLRENDERER_ENABLED
            auto* nds = m_nds_core->GetNDS();
            bool isAccel = nds && nds->GPU.GetRenderer3D().Accelerated;
            if (isAccel)
            {
                gw = 256; gh = 192 * 2 + 2;
            }
            else
#endif
            {
                gw = m_nds_core->GameWidth();
                gh = m_nds_core->GameHeight();
            }

            if (!m_rendererReady || (m_oglActive && m_renderer.texWidth() != gw))
            {
                std::string shaderPath;
                if (m_gameEntry.shaderEnabled && !m_gameEntry.shaderPath.empty())
                    shaderPath = m_gameEntry.shaderPath;
                if (m_renderer.init(gw, gh, false, shaderPath))
                {
                    m_rendererReady = true;
                    brls::Logger::info("NdsGameView: renderer init {}x{}", gw, gh);
                    m_fpsLastTime = std::chrono::steady_clock::now();
                    m_nextFrameTarget = std::chrono::steady_clock::now();
                }
                else
                {
                    brls::Logger::error("NdsGameView: renderer init FAILED {}x{}", gw, gh);
                }
            }

            if (m_rendererReady)
            {
                if (!GameSignal::instance().isPaused())
                    _stepEmulation();

                _renderOutput();

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
        }

        _drawOverlays(vg, x, y, width, height);
    }

    void NdsGameView::_drawOverlays(NVGcontext* vg, float x, float y, float w, float h)
    {
        auto& sig = GameSignal::instance();

        if (GET_SETTING_KEY_INT("display.showFps", 0))
        {
            if (m_currentFps > 0.f)
                GameOverlayRenderer::drawFps(vg, x, y, m_currentFps);
        }

        if (sig.isFastForward() && GET_SETTING_KEY_INT("display.showFfOverlay", 1))
            GameOverlayRenderer::drawFastForward(vg, x, y, w);

        if (sig.isPaused() && !sig.isFastForward())
            GameOverlayRenderer::drawPaused(vg, x, y, w);

        if (sig.isMuted() && GET_SETTING_KEY_INT("display.showMuteOverlay", 1))
            GameOverlayRenderer::drawMute(vg, x, y, w, h);
    }

    void NdsGameView::_stepEmulation()
    {
        if (!m_nds_core || !m_nds_core->IsReady()) return;

        auto& sig = GameSignal::instance();

        if (sig.isPaused()) return;

        if (sig.consumeReset()) {
            m_nds_core->Reset();
        }

        m_nds_core->SetButtonsFromSignal();

        bool ff = sig.isFastForward();
        m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
        if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;
        m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

        unsigned count = 1;
        if (ff) {
            count = static_cast<unsigned>(m_ffMultiplier);
            float frac = m_ffMultiplier - static_cast<float>(count);
            m_ffSlowAccum += frac;
            if (m_ffSlowAccum >= 1.0f) { m_ffSlowAccum -= 1.0f; ++count; }
            if (count == 0) count = 1;
        }

        for (unsigned i = 0; i < count; ++i)
            m_nds_core->RunFrame();

        if (count > 0)
        {
            _pushFrameAudio(ff, count);
            _updateFpsStats(count);
        }
    }

    void NdsGameView::_renderOutput()
    {
#ifdef OGLRENDERER_ENABLED
        auto* nds = m_nds_core ? m_nds_core->GetNDS() : nullptr;
        if (nds && nds->GPU.GetRenderer3D().Accelerated)
        {
            auto& renderer = static_cast<melonDS::GLRenderer&>(nds->GPU.GetRenderer3D());
            auto& compositor = renderer.GetCompositor();
            int fb = nds->GPU.FrontBuffer;
            compositor.BindOutputTexture(fb);
            GLint texId = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &texId);
            m_renderer.bindExternalTexture(static_cast<GLuint>(texId), 256, 192 * 2 + 2);
            return;
        }
#endif

        const uint32_t* top = nullptr, *bot = nullptr;
        if (m_nds_core && m_nds_core->IsReady())
        {
            top = m_nds_core->GetTopFramebuffer();
            bot = m_nds_core->GetBottomFramebuffer();
        }
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

        m_renderer.uploadFrame(frame);
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

    void NdsGameView::_updateFpsStats(unsigned framesRan)
    {
        using Clock = std::chrono::steady_clock;
        m_fpsFrameCount += framesRan;
        auto now = Clock::now();
        double elap = std::chrono::duration<double>(now - m_fpsLastTime).count();
        if (elap >= 1.0)
        {
            m_currentFps = static_cast<float>(m_fpsFrameCount / elap);
            m_fpsFrameCount = 0;
            m_fpsLastTime = now;
        }
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
            { EMU_A,      "a",      8  }, { EMU_B,      "b",      0  },
            { EMU_X,      "x",      9  }, { EMU_Y,      "y",      1  },
            { EMU_UP,     "up",     4  }, { EMU_DOWN,   "down",   5  },
            { EMU_LEFT,   "left",   6  }, { EMU_RIGHT,  "right",  7  },
            { EMU_L,      "l",      10 }, { EMU_R,      "r",      11 },
            { EMU_L2,     "l2",     12 }, { EMU_R2,     "r2",     13 },
            { EMU_L3,     "l3",     14 }, { EMU_R3,     "r3",     15 },
            { EMU_START,  "start",  3  }, { EMU_SELECT, "select", 2  },
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
                EmuFunctionKey emuKey; const char* cfgSuffix; unsigned retroId;
            };
            static const StickBtnInfo stickBtnInfos[] = {
                { EMU_LEFT_STICK_UP,     "lstick_up",    4 },
                { EMU_LEFT_STICK_DOWN,   "lstick_down",  5 },
                { EMU_LEFT_STICK_LEFT,   "lstick_left",  6 },
                { EMU_LEFT_STICK_RIGHT,  "lstick_right", 7 },
                { EMU_RIGHT_STICK_UP,    "rstick_up",    4 },
                { EMU_RIGHT_STICK_DOWN,  "rstick_down",  5 },
                { EMU_RIGHT_STICK_LEFT,  "rstick_left",  6 },
                { EMU_RIGHT_STICK_RIGHT, "rstick_right", 7 },
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
            _startLoadThread();
        }
    }

    void NdsGameView::_startLoadThread()
    {
        m_loadThread = std::thread([this]() {
            auto t0 = std::chrono::steady_clock::now();

            m_nds_core = new beiklive::melonds::CoreMelonDS();
            if (!m_nds_core->SetupGame(m_gameEntry))
            {
                brls::Logger::error("NDS core init failed: {}", m_gameEntry.path);
                delete m_nds_core;
                m_nds_core = nullptr;
                m_loadFailed.store(true);
                return;
            }

            auto t1 = std::chrono::steady_clock::now();
            brls::Logger::info("NDS core SetupGame took {:.1f}s",
                std::chrono::duration<double>(t1 - t0).count());

#ifdef OGLRENDERER_ENABLED
            brls::sync([this]() {
                if (m_nds_core && m_nds_core->IsReady())
                {
                    auto* nds = m_nds_core->GetNDS();
                    if (nds)
                    {
                        auto glrenderer = melonDS::GLRenderer::New();
                        glrenderer->SetRenderSettings(true, 1);
                        nds->GPU.SetRenderer3D(std::move(glrenderer));
                        m_oglActive = true;
                        brls::Logger::info("NdsGameView: GLRenderer activated");
                    }
                }
            });
#endif

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

            {
                int16_t discardBuf[4096];
                while (m_nds_core->ReadAudio(discardBuf, 2048) > 0)
                    ;
            }

            GameSignal::instance().resetAll();
            m_loadDone.store(true);
        });
    }

    void NdsGameView::_joinLoadThread()
    {
        if (m_loadThread.joinable())
            m_loadThread.join();
    }

} // namespace beiklive
