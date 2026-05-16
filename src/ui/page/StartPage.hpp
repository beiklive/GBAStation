#pragma once 

#include <atomic>
#include "core/common.h"
#include "ui/layout/SwitchLayout.hpp"
#include "ui/page/FileListPage.hpp" 
#include "ui/page/GamePage.hpp"
#include "ui/page/SettingPage.hpp"
#include "ui/page/AboutPage.hpp"
#include "ui/page/GameLibraryPage.hpp"
#include "ui/page/DataManagementPage.hpp"
#include "ui/utils/Box.hpp"
#include "ui/utils/GameOptionsSidebar.hpp"

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
        void _openGameLibrary();
        void _openFileList();
        void _openSettings();
        void _openAbout();
        void _openDataManagement();

        /// 显示游戏选项侧边栏
        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        /// 关闭游戏选项侧边栏
        void _hideGameOptionsPanel();

        beiklive::FileListPage* m_fileListPage = nullptr;
        beiklive::SwitchLayout* switchLayout = nullptr;
        beiklive::GamePage* m_gamePage = nullptr;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;
        std::atomic<bool> m_alive{true};
    };
} // namespace beiklive


