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

    void GameCard::setGameEntry(beiklive::GameEntry gameEntry, bool loadCover)
    {
        m_gameEntry = std::move(gameEntry);
        m_isEmpty = m_gameEntry.path.empty();

        if (m_titleLabel)
        {
            m_titleLabel->setText(m_isEmpty ? " " : m_gameEntry.title);
            m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
        }

        std::string playStr = "未游玩";
        if (!m_isEmpty && m_gameEntry.playTime > 0)
        {
            int h = m_gameEntry.playTime / 3600;
            int min = (m_gameEntry.playTime % 3600) / 60;
            playStr = "游玩时间: " + std::to_string(h) + "时" + std::to_string(min) + "分";
        }
        if (m_playTimeLabel)
        {
            m_playTimeLabel->setText(m_isEmpty ? " " : playStr);
            m_playTimeLabel->setVisibility(brls::Visibility::INVISIBLE);
        }

        std::string timeStr = m_gameEntry.lastPlayed.empty()
            ? "上次打开: 从未"
            : "上次打开: " + beiklive::tools::formatTimestampForDisplay(m_gameEntry.lastPlayed);
        if (m_lastPlayedLabel)
        {
            m_lastPlayedLabel->setText(m_isEmpty ? " " : timeStr);
            m_lastPlayedLabel->setVisibility(brls::Visibility::INVISIBLE);
        }

        m_infoAnimating = false;
        m_infoOffset = 0.f;
        if (m_playTimeLabel)
            m_playTimeLabel->setTranslationX(0.f);
        if (m_lastPlayedLabel)
            m_lastPlayedLabel->setTranslationX(0.f);

        if (m_coverImage)
        {
            if (m_isEmpty || m_gameEntry.logoPath.empty())
                m_coverImage->clear();
            else if (loadCover)
                loadCoverImage(m_gameEntry.logoPath);
        }

        invalidate();
    }

    void GameCard::updateLogo(const std::string &logoPath)
    {
        if (m_coverImage)
        {
            if (beiklive::g_forceRefreshPaths.erase(logoPath) > 0)
                m_coverImage->setImageFromFileForce(logoPath);
            else
                m_coverImage->setImageFromFile(logoPath);
        }
    }

    void GameCard::loadCoverImage(const std::string &logoPath)
    {
        if (!m_coverImage) return;
        if (logoPath.empty())
        {
            m_coverImage->clear();
            return;
        }
        m_coverImage->setImageFromFileForce(logoPath);
    }

    void GameCard::setLogoLayer(const std::string &path, bool visible)
    {
        if (!m_imageLayer) return;
        if (visible && !path.empty())
            m_imageLayer->setImageFromFileForce(path);
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
        if (!m_gameEntry.path.empty() && beiklive::GameDB)
        {
            beiklive::GameDB->set(m_gameEntry.path, "favourite", nlohmann::json(m_gameEntry.favourite));
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
        logobox->setHighlightCornerRadius(18.f);
        logobox->setShadowVisibility(true);
        logobox->setShadowType(brls::ShadowType::GENERIC);
        logobox->setBackgroundColor(nvgRGBA(128, 128, 128, 12));
        logobox->setCornerRadius(15.f);
        logobox->setBorderColor(nvgRGBA(125, 125, 125, 95));
        logobox->setBorderThickness(1.5f);

        m_coverImage = new brls::Image();
        m_coverImage->setWidth(COVER_WIDTH_SWITCH);
        m_coverImage->setFocusable(false);
        m_coverImage->setHeight(COVER_HEIGHT_SWITCH);
        m_coverImage->setScalingType(brls::ImageScalingType::FILL);
        m_coverImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_coverImage->setHighlightPadding(3.f);
        m_coverImage->setHideHighlightBackground(true);

        m_coverImage->setHighlightCornerRadius(18.f);
        m_coverImage->setCornerRadius(15.f);

        m_coverImage->clear();

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
            // this->registerAction(
            //     "收藏",
            //     brls::BUTTON_RT,
            //     [this](brls::View *)
            //     {
            //         _toggleFavourite();
            //         return true;
            //     },
            //     false,
            //     false,
            //     brls::SOUND_CLICK);
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

        // _updateFavouriteHint();
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

        float finalScale = m_scale * m_clickScale;

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
