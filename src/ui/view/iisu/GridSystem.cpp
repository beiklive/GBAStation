#include "GridSystem.hpp"

namespace beiklive
{
    void GridSystem::setArea(float x, float y, float width, float height)
    {
        const GridConfig& cfg = m_config;
        const float gridW = static_cast<float>(cfg.columns) * cfg.cellWidth +
            static_cast<float>(cfg.columns - 1) * cfg.gap;
        const float gridH = static_cast<float>(cfg.rows) * cfg.cellHeight +
            static_cast<float>(cfg.rows - 1) * cfg.gap;

        m_viewWidth = width;

        m_config.width = gridW;
        m_config.height = gridH;
        // 网格超出可视区时左对齐（配合横向滚动），否则居中
        m_config.x = x + std::max(0.f, (width - gridW) * 0.5f);
        m_config.y = y + std::max(0.f, (height - gridH) * 0.5f);
    }

    GridRect GridSystem::getItemRect(int x, int y, int w, int h) const
    {
        const GridConfig& cfg = m_config;
        GridRect rect;
        rect.left = cfg.x +
            static_cast<float>(x) * (cfg.cellWidth + cfg.gap) - m_scrollX;
        rect.top = cfg.y +
            static_cast<float>(y) * (cfg.cellHeight + cfg.gap);
        rect.width = static_cast<float>(w) * cfg.cellWidth +
            static_cast<float>(w - 1) * cfg.gap;
        rect.height = static_cast<float>(h) * cfg.cellHeight +
            static_cast<float>(h - 1) * cfg.gap;
        return rect;
    }

    GridRect GridSystem::getItemRect(const LayoutItem& item) const
    {
        return getItemRect(item.x, item.y, item.w, item.h);
    }

    float GridSystem::maxScrollX() const
    {
        return std::max(0.f, m_config.width - m_viewWidth);
    }
} // namespace beiklive
