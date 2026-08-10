#include "GridDebugRenderer.hpp"

#include <set>
#include <utility>

namespace beiklive
{
    void GridDebugRenderer::draw(NVGcontext* vg, const GridSystem& grid,
                                 const std::vector<LayoutItem>& items)
    {
        if (!vg)
            return;

        // 收集被 LayoutItem 占用的格子
        std::set<std::pair<int, int>> occupied;
        for (const auto& item : items) {
            if (!item.visible)
                continue;
            for (int y = item.y; y < item.y + item.h; ++y) {
                for (int x = item.x; x < item.x + item.w; ++x)
                    occupied.insert({x, y});
            }
        }

        const GridConfig& cfg = grid.config();
        const float dotRadius = 4.f;

        for (int r = 0; r < cfg.rows; ++r) {
            for (int c = 0; c < cfg.columns; ++c) {
                // 使用中的格子不再绘制
                if (occupied.count({c, r}) != 0)
                    continue;

                const float cx = cfg.x +
                    static_cast<float>(c) * (cfg.cellWidth + cfg.gap) +
                    cfg.cellWidth * 0.5f;
                const float cy = cfg.y +
                    static_cast<float>(r) * (cfg.cellHeight + cfg.gap) +
                    cfg.cellHeight * 0.5f;

                // 空闲格：仅绘制中心小点
                nvgBeginPath(vg);
                nvgCircle(vg, cx, cy, dotRadius);
                nvgFillColor(vg, nvgRGBA(128, 128, 128, 80));
                nvgFill(vg);
            }
        }
    }
} // namespace beiklive
