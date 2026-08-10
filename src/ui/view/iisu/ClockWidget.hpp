#pragma once

#include "LiveWidget.hpp"

namespace beiklive
{
    /// 时钟（LiveWidget）：时间 + 日期 + 星期
    class ClockWidget : public LiveWidget
    {
    public:
        ClockWidget();

        void draw(NVGcontext* vg, const GridRect& rect) override;

    private:
        std::shared_ptr<ClockProvider> m_clockProvider;
    };
} // namespace beiklive
