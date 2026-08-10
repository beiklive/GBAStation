#pragma once

#include <borealis.hpp>

#include "LayoutItem.hpp"

namespace beiklive
{
    /// 布局网格参数：固定格子尺寸，整体在主体区域内居中
    struct GridConfig
    {
        int columns = 6;
        int rows = 3;

        float cellWidth = 160.f;
        float cellHeight = 160.f;
        float gap = 12.f;
        float radius = 18.f;

        float x = 0.f; // 网格左上角像素坐标
        float y = 0.f;
        float width = 0.f; // 网格总宽/总高（像素）
        float height = 0.f;
    };

    /// 像素矩形（屏幕坐标）
    struct GridRect
    {
        float left = 0.f;
        float top = 0.f;
        float width = 0.f;
        float height = 0.f;
    };

    /// 网格坐标系统：LayoutItem(列,行,宽,高) → 像素矩形
    class GridSystem
    {
    public:
        /// 在给定区域内水平垂直居中放置固定尺寸的网格
        void setArea(float x, float y, float width, float height);

        GridRect getItemRect(int x, int y, int w, int h) const;
        GridRect getItemRect(const LayoutItem& item) const;

        GridConfig& config() { return m_config; }
        const GridConfig& config() const { return m_config; }

    private:
        GridConfig m_config;
    };
} // namespace beiklive
