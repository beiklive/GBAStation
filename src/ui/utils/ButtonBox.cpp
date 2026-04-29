#include "ButtonBox.hpp"

namespace beiklive
{

    ButtonBox::ButtonBox()
    {
        this->setAxis(brls::Axis::ROW);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setWidthPercentage(100.f);
        this->setHeight(48.f);
        this->setMarginBottom(16.f);
        this->setFocusable(true);
        this->setBackground(brls::ViewBackground::NONE);
        this->setHideHighlightBackground(true);
        this->setHideClickAnimation(false);

        m_accent = new brls::Rectangle();
        m_accent->setWidth(5.f);
        m_accent->setHeight(40.f);
        m_accent->setMarginLeft(5.f);
        m_accent->setColor(nvgRGBA(79, 193, 255, 255));
        m_accent->setVisibility(brls::Visibility::INVISIBLE);
        this->addView(m_accent);

        m_icon = new brls::Image();
        m_icon->setWidth(30.f);
        m_icon->setHeight(30.f);
        m_icon->setMarginLeft(5.f);
        m_icon->setMarginRight(25.f);
        m_icon->setScalingType(brls::ImageScalingType::FIT);
        m_icon->setInterpolation(brls::ImageInterpolation::LINEAR);
        this->addView(m_icon);

        m_label = new brls::Label();
        m_label->setFontSize(18.f);
        m_label->setGrow(1.f);
        m_label->setMarginTop(4.f);
        m_label->setVerticalAlign(brls::VerticalAlign::BOTTOM);
        m_label->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        this->addView(m_label);
    }

    void ButtonBox::setIcon(const std::string &iconPath)
    {
        m_icon->setImageFromFile(iconPath);
    }

    void ButtonBox::setText(const std::string &text)
    {
        m_label->setText(text);
    }

    void ButtonBox::onFocusGained()
    {
        Box::onFocusGained();
        m_accent->setVisibility(brls::Visibility::VISIBLE);

        if (onFocusGainedCallback) onFocusGainedCallback();
    }

    void ButtonBox::onFocusLost() 
    {
        Box::onFocusLost();
        m_accent->setVisibility(brls::Visibility::INVISIBLE);
        if (onFocusLostCallback) onFocusLostCallback();
    }



} // namespace beiklive
