#pragma once
#include "core/common.h"
#include <functional>

#include "ui/view/GameViewBase.hpp"
#include "ui/view/GameView.hpp"
#include "ui/view/MgbaGameView.hpp"
#include "ui/view/GameMenuView.hpp"
#include "ui/view/RewindSelectorView.hpp"

namespace beiklive
{
    /*
        游戏页面, 负责游戏的启动、初始化、调用渲染器等功能。
    */
    class GamePage : public beiklive::Box
    {
    public:
        GamePage(beiklive::DirListData gameData, bool exitToApplication = false);
        GamePage(beiklive::GameEntry gameEntry, bool exitToApplication = false);
        ~GamePage();

        void startGame();

    private:
        void PageInit();
        void GameEntryInitialize();
        void _initGameEntryPaths();  // 初始化 GameEntry 中的各路径字段
        void _tryUpdateLogoFromThumbnail(); // 尝试将默认图标封面替换为即时存档截图
        void updateGameCount();
        void GameViewInitialize();
        void GameMenuInitialize();
        void RewindSelectorViewInitialize(); // 初始化倒带选择界面
        void _finishExitAndPop();
        void _waitExitAutoSaveThenPop();
        void _showExitCleanupDialogThenPop();

        void _setupGame();
        bool _prepareArchiveGame();


        beiklive::DirListData m_gameData;
        beiklive::GameEntry m_gameEntry;                          // 游戏条目数据，包含路径、标题等信息
        beiklive::GameEntry m_runtimeGameEntry;                   // 传给核心的实际 ROM 路径（压缩包会是 temp.xxx）
        std::string m_archiveTempPath;
        bool m_archivePrepared = false;
        GameViewBase *m_gameView               = nullptr;         // 游戏视图实例，负责游戏的渲染显示和输入处理
        GameMenuView *m_gameMenuView           = nullptr;         // 游戏菜单视图实例，负责游戏菜单的渲染显示和输入处理
        RewindSelectorView *m_rewindSelectorView = nullptr;       // 可视化倒带选择界面（显示倒带缩略图列表）
        bool m_exitRequested = false;                            // 防止退出流程重复触发
        bool m_exitCleanupStarted = false;                        // 防止退出清理重复执行
        bool m_exitToApplication = false;                         // 直接启动时退出游戏即关闭应用
        int m_exitAutoSavePolls = 0;                              // 退出自动存档完成状态轮询次数
    };

}
