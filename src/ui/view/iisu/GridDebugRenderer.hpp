#pragma once

#include <borealis.hpp>

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

        float x = 0.f;
        float y = 0.f;
        float width = 0.f;
        float height = 0.f;
    };

    /// 网格占位渲染器：仅验证布局网格坐标系统（后续 Tile/Folder 往格子里填内容）
    class GridDebugRenderer
    {
    public:
        /// 在给定区域内水平垂直居中放置固定尺寸的网格
        void setArea(float x, float y, float width, float height);
        void draw(NVGcontext* vg, int fontId);

        GridConfig& config() { return m_config; }
        const GridConfig& config() const { return m_config; }

    private:
        GridConfig m_config;
    };
} // namespace beiklive
