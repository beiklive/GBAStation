#include "GridDebugRenderer.hpp"

#include <algorithm>
#include <string>

namespace beiklive
{
    void GridDebugRenderer::setArea(float x, float y, float width, float height)
    {
        const float gridW = static_cast<float>(m_config.columns) *
                m_config.cellWidth +
            static_cast<float>(m_config.columns - 1) * m_config.gap;
        const float gridH = static_cast<float>(m_config.rows) *
                m_config.cellHeight +
            static_cast<float>(m_config.rows - 1) * m_config.gap;

        m_config.width = gridW;
        m_config.height = gridH;
        m_config.x = x + std::max(0.f, (width - gridW) * 0.5f);
        m_config.y = y + std::max(0.f, (height - gridH) * 0.5f);
    }

    void GridDebugRenderer::draw(NVGcontext* vg, int fontId)
    {
        if (!vg)
            return;

        for (int r = 0; r < m_config.rows; ++r) {
            for (int c = 0; c < m_config.columns; ++c) {
                const float px = m_config.x +
                    static_cast<float>(c) * (m_config.cellWidth + m_config.gap);
                const float py = m_config.y +
                    static_cast<float>(r) * (m_config.cellHeight + m_config.gap);

                nvgBeginPath(vg);
                nvgRoundedRect(vg, px, py, m_config.cellWidth,
                               m_config.cellHeight, m_config.radius);
                nvgFillColor(vg, nvgRGBA(128, 128, 128, 80));
                nvgFill(vg);

                // 行列编号，验证坐标
                if (fontId >= 0) {
                    nvgFontFaceId(vg, fontId);
                    nvgFontSize(vg, 16.f);
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(vg, nvgRGBA(255, 255, 255, 110));
                    const std::string label =
                        std::to_string(c) + "," + std::to_string(r);
                    nvgText(vg, px + m_config.cellWidth * 0.5f,
                            py + m_config.cellHeight * 0.5f,
                            label.c_str(), nullptr);
                }
            }
        }
    }
} // namespace beiklive
