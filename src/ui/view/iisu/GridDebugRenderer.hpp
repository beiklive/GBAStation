#pragma once

#include <borealis.hpp>

#include "GridSystem.hpp"
#include "LayoutItem.hpp"

namespace beiklive
{
    /// 背景网格渲染器：仅绘制辅助格，不承载内容（LayoutItem 由 LayoutManager 绘制）
    class GridDebugRenderer
    {
    public:
        /// 被 LayoutItem 占用的格子不绘制；空闲格只绘制中心小点
        static void draw(NVGcontext* vg, const GridSystem& grid,
                         const std::vector<LayoutItem>& items);
    };
} // namespace beiklive
