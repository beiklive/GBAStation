#pragma once

#include "core/common.h"
#include "game/control/GameInputManager.hpp"
namespace beiklive
{
    // 游戏视图，负责游戏的渲染显示，输入处理等功能
    class GameView : public brls::Box
    {
        public:
            GameView(beiklive::GameEntry gameData);
            ~GameView();


            void onFocusGained() override;
            void onFocusLost() override;

            void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;


        private:
            bool _brls_inputLocked = false; // 输入锁定状态，防止在游戏视图失去焦点时继续处理输入

            void handleInput(); // 处理输入的函数，可以根据需要添加参数

        };
}