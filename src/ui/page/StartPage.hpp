#pragma once 

#include <atomic>
#include "core/common.h"
#include "ui/view/SwitchLayout.hpp"
#include "ui/page/FileListPage.hpp" 
#include "ui/page/GamePage.hpp"
#include "ui/page/SettingPage.hpp"
#include "ui/page/AboutPage.hpp"
#include "ui/page/GameLibraryPage.hpp"
#include "ui/page/DataManagementPage.hpp"
#include "ui/page/MultiplayerPage.hpp"
#include "ui/widget/Box.hpp"
#include "ui/view/GameOptionsSidebar.hpp"

namespace beiklive
{
    class StartPage : public beiklive::Box
    {
    public:
        StartPage();
        ~StartPage();

        void Init();
        void onResume();
        void willAppear(bool resetState) override;

    private:
        void _useSwitchLayout();
        void _openGameLibrary();
        void _openFileList();
        void _openSettings();
        void _openAbout();
        void _openDataManagement();
        void _openMultiplayer();
        void _applyRuntimeUiSettings();
        void _requestRecentGamesRefresh(bool defer);
        void _pushGameActivity(const beiklive::GameEntry& entry, beiklive::Box* previousPage);
        void _pushGameActivity(const beiklive::DirListData& dirItem, beiklive::Box* previousPage);

        /// 显示游戏选项侧边栏
        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        /// 关闭游戏选项侧边栏
        void _hideGameOptionsPanel();

        beiklive::FileListPage* m_fileListPage = nullptr;
        beiklive::SwitchLayout* switchLayout = nullptr;
        beiklive::Box* m_gamePage = nullptr;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;
        std::atomic<bool> m_alive{true};
        std::atomic<int> m_recentRefreshGen{0};
        bool m_resetCardFocusOnNextRefresh = false;
    };
} // namespace beiklive


