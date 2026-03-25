#pragma once

#include <borealis.hpp>
#include "core/common.h"
#include "Layout.hpp"

// switch 风格布局，模仿 Switch 主界面设计，顶部为横向游戏列表，底部为功能区域

namespace beiklive
{
    class SwitchLayout : public beiklive::Layout
    {
    public:
        SwitchLayout();
        ~SwitchLayout();

        void refreshGameList(const GameList& gameList) override;

    };
} // namespace beiklive