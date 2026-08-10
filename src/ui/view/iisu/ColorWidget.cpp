#include "ColorWidget.hpp"

#include "GridSystem.hpp"

namespace beiklive
{
    ColorWidget::ColorWidget(NVGcolor color) : m_color(color)
    {
    }

    void ColorWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        NVGcolor fill = m_color;
        fill.a *= m_focused ? 0.92f : 0.55f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, rect.left, rect.top, rect.width, rect.height, m_radius);
        nvgFillColor(vg, fill);
        nvgFill(vg);

        // 焦点视觉由 Widget 自己处理
        if (m_focused) {
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 220));
            nvgStrokeWidth(vg, 2.f);
            nvgStroke(vg);
        }
    }

    void ColorWidget::onFocus()
    {
        m_focused = true;
    }

    void ColorWidget::onBlur()
    {
        m_focused = false;
    }
} // namespace beiklive
