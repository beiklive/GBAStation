#include "SystemInfoWidget.hpp"

#include <string>

#include "core/Translation.hpp"
#include "GridSystem.hpp"

namespace beiklive
{
    SystemInfoWidget::SystemInfoWidget()
        : LiveWidget("system_info")
    {
        m_systemProvider = std::make_shared<SystemInfoProvider>();
        m_provider = m_systemProvider;
    }

    void SystemInfoWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        const int fontId = brls::Application::getDefaultFont();

        // 标题
        nvgFontFaceId(vg, fontId);
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(242, 245, 251, 235));
        nvgText(vg, rect.left + 12.f, rect.top + 10.f,
                L("系统状态").c_str(), nullptr);

        // 分隔线
        const float sepY = rect.top + 34.f;
        nvgBeginPath(vg);
        nvgMoveTo(vg, rect.left + 12.f, sepY);
        nvgLineTo(vg, rect.left + rect.width - 12.f, sepY);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 45));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        // 数据行
        const std::string fpsText =
            "FPS: " + std::to_string(static_cast<int>(m_systemProvider->fps() + 0.5f));
        const std::string gameText =
            L("游戏: ") + std::to_string(m_systemProvider->gameCount()) +
            L(" 款");
        const std::string platformText =
            L("平台: ") + std::to_string(m_systemProvider->platformCount()) +
            L(" 种");

        const float rowHeight = 24.f;
        const float startY = sepY + 12.f;
        const std::string lines[] = {fpsText, gameText, platformText};
        for (size_t i = 0; i < 3; ++i) {
            nvgFontSize(vg, 13.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(220, 227, 240, 220));
            nvgText(vg, rect.left + 12.f,
                    startY + static_cast<float>(i) * rowHeight +
                        rowHeight * 0.5f,
                    lines[i].c_str(), nullptr);
        }
    }
} // namespace beiklive
