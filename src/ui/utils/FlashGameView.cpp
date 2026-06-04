#include "FlashGameView.hpp"
#include "FlashGameMenuView.hpp"
#include "game/flash/flashnx_bridge.h"
#include "game/flash/FlashKeymap.hpp"
#include "game/audio/AudioManager.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "ui/utils/GameOverlayRenderer.hpp"
#include "core/Tools.hpp"

#include <filesystem>
#include <fstream>

namespace beiklive::flash {

FlashGameView::FlashGameView(beiklive::GameEntry gameData)
    : m_gameEntry(std::move(gameData))
{
    _brls_inputLocked = false;
    GameInputManager::instance().sayHello();
    HIDE_BRLS_HIGHLIGHT(this);

    m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
    if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;
    m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

    _registerFlashInput();
    _registerGameRuntime();
}

FlashGameView::~FlashGameView()
{
    if (m_flashCore) {
        delete m_flashCore;
        m_flashCore = nullptr;
    }

    GameInputManager::instance().clearEmuFunctionKeys();
    GameInputManager::instance().dropInput();
    GameInputManager::instance().setFlashInputMode(false);
}

void FlashGameView::onFocusGained()
{
    Box::onFocusGained();
    GameSignal::instance().requestPause(false);
    GameInputManager::instance().setInputEnabled(true);

    if (!_brls_inputLocked) {
        _brls_inputLocked = true;
        brls::Application::blockInputs(true);
    }
}

void FlashGameView::onFocusLost()
{
    Box::onFocusLost();
    GameSignal::instance().requestPause(true);
    GameInputManager::instance().setInputEnabled(false);
    GameInputManager::instance().dropInput();

    if (_brls_inputLocked) {
        _brls_inputLocked = false;
        brls::Application::unblockInputs();
    }
}

void FlashGameView::draw(NVGcontext* vg, float x, float y, float width, float height,
                         brls::Style style, brls::FrameContext* ctx)
{
    Box::draw(vg, x, y, width, height, style, ctx);

    GameInputManager::instance().handleInput();

    if (GameSignal::instance().consumeExit()) {
        _saveAndCommitPlayTime();
        brls::sync([this]() { brls::Application::popActivity(); });
        return;
    }

    if (GameSignal::instance().consumeOpenMenu()) {
        if (m_menuView) {
            brls::sync([this]() {
                GameSignal::instance().requestPause(true);
                AnimationHelper::slideInFromBottom(m_menuView, 60.f, 120);
                brls::Application::giveFocus(m_menuView);
                m_menuView->onShow();
            });
        }
    }

    if (GameSignal::instance().consumeReset()) {
        if (m_flashCore)
            m_flashCore->Restart();
    }

    _handleInputs();

    auto& sig = GameSignal::instance();

    if (sig.isPaused()) {
        _renderAndOverlay(vg, x, y, width, height);
        return;
    }

    bool ff = sig.isFastForward();
    uint64_t dt_us = 0;

    if (m_lastTickTime == 0) {
        m_lastTickTime = ruffle_tick_now();
    } else {
        uint64_t now = ruffle_tick_now();
        uint64_t freq = ruffle_tick_freq();
        if (freq > 0) {
            dt_us = (now - m_lastTickTime) * 1'000'000 / freq;
            if (dt_us > 100'000) dt_us = 100'000;
        }
        m_lastTickTime = now;
    }

    if (ff) {
        dt_us = static_cast<uint64_t>(dt_us * m_ffMultiplier);
    }

    dt_us = std::max<uint64_t>(dt_us, 1000);

    if (m_flashCore)
        m_flashCore->RenderFrame(dt_us);

    _renderAndOverlay(vg, x, y, width, height);

    _updateFpsStats();
}

void FlashGameView::_handleInputs()
{
    auto& sig = GameSignal::instance();
    uint32_t mask = sig.getGameButtonMask();

    static const struct {
        uint32_t flag;
        const char* btnName;
    } k_btnMap[] = {
        { beiklive::A_FLAG,     "A" },
        { beiklive::B_FLAG,     "B" },
        { beiklive::X_FLAG,     "X" },
        { beiklive::Y_FLAG,     "Y" },
        { beiklive::LB_FLAG,    "L" },
        { beiklive::RB_FLAG,    "R" },
        { beiklive::PLAY_FLAG,  "Plus" },
        { beiklive::UP_FLAG,    "Up" },
        { beiklive::DOWN_FLAG,  "Down" },
        { beiklive::LEFT_FLAG,  "Left" },
        { beiklive::RIGHT_FLAG, "Right" },
    };

    for (const auto& btn : k_btnMap) {
        bool pressed = (mask & btn.flag) != 0;
        std::string flashKey = FlashKeymap::lookup(btn.btnName);
        if (flashKey.empty() || flashKey == "(none)" || flashKey == "MouseLeft")
            continue;

        int skCode = SK_NONE;
        if (flashKey == "Space")     skCode = SK_SPACE;
        else if (flashKey == "Enter")    skCode = SK_ENTER;
        else if (flashKey == "Escape")   skCode = SK_ESCAPE;
        else if (flashKey == "Shift")    skCode = SK_SHIFT;
        else if (flashKey == "Control")  skCode = SK_CONTROL;
        else if (flashKey == "Alt")      skCode = SK_ALT;
        else if (flashKey == "Tab")      skCode = SK_TAB;
        else if (flashKey == "Backspace") skCode = SK_BACKSPACE;
        else if (flashKey == "Up")       skCode = SK_UP;
        else if (flashKey == "Down")     skCode = SK_DOWN;
        else if (flashKey == "Left")     skCode = SK_LEFT;
        else if (flashKey == "Right")    skCode = SK_RIGHT;
        else {
            char c = flashKey[0];
            if (c >= 'A' && c <= 'Z')      skCode = SK_A + (c - 'A');
            else if (c >= 'a' && c <= 'z') skCode = SK_A + (c - 'a');
            else if (c >= '0' && c <= '9') skCode = SK_0 + (c - '0');
        }

        if (skCode != SK_NONE && m_flashCore)
            m_flashCore->HandleKey(skCode, pressed);
    }

    float mx = sig.getMouseX();
    float my = sig.getMouseY();
    if (m_flashCore)
        m_flashCore->HandleMouseMove(
            static_cast<int>(mx * VIEWPORT_W),
            static_cast<int>(my * VIEWPORT_H));
    if (sig.isMouseDown()) {
        if (m_flashCore)
            m_flashCore->HandleMouseButton(true);
    }
}

void FlashGameView::_renderAndOverlay(NVGcontext* vg, float x, float y, float w, float h)
{
    if (m_flashCore && m_ready) {
        ruffle_redraw_paused();
    }

    _drawMouseCursor(vg, x, y, w, h);
    _drawOverlays(vg, x, y, w, h);
}

void FlashGameView::_drawMouseCursor(NVGcontext* vg, float x, float y, float w, float h)
{
    auto& sig = GameSignal::instance();
    if (sig.isPaused()) return;

    float mx = sig.getMouseX();
    float my = sig.getMouseY();
    bool  md = sig.isMouseDown();

    float cx = x + mx * w;
    float cy = y + my * h;

    MouseCursor cursor;
    cursor.draw(vg, cx, cy, md);
}

void FlashGameView::_drawOverlays(NVGcontext* vg, float x, float y, float w, float h)
{
    auto& sig = GameSignal::instance();

    if (GET_SETTING_KEY_INT("display.showFps", 0)) {
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

void FlashGameView::_updateFpsStats()
{
    auto now = std::chrono::steady_clock::now();
    ++m_fpsFrameCount;
    double elap = std::chrono::duration<double>(now - m_fpsLastTime).count();
    if (elap >= FPS_UPDATE_INTERVAL) {
        float fps = static_cast<float>(m_fpsFrameCount / elap);
        {
            std::lock_guard<std::mutex> lk(m_fpsMutex);
            m_currentFps = fps;
        }
        m_fpsFrameCount = 0;
        m_fpsLastTime = now;
    }
}

void FlashGameView::_registerFlashInput()
{
    GameInputManager::instance().registerFlashKeyBindings();
    GameInputManager::instance().setFlashInputMode(true);

    // 打开菜单
    {
        std::string val = GET_SETTING_KEY_STR("hotkey.menu.pad", "Minus+ZR");
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

    // 快进
    {
        std::string val = GET_SETTING_KEY_STR("handle.fastforward", "RSB");
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

    // 静音
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

void FlashGameView::_registerGameRuntime()
{
    if (m_gameEntry.platform != (int)beiklive::enums::EmuPlatform::EmuFlash) {
        brls::Logger::warning("非 Flash 平台: {}", m_gameEntry.platform);
        return;
    }

    m_flashCore = new FlashGameRun();
    if (m_flashCore->SetupGame(m_gameEntry)) {
        GameSignal::instance().resetAll();
        _initPlayTimeTracking();
        m_lastTickTime = ruffle_tick_now();
        m_fpsLastTime = std::chrono::steady_clock::now();
        m_ready = true;
        brls::Logger::info("FlashGameView: 核心初始化完成 {}", m_gameEntry.path);
    } else {
        brls::Logger::error("FlashGameView: 核心初始化失败 {}", m_gameEntry.path);
        delete m_flashCore;
        m_flashCore = nullptr;
    }
}

void FlashGameView::_initPlayTimeTracking()
{
    namespace fs = std::filesystem;
    std::string dir = m_gameEntry.savePath;
    if (dir.empty()) dir = beiklive::path::savePath();
    std::string stem = fs::path(m_gameEntry.path).stem().string();
    std::error_code ec;
    fs::create_directories(dir, ec);
    m_playTimeTempPath = dir + "/" + stem + ".playtime";
    m_playStartTime = std::chrono::steady_clock::now();
}

void FlashGameView::_savePlayTimeCheckpoint()
{
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_playStartTime).count();
    if (elapsed < 0.5) return;
    m_gameEntry.playTime += static_cast<int>(elapsed);
    m_playStartTime = now;
}

void FlashGameView::_saveAndCommitPlayTime()
{
    _savePlayTimeCheckpoint();
    if (beiklive::GameDB && m_gameEntry.playTime > 0) {
        beiklive::GameDB->set(m_gameEntry.path, "playTime", nlohmann::json(m_gameEntry.playTime));
    }
}

} // namespace beiklive::flash
