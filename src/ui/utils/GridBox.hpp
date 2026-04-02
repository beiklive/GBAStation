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
        /**
         * @param factory  单元格内容工厂函数，首次可见时调用，返回视觉内容视图。
         *                 返回值的 focusable 属性将被强制设为 false，焦点由 LazyCell 自身管理。
         * @param index    单元格在 GridBox 中的全局索引（0 起）
         */
        LazyCell(std::function<brls::View*()> factory, int index);
        ~LazyCell() = default;

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        void onFocusGained() override;
        void onFocusLost()   override;

        int getIndex() const { return m_index; }

        /// 单元格被 A 键点击时触发的回调（由 GridBox 注入）
        std::function<void(int index)> onClicked;
        /// 单元格获得焦点时的回调（由 GridBox 注入）
        std::function<void(int index)> onFocused;

    private:
        std::function<brls::View*()> m_factory;
        bool m_loaded = false; ///< 是否已完成延迟加载
        int  m_index  = -1;
    };

    // ─────────────────────────────────────────────────────────────────────────

    /**
     * GridBox - 网格布局容器
     *
     * 特性：
     *  - 可配置列数，行数自动根据元素总数计算
     *  - 每个单元格使用 LazyCell 延迟加载：首次进入视口时才执行工厂函数创建内容
     *  - 利用 setCustomNavigationRoute 为每个单元格明确配置上下左右四个方向的焦点路由，
     *    边界处循环跳转（首行上→末行，末行下→首行，首列左→末列，末列右→首列）
     *  - 内部使用垂直 ScrollingFrame，焦点移动时自动滚动到可见区域
     */
    class GridBox : public brls::Box
    {
    public:
        /**
         * @param columns 列数，默认 4
         */
        explicit GridBox(int columns = 4);
        ~GridBox() = default;

        // ── 布局配置 ───────────────────────────────────────────

        /// 设置列数，调用后自动重建网格布局
        void setColumns(int columns);
        int  getColumns() const { return m_columns; }

        // ── 单元格管理 ─────────────────────────────────────────

        /**
         * 追加一个延迟加载的单元格。
         * @param factory 工厂函数，首次显示时被调用，需返回有效的 brls::View*（或 nullptr 跳过）。
         *                工厂函数在 UI 线程中执行。
         */
        void addItem(std::function<brls::View*()> factory);

        /// 清除所有单元格（同时销毁已加载的子视图）
        void clearItems();

        /// 重建网格布局并重新配置所有焦点导航路由
        void rebuild();

        // ── 回调事件 ───────────────────────────────────────────

        /// 单元格被 A 键点击时触发，参数为单元格在列表中的全局索引（0 起）
        std::function<void(int index)> onItemClicked;

        /// 单元格获得焦点时触发，参数为单元格的全局索引
        std::function<void(int index)> onItemFocused;

    private:
        int m_columns = 4;

        brls::ScrollingFrame* m_scrollFrame = nullptr; ///< 垂直滚动容器
        brls::Box*            m_gridContent = nullptr; ///< COLUMN 方向网格内容盒子

        std::vector<std::function<brls::View*()>> m_factories; ///< 已注册的工厂函数列表
        std::vector<std::vector<LazyCell*>>       m_cells;     ///< [row][col] 单元格二维数组

        /// 遍历 m_cells，为每个 LazyCell 配置四个方向的焦点路由
        void setupNavigation();
    };

} // namespace beiklive
