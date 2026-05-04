#include "GameGridItem.hpp"

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


        // 封面图
        auto imgBox = new brls::Box();
        imgBox->setWidth(IMAGE_S);
        imgBox->setHeight(IMAGE_S);
        imgBox->setFocusable(true);

        m_image = new brls::Image();
        m_image->setWidth(IMAGE_S);
        m_image->setHeight(IMAGE_S);
        m_image->setScalingType(brls::ImageScalingType::FILL);
        m_image->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_image->setCornerRadius(3.f);
        m_image->setFocusable(false);

        if (!entry.logoPath.empty())
            m_image->setImageFromFile(entry.logoPath);

        imgBox->addView(m_image);
        this->addView(imgBox);

        // 标题 Label（失焦隐藏）
        m_title = new brls::Label();
        m_title->setWidth(IMAGE_S);
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
            "确认",
            brls::BUTTON_A,
            [this](brls::View*) -> bool {
                if (onItemClicked)
                    onItemClicked(m_entry);
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
        if (m_title)
            m_title->setVisibility(brls::Visibility::VISIBLE);
    }

    void GameGridItem::onParentFocusLost(brls::View *focusedView)
    {
        brls::Box::onParentFocusLost(focusedView);
        if (m_title)
            m_title->setVisibility(brls::Visibility::INVISIBLE);
    }

} // namespace beiklive
