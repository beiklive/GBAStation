#pragma once

#include <borealis.hpp>
#include <functional>
#include <vector>

#include "core/common.h"

namespace beiklive
{

    /**
     * LazyCell - 延迟加载单元格
     *
     * 首次绘制时通过工厂函数创建真实子视图，之后不再重复调用工厂函数。
     * 自身作为可聚焦元素，供 setCustomNavigationRoute 设置导航路由。
     */
    class LazyCell : public brls::Box
    {
    public:
        LazyCell(std::function<brls::View*()> factory, int index);
        ~LazyCell() = default;

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        void onFocusGained() override;
        void onFocusLost()   override;

        int getIndex() const { return m_index; }

        std::function<void(int index)> onClicked;
        std::function<void(int index)> onFocused;
        std::function<bool(int index)> onX;
        std::function<bool(int index)> onY;

    private:
        std::function<brls::View*()> m_factory;
        bool m_loaded = false;
        int  m_index  = -1;
    };

    // ─────────────────────────────────────────────────────────────────────────

    class GridBox : public brls::Box
    {
    public:
        explicit GridBox(int columns = 4);
        ~GridBox() = default;

        void hideHighlight(bool value) { m_hideHightlight = value; }
        void setColumns(int columns);
        int  getColumns() const { return m_columns; }

        /// 追加工厂（不重建布局）。调用后需 commit() 或 commitAppend()
        void addItem(std::function<brls::View*()> factory);

        /// 提交全部工厂：清空现有视图，重新渲染所有工厂（用于首次加载/排序/刷新）
        void commit();

        /// 增量提交：仅渲染自上次 commit/commitAppend 以来新增的工厂（保持已有视图不动）
        void commitAppend();

        brls::View* getItemView(int index) const;

        /// 清除全部数据与视图（重置渲染计数）
        void clearItems();

        /// 获取当前item索引
        int getItemIndex() const { return m_itemIndex; }
        void setItemIndex(int index) { m_itemIndex = index; }

        /// 获取内部 ScrollingFrame
        brls::ScrollingFrame* getScrollFrame() const { return m_scrollFrame; }

        std::function<void(int index)> onItemClicked;
        std::function<void(int index)> onItemFocused;
        std::function<void(int index)> onItemX;
        std::function<void(int index)> onItemY;
        

    private:
        int m_itemIndex = -1;
        int m_columns = 4;
        bool m_hideHightlight = false;
        int m_renderedCount = 0;

        brls::ScrollingFrame* m_scrollFrame = nullptr;
        brls::Box*            m_gridContent = nullptr;

        std::vector<std::function<brls::View*()>> m_factories;
        std::vector<std::vector<LazyCell*>>       m_cells;
        std::vector<brls::Box*>                   m_rowBoxes;

        void renderAll();
        void renderMore(int newTotal);
        void setupNavigation();

        LazyCell* makeCell(int idx, float widthPercent);
        brls::Box* makeRowBox();
    };

} // namespace beiklive
