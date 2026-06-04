#pragma once
#include "core/common.h"
#include <functional>

#include "ui/utils/GameView.hpp"
#include "ui/utils/GameMenuView.hpp"
#include "ui/utils/RewindSelectorView.hpp"
#include "ui/utils/FlashGameView.hpp"
#include "ui/utils/FlashGameMenuView.hpp"

namespace beiklive
{
    /*
        游戏页面, 负责游戏的启动、初始化、调用渲染器等功能。
    */
    class GamePage : public brls::Box
    {
    public:
        GamePage(beiklive::DirListData gameData);
        GamePage(beiklive::GameEntry gameEntry);
        ~GamePage();

    private:
        void PageInit();
        void GameEntryInitialize();
        void _initGameEntryPaths();  // 初始化 GameEntry 中的各路径字段
        void _tryUpdateLogoFromThumbnail(); // 尝试将默认图标封面替换为即时存档截图
        void updateGameCount();
        void GameViewInitialize();
        void GameMenuInitialize();
        void RewindSelectorViewInitialize(); // 初始化倒带选择界面

        void _setupGame();

        void _setupFlashGame();                                    // Flash 游戏初始化

        beiklive::DirListData m_gameData;
        beiklive::GameEntry m_gameEntry;                          // 游戏条目数据，包含路径、标题等信息
        GameView *m_gameView                   = nullptr;         // 游戏视图实例，负责游戏的渲染显示和输入处理
        GameMenuView *m_gameMenuView           = nullptr;         // 游戏菜单视图实例，负责游戏菜单的渲染显示和输入处理
        RewindSelectorView *m_rewindSelectorView = nullptr;       // 可视化倒带选择界面（显示倒带缩略图列表）

        // Flash 视图
        beiklive::flash::FlashGameView*     m_flashGameView = nullptr;
        beiklive::flash::FlashGameMenuView* m_flashMenuView = nullptr;
    };

}