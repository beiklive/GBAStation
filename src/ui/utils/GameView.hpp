#pragma once

#include "core/common.h"
#include "game/control/InputMap.hpp"

namespace beiklive
{
    // 游戏视图，负责游戏的渲染显示，输入处理等功能
    class GameView : public brls::Box
    {
        public:
            GameView(beiklive::GameEntry gameData);
            ~GameView();

            // 数据库操作


        };
    }