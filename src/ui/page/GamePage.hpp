#pragma once
#include "core/common.h"
#include <functional>

#include "ui/utils/GameView.hpp"
#include "ui/utils/GameMenuView.hpp"

namespace beiklive
{
    /*
        游戏页面, 负责游戏的启动、初始化、调用渲染器等功能。
    */
    class GamePage : public brls::Box
    {
    public:
    GamePage(beiklive::DirListData gameData);
    ~GamePage();
    
    void PageInit();
    void GameViewInitialize();
    void GameMenuInitialize();
    
    private:
        beiklive::DirListData m_gameData;
        GameView* m_gameView = nullptr; // 游戏视图实例，负责游戏的渲染显示和输入处理
        GameMenuView* m_gameMenuView = nullptr; // 游戏菜单视图实例，负责游戏菜单的渲染显示和输入处理
    };


}