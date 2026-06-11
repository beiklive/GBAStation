#include "HintsBar.hpp"

namespace beiklive
{

    HintsBar::HintsBar()
        : brls::Box(brls::Axis::COLUMN)
    {
        this->setWidthPercentage(100);
        this->setAlignItems(brls::AlignItems::STRETCH);
        // 外框：顶部 1px 分隔线 + 边距
        auto* outerBox = new brls::Box();
        outerBox->setFocusable(false);
        outerBox->setWidthPercentage(100);
        outerBox->setHeight(GET_STYLE("brls/applet_frame/footer_height"));
        outerBox->setMarginLeft(GET_STYLE("brls/hints/footer_margin_sides"));
        outerBox->setMarginRight(GET_STYLE("brls/hints/footer_margin_sides"));
        outerBox->setPaddingLeft(GET_STYLE("brls/hints/footer_padding_sides"));
        outerBox->setPaddingRight(GET_STYLE("brls/hints/footer_padding_sides"));
        outerBox->setLineColor(GET_THEME_COLOR("brls/applet_frame/separator"));
        outerBox->setLineTop(1.f);
        outerBox->setAlignItems(brls::AlignItems::STRETCH);

        // 内层：只有 Hints
        m_hints = new brls::Hints();
        m_hints->setGrow(1.f);
        outerBox->addView(m_hints);

        this->addView(outerBox);
    }

} // namespace beiklive
