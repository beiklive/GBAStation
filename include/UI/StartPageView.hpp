#pragma once

#include <borealis.hpp>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"
#include "UI/Pages/AppPage.hpp"
#include "UI/Pages/FileListPage.hpp"

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
//  StartPageView
// ─────────────────────────────────────────────────────────────────────────────
class StartPageView : public brls::Box
{
  public:
    StartPageView();
    ~StartPageView();
    void ActionInit();
    void Init();
    void onFocusGained() override;
    void onFocusLost() override;
    // void onLayout() override;

    static brls::View* create();

  private:
    brls::Image* m_bgImage = nullptr;

    AppPage*           m_appPage       = nullptr;
    FileListPage*      m_fileListPage  = nullptr;
    FileSettingsPanel* m_settingsPanel = nullptr;

    void showAppPage();
    void openFileListPage();
    void createAppPage();
    void createFileListPage();

    /// Called by the FileListPage callback when BUTTON_X is pressed.
    void onFileSettingsRequested(const FileListItem& item, int itemIndex);
};

