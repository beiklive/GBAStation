#include "ButtonBox.hpp"
#include "core/common.h"

#include <borealis/core/font.hpp>

namespace
{
    class MaterialIconLabel final : public brls::Label
    {
    public:
        MaterialIconLabel()
        {
            font = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        }
    };

    std::string encodeUtf8(char32_t codepoint)
    {
        std::string result;
        if (codepoint <= 0x7F)
        {
            result.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7FF)
        {
            result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
            result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return result;
    }
}

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
        this->setHideClickAnimation(true);

        m_accent = new brls::Rectangle();
        m_accent->setWidth(5.f);
        m_accent->setCornerRadius(2.5f);
        m_accent->setHeight(40.f);
        m_accent->setMarginLeft(5.f);
        m_accent->setColor(nvgRGBA(79, 193, 255, 255));
        m_accent->setVisibility(brls::Visibility::INVISIBLE);
        this->addView(m_accent);

        m_iconImage = new brls::Image();
        m_iconImage->setWidth(30.f);
        m_iconImage->setHeight(30.f);
        m_iconImage->setMarginLeft(5.f);
        m_iconImage->setMarginRight(25.f);
        m_iconImage->setScalingType(brls::ImageScalingType::FIT);
        m_iconImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_iconImage->setVisibility(brls::Visibility::GONE);
        this->addView(m_iconImage);

        m_materialIcon = new MaterialIconLabel();
        m_materialIcon->setWidth(30.f);
        m_materialIcon->setHeight(30.f);
        m_materialIcon->setFontSize(28.f);
        m_materialIcon->setTextColor(uiIconPrimary());
        m_materialIcon->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_materialIcon->setVerticalAlign(brls::VerticalAlign::CENTER);
        m_materialIcon->setMarginLeft(5.f);
        m_materialIcon->setMarginRight(25.f);
        m_materialIcon->setFocusable(false);
        m_materialIcon->setVisibility(brls::Visibility::GONE);
        this->addView(m_materialIcon);

        m_label = new brls::Label();
        m_label->setFontSize(18.f);
        m_label->setGrow(1.f);
        m_label->setMarginTop(4.f);
        m_label->setVerticalAlign(brls::VerticalAlign::BOTTOM);
        m_label->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        m_label->setTextColor(uiTextPrimary());
        this->addView(m_label);

        addGestureRecognizer(new brls::TapGestureRecognizer(this));

    }

    void ButtonBox::setIcon(const std::string& iconPath)
    {
        m_materialIcon->setVisibility(brls::Visibility::GONE);
        m_iconImage->setImageFromFile(iconPath);
        m_iconImage->setVisibility(brls::Visibility::VISIBLE);
    }

    void ButtonBox::setIcon(char32_t iconCodepoint)
    {
        m_iconImage->setVisibility(brls::Visibility::GONE);
        m_materialIcon->setText(encodeUtf8(iconCodepoint));
        m_materialIcon->setVisibility(brls::Visibility::VISIBLE);
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
