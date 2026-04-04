#include "GridItem.hpp"

namespace beiklive
{

    // ============================================================
    // 构造 / 析构
    // ============================================================

    GridItem::GridItem(GridItemMode mode, int index)
        : m_mode(mode)
        , m_index(index)
    {
        // 自适应宽度（由父 GridBox 确定），固定高度
        this->setAxis(brls::Axis::ROW);
        this->setGrow(1.f);
        this->setHeight(ITEM_HEIGHT);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setFocusable(true);
        // 设置边框和阴影
        this->setBorderColor(nvgRGBA(128, 128, 128, 120));
        this->setBorderThickness(1.f);
        this->setCornerRadius(3.f);
        this->setShadowVisibility(true);
        this->setShadowType(brls::ShadowType::GENERIC);
        this->setHideHighlightBackground(true);
        this->setHighlightCornerRadius(0.f);
        this->setBackground(brls::ViewBackground::NONE);
        // HIDE_BRLS_HIGHLIGHT(this);

        // 注册 A 键点击动作
        this->registerAction(
            "确认",
            brls::BUTTON_A,
            [this](brls::View*) -> bool
            {
                if (onItemClicked)
                    onItemClicked(m_index);
                return true;
            },
            false,
            false,
            brls::SOUND_CLICK);

        _initLayout();
    }

    void GridItem::draw(NVGcontext *vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext *ctx)
    {
        // 绘制背景（如果有的话）
        brls::Box::draw(vg, x, y, width, height, style, ctx);
        // 绘制边框
    }

    // ============================================================
    // _initLayout – 构建控件布局
    // ============================================================

    void GridItem::_initLayout()
    {
        // ─── 空状态 Label ───────────────────────────────────────────────────
        m_emptyLabel = new brls::Label();
        m_emptyLabel->setFontSize(16.f);
        m_emptyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_emptyLabel->setMarginTop(4.f);
        m_emptyLabel->setHeightPercentage(100.f);
        m_emptyLabel->setWidthPercentage(100.f);
        m_emptyLabel->setSingleLine(true);
        m_emptyLabel->setAnimated(true);
        m_emptyLabel->setAutoAnimate(true);
        this->addView(m_emptyLabel);

        // ─── 有数据主布局（横向）────────────────────────────────────────────
        m_dataLayout = new brls::Box(brls::Axis::ROW);
        m_dataLayout->setWidthPercentage(100.f);
        m_dataLayout->setAlignItems(brls::AlignItems::CENTER);
        m_dataLayout->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_dataLayout->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_dataLayout);

        #undef ABSOLUTE
        // 左侧：正方形封面图（宽 = 高 = ITEM_HEIGHT）
        auto logobox = new brls::Box();
        logobox->setWidth(ITEM_HEIGHT-10);
        logobox->setHeight(ITEM_HEIGHT-10);
        logobox->setMarginLeft(5.f);

        m_image = new brls::Image();
        m_image->setWidth(ITEM_HEIGHT-10);
        m_image->setHeight(ITEM_HEIGHT-10);
        m_image->setScalingType(brls::ImageScalingType::FILL);
        m_image->setFocusable(false);
        m_image->setCornerRadius(3.f);

        m_imageLayer = new brls::Image();
        m_imageLayer->setWidth(ITEM_HEIGHT-10);
        m_imageLayer->setHeight(ITEM_HEIGHT-10);        
        m_imageLayer->setPositionTop(0.f);
        m_imageLayer->setPositionLeft(0.f);
        m_imageLayer->setPositionType(brls::PositionType::ABSOLUTE);
        m_imageLayer->setScalingType(brls::ImageScalingType::FILL);
        m_imageLayer->setFocusable(false);
        m_imageLayer->setVisibility(brls::Visibility::GONE);
        m_imageLayer->setCornerRadius(3.f);

        logobox->addView(m_image);
        logobox->addView(m_imageLayer);
        m_dataLayout->addView(logobox);

        // 右侧：纵向容器
        m_rightBox = new brls::Box(brls::Axis::COLUMN);

        m_rightBox->setGrow(1.f);
        m_rightBox->setAlignItems(brls::AlignItems::FLEX_START);
        // m_rightBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_rightBox->setFocusable(false);
        m_rightBox->setPaddingLeft(10.f);
        m_rightBox->setPaddingRight(8.f);
        HIDE_BRLS_HIGHLIGHT(m_rightBox);

        // Row1：横向容器（徽标 + 标题）
        m_row1 = new brls::Box(brls::Axis::ROW);
        m_row1->setGrow(0.f);
        m_row1->setAlignItems(brls::AlignItems::CENTER);
        m_row1->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_row1->setFocusable(false);
        m_row1->setWidthPercentage(100.f);
        m_row1->setMarginBottom(10.f);
        HIDE_BRLS_HIGHLIGHT(m_row1);

        // 徽标背景框（SAVE_STATE 模式下初始隐藏）
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

        // SAVE_STATE 模式隐藏徽标
        if (m_mode == GridItemMode::SAVE_STATE)
            m_badgeBox->setVisibility(brls::Visibility::GONE);

        m_row1->addView(m_badgeBox);

        // 主标题 Label
        m_titleLabel = new brls::Label();
        m_titleLabel->setFontSize(16.f);
        m_titleLabel->setSingleLine(true);
        m_titleLabel->setAnimated(true);
        m_titleLabel->setAutoAnimate(true);
        m_titleLabel->setGrow(1.f);
        m_titleLabel->setFocusable(false);
        m_row1->addView(m_titleLabel);

        m_rightBox->addView(m_row1);

        // Row2：时间文字
        m_subLabel = new brls::Label();
        m_subLabel->setFontSize(14.f);
        m_subLabel->setTextColor(nvgRGBA(130, 130, 130, 255));
        m_subLabel->setSingleLine(true);
        m_subLabel->setAnimated(true);
        m_subLabel->setAutoAnimate(true);
        m_subLabel->setGrow(0.f);
        // m_subLabel->setMarginBottom(5.f);

        m_subLabel->setFocusable(false);
        
        // Row3：游玩时长（SAVE_STATE 模式下恒为 GONE）
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
        m_rightBox->addView(m_subLabel);

        m_dataLayout->addView(m_rightBox);
        this->addView(m_dataLayout);

        // 初始为空状态：显示 emptyLabel，隐藏 dataLayout
        m_dataLayout->setVisibility(brls::Visibility::GONE);
        m_isEmpty = true;
    }

    // ============================================================
    // 数据设置接口
    // ============================================================

    void GridItem::setImagePath(const std::string& path)
    {
        if (!m_image) return;
        if (!path.empty())
            m_image->setImageFromFile(path);
    }

    void GridItem::setBadge(const std::string& text, PlatformBadgeColor color)
    {
        if (!m_badgeBox || !m_badgeLabel) return;
        if (color == PlatformBadgeColor::NONE || m_mode == GridItemMode::SAVE_STATE)
        {
            m_badgeBox->setVisibility(brls::Visibility::GONE);
            return;
        }
        m_badgeLabel->setText(text);
        m_badgeBox->setBackgroundColor(_getBadgeColor(color));
        m_badgeBox->setVisibility(brls::Visibility::VISIBLE);
    }

    void GridItem::setTitle(const std::string& title)
    {
        if (m_titleLabel)
            m_titleLabel->setText(title);
        // 同步更新空状态 emptyLabel（便于 SAVE_STATE 模式的槽位名称）
        if (m_emptyLabel)
            m_emptyLabel->setText(title);
    }

    void GridItem::setSubText(const std::string& text)
    {
        if (m_subLabel)
            m_subLabel->setText(
                m_mode == GridItemMode::GAME_LIBRARY ? "上次游玩: " + text : "存档时间: " +
                text);
    }

    void GridItem::setPlayTime(const std::string& text)
    {
        if (!m_playLabel) return;
        if (m_mode == GridItemMode::SAVE_STATE) return; // SAVE_STATE 模式不显示
        std::string display = text.empty() ? "未游玩" : "已游玩 " + text;
        m_playLabel->setText(display);
        m_playLabel->setVisibility(brls::Visibility::VISIBLE);
    }

    void GridItem::setEmpty(const std::string& slotName)
    {
        if (m_emptyLabel)
            m_emptyLabel->setText(slotName);
        m_emptyLabel->setVisibility(brls::Visibility::VISIBLE);
        if (m_dataLayout)
            m_dataLayout->setVisibility(brls::Visibility::GONE);
        m_isEmpty = true;
    }

    void GridItem::setDataLoaded()
    {
        m_emptyLabel->setVisibility(brls::Visibility::GONE);
        if (m_dataLayout)
            m_dataLayout->setVisibility(brls::Visibility::VISIBLE);
        m_isEmpty = false;
    }

    // ============================================================
    // 焦点回调（高亮效果）
    // ============================================================

    void GridItem::setImageLayer(const std::string &path, bool visible)
    {
        if (!m_imageLayer) return;
        if (visible && !path.empty())
            m_imageLayer->setImageFromFile(path);
        m_imageLayer->setVisibility(visible ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void GridItem::onFocusGained()
    {
        brls::Box::onFocusGained();
        // this->setBackgroundColor(nvgRGBA(255, 255, 255, 20));
    }

    void GridItem::onFocusLost()
    {
        brls::Box::onFocusLost();
        // this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
    }

    // ============================================================
    // 平台徽标颜色映射
    // ============================================================

    NVGcolor GridItem::_getBadgeColor(PlatformBadgeColor color)
    {
        switch (color)
        {
            case PlatformBadgeColor::GBA:  return nvgRGBA(108, 77,  191, 220); // 紫色
            case PlatformBadgeColor::GBC:  return nvgRGBA(0,   112, 221, 220); // 蓝色
            case PlatformBadgeColor::GB:   return nvgRGBA(0,   168, 107, 220); // 绿色
            default:                       return nvgRGBA(100, 100, 100, 200);
        }
    }

} // namespace beiklive
