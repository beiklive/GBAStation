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

        brls::Label *m_titleLabel = nullptr;
        brls::Image *m_coverImage = nullptr;
        brls::Image *m_imageLayer = nullptr;
        brls::Label *m_playTimeLabel = nullptr;
        brls::Label *m_lastPlayedLabel = nullptr;

        float m_scale = 1.0f; ///< 当前渲染缩放比，由 draw() 平滑插值
        bool m_clickAnimating = false;
        float m_clickT = 0.0f;
        float m_clickScale = 1.0f;

        // 入场动画
        bool m_enterAnimating = false;
        float m_enterT = 0.0f;
        float m_enterScale = 1.0f;

        // 额外信息标签滑入动画
        bool m_infoAnimating = false;
        float m_infoT = 0.0f;
        float m_infoOffset = 300.f; ///< 从右侧偏移量（动画结束后归零）

        // 空卡片状态
        bool m_isEmpty = false;

        void triggerClickBounce();
        void _updateFavouriteHint();
        void _toggleFavourite();

        void _switchCardLayout();

    public:
        GameCard(beiklive::enums::ThemeLayout type, beiklive::GameEntry gameEntry, int index = 0);
        ~GameCard();
        void applyThemeLayout();

        void updateLogo(const std::string &logoPath);
        void setLogoLayer(const std::string &path, bool visible);
        void setLayoutType(beiklive::enums::ThemeLayout type) { m_layoutType = type; }
        beiklive::enums::ThemeLayout getLayoutType() const { return m_layoutType; }

        /// 更新卡片数据并刷新显示（复用视图，不重建）
        void updateGameEntry(const beiklive::GameEntry& entry);

        bool isEmpty() const { return m_isEmpty; }
        beiklive::GameEntry& getGameEntry() { return m_gameEntry; }

        void onChildFocusGained(brls::View *directChild, brls::View *focusedView) override;
        void onChildFocusLost(brls::View *directChild, brls::View *focusedView) override;

        void draw(NVGcontext *vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext *ctx) override;

        std::function<void(beiklive::GameEntry &)> onCardClicked; // 卡片被点击时触发，参数为游戏条目数据
        std::function<void(beiklive::GameEntry &)> onFavouriteToggled; // 收藏状态切换后触发，用于外部刷新
    };

} // namespace beiklive
