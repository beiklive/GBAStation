#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "core/GameTimer.hpp"
#include "game/control/GameInputManager.hpp"
#include "game/melonds/CoreMelonDS.hpp"
#include "game/render/GameRenderer.hpp"
#include "ui/utils/GameOverlayRenderer.hpp"

#include <chrono>
#include <vector>

namespace beiklive
{
    class NdsGameMenuView;

    class NdsGameView : public brls::Box
    {
    public:
        NdsGameView(beiklive::GameEntry gameData);
        ~NdsGameView();

        void onFocusGained() override;
        void onFocusLost() override;

        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

        void setGameMenuView(NdsGameMenuView* menuView) { m_gameMenuView = menuView; }

    private:
        static constexpr unsigned kScreenW = 256;
        static constexpr unsigned kScreenH = 192;

        bool _brls_inputLocked = false;
        beiklive::GameEntry m_gameEntry;

        float m_ffMultiplier = 4.0f;
        float m_ffSlowAccum  = 0.0f;
        bool  m_ffMute       = true;

        beiklive::melonds::CoreMelonDS* m_nds_core = nullptr;

        beiklive::GameRenderer m_renderer;
        bool m_rendererReady = false;

        beiklive::ScreenMode m_screenMode = beiklive::ScreenMode::Fit;

        std::vector<int16_t> m_audioDrainBuf;

        float    m_currentFps    = 0.0f;
        unsigned m_fpsFrameCount = 0;
        std::chrono::steady_clock::time_point m_fpsLastTime;

        NdsGameMenuView* m_gameMenuView = nullptr;

        void _registerGameInput();
        void _registerGameRuntime();

        void _stepEmulation();
        void _renderOutput();

        void _drawOverlays(NVGcontext* vg, float x, float y, float w, float h);

        void _pushFrameAudio(bool ff, unsigned framesRan);

        void _updateFpsStats(unsigned framesRan);
    };

} // namespace beiklive
