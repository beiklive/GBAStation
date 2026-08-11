#pragma once

#include "core/pico8/Pico8Core.hpp"
#include "core/pico8/Pico8Filesystem.hpp"
#include "core/pico8/Pico8Input.hpp"
#include "core/pico8/Pico8Video.hpp"

#include <chrono>
#include <array>
#include <deque>
#include <future>
#include <string>
#include <vector>

#include <borealis.hpp>

namespace beiklive
{
    class SwitchLayout;
    class IisuLayout;

    class Pico8Page : public brls::Box
    {
    public:
        explicit Pico8Page(SwitchLayout* homeLayout);
        explicit Pico8Page(IisuLayout* homeLayout);
        ~Pico8Page() override;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;
        bool isTranslucent() override { return true; }
        brls::View* getDefaultFocus() override { return this; }

    private:
        enum class State
        {
            Waiting,
            Entering,
            Empty,
            Library,
            Launching,
            Running,
            Pausing,
            PausedLibrary,
            Exiting,
        };

        enum class MenuScreen
        {
            Main,
            Settings,
        };

        enum class VideoFilter
        {
            None,
            Dot,
            Crt,
        };

        void _beginLaunch();
        void _openMenu();
        void _closeMenu();
        void _handleMenuInput(float dt);
        void _activateMenuItem();
        void _adjustMenuSetting(int direction);
        void _restartGame();
        void _returnToGameList();
        void _applyRuntimeSettings();
        void _beginClose();
        void _returnToRunningGame();
        bool _quickSave();
        bool _quickLoad();
        bool _writeQuickState(const std::string& gamePath,
                              const std::vector<uint8_t>& state);
        bool _readQuickState(const std::string& gamePath,
                             std::vector<uint8_t>& state);
        void _pollBackgroundTasks();
        void _handleLibraryInput(float dt);
        void _handleEmptyInput();
        void _captureInputState();
        void _selectRelative(int direction);
        bool _selectedIsLoadedGame() const;
        void _releaseCover(NVGcontext* vg);
        void _ensureCover(NVGcontext* vg);
        void _ensureFonts(NVGcontext* vg);
        void _finishHomeReturn();
        void _captureTraceInput();
        void _updateInputTrace(float dt);

        void _drawLibrary(NVGcontext* vg, float x, float y, float width,
                          float height, float logoProgress,
                          float listProgress, float previewProgress,
                          float alpha, bool allowRuntimePreview);
        void _drawList(NVGcontext* vg, float x, float y, float width,
                       float height, float progress, float alpha);
        void _drawPreview(NVGcontext* vg, float x, float y, float width,
                          float height, float alpha, bool allowRuntimePreview);
        void _drawTransitionPreview(NVGcontext* vg, float x, float y,
                                    float width, float height, float progress,
                                    float alpha, bool useRuntimeFrame);
        void _drawGame(NVGcontext* vg, float x, float y, float width,
                       float height, float alpha);
        void _drawGameRect(NVGcontext* vg, float x, float y, float width,
                           float height, float alpha);
        void _drawVideoFilter(NVGcontext* vg, float x, float y, float width,
                              float height, float alpha);
        void _drawPauseMenu(NVGcontext* vg, float x, float y, float width,
                            float height, float alpha);
        void _drawCoverRect(NVGcontext* vg, float x, float y, float width,
                            float height, float alpha);
        void _drawHeaderLogo(NVGcontext* vg, float x, float y, float width,
                             float height, float cornerProgress, float alpha);
        void _drawReturningLogo(NVGcontext* vg, float x, float y, float width,
                                float height, float progress, float alpha);
        void _drawEmptyState(NVGcontext* vg, float x, float y, float width,
                             float height, float alpha);
        void _drawGameControls(NVGcontext* vg, float x, float y, float width,
                               float height, float alpha);
        void _drawInputTrace(NVGcontext* vg, float x, float y,
                             float width, float height, float alpha);
        void _drawLibraryHints(NVGcontext* vg, float x, float y,
                               float width, float height, float alpha);
        void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                       const std::string& label, float x, float y, float alpha);

        SwitchLayout* m_homeLayout = nullptr;
        IisuLayout* m_iisuHomeLayout = nullptr;
        pico8::Core m_core;
        pico8::Input m_input;
        pico8::Video m_video;
        std::future<std::vector<pico8::GameEntry>> m_scanFuture;
        std::future<bool> m_coreFuture;
        std::vector<pico8::GameEntry> m_games;
        std::vector<uint8_t> m_quickState;
        std::string m_quickStateGamePath;

        struct InputTraceEntry
        {
            std::string label;
            float age = 0.f;
        };
        std::deque<InputTraceEntry> m_inputTrace;
        std::array<bool, static_cast<size_t>(brls::_BUTTON_MAX)>
            m_tracePreviousButtons{};

        State m_state = State::Waiting;
        int m_selectedIndex = 0;
        float m_stateTime = 0.f;
        float m_listScroll = 0.f;
        float m_selectionIdle = 0.f;
        float m_navHold = 0.f;
        float m_navRepeat = 0.f;
        int m_navDirection = 0;
        bool m_navAwaitRelease = false;
        bool m_scanFinished = false;
        bool m_coreReady = false;
        bool m_coreFinished = false;
        bool m_loadAttempted = false;
        bool m_launchUsesRuntime = false;
        bool m_frameDirty = false;
        bool m_popScheduled = false;
        bool m_homeReturnStarted = false;
        bool m_exitHasLibrary = false;
        bool m_exitUsesRuntimePreview = false;
        bool m_returnToGamePending = false;
        float m_returnToGameTime = 0.f;
        bool m_menuOpen = false;
        MenuScreen m_menuScreen = MenuScreen::Main;
        int m_menuSelectedIndex = 0;
        float m_menuTime = 0.f;
        bool m_returningToGameList = false;
        VideoFilter m_videoFilter = VideoFilter::None;
        int m_sfxVolume = 5;
        int m_musicVolume = 5;
        bool m_invertButtons = false;
        bool m_prevUp = false;
        bool m_prevDown = false;
        bool m_prevLeft = false;
        bool m_prevRight = false;
        bool m_prevA = false;
        bool m_prevB = false;
        bool m_prevStart = false;
        int m_logoImageHandle = 0;
        int m_coverImageHandle = 0;
        int m_fontId = -1;
        int m_switchIconFontId = -1;
        bool m_fontAttempted = false;
        std::string m_loadedGamePath;
        std::string m_coverImagePath;
        std::string m_coverResolvedCartPath;
        std::string m_errorText;
        std::chrono::steady_clock::time_point m_lastFrameTime;
    };
}
