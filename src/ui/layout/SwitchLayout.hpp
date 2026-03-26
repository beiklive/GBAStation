#pragma once

#include <borealis.hpp>
#include "core/common.h"
#include "Layout.hpp"
#include "ui/utils/GameCard.hpp"
#include "ui/utils/RoundButton.hpp"

// switch 风格布局，模仿 Switch 主界面设计，顶部为横向游戏列表，底部为功能区域

namespace beiklive
{
    class SwitchLayout : public beiklive::Layout
    {
    public:
        SwitchLayout();
        ~SwitchLayout() = default;

        void refreshGameList(beiklive::GameList* gameList) override;

        void buildCardRow(beiklive::GameList* gameList);
        void buildFunctionArea(); // 构建功能区域，包含游戏库、设置等功能入口
    private:
        brls::HScrollingFrame* m_frame;
        brls::Box* m_cardRow;

        brls::Box* m_functionArea; // 功能区域，包含游戏库、设置等功能入口


    };
} // namespace beiklive