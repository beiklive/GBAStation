#pragma once

#include <borealis.hpp>

#include "common.hpp"
#include "XMLUI/Pages/AppPage.hpp"
#include "XMLUI/Pages/FileListPage.hpp"

class StartPageView : public brls::Box
{
  public:
    StartPageView();
    ~StartPageView();
    void Init();
    void onFocusGained() override;
    void onFocusLost() override;
    void onLayout() override;

    static brls::View* create();

  private:
    brls::Image* m_bgImage = nullptr;

    AppPage*      m_appPage      = nullptr;
    FileListPage* m_fileListPage = nullptr;
    int           m_activeIndex  = 0; ///< 0 = AppPage, 1 = FileListPage

    void showAppPage();
    void showFileListPage();
    void createAppPage();
    void createFileListPage();
};

