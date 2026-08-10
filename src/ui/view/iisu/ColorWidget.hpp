#pragma once

#include "Widget.hpp"

namespace beiklive
{
    /// 调试用：纯色块 Widget（验证 Widget 链路与焦点联动）
    class ColorWidget : public Widget
    {
    public:
        explicit ColorWidget(NVGcolor color);

        void draw(NVGcontext* vg, const GridRect& rect) override;
        void onFocus() override;
        void onBlur() override;

        std::string typeName() const override { return "color"; }

    private:
        NVGcolor m_color;
        bool m_focused = false;
    };
} // namespace beiklive
