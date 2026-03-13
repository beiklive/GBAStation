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
//  FileSettingsPanel – absolute-positioned overlay for file operations
// ─────────────────────────────────────────────────────────────────────────────
class FileSettingsPanel : public brls::Box
{
  public:
    FileSettingsPanel();

    /// Show the panel for a specific item.
    /// @param item        The file/dir the operations act on.
    /// @param itemIndex   Index in the FileListPage data source.
    /// @param page        Pointer to the owning FileListPage (for callbacks).
    void showForItem(const FileListItem& item, int itemIndex, FileListPage* page);

    /// Hide the panel and restore focus to the file list.
    void close();

  private:
    brls::Box*   m_titleBar    = nullptr;
    brls::Label* m_titleLabel  = nullptr;
    brls::Box*   m_optionsBox  = nullptr;
    brls::Box*   m_bottomHints = nullptr;

    FileListPage* m_fileListPage   = nullptr;
    int           m_currentItemIdx = -1;

    void addOptionButton(const std::string& label, std::function<void()> action);
    void clearOptions();
};

// ─────────────────────────────────────────────────────────────────────────────
//  AppGameSettingsPanel – absolute-positioned overlay for AppPage game cards
// ─────────────────────────────────────────────────────────────────────────────
class AppGameSettingsPanel : public brls::Box
{
  public:
    AppGameSettingsPanel();

    /// Show the panel for a specific game entry.
    /// @param entry  The game card's entry data.
    /// @param page   Pointer to the owning AppPage (for remove/update callbacks).
    void showForEntry(const GameEntry& entry, AppPage* page);

    /// Hide the panel and restore focus to the AppPage.
    void close();

  private:
    brls::Box*   m_titleBar   = nullptr;
    brls::Label* m_titleLabel = nullptr;
    brls::Box*   m_optionsBox = nullptr;

    AppPage*   m_appPage = nullptr;
    GameEntry  m_entry;

    void addOptionButton(const std::string& label, std::function<void()> action);
    void clearOptions();
};

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

    static brls::View* create();

  private:

    AppPage*             m_appPage            = nullptr;
    AppGameSettingsPanel* m_appSettingsPanel  = nullptr;

    void showAppPage();
    void openFileListPage();
    void openSettingsPage();
    void createAppPage();
    /// Re-read recent games from SettingManager and update the AppPage game list.
    void refreshRecentGames();
};

