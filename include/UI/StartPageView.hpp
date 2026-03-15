#pragma once

#include <borealis.hpp>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"
#include "UI/Pages/AppPage.hpp"
#include "UI/Pages/FileListPage.hpp"
#include "UI/Pages/SettingPage.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  StartPageView
// ─────────────────────────────────────────────────────────────────────────────
class StartPageView : public beiklive::UI::BBox
{
  public:
    StartPageView();
    ~StartPageView();
    void ActionInit();
    void Init();
    void onFocusGained() override;
    void onFocusLost() override;
    void draw(NVGcontext* vg, float x, float y, float width, float height,
              brls::Style style, brls::FrameContext* ctx) override;

    /// Always return AppPage's first focusable card so borealis restores
    /// focus correctly after a child activity (game / file list) is popped.
    brls::View* getDefaultFocus() override;

    static brls::View* create();

  private:

    AppPage*             m_appPage            = nullptr;
    void showAppPage();
    void openFileListPage();
    void openSettingsPage();
    void createAppPage();
    /// Re-read recent games from SettingManager and update the AppPage game list.
    void refreshRecentGames();
};

