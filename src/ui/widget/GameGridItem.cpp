#include "GameGridItem.hpp"
#include "core/Translation.hpp"

namespace beiklive
{
    GameGridItem::GameGridItem(const beiklive::GameEntry& entry)
        : m_entry(entry)
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setWidth(ITEM_W);
        this->setHeight(ITEM_H);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::FLEX_START);
        this->setFocusable(false);
        this->setCornerRadius(3.f);
        this->setHideHighlightBackground(true);
        this->setHighlightCornerRadius(6.f);
        this->setMarginLeft(8.f);
        this->setMarginRight(8.f);
        this->setMarginTop(8.f);
        this->setMarginBottom(8.f);

        imgBox = new brls::Box();
        imgBox->setWidth(IMAGE_S - 10.f);
        imgBox->setHeight(IMAGE_S - 10.f);
        imgBox->setFocusable(true);
        imgBox->setMarginTop(10.f);
        imgBox->setBorderThickness(1.f);
        imgBox->setCornerRadius(8.f);
        imgBox->setShadowVisibility(true);
        imgBox->setShadowType(brls::ShadowType::GENERIC);
        imgBox->setBorderColor(nvgRGBA(128, 128, 128, 120));

        m_image = new brls::Image();
        m_image->setWidth(IMAGE_S - 20.f);
        m_image->setHeight(IMAGE_S - 20.f);
        m_image->setScalingType(brls::ImageScalingType::FILL);
        m_image->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_image->setCornerRadius(5.f);
        m_image->setFocusable(false);
        m_image->setMarginTop(5.f);
        m_image->setMarginLeft(5.f);

        if (!entry.logoPath.empty())
            m_image->setImageFromFile(entry.logoPath);

        imgBox->addView(m_image);
        this->addView(imgBox);

        m_title = new brls::Label();
        m_title->setHeight(20.f);
        m_title->setFontSize(16.f);
        m_title->setText(entry.title.empty() ? entry.path : entry.title);
        m_title->setTextColor(GET_THEME_COLOR("brls/text"));
        m_title->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_title->setSingleLine(true);
        m_title->setAnimated(true);
        m_title->setAutoAnimate(true);
        m_title->setVisibility(brls::Visibility::INVISIBLE);
        m_title->setMarginTop(6.f);
        m_title->setFocusable(false);
        this->addView(m_title);

        this->registerAction(
            L("确认"), brls::BUTTON_A,
            [this](brls::View*) -> bool {
                if (onItemClicked) onItemClicked(m_entry);
                return true;
            },
            false, false, brls::SOUND_CLICK);
    }

    void GameGridItem::setImagePath(const std::string& path)
    {
        if (m_image && !path.empty())
            m_image->setImageFromFile(path);
    }

    void GameGridItem::onParentFocusGained(brls::View* focusedView)
    {
        brls::Box::onParentFocusGained(focusedView);
        if (m_title) m_title->setVisibility(brls::Visibility::VISIBLE);
        imgBox->setBorderThickness(5.f);
        imgBox->setBorderColor(nvgRGBA(128, 128, 255, 120));
    }

    void GameGridItem::onParentFocusLost(brls::View* focusedView)
    {
        brls::Box::onParentFocusLost(focusedView);
        if (m_title) m_title->setVisibility(brls::Visibility::INVISIBLE);
        imgBox->setBorderColor(nvgRGBA(128, 128, 128, 120));
        imgBox->setBorderThickness(1.f);
    }

} // namespace beiklive
