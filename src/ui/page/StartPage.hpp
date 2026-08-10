#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>
#include "core/common.h"
#include "ui/view/SwitchLayout.hpp"
#include "ui/view/iisu/IisuLayout.hpp"
#include "ui/page/FileListPage.hpp"
#include "ui/page/GamePage.hpp"
#include "ui/page/SettingPage.hpp"
#include "ui/page/AboutPage.hpp"
#include "ui/page/GameLibraryPage.hpp"
#include "ui/page/Pico8Page.hpp"
#include "ui/page/DataManagementPage.hpp"
#include "ui/widget/Box.hpp"
#include "ui/view/GameOptionsSidebar.hpp"

namespace beiklive
{
    class HomeShortcutSettingsOverlay;
    class PlatformPickerOverlay;


    class StartPage : public beiklive::Box
    {
    public:
        StartPage();
        ~StartPage();

        void Init();
        void onResume();
        void onActivityResume() override;
        void willAppear(bool resetState) override;

    private:
        void _useSwitchLayout();
        void _useIisuLayout();
        void _openGameLibrary();
        void _openFileList();
        void _openSettings();
        void _openAbout();
        void _openDataManagement();
        void _openPico8Page();
        void _showShortcutSettings();
        void _applyRuntimeUiSettings();
        void _requestRecentGamesRefresh(bool defer);
        bool _pushGameActivity(const beiklive::GameEntry& entry,
                               beiklive::Box* previousPage);
        void _pushGameActivity(const beiklive::DirListData& dirItem, beiklive::Box* previousPage);
        // 按机种启动文件列表条目（无歧义判定，供弹窗选择后调用）。
        void _launchDirItem(const beiklive::DirListData& dirItem, beiklive::Box* previousPage);
        // 歧义后缀弹窗：列出候选机种供选择。
        void _showPlatformPicker(const beiklive::DirListData& dirItem,
                                 beiklive::Box* previousPage,
                                 const std::vector<int>& candidates,
                                 int defaultIndex);


        /// 显示游戏选项侧边栏
        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        /// 关闭游戏选项侧边栏
        void _hideGameOptionsPanel();
        void _closeGameOptionsPanelAnimated(std::function<void()> completion = {},
                                            bool launchTransition = false);

        beiklive::FileListPage* m_fileListPage = nullptr;
        beiklive::SwitchLayout* switchLayout = nullptr;
        beiklive::IisuLayout* iisuLayout = nullptr;
        beiklive::HomeShortcutSettingsOverlay* m_shortcutSettingsOverlay = nullptr;
        beiklive::PlatformPickerOverlay* m_platformPicker = nullptr;

        beiklive::Box* m_gamePage = nullptr;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;
        std::atomic<bool> m_alive{true};
        std::shared_ptr<std::atomic<bool>> m_aliveToken =
            std::make_shared<std::atomic<bool>>(true);
        std::atomic<int> m_recentRefreshGen{0};
        bool m_resetCardFocusOnNextRefresh = false;
        bool m_gameLaunchPending = false;
        bool m_homeDeletePending = false;
        beiklive::GameLibraryPage::PreparedData m_libraryPreparedData;
    };
} // namespace beiklive


