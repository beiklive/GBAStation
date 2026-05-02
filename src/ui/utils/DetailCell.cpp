#include "DetailCell.hpp"
#include "core/common.h"

namespace beiklive
{

    DetailCell::DetailCell()
        : brls::Box(brls::Axis::ROW)
    {
        this->setFocusable(true);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setHideHighlightBackground(true);
        this->setHeight(60.f);
        this->setPadding(12.f, 20.f, 12.f, 20.f);

        // ── 左侧 Label ──
        m_leftLabel = new brls::Label();
        m_leftLabel->setFontSize(20.f);
        m_leftLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_leftLabel->setGrow(1.f);
        m_leftLabel->setSingleLine(true);
        m_leftLabel->setAnimated(true);
        m_leftLabel->setFocusable(false);
        this->addView(m_leftLabel);

        // ── 左侧 Image（默认隐藏）──
        m_leftImage = new brls::Image();
        m_leftImage->setWidth(32.f);
        m_leftImage->setHeight(32.f);
        m_leftImage->setScalingType(brls::ImageScalingType::FIT);
        m_leftImage->setVisibility(brls::Visibility::GONE);
        m_leftImage->setFocusable(false);
        m_leftImage->setMarginRight(10.f);
        // 插入到第一个位置（label 之前）
        this->addView(m_leftImage, 0);
    }

    void DetailCell::setLeftText(const std::string& text)
    {
        m_leftLabel->setText(text);
    }

    void DetailCell::setLeftTextSize(float size)
    {
        m_leftLabel->setFontSize(size);
    }

    void DetailCell::setLeftTextColor(NVGcolor color)
    {
        m_leftLabel->setTextColor(color);
    }

    void DetailCell::setLeftImage(const std::string& path)
    {
        m_leftImage->setImageFromFile(path);
        m_leftImage->setVisibility(brls::Visibility::VISIBLE);
        // 左侧文本缩小到右边填充
        m_leftLabel->setGrow(1.f);
    }

    void DetailCell::setLeftImageSize(float w, float h)
    {
        m_leftImage->setWidth(w);
        m_leftImage->setHeight(h);
    }

    // ── 右侧 ──

    void DetailCell::_ensureRightBox()
    {
        if (!m_rightBox)
        {
            m_rightBox = new brls::Box(brls::Axis::ROW);
            m_rightBox->setAlignItems(brls::AlignItems::CENTER);
            m_rightBox->setFocusable(false);
            m_rightBox->setShrink(0.f);
            this->addView(m_rightBox);
        }
    }

    brls::Label* DetailCell::addRightLabel(const std::string& text)
    {
        _ensureRightBox();
        auto* lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(16.f);
        lbl->setTextColor(GET_THEME_COLOR("brls/text"));
        lbl->setSingleLine(true);
        lbl->setFocusable(false);
        lbl->setMarginLeft(8.f);
        lbl->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
        m_rightBox->addView(lbl);
        return lbl;
    }

    brls::Image* DetailCell::addRightImage(const std::string& path)
    {
        _ensureRightBox();
        auto* img = new brls::Image();
        if (!path.empty())
            img->setImageFromFile(path);
        img->setWidth(28.f);
        img->setHeight(28.f);
        img->setScalingType(brls::ImageScalingType::FIT);
        img->setInterpolation(brls::ImageInterpolation::LINEAR);
        img->setFocusable(false);
        img->setMarginLeft(8.f);
        m_rightBox->addView(img);
        return img;
    }

    void DetailCell::clearRightViews()
    {
        if (m_rightBox)
        {
            m_rightBox->clearViews(true);
            m_rightBox = nullptr;
        }
    }

    void DetailCell::setRightText(const std::string& text)
    {
        clearRightViews();
        addRightLabel(text);
    }

} // namespace beiklive
