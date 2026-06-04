#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "game/control/GameInputManager.hpp"
#include "game/flash/FlashGameRun.hpp"
#include "ui/utils/MouseCursor.hpp"

#include <atomic>
#include <chrono>
#include <mutex>

namespace beiklive::flash {

class FlashGameMenuView;

class FlashGameView : public brls::Box {
public:
    FlashGameView(beiklive::GameEntry gameData);
    ~FlashGameView();

    void onFocusGained() override;
    void onFocusLost() override;
    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

    void setFlashMenuView(FlashGameMenuView* menuView) { m_menuView = menuView; }
    void _onKeymapChanged() {}

private:
    bool _brls_inputLocked = false;
    beiklive::GameEntry m_gameEntry;

    FlashGameRun* m_flashCore = nullptr;
    bool m_ready = false;

    FlashGameMenuView* m_menuView = nullptr;

    float m_ffMultiplier = 4.0f;
    bool  m_ffMute = true;

    uint64_t m_lastTickTime = 0;

    std::chrono::steady_clock::time_point m_playStartTime;
    std::string m_playTimeTempPath;
    mutable std::mutex m_fpsMutex;
    float m_currentFps = 0.f;
    unsigned m_fpsFrameCount = 0;
    std::chrono::steady_clock::time_point m_fpsLastTime;
    static constexpr double FPS_UPDATE_INTERVAL = 1.0;

    void _registerFlashInput();
    void _registerGameRuntime();

    void _handleInputs();
    void _renderAndOverlay(NVGcontext* vg, float x, float y, float w, float h);

    void _drawMouseCursor(NVGcontext* vg, float x, float y, float w, float h);
    void _drawOverlays(NVGcontext* vg, float x, float y, float w, float h);
    void _updateFpsStats();

    void _initPlayTimeTracking();
    void _saveAndCommitPlayTime();
    void _savePlayTimeCheckpoint();
};

} // namespace beiklive::flash
