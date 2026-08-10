#pragma once

#include "LiveWidget.hpp"

namespace beiklive
{
    /// 系统状态（LiveWidget）：UI 帧率 + 游戏库统计
    class SystemInfoWidget : public LiveWidget
    {
    public:
        SystemInfoWidget();

        void draw(NVGcontext* vg, const GridRect& rect) override;

    private:
        std::shared_ptr<SystemInfoProvider> m_systemProvider;
    };
} // namespace beiklive
