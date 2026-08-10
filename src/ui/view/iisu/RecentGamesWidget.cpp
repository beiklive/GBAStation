#include "RecentGamesWidget.hpp"

#include <algorithm>

#include "core/Tools.hpp"
#include "core/Translation.hpp"
#include "GridSystem.hpp"

namespace beiklive
{
    RecentGamesWidget::RecentGamesWidget()
        : LiveWidget("recent_games")
    {
        m_recentProvider = std::make_shared<RecentGameProvider>();
        m_provider = m_recentProvider;
    }

    void RecentGamesWidget::draw(NVGcontext* vg, const GridRect& rect)
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
                L("最近运行").c_str(), nullptr);

        // 分隔线
        const float sepY = rect.top + 34.f;
        nvgBeginPath(vg);
        nvgMoveTo(vg, rect.left + 12.f, sepY);
        nvgLineTo(vg, rect.left + rect.width - 12.f, sepY);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 45));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        // 列表（最多 4 行）
        const float rowHeight = 24.f;
        const float startY = sepY + 12.f;
        const size_t count = std::min<size_t>(4, m_recentProvider->games().size());
        for (size_t i = 0; i < count; ++i) {
            const auto& game = m_recentProvider->games()[i];
            const float y = startY + static_cast<float>(i) * rowHeight;

            nvgFontSize(vg, 13.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(220, 227, 240, 220));
            nvgText(vg, rect.left + 12.f, y + rowHeight * 0.5f,
                    game.title.c_str(), nullptr);

            if (game.playTime > 0) {
                const std::string playText =
                    beiklive::tools::formatPlayTime(game.playTime);
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(170, 180, 200, 170));
                nvgText(vg, rect.left + rect.width - 12.f,
                        y + rowHeight * 0.5f, playText.c_str(), nullptr);
            }
        }
    }
} // namespace beiklive
