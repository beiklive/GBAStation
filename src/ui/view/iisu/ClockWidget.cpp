#include "ClockWidget.hpp"

#include "GridSystem.hpp"

namespace beiklive
{
    ClockWidget::ClockWidget() : LiveWidget("clock")
    {
        m_clockProvider = std::make_shared<ClockProvider>();
        m_provider = m_clockProvider;
    }

    void ClockWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        const int fontId = brls::Application::getDefaultFont();
        const float cx = rect.left + rect.width * 0.5f;

        // 时间（大字）
        nvgFontFaceId(vg, fontId);
        nvgFontSize(vg, 34.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(250, 251, 255, 240));
        nvgText(vg, cx, rect.top + rect.height * 0.40f,
                m_clockProvider->timeText().c_str(), nullptr);

        // 日期 + 星期
        nvgFontSize(vg, 14.f);
        nvgFillColor(vg, nvgRGBA(200, 209, 225, 200));
        nvgText(vg, cx, rect.top + rect.height * 0.70f,
                m_clockProvider->dateText().c_str(), nullptr);
    }
} // namespace beiklive
