#pragma once

#include <borealis.hpp>
#include <functional>

#include "core/common.h"

namespace beiklive
{
    /**
     * GameGridItem – 数据管理页面的正方形卡片控件
     *
     * 尺寸：200 x 250
     * 布局：上方 200x200 正方形封面图，下方标题 Label（聚焦时显示）
     */
    class GameGridItem : public brls::Box
    {
    public:
        explicit GameGridItem(const beiklive::GameEntry& entry);
        ~GameGridItem() = default;

        void onParentFocusGained(brls::View* focusedView) override;
        void onParentFocusLost(brls::View *focusedView) override;

        std::function<void(const beiklive::GameEntry&)> onItemClicked;

        void setImagePath(const std::string& path);

    private:
        beiklive::GameEntry m_entry;
        brls::Image* m_image = nullptr;
        brls::Label* m_title = nullptr;

        static constexpr float ITEM_W = 180.f;
        static constexpr float ITEM_H = 180.f;
        static constexpr float IMAGE_S = 180.f;
    };

} // namespace beiklive
