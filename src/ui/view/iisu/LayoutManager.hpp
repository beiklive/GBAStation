#pragma once

#include <borealis.hpp>

#include <cstddef>
#include <vector>

#include "FocusManager.hpp"
#include "GridSystem.hpp"
#include "LayoutItem.hpp"
#include "UIAction.hpp"

namespace beiklive
{
    /// 布局管理器：管理多个 LayoutItem，持有网格坐标系统与焦点系统，驱动遍历绘制
    class LayoutManager
    {
    public:
        LayoutManager() = default;

        void addItem(const LayoutItem& item);
        void removeItem(size_t index);
        void clear();

        size_t size() const { return m_items.size(); }
        const std::vector<LayoutItem>& items() const { return m_items; }
        std::vector<LayoutItem>& items() { return m_items; }

        /// 网格坐标系统（背景格与 Tile 共用的唯一数据源）
        GridSystem& grid() { return m_grid; }
        const GridSystem& grid() const { return m_grid; }
        void setArea(float x, float y, float width, float height)
        {
            m_grid.setArea(x, y, width, height);
        }

        /// 焦点系统
        FocusManager& focus() { return m_focus; }
        const FocusManager& focus() const { return m_focus; }
        void moveFocus(UIAction action);
        void resetFocusToFirst();
        /// 覆盖当前焦点格的 LayoutItem（空格返回 nullptr）
        LayoutItem* currentItem();
        /// 焦点框是否可见（切换到底部功能区等场景时隐藏）
        void setFocusVisible(bool visible) { m_focusVisible = visible; }

        /// 更新所有 Widget
        void update(float delta);

        /// 遍历所有 LayoutItem：网格坐标 → Widget::draw（time 用于流光焦点动画）
        void draw(NVGcontext* vg, float time);

    private:
        void applyFocusChange(LayoutItem* oldFocus, LayoutItem* newFocus);
        /// 卡片背景：阴影 + 边缘（参考 switch 布局游戏卡片）
        void drawCard(NVGcontext* vg, const GridRect& rect, float radius);

        std::vector<LayoutItem> m_items;
        GridSystem m_grid;
        FocusManager m_focus;
        bool m_focusVisible = true;
    };
} // namespace beiklive
