#pragma once

#include "LiveWidget.hpp"

namespace beiklive
{
    /// 最近运行列表（LiveWidget）
    class RecentGamesWidget : public LiveWidget
    {
    public:
        RecentGamesWidget();

        void draw(NVGcontext* vg, const GridRect& rect) override;

    private:
        std::shared_ptr<RecentGameProvider> m_recentProvider;
    };
} // namespace beiklive
