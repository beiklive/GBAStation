#include "GameCard.hpp"
#include <cmath>

namespace beiklive
{
    static constexpr float CARD_WIDTH_SWITCH = 220.f;
    static constexpr float CARD_HEIGHT_SWITCH = 270.f;
    static constexpr float COVER_WIDTH_SWITCH = 220.f;
    static constexpr float COVER_HEIGHT_SWITCH = 220.f;

    GameCard::GameCard(beiklive::enums::ThemeLayout type, beiklive::GameEntry gameEntry, int index)
    {
        m_layoutType = type;
        m_gameEntry = std::move(gameEntry);

        brls::Logger::debug("GameCard created for game: " + m_gameEntry.title);

        addGestureRecognizer(new brls::TapGestureRecognizer(this));

        // ✅ 入场动画初始化
        m_enterT = -index * 0.07f;
        m_enterScale = 0.5f;
        m_enterAnimating = true;

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

    void GameCard::setLogoLayer(const std::string &path, bool visible)
    {
        if (!m_imageLayer) return;
        if (visible && !path.empty())
            m_imageLayer->setImageFromFile(path);
        m_imageLayer->setVisibility(visible ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    // Switch卡片布局
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

        auto logobox = new brls::Box();
        logobox->setFocusable(true);
        logobox->setWidth(COVER_WIDTH_SWITCH);
        logobox->setHeight(COVER_HEIGHT_SWITCH);
        logobox->setBorderColor(nvgRGBA(128, 128, 128, 120));
        logobox->setCornerRadius(3.f);
        logobox->setBorderThickness(1.f);
        m_coverImage = new brls::Image();
        m_coverImage->setWidth(COVER_WIDTH_SWITCH);
        m_coverImage->setFocusable(false);
        m_coverImage->setHeight(COVER_HEIGHT_SWITCH);
        m_coverImage->setScalingType(brls::ImageScalingType::FILL);
        m_coverImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_coverImage->setImageFromFile(m_gameEntry.logoPath);
        m_coverImage->setHighlightPadding(3.f);
        m_coverImage->setHideHighlightBackground(true);
        m_coverImage->setShadowVisibility(true);
        m_coverImage->setShadowType(brls::ShadowType::GENERIC);
        m_coverImage->setHighlightCornerRadius(12.f);
        m_coverImage->setCornerRadius(3.f);
        #undef ABSOLUTE

        m_imageLayer = new brls::Image();
        m_imageLayer->setWidth(COVER_WIDTH_SWITCH);
        m_imageLayer->setHeight(COVER_HEIGHT_SWITCH);        
        m_imageLayer->setFocusable(false);
        m_imageLayer->setPositionTop(0.f);
        m_imageLayer->setPositionLeft(0.f);
        m_imageLayer->setPositionType(brls::PositionType::ABSOLUTE);
        m_imageLayer->setScalingType(brls::ImageScalingType::FILL);
        m_imageLayer->setFocusable(false);
        m_imageLayer->setVisibility(brls::Visibility::GONE);
        m_imageLayer->setCornerRadius(3.f);

        logobox->addView(m_coverImage);
        logobox->addView(m_imageLayer);

        m_titleLabel = new brls::Label();
        m_titleLabel->setWidth(CARD_WIDTH_SWITCH * 1.5f);
        m_titleLabel->setFontSize(26.f);
        m_titleLabel->setText(m_gameEntry.title);
        m_titleLabel->setTextColor(GET_THEME_COLOR("beiklive/CardText/color"));
        m_titleLabel->setSingleLine(true);
        m_titleLabel->setAnimated(true);
        m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_titleLabel->setVerticalAlign(brls::VerticalAlign::CENTER);
        m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
        m_titleLabel->setMarginBottom(10.f);

        this->addView(m_titleLabel);
        this->addView(logobox);

        this->registerAction(
            "打开",
            brls::BUTTON_A,
            [this](brls::View *)
            {
                brls::Application::notify("正在启动 " + m_gameEntry.title + "...");
                triggerClickBounce();
                return true;
            },
            false,
            false,
            brls::SOUND_CLICK);
    }

    void GameCard::onChildFocusGained(brls::View *directChild, brls::View *focusedView)
    {
        brls::Box::onChildFocusGained(directChild, focusedView);

        if (m_titleLabel)
            m_titleLabel->setVisibility(brls::Visibility::VISIBLE);
    }

    void GameCard::onChildFocusLost(brls::View *directChild, brls::View *focusedView)
    {
        brls::Box::onChildFocusLost(directChild, focusedView);

        if (m_titleLabel)
            m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
    }

    void GameCard::draw(NVGcontext *vg, float x, float y, float w, float h,
                        brls::Style style, brls::FrameContext *ctx)
    {
        // ===== ✅ 入场动画 =====
        if (m_enterAnimating)
        {
            m_enterT += 1.0f / 120.0f; // 用60更稳定

            float duration = 0.35f;
            float t = m_enterT / duration;

            if (t >= 1.0f)
            {
                m_enterScale = 1.0f;
                m_enterAnimating = false;
            }
            else
            {
                // 弹性 easing（关键：t ∈ [0,1]）
                float overshoot = 1.2f;
                float p = t - 1.0f;

                float ease = 1.0f + overshoot * (p * p * p + p * p);

                float start = 0.75f;
                m_enterScale = start + (1.0f - start) * ease;
            }

            invalidate();
        }

        // ===== ✅ 点击动画 =====
        if (m_clickAnimating)
        {
            m_clickT += 1.0f / 120.0f;

            if (m_clickT < 0.06f)
            {
                float t = m_clickT / 0.06f;
                m_clickScale = 1.0f - 0.10f * t;
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
                        onCardClicked(m_gameEntry);
                }
            }

            invalidate();
        }

        // ===== ✅ 合并缩放 =====
        float finalScale = m_scale * m_clickScale * m_enterScale;

        const float cx = x + w * 0.5f;
        const float cy = y + h * 0.5f;

        nvgSave(vg);
        nvgTranslate(vg, cx, cy);
        nvgScale(vg, finalScale, finalScale);
        nvgTranslate(vg, -cx, -cy);

        brls::Box::draw(vg, x, y, w, h, style, ctx);

        nvgRestore(vg);
    }

} // namespace beiklive