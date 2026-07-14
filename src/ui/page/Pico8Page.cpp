#include "Pico8Page.hpp"

#include "core/common.h"
#include "ui/utils/Pico8Transition.hpp"
#include "ui/view/SwitchLayout.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float LOGO_MOVE_DURATION = 0.34f;
    constexpr float LIST_ENTER_DELAY = 0.27f;
    constexpr float LIST_ENTER_DURATION = 0.46f;
    constexpr float PREVIEW_ENTER_DELAY = 0.37f;
    constexpr float PREVIEW_ENTER_DURATION = 0.32f;
    constexpr float ENTER_DURATION = 0.78f;
    constexpr float LAUNCH_LIST_DURATION = 0.27f;
    constexpr float LAUNCH_MOVE_DELAY = 0.10f;
    constexpr float LAUNCH_MOVE_DURATION = 0.42f;
    constexpr float LAUNCH_DURATION = 0.55f;
    constexpr float PAUSE_DURATION = 0.55f;
    constexpr float EXIT_LIBRARY_DURATION = 0.70f;
    constexpr float GAME_FADE_DURATION = 0.16f;
    constexpr float NAV_DELAY = 0.26f;
    constexpr float NAV_REPEAT = 0.075f;
    constexpr float RETURN_TO_GAME_DELAY = 0.30f;
    constexpr float INPUT_TRACE_LIFETIME = 2.5f;
    constexpr float LIST_ITEM_HEIGHT = 51.f;
    constexpr int LIST_VISIBLE_ITEMS = 9;

    struct Rect
    {
        float x = 0.f;
        float y = 0.f;
        float width = 0.f;
        float height = 0.f;
    };

    float clamp01(float value)
    {
        return std::max(0.f, std::min(1.f, value));
    }

    float smoothStep(float value)
    {
        value = clamp01(value);
        return value * value * (3.f - 2.f * value);
    }

    float easeOutBack(float value)
    {
        value = clamp01(value);
        constexpr float c1 = 1.16f;
        constexpr float c3 = c1 + 1.f;
        const float shifted = value - 1.f;
        return 1.f + c3 * shifted * shifted * shifted +
            c1 * shifted * shifted;
    }

    float livelyProgress(float value)
    {
        value = clamp01(value);
        if (value >= 1.f)
            return 1.f;
        return 1.f - std::exp(-6.2f * value) *
            std::cos(9.5f * value);
    }

    float lerp(float from, float to, float progress)
    {
        return from + (to - from) * progress;
    }

    Rect lerpRect(const Rect& from, const Rect& to, float progress)
    {
        return {
            lerp(from.x, to.x, progress),
            lerp(from.y, to.y, progress),
            lerp(from.width, to.width, progress),
            lerp(from.height, to.height, progress),
        };
    }

    unsigned char alphaByte(float value)
    {
        return static_cast<unsigned char>(255.f * clamp01(value));
    }

    std::string encodeUtf8(char32_t codepoint)
    {
        std::string output;
        if (codepoint <= 0x7f) {
            output.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        } else if (codepoint <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        } else {
            output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        }
        return output;
    }

    Rect centerLogoRect(float x, float y, float width, float height)
    {
        const auto geometry = beiklive::pico8_transition::geometry(
            x, y, width, height);
        return {geometry.centerLogoX, geometry.centerLogoY,
                geometry.centerLogoWidth, geometry.centerLogoHeight};
    }

    Rect cornerLogoRect(float x, float y)
    {
        return {x + 42.f, y + 34.f, 190.f, 68.f};
    }

    Rect previewRect(float x, float y, float width, float height)
    {
        const float rightLeft = x + width * 0.43f;
        const float rightWidth = x + width - 42.f - rightLeft;
        const float size = std::max(220.f, std::min(450.f, height - 164.f));
        return {
            rightLeft + (rightWidth - size) * 0.5f,
            y + (height - size) * 0.5f - 2.f,
            size,
            size,
        };
    }

    Rect runtimeRect(float x, float y, float width, float height)
    {
        const float scale = std::max(1.f, std::floor(
            std::min(width, height) / 128.f));
        const float size = 128.f * scale;
        return {x + (width - size) * 0.5f,
                y + (height - size) * 0.5f, size, size};
    }
}

namespace beiklive
{
    Pico8Page::Pico8Page(SwitchLayout* homeLayout)
        : brls::Box(brls::Axis::COLUMN), m_homeLayout(homeLayout)
    {
        setFocusable(true);
        setGrow(1.f);
        setBackground(brls::ViewBackground::NONE);
        setHideHighlightBackground(true);
        setHideHighlightBorder(true);
        setHideClickAnimation(true);
        m_lastFrameTime = std::chrono::steady_clock::now();

        auto consume = [](brls::View*) { return true; };
        registerAction("", brls::BUTTON_A, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_B, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_UP, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LEFT, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_START, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LT, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RT, consume, true, false, brls::SOUND_NONE);

        pico8::Filesystem::ensureDirectories();
        m_scanFuture = std::async(std::launch::async, []() {
            return pico8::Filesystem::scanGames();
        });
        m_coreFuture = std::async(std::launch::async, [this]() {
            return m_core.Initialize();
        });
        _captureInputState();
    }

    Pico8Page::~Pico8Page()
    {
        if (m_scanFuture.valid())
            m_scanFuture.wait();
        if (m_coreFuture.valid())
            m_coreFuture.wait();
        m_core.Shutdown();
        if (auto* vg = brls::Application::getNVGContext()) {
            _releaseCover(vg);
            m_video.shutdown(vg);
            if (m_logoImageHandle > 0)
                nvgDeleteImage(vg, m_logoImageHandle);
        }
    }

    void Pico8Page::_captureInputState()
    {
        const auto& input = brls::Application::getControllerState();
        const float ly = input.axes[static_cast<int>(brls::LEFT_Y)];
        const float ry = input.axes[static_cast<int>(brls::RIGHT_Y)];
        const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
        m_prevUp = input.buttons[static_cast<int>(brls::BUTTON_UP)] || navY < -0.5f;
        m_prevDown = input.buttons[static_cast<int>(brls::BUTTON_DOWN)] || navY > 0.5f;
        m_prevA = input.buttons[static_cast<int>(brls::BUTTON_A)];
        m_prevB = input.buttons[static_cast<int>(brls::BUTTON_B)];
        m_prevStart = input.buttons[static_cast<int>(brls::BUTTON_START)];
        m_prevLt = input.buttons[static_cast<int>(brls::BUTTON_LT)];
        m_prevRt = input.buttons[static_cast<int>(brls::BUTTON_RT)];
        const bool up = input.buttons[static_cast<int>(brls::BUTTON_UP)] ||
            input.buttons[static_cast<int>(brls::BUTTON_NAV_UP)] || navY < -0.5f;
        const bool down = input.buttons[static_cast<int>(brls::BUTTON_DOWN)] ||
            input.buttons[static_cast<int>(brls::BUTTON_NAV_DOWN)] || navY > 0.5f;
        m_navAwaitRelease = up || down;
        m_navDirection = 0;
        m_navHold = 0.f;
        m_navRepeat = 0.f;
    }

    void Pico8Page::_pollBackgroundTasks()
    {
        using namespace std::chrono_literals;
        if (!m_scanFinished && m_scanFuture.valid() &&
            m_scanFuture.wait_for(0ms) == std::future_status::ready) {
            m_games = m_scanFuture.get();
            m_scanFinished = true;
            m_selectedIndex = 0;
            m_listScroll = 0.f;
            m_selectionIdle = 0.f;
        }
        if (!m_coreReady && m_coreFuture.valid() &&
            m_coreFuture.wait_for(0ms) == std::future_status::ready) {
            m_coreReady = m_coreFuture.get();
            m_coreFinished = true;
            if (!m_coreReady)
                m_errorText = "FAKE-08 Runtime 初始化失败";
        }
    }

    bool Pico8Page::_selectedIsLoadedGame() const
    {
        return m_core.isGameLoaded() && !m_loadedGamePath.empty() &&
            m_selectedIndex >= 0 &&
            static_cast<size_t>(m_selectedIndex) < m_games.size() &&
            m_games[static_cast<size_t>(m_selectedIndex)].path == m_loadedGamePath;
    }

    void Pico8Page::_selectRelative(int direction)
    {
        if (m_games.empty())
            return;
        const int count = static_cast<int>(m_games.size());
        m_selectedIndex = (m_selectedIndex + direction + count) % count;
        m_coverImagePath.clear();
        m_coverResolvedCartPath.clear();
        m_selectionIdle = 0.f;
    }

    void Pico8Page::_handleLibraryInput(float dt)
    {
        m_selectionIdle += dt;
        const auto& input = brls::Application::getControllerState();
        const float ly = input.axes[static_cast<int>(brls::LEFT_Y)];
        const float ry = input.axes[static_cast<int>(brls::RIGHT_Y)];
        const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
        const bool up = input.buttons[static_cast<int>(brls::BUTTON_UP)] ||
            input.buttons[static_cast<int>(brls::BUTTON_NAV_UP)] || navY < -0.5f;
        const bool down = input.buttons[static_cast<int>(brls::BUTTON_DOWN)] ||
            input.buttons[static_cast<int>(brls::BUTTON_NAV_DOWN)] || navY > 0.5f;
        const bool a = input.buttons[static_cast<int>(brls::BUTTON_A)];
        const bool b = input.buttons[static_cast<int>(brls::BUTTON_B)];
        const bool start = input.buttons[static_cast<int>(brls::BUTTON_START)];

        const int direction = up == down ? 0 : (up ? -1 : 1);
        if (m_navAwaitRelease) {
            if (direction == 0)
                m_navAwaitRelease = false;
            m_navDirection = 0;
            m_navHold = 0.f;
            m_navRepeat = 0.f;
        } else if (direction == 0) {
            m_navDirection = 0;
            m_navHold = 0.f;
            m_navRepeat = 0.f;
        } else if (direction != m_navDirection) {
            m_navDirection = direction;
            m_navHold = 0.f;
            m_navRepeat = 0.f;
            _selectRelative(direction);
        } else {
            const float previousHold = m_navHold;
            m_navHold += dt;
            if (previousHold < NAV_DELAY && m_navHold >= NAV_DELAY) {
                _selectRelative(direction);
                m_navRepeat = 0.f;
            } else if (m_navHold >= NAV_DELAY) {
                m_navRepeat += dt;
                while (m_navRepeat >= NAV_REPEAT) {
                    m_navRepeat -= NAV_REPEAT;
                    _selectRelative(direction);
                }
            }
        }

        if (a && !m_prevA)
            _beginLaunch();
        if (b && !m_prevB && m_state == State::PausedLibrary)
            _returnToRunningGame();
        if (start && !m_prevStart)
            _beginClose();

        m_prevUp = up;
        m_prevDown = down;
        m_prevA = a;
        m_prevB = b;
        m_prevStart = start;
    }

    void Pico8Page::_handleEmptyInput()
    {
        const auto& input = brls::Application::getControllerState();
        const bool b = input.buttons[static_cast<int>(brls::BUTTON_B)];
        const bool start = input.buttons[static_cast<int>(brls::BUTTON_START)];
        if ((b && !m_prevB) || (start && !m_prevStart))
            _beginClose();
        m_prevB = b;
        m_prevA = input.buttons[static_cast<int>(brls::BUTTON_A)];
        m_prevStart = start;
    }

    void Pico8Page::_beginLaunch()
    {
        if ((m_state != State::Library && m_state != State::PausedLibrary) ||
            m_games.empty() || m_selectedIndex < 0 ||
            static_cast<size_t>(m_selectedIndex) >= m_games.size())
            return;
        if (m_coreFinished && !m_coreReady) {
            m_errorText = "FAKE-08 Runtime 初始化失败，请查看 pico.log";
            return;
        }
        m_launchUsesRuntime = _selectedIsLoadedGame();
        m_returnToGamePending = false;
        m_state = State::Launching;
        m_stateTime = 0.f;
        m_loadAttempted = false;
        m_errorText.clear();
        brls::Application::blockInputs();
        _captureInputState();
    }

    void Pico8Page::_beginPause()
    {
        if (m_state != State::Running)
            return;
        m_core.Pause();
        m_input.reset();
        m_state = State::Pausing;
        m_stateTime = 0.f;
        brls::Application::blockInputs();
        _captureInputState();
    }

    void Pico8Page::_returnToRunningGame()
    {
        if (m_state != State::PausedLibrary || m_loadedGamePath.empty())
            return;
        const auto it = std::find_if(
            m_games.begin(), m_games.end(), [this](const pico8::GameEntry& game) {
                return game.path == m_loadedGamePath;
            });
        if (it == m_games.end())
            return;
        const int index = static_cast<int>(std::distance(m_games.begin(), it));
        if (m_selectedIndex == index) {
            _beginLaunch();
            return;
        }
        m_selectedIndex = index;
        m_coverImagePath.clear();
        m_coverResolvedCartPath.clear();
        m_selectionIdle = 1.f;
        m_returnToGamePending = true;
        m_returnToGameTime = 0.f;
        m_navDirection = 0;
        m_navHold = 0.f;
        m_navRepeat = 0.f;
        _captureInputState();
    }

    void Pico8Page::_quickSave()
    {
        if (m_state != State::Running)
            return;
        if (m_core.SaveState(m_quickState))
            brls::Application::notify("PICO-8 快速存档完成");
        else
            brls::Application::notify("PICO-8 快速存档失败，请查看 pico.log");
    }

    void Pico8Page::_quickLoad()
    {
        if (m_state != State::Running)
            return;
        if (m_quickState.empty()) {
            brls::Application::notify("没有可读取的 PICO-8 快速存档");
            return;
        }
        if (m_core.LoadState(m_quickState.data(), m_quickState.size())) {
            m_input.reset();
            m_frameDirty = true;
            _captureInputState();
            _captureTraceInput();
            brls::Application::notify("PICO-8 快速读档完成");
        } else {
            brls::Application::notify("PICO-8 快速读档失败，请查看 pico.log");
        }
    }

    void Pico8Page::_captureTraceInput()
    {
        const auto& input = brls::Application::getControllerState();
        for (size_t i = 0; i < m_tracePreviousButtons.size(); ++i)
            m_tracePreviousButtons[i] = input.buttons[i];
    }

    void Pico8Page::_updateInputTrace(float dt)
    {
        for (auto& entry : m_inputTrace)
            entry.age += dt;
        while (!m_inputTrace.empty() &&
               m_inputTrace.front().age >= INPUT_TRACE_LIFETIME)
            m_inputTrace.pop_front();

        const auto& input = brls::Application::getControllerState();
        struct WatchedButton
        {
            brls::ControllerButton button;
            const char* label;
        };
        constexpr WatchedButton watched[] = {
            {brls::BUTTON_NAV_UP, "上"},
            {brls::BUTTON_NAV_DOWN, "下"},
            {brls::BUTTON_NAV_LEFT, "左"},
            {brls::BUTTON_NAV_RIGHT, "右"},
            {brls::BUTTON_A, "A"},
            {brls::BUTTON_B, "B"},
            {brls::BUTTON_X, "X"},
            {brls::BUTTON_Y, "Y"},
            {brls::BUTTON_LB, "L"},
            {brls::BUTTON_RB, "R"},
            {brls::BUTTON_LT, "ZL"},
            {brls::BUTTON_RT, "ZR"},
            {brls::BUTTON_START, "+"},
        };
        for (const auto& watchedButton : watched) {
            const size_t index = static_cast<size_t>(watchedButton.button);
            const bool pressed = input.buttons[index];
            if (pressed && !m_tracePreviousButtons[index]) {
                if (m_inputTrace.size() >= 11)
                    m_inputTrace.pop_front();
                m_inputTrace.push_back({watchedButton.label, 0.f});
            }
            m_tracePreviousButtons[index] = pressed;
        }
    }

    void Pico8Page::_beginClose()
    {
        if ((m_state != State::Waiting && m_state != State::Empty &&
             m_state != State::Library && m_state != State::PausedLibrary) ||
            m_popScheduled)
            return;

        m_exitHasLibrary = !m_games.empty() && m_state != State::Waiting;
        m_exitUsesRuntimePreview =
            m_state == State::PausedLibrary && _selectedIsLoadedGame();
        if (!m_exitHasLibrary) {
            brls::Application::blockInputs();
            if (m_homeLayout) {
                m_homeLayout->beginPico8ReturnAnimation();
                m_homeLayout->finishPico8ReturnAnimation();
            }
            m_popScheduled = true;
            brls::sync([]() {
                brls::Application::popActivity(
                    brls::TransitionAnimation::NONE,
                    []() { brls::Application::unblockInputs(); });
            });
            return;
        }
        m_state = State::Exiting;
        m_stateTime = 0.f;
        m_homeReturnStarted = false;
        brls::Application::blockInputs();
        invalidate();
    }

    void Pico8Page::_finishHomeReturn()
    {
        if (m_popScheduled)
            return;
        m_popScheduled = true;
        if (m_homeLayout)
            m_homeLayout->finishPico8ReturnAnimation();
        brls::sync([]() {
            brls::Application::popActivity(
                brls::TransitionAnimation::NONE,
                []() { brls::Application::unblockInputs(); });
        });
    }

    void Pico8Page::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;

        _pollBackgroundTasks();
        m_stateTime += dt;

        switch (m_state) {
            case State::Waiting:
                _handleEmptyInput();
                if (m_state == State::Waiting && m_scanFinished) {
                    m_state = m_games.empty() ? State::Empty : State::Entering;
                    m_stateTime = 0.f;
                    if (!m_games.empty())
                        brls::Application::blockInputs();
                    else
                        brls::Application::unblockInputs();
                    _captureInputState();
                }
                break;
            case State::Entering:
                m_selectionIdle += dt;
                if (m_stateTime >= ENTER_DURATION) {
                    m_state = State::Library;
                    m_stateTime = 0.f;
                    m_selectedIndex = 0;
                    brls::Application::unblockInputs();
                    _captureInputState();
                }
                break;
            case State::Empty:
                _handleEmptyInput();
                break;
            case State::Library:
            case State::PausedLibrary:
                if (m_returnToGamePending && m_state == State::PausedLibrary) {
                    m_returnToGameTime += dt;
                    m_selectionIdle += dt;
                    if (m_returnToGameTime >= RETURN_TO_GAME_DELAY) {
                        m_returnToGamePending = false;
                        _beginLaunch();
                    }
                } else {
                    _handleLibraryInput(dt);
                }
                break;
            case State::Launching:
                if (m_stateTime >= LAUNCH_DURATION && !m_loadAttempted &&
                    m_coreReady) {
                    m_loadAttempted = true;
                    const auto& game = m_games[static_cast<size_t>(m_selectedIndex)];
                    bool loaded = false;
                    if (m_launchUsesRuntime) {
                        m_core.Resume();
                        loaded = true;
                    } else {
                        loaded = m_core.LoadGame(game.path);
                    }
                    if (loaded) {
                        if (!m_launchUsesRuntime)
                            m_quickState.clear();
                        m_loadedGamePath = game.path;
                        m_input.reset();
                        m_inputTrace.clear();
                        m_state = State::Running;
                        m_stateTime = 0.f;
                        m_frameDirty = true;
                        brls::Application::unblockInputs();
                        _captureInputState();
                        _captureTraceInput();
                    } else {
                        m_errorText = m_core.lastError();
                        if (!m_core.isGameLoaded())
                            m_loadedGamePath.clear();
                        m_state = m_core.isGameLoaded()
                            ? State::PausedLibrary : State::Library;
                        m_stateTime = 0.f;
                        brls::Application::unblockInputs();
                        _captureInputState();
                    }
                } else if (m_stateTime >= LAUNCH_DURATION &&
                           m_coreFinished && !m_coreReady) {
                    m_errorText = "FAKE-08 Runtime 初始化失败，请查看 pico.log";
                    m_state = m_core.isGameLoaded()
                        ? State::PausedLibrary : State::Library;
                    m_stateTime = 0.f;
                    brls::Application::unblockInputs();
                    _captureInputState();
                }
                break;
            case State::Running: {
                const auto& raw = brls::Application::getControllerState();
                const bool start = raw.buttons[static_cast<int>(brls::BUTTON_START)];
                const bool lt = raw.buttons[static_cast<int>(brls::BUTTON_LT)];
                const bool rt = raw.buttons[static_cast<int>(brls::BUTTON_RT)];
                _updateInputTrace(dt);
                if (start && !m_prevStart) {
                    _beginPause();
                    break;
                }
                if (rt && !m_prevRt)
                    _quickSave();
                if (lt && !m_prevLt)
                    _quickLoad();
                m_prevStart = start;
                m_prevLt = lt;
                m_prevRt = rt;
                m_core.SetInput(m_input.poll());
                if (m_core.RunFrame(dt))
                    m_frameDirty = true;
                break;
            }
            case State::Pausing:
                if (m_stateTime >= PAUSE_DURATION) {
                    m_state = State::PausedLibrary;
                    m_stateTime = 0.f;
                    m_selectionIdle = 1.f;
                    brls::Application::unblockInputs();
                    _captureInputState();
                }
                break;
            case State::Exiting: {
                const float uiDuration = m_exitHasLibrary
                    ? EXIT_LIBRARY_DURATION : 0.f;
                if (m_stateTime >= uiDuration && !m_homeReturnStarted) {
                    m_homeReturnStarted = true;
                    if (m_homeLayout)
                        m_homeLayout->beginPico8ReturnAnimation();
                }
                if (m_homeReturnStarted) {
                    const float progress = clamp01(
                        (m_stateTime - uiDuration) /
                        pico8_transition::TRANSITION_DURATION);
                    if (m_homeLayout)
                        m_homeLayout->setPico8ReturnProgress(progress);
                    if (progress >= 1.f)
                        _finishHomeReturn();
                }
                break;
            }
        }
        invalidate();
    }

    void Pico8Page::_ensureFonts(NVGcontext* vg)
    {
        if (m_fontId < 0)
            m_fontId = brls::Application::getFont(brls::FONT_REGULAR);
        if (m_switchIconFontId < 0)
            m_switchIconFontId = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        if (m_fontAttempted)
            return;
        m_fontAttempted = true;
        const std::string path = pico8::Filesystem::fontPath();
        std::error_code ec;
        if (std::filesystem::is_regular_file(path, ec)) {
            const int custom = nvgCreateFont(vg, "pico8-page", path.c_str());
            if (custom >= 0)
                m_fontId = custom;
        }
    }

    void Pico8Page::_releaseCover(NVGcontext* vg)
    {
        if (m_coverImageHandle > 0 && vg)
            nvgDeleteImage(vg, m_coverImageHandle);
        m_coverImageHandle = 0;
        m_coverImagePath.clear();
    }

    void Pico8Page::_ensureCover(NVGcontext* vg)
    {
        if (m_games.empty() || m_selectedIndex < 0 ||
            static_cast<size_t>(m_selectedIndex) >= m_games.size()) {
            _releaseCover(vg);
            return;
        }
        auto& game = m_games[static_cast<size_t>(m_selectedIndex)];
        if (m_coverResolvedCartPath != game.path) {
            if (m_coverImageHandle > 0)
                _releaseCover(vg);
            if (m_selectionIdle < 0.12f)
                return;
            game.coverPath = pico8::Filesystem::resolveCover(game);
            m_coverResolvedCartPath = game.path;
        }
        const std::string& resolvedPath = game.coverPath;
        if (resolvedPath == m_coverImagePath)
            return;
        _releaseCover(vg);
        m_coverImagePath = resolvedPath;
        if (!resolvedPath.empty())
            m_coverImageHandle = nvgCreateImage(
                vg, resolvedPath.c_str(), NVG_IMAGE_NEAREST);
    }

    void Pico8Page::_drawHint(NVGcontext* vg, brls::ControllerButton button,
                              const std::string& label, float x, float y,
                              float alpha)
    {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_switchIconFontId);
        nvgFontSize(vg, 29.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, alphaByte(alpha)));
        nvgText(vg, x, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 21.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + 22.f, y, label.c_str(), nullptr);
    }

    void Pico8Page::_drawHeaderLogo(NVGcontext* vg, float x, float y,
                                    float width, float height,
                                    float cornerProgress, float alpha)
    {
        if (m_logoImageHandle == 0)
            m_logoImageHandle = nvgCreateImage(
                vg, BK_RES("img/pico8_logo_vector.png").c_str(), 0);
        if (m_logoImageHandle <= 0 || alpha <= 0.f)
            return;
        const float eased = livelyProgress(cornerProgress);
        Rect rect = lerpRect(centerLogoRect(x, y, width, height),
                             cornerLogoRect(x, y), eased);
        rect.y -= 20.f * std::sin(PI * clamp01(cornerProgress));
        const float rotation = -0.055f *
            std::sin(PI * clamp01(cornerProgress));
        const float centerX = rect.x + rect.width * 0.5f;
        const float centerY = rect.y + rect.height * 0.5f;
        nvgSave(vg);
        nvgGlobalAlpha(vg, clamp01(alpha));
        nvgTranslate(vg, centerX, centerY);
        nvgRotate(vg, rotation);
        nvgTranslate(vg, -centerX, -centerY);
        nvgBeginPath(vg);
        nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
        nvgFillPaint(vg, nvgImagePattern(
            vg, rect.x, rect.y, rect.width, rect.height, 0.f,
            m_logoImageHandle, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }

    void Pico8Page::_drawReturningLogo(NVGcontext* vg, float x, float y,
                                       float width, float height,
                                       float progress, float alpha)
    {
        if (m_logoImageHandle == 0)
            m_logoImageHandle = nvgCreateImage(
                vg, BK_RES("img/pico8_logo_vector.png").c_str(), 0);
        if (m_logoImageHandle <= 0 || alpha <= 0.f)
            return;
        const auto geometry = pico8_transition::geometry(x, y, width, height);
        const auto pose = pico8_transition::logoPose(
            geometry, 1.f - clamp01(progress));
        const float centerX = pose.x + pose.width * 0.5f;
        const float centerY = pose.y + pose.height * 0.5f;
        nvgSave(vg);
        nvgGlobalAlpha(vg, clamp01(alpha));
        nvgTranslate(vg, centerX, centerY);
        nvgRotate(vg, pose.rotation);
        nvgTranslate(vg, -centerX, -centerY);
        nvgBeginPath(vg);
        nvgRect(vg, pose.x, pose.y, pose.width, pose.height);
        nvgFillPaint(vg, nvgImagePattern(
            vg, pose.x, pose.y, pose.width, pose.height, 0.f,
            m_logoImageHandle, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }

    void Pico8Page::_drawList(NVGcontext* vg, float x, float y, float width,
                              float height, float progress, float alpha)
    {
        if (m_games.empty() || progress <= 0.f || alpha <= 0.f)
            return;
        const Rect logo = cornerLogoRect(x, y);
        const float listX = x + 42.f;
        const float listY = logo.y + logo.height + 22.f;
        const float listW = std::min(370.f, width * 0.34f - 50.f);
        const float listH = height - (listY - y) - 72.f;
        const float visibleHeight = std::min(
            listH, LIST_ITEM_HEIGHT * static_cast<float>(LIST_VISIBLE_ITEMS));
        const float targetScroll = std::max(0.f,
            (m_selectedIndex + 0.5f) * LIST_ITEM_HEIGHT - visibleHeight * 0.5f);
        const float maxScroll = std::max(0.f,
            static_cast<float>(m_games.size()) * LIST_ITEM_HEIGHT - visibleHeight);
        m_listScroll += (std::min(targetScroll, maxScroll) - m_listScroll) * 0.22f;

        nvgSave(vg);
        nvgIntersectScissor(vg, listX - 12.f, listY - 6.f,
                            listW + 24.f, visibleHeight + 12.f);
        const int firstVisible = std::max(0,
            static_cast<int>(std::floor(m_listScroll / LIST_ITEM_HEIGHT)));
        const float timeline = progress *
            (0.46f + (LIST_VISIBLE_ITEMS - 1) * 0.055f);
        for (size_t i = 0; i < m_games.size(); ++i) {
            const float itemY = listY + static_cast<float>(i) *
                LIST_ITEM_HEIGHT - m_listScroll;
            if (itemY + LIST_ITEM_HEIGHT < listY ||
                itemY > listY + visibleHeight)
                continue;
            const int staggerIndex = std::max(
                0, static_cast<int>(i) - firstVisible);
            const float itemProgress = easeOutBack(clamp01(
                (timeline - staggerIndex * 0.055f) / 0.46f));
            if (itemProgress <= 0.f)
                continue;
            const bool selected = static_cast<int>(i) == m_selectedIndex;
            const float itemX = listX - (1.f - itemProgress) * 42.f;
            const float itemAlpha = alpha * clamp01(itemProgress);

            nvgSave(vg);
            nvgGlobalAlpha(vg, itemAlpha);
            if (selected) {
                const NVGpaint shadow = nvgBoxGradient(
                    vg, itemX + 4.f, itemY + 6.f,
                    listW, LIST_ITEM_HEIGHT - 6.f, 8.f, 5.f,
                    nvgRGBA(0, 0, 0, 78), nvgRGBA(0, 0, 0, 0));
                nvgBeginPath(vg);
                nvgRect(vg, itemX - 3.f, itemY,
                        listW + 12.f, LIST_ITEM_HEIGHT + 8.f);
                nvgRoundedRect(vg, itemX, itemY + 2.f,
                               listW, LIST_ITEM_HEIGHT - 6.f, 7.f);
                nvgPathWinding(vg, NVG_HOLE);
                nvgFillPaint(vg, shadow);
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, itemX, itemY + 2.f,
                               listW, LIST_ITEM_HEIGHT - 6.f, 7.f);
                nvgFillPaint(vg, nvgLinearGradient(
                    vg, itemX, itemY, itemX + listW, itemY + LIST_ITEM_HEIGHT,
                    nvgRGBA(255, 0, 77, 112),
                    nvgRGBA(41, 173, 255, 76)));
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(255, 241, 232, 220));
                nvgStrokeWidth(vg, 1.5f);
                nvgStroke(vg);
            }
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, selected ? 21.f : 19.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected
                ? nvgRGBA(255, 255, 255, 246)
                : nvgRGBA(208, 213, 223, 210));
            nvgText(vg, itemX + 16.f, itemY + LIST_ITEM_HEIGHT * 0.5f - 1.f,
                    m_games[i].name.c_str(), nullptr);
            nvgRestore(vg);
        }
        nvgRestore(vg);
    }

    void Pico8Page::_drawCoverRect(NVGcontext* vg, float x, float y,
                                   float width, float height, float alpha)
    {
        _ensureCover(vg);
        nvgSave(vg);
        nvgGlobalAlpha(vg, clamp01(alpha));
        const NVGpaint shadow = nvgBoxGradient(
            vg, x + 5.f, y + 6.f, width, height, 12.f, 5.f,
            nvgRGBA(0, 0, 0, 96), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x - 3.f, y - 3.f, width + 14.f, height + 15.f);
        nvgRoundedRect(vg, x, y, width, height, 11.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, 11.f);
        if (m_coverImageHandle > 0) {
            nvgFillPaint(vg, nvgImagePattern(
                vg, x, y, width, height, 0.f, m_coverImageHandle, 1.f));
        } else {
            nvgFillColor(vg, nvgRGBA(20, 23, 31, 255));
        }
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 72));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgRestore(vg);
    }

    void Pico8Page::_drawGameRect(NVGcontext* vg, float x, float y,
                                  float width, float height, float alpha)
    {
        if (!m_video.initialize(vg))
            return;
        if (m_frameDirty) {
            m_video.upload(m_core.GetFrameBuffer());
            m_frameDirty = false;
        }
        nvgSave(vg);
        nvgGlobalAlpha(vg, clamp01(alpha));
        nvgBeginPath(vg);
        nvgRect(vg, x, y, width, height);
        nvgFillPaint(vg, nvgImagePattern(
            vg, x, y, width, height, 0.f, m_video.imageHandle(), 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }

    void Pico8Page::_drawPreview(NVGcontext* vg, float x, float y,
                                 float width, float height, float alpha,
                                 bool allowRuntimePreview)
    {
        if (m_games.empty() || alpha <= 0.f)
            return;
        const Rect target = previewRect(x, y, width, height);
        const float pop = livelyProgress(alpha);
        const float startScale = 0.78f;
        const float scale = startScale + (1.f - startScale) * pop;
        Rect rect;
        rect.width = target.width * scale;
        rect.height = target.height * scale;
        rect.x = target.x + (target.width - rect.width) * 0.5f +
            (1.f - pop) * 72.f;
        rect.y = target.y + (target.height - rect.height) * 0.5f -
            16.f * std::sin(PI * clamp01(alpha));
        if (allowRuntimePreview && _selectedIsLoadedGame())
            _drawGameRect(vg, rect.x, rect.y, rect.width, rect.height, alpha);
        else
            _drawCoverRect(vg, rect.x, rect.y, rect.width, rect.height, alpha);

        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 23.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, alphaByte(alpha * 0.94f)));
        nvgText(vg, rect.x + rect.width * 0.5f,
                rect.y + rect.height + 20.f,
                m_games[static_cast<size_t>(m_selectedIndex)].name.c_str(),
                nullptr);
    }

    void Pico8Page::_drawTransitionPreview(NVGcontext* vg, float x, float y,
                                           float width, float height,
                                           float progress, float alpha,
                                           bool useRuntimeFrame)
    {
        const Rect from = previewRect(x, y, width, height);
        const Rect to = runtimeRect(x, y, width, height);
        const float lively = livelyProgress(progress);
        Rect rect = lerpRect(from, to, lively);
        rect.y -= 22.f * std::sin(PI * clamp01(progress));
        if (useRuntimeFrame)
            _drawGameRect(vg, rect.x, rect.y, rect.width, rect.height, alpha);
        else
            _drawCoverRect(vg, rect.x, rect.y, rect.width, rect.height, alpha);
    }

    void Pico8Page::_drawLibrary(NVGcontext* vg, float x, float y,
                                 float width, float height,
                                 float logoProgress, float listProgress,
                                 float previewProgress, float alpha,
                                 bool allowRuntimePreview)
    {
        _drawHeaderLogo(vg, x, y, width, height, logoProgress, alpha);
        _drawList(vg, x, y, width, height, listProgress, alpha);
        _drawPreview(vg, x, y, width, height,
                     previewProgress * alpha, allowRuntimePreview);
        if (!m_errorText.empty()) {
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 18.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 119, 168, alphaByte(alpha)));
            nvgText(vg, x + width * 0.5f, y + height - 34.f,
                    m_errorText.c_str(), nullptr);
        }
        const float controlsAlpha = alpha * listProgress;
        _drawLibraryHints(vg, x, y, width, height, controlsAlpha);
    }

    void Pico8Page::_drawLibraryHints(NVGcontext* vg, float x, float y,
                                      float width, float height, float alpha)
    {
        struct HintItem
        {
            brls::ControllerButton button;
            const char* label;
        };
        const HintItem hints[] = {
            {brls::BUTTON_START, "关闭P8"},
            {brls::BUTTON_A, "选择游戏"},
            {brls::BUTTON_B, "返回游戏"},
        };
        const size_t count = m_core.isGameLoaded() ? 3u : 2u;
        float cursor = x + width - 35.f;
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 21.f);
        for (size_t index = count; index-- > 0;) {
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, hints[index].label, nullptr, bounds);
            const float itemWidth = 42.f + bounds[2] - bounds[0];
            cursor -= itemWidth;
            _drawHint(vg, hints[index].button, hints[index].label,
                      cursor + 14.f, y + height - 31.f, alpha);
            cursor -= 18.f;
        }
    }

    void Pico8Page::_drawEmptyState(NVGcontext* vg, float x, float y,
                                    float width, float height, float alpha)
    {
        _drawHeaderLogo(vg, x, y, width, height, 0.f, alpha);
        const Rect logo = centerLogoRect(x, y, width, height);
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 21.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(225, 229, 237, alphaByte(alpha * 0.9f)));
        nvgText(vg, x + width * 0.5f, logo.y + logo.height + 28.f,
                "没有找到 p8 游戏，请去在线下载中获取。", nullptr);
        _drawHint(vg, brls::BUTTON_START, "关闭P8",
                  x + width - 126.f, y + height - 31.f, alpha);
    }

    void Pico8Page::_drawGame(NVGcontext* vg, float x, float y,
                              float width, float height, float alpha)
    {
        const Rect rect = runtimeRect(x, y, width, height);
        _drawHeaderLogo(vg, x, y, width, height, 1.f, alpha);
        _drawGameRect(vg, rect.x, rect.y, rect.width, rect.height, alpha);
        _drawInputTrace(vg, x, y, width, height, alpha);
        _drawGameControls(vg, x + 30.f, y + height - 31.f, alpha * 0.9f);

        struct HintItem
        {
            brls::ControllerButton button;
            const char* label;
        };
        constexpr HintItem hints[] = {
            {brls::BUTTON_RT, "快速存档"},
            {brls::BUTTON_LT, "快速读档"},
            {brls::BUTTON_START, "游戏列表"},
        };
        float cursor = x + width - 35.f;
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 21.f);
        for (size_t index = 3u; index-- > 0;) {
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, hints[index].label, nullptr, bounds);
            const float itemWidth = 42.f + bounds[2] - bounds[0];
            cursor -= itemWidth;
            _drawHint(vg, hints[index].button, hints[index].label,
                      cursor + 14.f, y + height - 31.f, alpha * 0.88f);
            cursor -= 18.f;
        }
    }

    void Pico8Page::_drawGameControls(NVGcontext* vg, float x, float y,
                                      float alpha)
    {
        struct ControlGlyph
        {
            char32_t codepoint;
            const char* label;
            float gapAfter;
        };
        constexpr ControlGlyph controls[] = {
            {0xE0C0, "", 15.f},
            {0xE0D0, "", 15.f},
            {0xE0E0, "", 15.f},
            {0xE0E1, "", 0.f},
        };
        float cursor = x;
        for (const auto& control : controls) {
            const std::string glyph = encodeUtf8(control.codepoint);
            nvgFontFaceId(vg, m_switchIconFontId);
            nvgFontSize(vg, 30.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, alphaByte(alpha)));
            nvgText(vg, cursor, y, glyph.c_str(), nullptr);
            float bounds[4]{};
            nvgTextBounds(vg, cursor, y, glyph.c_str(), nullptr, bounds);
            cursor = bounds[2] + 7.f;
            if (control.label[0] != '\0') {
                nvgFontSize(vg, 18.f);
                nvgFillColor(vg, nvgRGBA(225, 229, 237,
                    alphaByte(alpha * 0.9f)));
                nvgText(vg, cursor, y, control.label, nullptr);
                nvgTextBounds(vg, cursor, y, control.label, nullptr, bounds);
                cursor = bounds[2];
            }
            cursor += control.gapAfter;
        }
    }

    void Pico8Page::_drawInputTrace(NVGcontext* vg, float x, float y,
                                    float width, float height, float alpha)
    {
        if (m_inputTrace.empty() || alpha <= 0.f)
            return;
        const Rect game = runtimeRect(x, y, width, height);
        const float traceX = game.x + game.width + 42.f;
        const float traceRight = x + width - 38.f;
        if (traceRight - traceX < 90.f)
            return;

        constexpr float rowHeight = 34.f;
        const float totalHeight = rowHeight *
            static_cast<float>(m_inputTrace.size());
        const float startY = y + (height - totalHeight) * 0.5f;
        size_t index = 0;
        for (const auto& entry : m_inputTrace) {
            const float enter = easeOutBack(entry.age / 0.16f);
            const float fade = 1.f - smoothStep(
                (entry.age - 1.15f) /
                (INPUT_TRACE_LIFETIME - 1.15f));
            const float rowAlpha = alpha * fade;
            if (rowAlpha <= 0.f) {
                ++index;
                continue;
            }
            const float rowY = startY + static_cast<float>(index) * rowHeight;
            const float rowX = traceX + (1.f - enter) * 24.f +
                smoothStep(entry.age / INPUT_TRACE_LIFETIME) * 9.f;
            const float accentWidth = 16.f + 18.f * clamp01(enter);

            nvgBeginPath(vg);
            nvgRoundedRect(vg, rowX, rowY + 13.f, accentWidth, 3.f, 1.5f);
            nvgFillColor(vg, nvgRGBA(255, 0, 77,
                alphaByte(rowAlpha * 0.9f)));
            nvgFill(vg);

            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 21.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 241, 232,
                alphaByte(rowAlpha)));
            nvgText(vg, rowX + accentWidth + 12.f, rowY + 15.f,
                    entry.label.c_str(), nullptr);
            ++index;
        }
    }

    void Pico8Page::draw(NVGcontext* vg, float x, float y,
                         float width, float height, brls::Style,
                         brls::FrameContext*)
    {
        if (!vg)
            return;
        _ensureFonts(vg);

        float backgroundAlpha = 1.f;
        if (m_state == State::Exiting && m_homeReturnStarted) {
            const float uiDuration = m_exitHasLibrary
                ? EXIT_LIBRARY_DURATION : 0.f;
            backgroundAlpha = 1.f - smoothStep(
                (m_stateTime - uiDuration) /
                pico8_transition::TRANSITION_DURATION);
        }
        nvgBeginPath(vg);
        nvgRect(vg, x, y, width, height);
        nvgFillColor(vg, nvgRGBA(7, 9, 14, alphaByte(backgroundAlpha)));
        nvgFill(vg);

        switch (m_state) {
            case State::Waiting:
                _drawHeaderLogo(vg, x, y, width, height, 0.f, 1.f);
                break;
            case State::Entering: {
                const float logo = clamp01(m_stateTime / LOGO_MOVE_DURATION);
                const float list = smoothStep(
                    (m_stateTime - LIST_ENTER_DELAY) / LIST_ENTER_DURATION);
                const float preview = smoothStep(
                    (m_stateTime - PREVIEW_ENTER_DELAY) /
                    PREVIEW_ENTER_DURATION);
                _drawLibrary(vg, x, y, width, height,
                             logo, list, preview, 1.f, false);
                break;
            }
            case State::Empty:
                _drawEmptyState(vg, x, y, width, height, 1.f);
                break;
            case State::Library:
                _drawLibrary(vg, x, y, width, height,
                             1.f, 1.f, 1.f, 1.f, false);
                break;
            case State::PausedLibrary:
                _drawLibrary(vg, x, y, width, height,
                             1.f, 1.f, 1.f, 1.f, true);
                break;
            case State::Launching: {
                const float list = 1.f - smoothStep(
                    m_stateTime / LAUNCH_LIST_DURATION);
                const float move = clamp01(
                    (m_stateTime - LAUNCH_MOVE_DELAY) / LAUNCH_MOVE_DURATION);
                _drawHeaderLogo(vg, x, y, width, height, 1.f,
                                1.f);
                _drawList(vg, x, y, width, height, list, 1.f);
                _drawTransitionPreview(vg, x, y, width, height,
                                       move, 1.f, m_launchUsesRuntime);
                break;
            }
            case State::Running: {
                const float gameAlpha = m_launchUsesRuntime
                    ? 1.f : smoothStep(m_stateTime / GAME_FADE_DURATION);
                _drawGame(vg, x, y, width, height, gameAlpha);
                if (!m_launchUsesRuntime && gameAlpha < 1.f)
                    _drawTransitionPreview(vg, x, y, width, height,
                                           1.f, 1.f - gameAlpha, false);
                break;
            }
            case State::Pausing: {
                const float move = livelyProgress(m_stateTime / 0.42f);
                const float list = smoothStep(
                    (m_stateTime - 0.13f) / 0.38f);
                const float logo = smoothStep(
                    (m_stateTime - 0.08f) / 0.30f);
                const Rect from = runtimeRect(x, y, width, height);
                const Rect to = previewRect(x, y, width, height);
                Rect rect = lerpRect(from, to, move);
                rect.y -= 20.f * std::sin(PI * clamp01(m_stateTime / 0.42f));
                _drawGameRect(vg, rect.x, rect.y,
                              rect.width, rect.height, 1.f);
                _drawHeaderLogo(vg, x, y, width, height, 1.f, logo);
                _drawList(vg, x, y, width, height, list, 1.f);
                if (list > 0.7f) {
                    _drawLibraryHints(vg, x, y, width, height, list);
                }
                break;
            }
            case State::Exiting: {
                const float uiDuration = m_exitHasLibrary
                    ? EXIT_LIBRARY_DURATION : 0.f;
                if (m_exitHasLibrary && m_stateTime < uiDuration) {
                    const float list = 1.f - smoothStep(m_stateTime / 0.36f);
                    const float preview = 1.f - smoothStep(m_stateTime / 0.30f);
                    const float logoBack = smoothStep(
                        (m_stateTime - 0.28f) / 0.40f);
                    _drawHeaderLogo(vg, x, y, width, height,
                                    1.f - logoBack, 1.f);
                    _drawList(vg, x, y, width, height, list, 1.f);
                    _drawPreview(vg, x, y, width, height, preview,
                                 m_exitUsesRuntimePreview);
                } else {
                    const float progress = clamp01(
                        (m_stateTime - uiDuration) /
                        pico8_transition::TRANSITION_DURATION);
                    _drawReturningLogo(vg, x, y, width, height,
                                       progress, 1.f);
                }
                break;
            }
        }
    }
}
