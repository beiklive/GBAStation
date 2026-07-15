#include "GridItem.hpp"
#include "core/Tools.hpp"

#include <functional>

namespace beiklive
{

    GridItem::GridItem(GridItemMode mode, int index)
        : brls::Box()
        , m_mode(mode)
        , m_index(index)
    {
        this->setAxis(brls::Axis::ROW);
        this->setGrow(1.f);
        this->setHeight(ITEM_HEIGHT);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setBorderColor(nvgRGBA(128, 128, 128, 120));
        this->setBorderThickness(1.f);
        this->setCornerRadius(3.f);
        this->setShadowVisibility(true);
        this->setShadowType(brls::ShadowType::GENERIC);
        this->setHideHighlightBackground(true);
        this->setHighlightCornerRadius(0.f);
        this->setFocusable(true);

        _initLayout();
    }

    void GridItem::draw(NVGcontext* vg, float x, float y, float w, float h,
                         brls::Style style, brls::FrameContext* ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);
    }

    void GridItem::_initLayout()
    {
        m_emptyLabel = new brls::Label();
        m_emptyLabel->setFontSize(16.f);
        m_emptyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_emptyLabel->setMarginTop(4.f);
        m_emptyLabel->setHeightPercentage(100.f);
        m_emptyLabel->setWidthPercentage(100.f);
        m_emptyLabel->setSingleLine(true);
        m_emptyLabel->setAnimated(true);
        m_emptyLabel->setAutoAnimate(true);
        m_emptyLabel->setHideHighlightBackground(true);
        this->addView(m_emptyLabel);

        m_dataLayout = new brls::Box(brls::Axis::ROW);
        m_dataLayout->setWidthPercentage(100.f);
        m_dataLayout->setAlignItems(brls::AlignItems::CENTER);
        m_dataLayout->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_dataLayout->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_dataLayout);

        auto logobox = new brls::Box();
        logobox->setWidth(ITEM_HEIGHT - 10);
        logobox->setHeight(ITEM_HEIGHT - 10);
        logobox->setMarginLeft(5.f);
        logobox->setCornerRadius(3.f);

        m_image = new brls::Image();
        m_image->setWidth(ITEM_HEIGHT - 10);
        m_image->setHeight(ITEM_HEIGHT - 10);
        m_image->setScalingType(brls::ImageScalingType::FILL);
        m_image->setFocusable(false);
        m_image->setCornerRadius(3.f);
        m_image->setImageFromFile(BK_RES("img/ui/menu/empty.png"));

        logobox->addView(m_image);
        m_dataLayout->addView(logobox);

        m_rightBox = new brls::Box(brls::Axis::COLUMN);
        m_rightBox->setGrow(1.f);
        m_rightBox->setAlignItems(brls::AlignItems::FLEX_START);
        m_rightBox->setFocusable(false);
        m_rightBox->setPaddingLeft(10.f);
        m_rightBox->setPaddingRight(8.f);
        HIDE_BRLS_HIGHLIGHT(m_rightBox);

        m_row1 = new brls::Box(brls::Axis::ROW);
        m_row1->setGrow(0.f);
        m_row1->setAlignItems(brls::AlignItems::CENTER);
        m_row1->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_row1->setFocusable(false);
        m_row1->setWidthPercentage(100.f);
        m_row1->setMarginBottom(10.f);
        HIDE_BRLS_HIGHLIGHT(m_row1);

        m_badgeBox = new brls::Box();
        m_badgeBox->setAxis(brls::Axis::ROW);
        m_badgeBox->setAlignItems(brls::AlignItems::CENTER);
        m_badgeBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_badgeBox->setWidth(36.f);
        m_badgeBox->setHeight(20.f);
        m_badgeBox->setCornerRadius(4.f);
        m_badgeBox->setFocusable(false);
        m_badgeBox->setMarginRight(6.f);
        HIDE_BRLS_HIGHLIGHT(m_badgeBox);

        m_badgeLabel = new brls::Label();
        m_badgeLabel->setFontSize(12.f);
        m_badgeLabel->setTextColor(nvgRGBA(255, 255, 255, 255));
        m_badgeLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_badgeLabel->setSingleLine(true);
        m_badgeLabel->setFocusable(false);
        m_badgeBox->addView(m_badgeLabel);

        if (m_mode == GridItemMode::SAVE_STATE)
            m_badgeBox->setVisibility(brls::Visibility::GONE);

        m_row1->addView(m_badgeBox);

        m_titleLabel = new brls::Label();
        m_titleLabel->setFontSize(16.f);
        m_titleLabel->setSingleLine(true);
        m_titleLabel->setAnimated(true);
        m_titleLabel->setAutoAnimate(true);
        m_titleLabel->setGrow(1.f);
        m_titleLabel->setFocusable(false);
        m_row1->addView(m_titleLabel);

        m_rightBox->addView(m_row1);

        m_playLabel = new brls::Label();
        m_playLabel->setFontSize(14.f);
        m_playLabel->setTextColor(nvgRGBA(121, 201, 249, 255));
        m_playLabel->setSingleLine(true);
        m_playLabel->setAnimated(true);
        m_playLabel->setAutoAnimate(true);
        m_playLabel->setGrow(0.f);
        m_playLabel->setMarginBottom(10.f);
        m_playLabel->setFocusable(false);
        if (m_mode == GridItemMode::SAVE_STATE)
            m_playLabel->setVisibility(brls::Visibility::GONE);
        m_rightBox->addView(m_playLabel);

        m_subLabel = new brls::Label();
        m_subLabel->setFontSize(14.f);
        m_subLabel->setTextColor(nvgRGBA(130, 130, 130, 255));
        m_subLabel->setSingleLine(true);
        m_subLabel->setAnimated(true);
        m_subLabel->setAutoAnimate(true);
        m_subLabel->setText("  ");
        m_subLabel->setGrow(0.f);
        m_subLabel->setFocusable(false);
        m_rightBox->addView(m_subLabel);

        m_dataLayout->addView(m_rightBox);
        this->addView(m_dataLayout);

        m_dataLayout->setVisibility(brls::Visibility::GONE);
        m_isEmpty = true;
    }

    void GridItem::setImagePath(const std::string& path)
    {
        if (!m_image) return;
        if (path.empty()) { m_image->clear(); return; }
        if (beiklive::g_forceRefreshPaths.erase(path) > 0)
            m_image->setImageFromFileForce(path);
        else
            m_image->setImageFromFile(path);
    }

    void GridItem::setBadge(const std::string& text, PlatformBadgeColor color)
    {
        if (!m_badgeBox || !m_badgeLabel) return;
        if (color == PlatformBadgeColor::NONE || m_mode == GridItemMode::SAVE_STATE) {
            m_badgeBox->setVisibility(brls::Visibility::GONE);
            return;
        }
        m_badgeLabel->setText(text);
        m_badgeBox->setBackgroundColor(_getBadgeColor(color));
        m_badgeBox->setVisibility(brls::Visibility::VISIBLE);
    }

    void GridItem::setTitle(const std::string& title)
    {
        if (m_titleLabel) m_titleLabel->setText(title);
        if (m_emptyLabel) m_emptyLabel->setText(title);
    }

    void GridItem::setSubText(const std::string& text)
    {
        if (m_subLabel) m_subLabel->setText(
            m_mode == GridItemMode::GAME_LIBRARY ? "上次游玩: " + text : "存档时间: " + text);
    }

    void GridItem::setPlayTime(const std::string& text)
    {
        if (!m_playLabel || m_mode == GridItemMode::SAVE_STATE) return;
        std::string display = text.empty() ? "未游玩" : "已游玩 " + text;
        m_playLabel->setText(display);
        m_playLabel->setVisibility(brls::Visibility::VISIBLE);
    }

    void GridItem::setSubTextEmpty()
    {
        if (m_subLabel) m_subLabel->setText("  ");
    }

    void GridItem::setEmpty(const std::string& slotName)
    {
        if (m_emptyLabel) m_emptyLabel->setText(slotName);
        m_emptyLabel->setVisibility(brls::Visibility::VISIBLE);
        if (m_dataLayout) m_dataLayout->setVisibility(brls::Visibility::GONE);
        m_isEmpty = true;
    }

    void GridItem::setDataLoaded()
    {
        m_emptyLabel->setVisibility(brls::Visibility::GONE);
        if (m_dataLayout) m_dataLayout->setVisibility(brls::Visibility::VISIBLE);
        m_isEmpty = false;
    }

    NVGcolor GridItem::_getBadgeColor(PlatformBadgeColor color)
    {
        switch (color) {
            case PlatformBadgeColor::GBA:     return nvgRGBA(108, 77,  191, 220);
            case PlatformBadgeColor::GBC:     return nvgRGBA(0,   112, 221, 220);
            case PlatformBadgeColor::GB:      return nvgRGBA(0,   168, 107, 220);
            case PlatformBadgeColor::NES:     return nvgRGBA(218, 41,  28,  220);
            case PlatformBadgeColor::SNES:    return nvgRGBA(160, 100, 180, 220);
            case PlatformBadgeColor::NDS:     return nvgRGBA(54,  150, 190, 220);
            case PlatformBadgeColor::THREEDS: return nvgRGBA(230, 79,  91,  220);
            default:                          return nvgRGBA(100, 100, 100, 200);
        }
    }

    void GridItem::populateFromGameEntry(GridDrawItem& item, const GameEntry& entry,
                                          GridItemMode mode)
    {
        item.reset();
        item.populated = true;
        item.empty = false;
        item.title = entry.title.empty() ? entry.path : entry.title;
        item.imagePath = entry.logoPath;

        item.imageLayerPath = GetGameLogoLayerPath(entry.platform);
        item.imageLayerVisible = !item.imageLayerPath.empty();
        item.platformImagePath = entry.logoPath;
        if (static_cast<beiklive::enums::EmuPlatform>(entry.platform) ==
            beiklive::enums::EmuPlatform::EmuNDS) {
            item.platformImagePath = beiklive::GetNdsIconCachePath(entry.path);
            item.platformImageSourcePath = entry.path;
        }

        // 图片载入前使用平台常见封面比例作为占位；纹理就绪后渲染器会优先
        // 使用图片自身的宽高比。这里的值为 width / height。
        switch (static_cast<beiklive::enums::EmuPlatform>(entry.platform)) {
            case beiklive::enums::EmuPlatform::EmuGBA:  item.coverAspect = 1.12f; break;
            case beiklive::enums::EmuPlatform::EmuGBC:  item.coverAspect = 1.00f; break;
            case beiklive::enums::EmuPlatform::EmuGB:   item.coverAspect = 1.00f; break;
            case beiklive::enums::EmuPlatform::EmuNES:  item.coverAspect = 0.73f; break;
            case beiklive::enums::EmuPlatform::EmuSNES: item.coverAspect = 1.00f; break;
            case beiklive::enums::EmuPlatform::EmuNDS:  item.coverAspect = 0.90f; break;
            case beiklive::enums::EmuPlatform::Emu3DS:  item.coverAspect = 0.89f; break;
            default:                                     item.coverAspect = 0.78f; break;
        }

        item.subText = formatSubText(entry, mode);

        if (mode == GridItemMode::GAME_LIBRARY) {
            item.playTime = formatPlayTime(entry.playTime);
        }

        std::string badgeText = beiklive::tools::platformBadgeName(entry.platform);
    switch (static_cast<beiklive::enums::EmuPlatform>(entry.platform)) {
        case beiklive::enums::EmuPlatform::EmuGBA: item.badgeColor = PlatformBadgeColor::GBA; break;
        case beiklive::enums::EmuPlatform::EmuGBC: item.badgeColor = PlatformBadgeColor::GBC; break;
        case beiklive::enums::EmuPlatform::EmuGB:  item.badgeColor = PlatformBadgeColor::GB;  break;
        case beiklive::enums::EmuPlatform::EmuNES: item.badgeColor = PlatformBadgeColor::NES; break;
        case beiklive::enums::EmuPlatform::EmuSNES: item.badgeColor = PlatformBadgeColor::SNES; break;
        case beiklive::enums::EmuPlatform::EmuNDS: item.badgeColor = PlatformBadgeColor::NDS; break;
        case beiklive::enums::EmuPlatform::Emu3DS: item.badgeColor = PlatformBadgeColor::THREEDS; break;
        default: item.badgeColor = PlatformBadgeColor::NONE; break;
    }
        item.badgeText = badgeText;

        item.favorite = entry.favourite;
        item.gameId = static_cast<uint64_t>(std::hash<std::string>{}(entry.path));
    }

    void GridItem::populateEmpty(GridDrawItem& item, const std::string& slotName)
    {
        item.reset();
        item.populated = true;
        item.empty = true;
        item.title = slotName;
    }

    std::string GridItem::formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "";
        return beiklive::tools::formatPlayTime(seconds);
    }

    std::string GridItem::formatSubText(const GameEntry& entry, GridItemMode mode)
    {
        switch (mode) {
            case GridItemMode::GAME_LIBRARY:
                return entry.lastPlayed.empty()
                    ? "从未游玩"
                    : "上次游玩: " + beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
            case GridItemMode::SAVE_STATE:
                return entry.lastPlayed.empty()
                    ? ""
                    : "存档时间: " + beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
        }
        return "";
    }

} // namespace beiklive
