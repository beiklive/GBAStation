#include "UI/Pages/AboutPage.hpp"

AboutPage::AboutPage()
{
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::STRETCH);
    setJustifyContent(brls::JustifyContent::CENTER);
    setBackground(brls::ViewBackground::NONE);
    setGrow(1.0f);

    buildUI();
}

void AboutPage::buildUI()
{
    auto* m_header = new beiklive::UI::BrowserHeader();
    m_header->setTitle("GBAStation");
    m_header->setPath("version 0.1.0");
    addView(m_header);

    // ──────────────────────────────────────────────────────────────────────────
    // 顶部：mgba.png 图片
    // ──────────────────────────────────────────────────────────────────────────
    // auto* topBox = new brls::Box(brls::Axis::COLUMN);
    // topBox->setAlignItems(brls::AlignItems::CENTER);
    // topBox->setJustifyContent(brls::JustifyContent::CENTER);
    // topBox->setHeight(200.f);
    // topBox->setWidth(View::AUTO);
    // topBox->setMarginBottom(20.f);
    // topBox->setFocusable(false);

    // m_topImage = new brls::Image();
    // m_topImage->setImageFromFile(BK_RES("img/mgba.png"));
    // m_topImage->setScalingType(brls::ImageScalingType::FIT);
    // m_topImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    // m_topImage->setHeight(160.f);
    // m_topImage->setWidth(View::AUTO);
    // m_topImage->setFocusable(false);
    // topBox->addView(m_topImage);

    // addView(topBox);

    // ──────────────────────────────────────────────────────────────────────────
    // 中部：项目描述文字
    // ──────────────────────────────────────────────────────────────────────────
    auto* midBox = new brls::Box(brls::Axis::COLUMN);

    midBox->setAlignItems(brls::AlignItems::FLEX_START);
    midBox->setJustifyContent(brls::JustifyContent::CENTER);
    midBox->setGrow(1.0f);
    midBox->setPadding(10.f, 80.f, 10.f, 80.f);
    midBox->setFocusable(true);


    std::vector<std::string> descLines = {
        "本项目基于 libretro 核心接口构建，目前内置 mGBA 模拟器核心。",
        "目前已实现功能:",
        "    - 游戏库功能（运行过的游戏会被自动添加到游戏库中）",
        "    - 存档/截图浏览",
        "    - 定时存档功能",
        "    - 键位自定义",
        "    - 金手指功能",
        "    - 封面设置",
        "    - 游戏时间统计",
        "    - RA着色器及参数修改支持（还不完善）",
        "    - 遮罩功能",
        "    - 快进倒带"
    };

    for(const auto& line : descLines) {
        auto* m_descLabel = new brls::Label();
        m_descLabel->setText(line);
        m_descLabel->setFontSize(20.f);
        m_descLabel->setHeight(24.f);
        m_descLabel->setWidth(brls::View::AUTO);
        m_descLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_descLabel->setFocusable(false);
        midBox->addView(m_descLabel);
    }



    // ──────────────────────────────────────────────────────────────────────────
    // 底部：作者信息 Box（左：头像+名称，右：Github/BiliBili）
    // ──────────────────────────────────────────────────────────────────────────
    auto* bottomBox = new brls::Box(brls::Axis::ROW);
    bottomBox->setAlignItems(brls::AlignItems::CENTER);
    bottomBox->setJustifyContent(brls::JustifyContent::CENTER);
    bottomBox->setWidth(View::AUTO);
    bottomBox->setHeight(160.f);
    bottomBox->setMarginTop(20.f);
    bottomBox->setMarginBottom(5.f);
    bottomBox->setPadding(10.f, 80.f, 10.f, 80.f);
    bottomBox->setCornerRadius(12.f);
    bottomBox->setBackgroundColor(nvgRGBA(40, 40, 40, 0));
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
    m_biliLabel->setText("BiliBili:   BEIKLIVE");
    m_biliLabel->setFontSize(24.f);
    m_biliLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_biliLabel->setFocusable(false);
    linksBox->addView(m_biliLabel);

    bottomBox->addView(linksBox);
    bottomBox->addView(new brls::Padding());
    addView(bottomBox);
    addView(midBox);


    addView(new brls::BottomBar());
}
