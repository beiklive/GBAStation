#include "GameCard.hpp"

namespace beiklive
{
    static constexpr float CARD_WIDTH_SWITCH = 220.f;   // switch卡片宽度
    static constexpr float CARD_HEIGHT_SWITCH = 270.f;  // switch卡片高度
    static constexpr float COVER_WIDTH_SWITCH = 220.f;  // 卡片图片宽度
    static constexpr float COVER_HEIGHT_SWITCH = 220.f; // 卡片图片高度

    GameCard::GameCard(beiklive::enums::ThemeLayout type, GameEntry *gameEntry)
    {
        m_layoutType = type;
        m_gameEntry = gameEntry;
    }

    GameCard::~GameCard()
    {
    }
    void GameCard::triggerClickBounce()
    {
        m_clickAnimating = true;
        m_clickT = 0.0f;
        m_clickScale = 1.0f;
        invalidate();
    }
    void GameCard::applyThemeLayout()
    {
        switch (m_layoutType)
        {
        case beiklive::enums::ThemeLayout::DEFAULT_THEME:
        case beiklive::enums::ThemeLayout::SWITCH_THEME:
            _switchCardLayout();
            break;

        default:
            break;
        }
    }
    // 使用switch卡片布局， 上下布局，封面在上，标题在下，标题显影
    void GameCard::_switchCardLayout()
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::SPACE_AROUND);
        this->setFocusable(false);
        this->setHideHighlightBackground(true);
        this->setHideClickAnimation(true);
        this->setHeight(CARD_HEIGHT_SWITCH);
        this->setWidth(CARD_WIDTH_SWITCH);
        m_coverImage = new brls::Image();
        m_coverImage->setFocusable(true);
        m_coverImage->setWidth(COVER_WIDTH_SWITCH);
        m_coverImage->setHeight(COVER_HEIGHT_SWITCH);
        m_coverImage->setScalingType(brls::ImageScalingType::FILL);
        m_coverImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_coverImage->setImageFromFile(m_gameEntry->logoPath);
        m_coverImage->setHighlightPadding(3.f);
        m_coverImage->setHideHighlightBackground(true);
        m_coverImage->setShadowVisibility(true);
        m_coverImage->setShadowType(brls::ShadowType::GENERIC);
        m_coverImage->setHighlightCornerRadius(12.f);
        m_coverImage->setCornerRadius(10.f);

        m_titleLabel = new brls::Label();
        m_titleLabel->setWidth(CARD_WIDTH_SWITCH * 1.5f); // 标题宽度略大于卡片宽度，避免过早截断
        m_titleLabel->setFontSize(26.f);
        m_titleLabel->setText(m_gameEntry->title);
        m_titleLabel->setTextColor(GET_THEME_COLOR("beiklive/CardText/color"));
        m_titleLabel->setSingleLine(true);
        m_titleLabel->setAnimated(true);
        m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_titleLabel->setVerticalAlign(brls::VerticalAlign::CENTER);
        m_titleLabel->setVisibility(brls::Visibility::INVISIBLE); // 默认隐藏标题，获得焦点时才显示
        m_titleLabel->setMarginBottom(10.f);

        this->addView(m_titleLabel);
        this->addView(m_coverImage);

        this->registerAction(
            "打开",
            brls::BUTTON_A,
            [this](brls::View *)
            {
                triggerClickBounce();
                return true;
            }, /*hidden=*/false, /*repeat=*/false, brls::SOUND_CLICK);
    }

    void GameCard::onChildFocusGained(brls::View *directChild, brls::View *focusedView)
    {
        brls::Box::onChildFocusGained(directChild, focusedView);
        switch (m_layoutType)
        {
        case beiklive::enums::ThemeLayout::DEFAULT_THEME:
        case beiklive::enums::ThemeLayout::SWITCH_THEME:
            if (m_titleLabel)
                m_titleLabel->setVisibility(brls::Visibility::VISIBLE);
            break;

        default:
            break;
        }
    }

    void GameCard::onChildFocusLost(brls::View *directChild, brls::View *focusedView)
    {
        brls::Box::onChildFocusLost(directChild, focusedView);
        switch (m_layoutType)
        {
        case beiklive::enums::ThemeLayout::DEFAULT_THEME:
        case beiklive::enums::ThemeLayout::SWITCH_THEME:
            if (m_titleLabel)
                m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
            break;
        default:
            break;
        }
    }
    void GameCard::draw(NVGcontext *vg, float x, float y, float w, float h,
                        brls::Style style, brls::FrameContext *ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        switch (m_layoutType)
        {
        case beiklive::enums::ThemeLayout::DEFAULT_THEME:
        case beiklive::enums::ThemeLayout::SWITCH_THEME:
        {
            // 点击弹性动画（先压缩，再阻尼回弹）
            if (m_clickAnimating)
            {
                m_clickT += 1.0f / 120.0f; // 近似按 60fps 推进
                if (m_clickT < 0.06f)
                {
                    float t = m_clickT / 0.06f;      // 0~1
                    m_clickScale = 1.0f - 0.10f * t; // 压到 0.90
                }
                else
                {
                    float u = m_clickT - 0.06f;
                    m_clickScale = 1.0f + 0.12f * std::exp(-14.0f * u) * std::sin(45.0f * u);

                    if (u > 0.28f && std::abs(m_clickScale - 1.0f) < 0.003f)
                    {
                        m_clickScale = 1.0f;
                        m_clickAnimating = false;
                        if (onCardClicked)
                            onCardClicked(*m_gameEntry);
                    }
                }
                invalidate();
            }

            float finalScale = m_scale * m_clickScale;

            const float cx = x + w * 0.5f;
            const float cy = y + h * 0.5f;
            nvgSave(vg);
            nvgTranslate(vg, cx, cy);
            nvgScale(vg, finalScale, finalScale);
            nvgTranslate(vg, -cx, -cy);
            brls::Box::draw(vg, x, y, w, h, style, ctx);
            nvgRestore(vg);
        }
        break;

        default:
            break;
        }
    }

} // namespace beiklive