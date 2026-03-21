#include "UI/Pages/AboutPage.hpp"

AboutPage::AboutPage()
{
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setJustifyContent(brls::JustifyContent::CENTER);
    setBackground(brls::ViewBackground::NONE);
    setGrow(1.0f);

    buildUI();
}

void AboutPage::buildUI()
{
    // ──────────────────────────────────────────────────────────────────────────
    // 顶部：mgba.png 图片
    // ──────────────────────────────────────────────────────────────────────────
    auto* topBox = new brls::Box(brls::Axis::COLUMN);
    topBox->setAlignItems(brls::AlignItems::CENTER);
    topBox->setJustifyContent(brls::JustifyContent::CENTER);
    topBox->setHeight(200.f);
    topBox->setWidth(View::AUTO);
    topBox->setMarginBottom(20.f);
    topBox->setFocusable(false);

    m_topImage = new brls::Image();
    m_topImage->setImageFromFile(BK_RES("img/mgba.png"));
    m_topImage->setScalingType(brls::ImageScalingType::FIT);
    m_topImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_topImage->setHeight(160.f);
    m_topImage->setWidth(View::AUTO);
    m_topImage->setFocusable(false);
    topBox->addView(m_topImage);

    addView(topBox);

    // ──────────────────────────────────────────────────────────────────────────
    // 中部：项目描述文字
    // ──────────────────────────────────────────────────────────────────────────
    auto* midBox = new brls::Box(brls::Axis::COLUMN);
    midBox->setAlignItems(brls::AlignItems::CENTER);
    midBox->setJustifyContent(brls::JustifyContent::CENTER);
    midBox->setWidth(View::AUTO);
    midBox->setGrow(1.0f);
    midBox->setPadding(10.f, 80.f, 10.f, 80.f);
    midBox->setFocusable(false);

    m_descLabel = new brls::Label();
    m_descLabel->setText(
        "本项目基于 libretro 核心接口构建，当前内置 mGBA 模拟器核心。\n"
        "未来计划支持更多游戏平台，为玩家提供一站式复古游戏体验。");
    m_descLabel->setFontSize(28.f);
    m_descLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_descLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_descLabel->setFocusable(false);
    midBox->addView(m_descLabel);

    addView(midBox);

    // ──────────────────────────────────────────────────────────────────────────
    // 底部：作者信息 Box（左：头像+名称，右：Github/BiliBili）
    // ──────────────────────────────────────────────────────────────────────────
    auto* bottomBox = new brls::Box(brls::Axis::ROW);
    bottomBox->setAlignItems(brls::AlignItems::CENTER);
    bottomBox->setJustifyContent(brls::JustifyContent::CENTER);
    bottomBox->setWidth(View::AUTO);
    bottomBox->setHeight(160.f);
    bottomBox->setMarginTop(20.f);
    bottomBox->setMarginBottom(30.f);
    bottomBox->setPadding(10.f, 40.f, 10.f, 40.f);
    bottomBox->setCornerRadius(12.f);
    bottomBox->setBackgroundColor(nvgRGBA(40, 40, 40, 120));
    bottomBox->setFocusable(false);

    // 左侧：圆形头像 + 作者名称（纵向布局）
    auto* authorBox = new brls::Box(brls::Axis::COLUMN);
    authorBox->setAlignItems(brls::AlignItems::CENTER);
    authorBox->setJustifyContent(brls::JustifyContent::CENTER);
    authorBox->setMarginRight(60.f);
    authorBox->setFocusable(false);

    m_authorImage = new brls::Image();
    m_authorImage->setImageFromFile(BK_RES("img/beiklive.png"));
    m_authorImage->setScalingType(brls::ImageScalingType::FIT);
    m_authorImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_authorImage->setWidth(80.f);
    m_authorImage->setHeight(80.f);
    m_authorImage->setCornerRadius(40.f);  // 圆形头像
    m_authorImage->setFocusable(false);
    authorBox->addView(m_authorImage);

    m_authorName = new brls::Label();
    m_authorName->setText("beiklive");
    m_authorName->setFontSize(24.f);
    m_authorName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_authorName->setTextColor(GET_THEME_COLOR("brls/text"));
    m_authorName->setMarginTop(8.f);
    m_authorName->setFocusable(false);
    authorBox->addView(m_authorName);

    bottomBox->addView(authorBox);

    // 右侧：Github + BiliBili 链接（纵向两行）
    auto* linksBox = new brls::Box(brls::Axis::COLUMN);
    linksBox->setAlignItems(brls::AlignItems::FLEX_START);
    linksBox->setJustifyContent(brls::JustifyContent::CENTER);
    linksBox->setFocusable(false);

    m_githubLabel = new brls::Label();
    m_githubLabel->setText("GitHub:   https://github.com/beiklive/GBAStation");
    m_githubLabel->setFontSize(24.f);
    m_githubLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_githubLabel->setMarginBottom(12.f);
    m_githubLabel->setFocusable(false);
    linksBox->addView(m_githubLabel);

    m_biliLabel = new brls::Label();
    m_biliLabel->setText("BiliBili: BEIKLIVE");
    m_biliLabel->setFontSize(24.f);
    m_biliLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_biliLabel->setFocusable(false);
    linksBox->addView(m_biliLabel);

    bottomBox->addView(linksBox);

    addView(bottomBox);
}
