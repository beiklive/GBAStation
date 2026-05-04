#include "GameCard.hpp"
#include "core/Tools.hpp"
#include <cmath>
#include "borealis/core/cache_helper.hpp"

namespace beiklive
{
    static constexpr float CARD_WIDTH_SWITCH     = 220.f;
    static constexpr float CARD_HEIGHT_SWITCH    = 350.f;
    static constexpr float COVER_WIDTH_SWITCH    = 220.f;
    static constexpr float COVER_HEIGHT_SWITCH   = 220.f;
    static constexpr float INFO_ANIM_DURATION    = 0.25f;

    GameCard::GameCard(beiklive::enums::ThemeLayout type, beiklive::GameEntry gameEntry, int index)
    {
        m_layoutType = type;
        m_gameEntry  = std::move(gameEntry);
        m_isEmpty    = m_gameEntry.path.empty();

        brls::Logger::debug("GameCard created for game: " + m_gameEntry.title);

        addGestureRecognizer(new brls::TapGestureRecognizer(this));

        m_enterT        = -index * 0.01f;
        m_enterScale    = 1.0f;
        m_enterAnimating = false;
    }

    GameCard::~GameCard() {}

    void GameCard::triggerClickBounce()
    {
        m_clickAnimating = true;
        m_clickT         = 0.0f;
        m_clickScale     = 1.0f;
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

    void GameCard::updateLogo(const std::string &logoPath)
    {
        if (m_coverImage)
        {
            int oldTex = brls::TextureCache::instance().getCache(logoPath);
            if (oldTex > 0) {
                brls::TextureCache::instance().removeCache(static_cast<size_t>(oldTex));
                brls::TextureCache::instance().markDirty(static_cast<size_t>(oldTex));
            }
            m_coverImage->setImageFromFile(logoPath);
        }
    }

    void GameCard::setLogoLayer(const std::string &path, bool visible)
    {
        if (!m_imageLayer) return;
        if (visible && !path.empty())
            m_imageLayer->setImageFromFile(path);
        m_imageLayer->setVisibility(visible ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void GameCard::_updateFavouriteHint()
    {
        if (m_isEmpty) return;
        for (auto& action : this->getActions())
        {
            if (action->getButton() == brls::BUTTON_RT)
            {
                bool fav = m_gameEntry.favourite;
                action->setHintText(fav ? "取消收藏" : "收藏");
            }
        }
    }

    void GameCard::_toggleFavourite()
    {
        if (m_isEmpty) return;
        m_gameEntry.favourite = !m_gameEntry.favourite;
        int crc = m_gameEntry.crc32;
        if (crc != 0 && beiklive::GameDB)
        {
            beiklive::GameDB->set(crc, "favourite", nlohmann::json(m_gameEntry.favourite));
            beiklive::GameDB->flush();
        }
        _updateFavouriteHint();
        if (onFavouriteToggled)
            onFavouriteToggled(m_gameEntry);
    }

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

        // if (m_isEmpty)
        // {
        //     this->setHeight(COVER_HEIGHT_SWITCH + 10.f);
        // }

        auto logobox = new brls::Box();
        logobox->setFocusable(true);
        logobox->setWidth(COVER_WIDTH_SWITCH);
        logobox->setHeight(COVER_HEIGHT_SWITCH);
        logobox->setHideHighlightBackground(true);

            logobox->setBorderColor(nvgRGBA(128, 128, 128, 120));
        logobox->setCornerRadius(7.f);
        logobox->setBorderThickness(1.f);

        m_coverImage = new brls::Image();
        m_coverImage->setWidth(COVER_WIDTH_SWITCH);
        m_coverImage->setFocusable(false);
        m_coverImage->setHeight(COVER_HEIGHT_SWITCH);
        m_coverImage->setScalingType(brls::ImageScalingType::FILL);
        m_coverImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_coverImage->setHighlightPadding(3.f);
        m_coverImage->setHideHighlightBackground(true);
        m_coverImage->setShadowVisibility(true);
        m_coverImage->setShadowType(brls::ShadowType::GENERIC);
        m_coverImage->setHighlightCornerRadius(12.f);
        m_coverImage->setCornerRadius(7.f);

        if (!m_isEmpty && !m_gameEntry.logoPath.empty())
            m_coverImage->setImageFromFile(m_gameEntry.logoPath);
        else if (m_isEmpty)
            m_coverImage->setImageFromFile(""); // 空卡片不加载图片

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

            m_titleLabel = new brls::Label();
            m_titleLabel->setWidth(CARD_WIDTH_SWITCH * 2.5f);
            m_titleLabel->setHeight(30.f);
            m_titleLabel->setFontSize(26.f);
            m_titleLabel->setText(m_isEmpty? " " : m_gameEntry.title);
            m_titleLabel->setTextColor(GET_THEME_COLOR("beiklive/CardText/color"));
            m_titleLabel->setSingleLine(true);
            m_titleLabel->setAnimated(true);
            m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            m_titleLabel->setVerticalAlign(brls::VerticalAlign::CENTER);
            m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
            m_titleLabel->setMarginBottom(20.f);

            this->addView(m_titleLabel);

        this->addView(logobox);

        // if (!m_isEmpty)
            // 游玩时间标签（左对齐，焦点时从右滑入）
            m_playTimeLabel = new brls::Label();
            m_playTimeLabel->setWidth(CARD_WIDTH_SWITCH);
            m_playTimeLabel->setHeight(20.f);
            m_playTimeLabel->setFontSize(16.f);
            m_playTimeLabel->setTextColor(nvgRGBA(200, 200, 200, 200));
            m_playTimeLabel->setSingleLine(true);
            m_playTimeLabel->setAnimated(true);
            m_playTimeLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
            m_playTimeLabel->setVisibility(brls::Visibility::INVISIBLE);
            m_playTimeLabel->setMarginTop(15.f);
            m_playTimeLabel->setMarginLeft(20.f);

            std::string playStr = "未游玩";
        if (!m_isEmpty)
        {
            if (m_gameEntry.playTime > 0)
            {
                int h = m_gameEntry.playTime / 3600;
                int min = (m_gameEntry.playTime % 3600) / 60;
                playStr = "游玩时间: " + std::to_string(h) + "时" + std::to_string(min) + "分";
            }
        }
        m_playTimeLabel->setText(m_isEmpty ? " " : playStr);
        this->addView(m_playTimeLabel);

            // 上次打开时间标签（左对齐，焦点时从右滑入）
            m_lastPlayedLabel = new brls::Label();
            m_lastPlayedLabel->setHeight(20.f);
            m_lastPlayedLabel->setWidth(CARD_WIDTH_SWITCH);
            m_lastPlayedLabel->setFontSize(16.f);
            m_lastPlayedLabel->setTextColor(nvgRGBA(200, 200, 200, 200));
            m_lastPlayedLabel->setSingleLine(true);
            m_lastPlayedLabel->setAnimated(true);
            m_lastPlayedLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
            m_lastPlayedLabel->setVisibility(brls::Visibility::INVISIBLE);
            m_lastPlayedLabel->setMarginLeft(20.f);
            m_lastPlayedLabel->setMarginTop(10.f);
            std::string timeStr = m_gameEntry.lastPlayed.empty()
                ? "上次打开: 从未"
                : "上次打开: " + beiklive::tools::formatTimestampForDisplay(m_gameEntry.lastPlayed);
        m_lastPlayedLabel->setText(m_isEmpty ? " " : timeStr);
        this->addView(m_lastPlayedLabel);
        
        this->registerAction(
            "打开",
            brls::BUTTON_A,
            [this](brls::View *)
            {
                if (m_isEmpty) return true;
                brls::Application::notify("正在启动 " + m_gameEntry.title + "...");
                triggerClickBounce();
                return true;
            },
            false,
            false,
            brls::SOUND_CLICK);

        if (!m_isEmpty)
        {
            this->registerAction(
                "收藏",
                brls::BUTTON_RT,
                [this](brls::View *)
                {
                    _toggleFavourite();
                    return true;
                },
                false,
                false,
                brls::SOUND_CLICK);
        }
    }

    void GameCard::onChildFocusGained(brls::View *directChild, brls::View *focusedView)
    {
        brls::Box::onChildFocusGained(directChild, focusedView);

        if (m_isEmpty) return;

        if (m_titleLabel)
            m_titleLabel->setVisibility(brls::Visibility::VISIBLE);

        if (m_playTimeLabel)
            m_playTimeLabel->setVisibility(brls::Visibility::VISIBLE);
        if (m_lastPlayedLabel)
            m_lastPlayedLabel->setVisibility(brls::Visibility::VISIBLE);

        // 启动信息标签滑入动画
        m_infoAnimating = true;
        m_infoT         = 0.0f;
        m_infoOffset    = 300.f;

        _updateFavouriteHint();
        invalidate();
    }

    void GameCard::onChildFocusLost(brls::View *directChild, brls::View *focusedView)
    {
        brls::Box::onChildFocusLost(directChild, focusedView);

        if (m_isEmpty) return;

        if (m_titleLabel)
            m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
        if (m_playTimeLabel)
            m_playTimeLabel->setVisibility(brls::Visibility::INVISIBLE);
        if (m_lastPlayedLabel)
            m_lastPlayedLabel->setVisibility(brls::Visibility::INVISIBLE);
    }

    void GameCard::draw(NVGcontext *vg, float x, float y, float w, float h,
                        brls::Style style, brls::FrameContext *ctx)
    {
        // 入场动画
        if (m_enterAnimating)
        {
            m_enterT += 1.0f / 120.0f;
            float duration = 0.35f;
            float t = m_enterT / duration;
            if (t >= 1.0f)
            {
                m_enterScale     = 1.0f;
                m_enterAnimating = false;
            }
            else
            {
                float overshoot = 1.2f;
                float p = t - 1.0f;
                float ease = 1.0f + overshoot * (p * p * p + p * p);
                float start = 0.75f;
                m_enterScale = start + (1.0f - start) * ease;
            }
            invalidate();
        }

        // 信息标签滑入动画
        if (m_infoAnimating)
        {
            m_infoT += 1.0f / 120.0f;
            float t = m_infoT / INFO_ANIM_DURATION;
            if (t >= 1.0f)
            {
                m_infoOffset    = 0.f;
                m_infoAnimating = false;
            }
            else
            {
                // ease out quad
                m_infoOffset = 300.f * (1.f - t) * (1.f - t);
            }
            invalidate();
        }

        // 点击动画
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
                    m_clickScale     = 1.0f;
                    m_clickAnimating = false;
                    if (onCardClicked)
                        onCardClicked(m_gameEntry);
                }
            }
            invalidate();
        }

        float finalScale = m_scale * m_clickScale * m_enterScale;

        const float cx = x + w * 0.5f;
        const float cy = y + h * 0.5f;

        nvgSave(vg);
        nvgTranslate(vg, cx, cy);
        nvgScale(vg, finalScale, finalScale);
        nvgTranslate(vg, -cx, -cy);

        // 先绘制子视图，再叠加信息标签的滑动偏移
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        // 应用信息标签滑入偏移
        if (m_infoAnimating && m_infoOffset > 0.f && !m_isEmpty)
        {
            if (m_playTimeLabel && m_playTimeLabel->getVisibility() == brls::Visibility::VISIBLE)
                m_playTimeLabel->setTranslationX(m_infoOffset);
            if (m_lastPlayedLabel && m_lastPlayedLabel->getVisibility() == brls::Visibility::VISIBLE)
                m_lastPlayedLabel->setTranslationX(m_infoOffset);
        }

        nvgRestore(vg);
    }

} // namespace beiklive
