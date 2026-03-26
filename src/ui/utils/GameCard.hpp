#pragma once

#include <borealis.hpp>
#include <functional>

#include "core/common.h"

namespace beiklive
{
    class GameCard : public brls::Box
    {
    private:
        /* data */
        beiklive::enums::ThemeLayout m_layoutType = beiklive::enums::ThemeLayout::DEFAULT_THEME;
        beiklive::GameEntry m_gameEntry;

        brls::Label* m_titleLabel = nullptr;
        brls::Image* m_coverImage = nullptr;

    float m_scale   = 1.0f;   ///< 当前渲染缩放比，由 draw() 平滑插值
    bool  m_clickAnimating = false;
    float m_clickT         = 0.0f;
    float m_clickScale     = 1.0f;

    void triggerClickBounce();


        void _switchCardLayout();


    public:
        GameCard(beiklive::enums::ThemeLayout type, beiklive::GameEntry gameEntry);
        ~GameCard();
        void applyThemeLayout();

        void setLayoutType(beiklive::enums::ThemeLayout type) { m_layoutType = type; }
        beiklive::enums::ThemeLayout getLayoutType() const { return m_layoutType; }

        void onChildFocusGained(brls::View* directChild, brls::View* focusedView) override;
        void onChildFocusLost(brls::View* directChild, brls::View* focusedView) override;

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;


        std::function<void(beiklive::GameEntry&)> onCardClicked; // 卡片被点击时触发，参数为游戏条目数据
        



    };
    



} // namespace beiklive
