#pragma once 

#include "core/common.h"
#include "ui/layout/SwitchLayout.hpp"
#include "ui/page/FileListPage.hpp" 
#include "ui/page/GamePage.hpp"
#include "ui/page/SettingPage.hpp"
#include "ui/page/AboutPage.hpp"
#include "ui/utils/Box.hpp"

namespace beiklive
{
    class StartPage : public beiklive::Box
    {
    public:
        StartPage();
        ~StartPage();

        void Init();
        void onResume();

    private:
        void _useSwitchLayout();
        void _openFileList();
        void _openSettings();
        void _openAbout();

        beiklive::FileListPage* m_fileListPage = nullptr;
        beiklive::SwitchLayout* switchLayout = nullptr;
        beiklive::GamePage* m_gamePage = nullptr;
    };
} // namespace beiklive

