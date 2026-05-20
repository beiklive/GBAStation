#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "core/GameTimer.hpp"
#include "game/control/GameInputManager.hpp"
#include "game/core/IEmulatorCore.hpp"
#include "game/melonds/CoreMelonDS.hpp"
#include "game/render/GameRenderer.hpp"
#include "ui/utils/GameOverlayRenderer.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
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
        static constexpr double FPS_UPDATE_INTERVAL = 1.0;
        static constexpr unsigned kScreenW = 256;
        static constexpr unsigned kScreenH = 192;

        bool _brls_inputLocked = false;
        beiklive::GameEntry m_gameEntry;

        float m_ffMultiplier = 4.0f;
        float m_ffSlowAccum  = 0.0f;
        bool  m_ffMute       = true;

        beiklive::core::IEmulatorCore* m_core = nullptr;
        beiklive::melonds::CoreMelonDS* m_nds_core = nullptr;

        beiklive::GameRenderer m_renderer;
        bool m_rendererReady = false;

        beiklive::ScreenMode m_screenMode = beiklive::ScreenMode::Fit;

        mutable std::mutex m_frameMutex;
        LibretroLoader::VideoFrame m_pendingFrame;
        bool m_frameReady = false;

        std::vector<int16_t> m_audioDrainBuf;

        std::thread       m_gameThread;
        std::atomic<bool> m_running{false};

        mutable std::mutex m_fpsMutex;
        unsigned m_fpsFrameCount = 0;
        float    m_currentFps    = 0.0f;
        std::chrono::steady_clock::time_point m_fpsLastTime;

        NdsGameMenuView* m_gameMenuView = nullptr;

        void _registerGameInput();
        void _registerGameRuntime();

        void _startGameThread();
        void _stopGameThread();
        void _gameLoop();

        void _uploadPendingFrame();

        void _drawOverlays(NVGcontext* vg, float x, float y, float w, float h);

        void _captureVideoFrame();
        void _pushFrameAudio(bool ff, unsigned framesRan);
        void _updateFpsStats(unsigned framesRan,
                             std::chrono::steady_clock::time_point& lastTime,
                             unsigned& counter);
        void _throttleFrameRate(bool ff,
                                std::chrono::steady_clock::time_point& nextTarget,
                                std::chrono::nanoseconds frameDurNs,
                                std::chrono::nanoseconds spinGuardNs);
    };

} // namespace beiklive
