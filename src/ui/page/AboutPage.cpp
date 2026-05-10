#include "ui/page/AboutPage.hpp"

namespace beiklive
{
    AboutPage::AboutPage()
    {
        brls::sync([this]()
        {
            this->showFooter(true);
            this->showHeader(false);
            buildUI();
        });
    }

    void AboutPage::buildUI()
    {


        auto *contentBox = this->getContentBox();
        contentBox->setAxis(brls::Axis::COLUMN);
        contentBox->setAlignItems(brls::AlignItems::STRETCH);
        contentBox->setJustifyContent(brls::JustifyContent::CENTER);
        contentBox->addView(new brls::Padding());

        auto *authorCard = new brls::Box(brls::Axis::ROW);
        authorCard->setCornerRadius(16.f);
        authorCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
        authorCard->setShadowVisibility(true);
        authorCard->setShadowType(brls::ShadowType::GENERIC);
        authorCard->setPadding(24.f, 36.f, 24.f, 36.f);
        authorCard->setAlignItems(brls::AlignItems::CENTER);
        authorCard->setFocusable(true);
        authorCard->setHideHighlightBackground(true);
        authorCard->setHideHighlightBorder(true);
        authorCard->setHeight(brls::View::AUTO);

        m_authorImage = new brls::Image();
        m_authorImage->setImageFromFile(BK_RES("img/beiklive.png"));
        m_authorImage->setWidth(80.f);
        m_authorImage->setHeight(80.f);
        m_authorImage->setCornerRadius(40.f);
        m_authorImage->setScalingType(brls::ImageScalingType::FIT);
        m_authorImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_authorImage->setFocusable(false);
        m_authorImage->setMarginRight(30.f);

        auto *infoBox = new brls::Box(brls::Axis::COLUMN);
        infoBox->setAlignItems(brls::AlignItems::FLEX_START);
        infoBox->setJustifyContent(brls::JustifyContent::CENTER);
        infoBox->setFocusable(false);

        m_authorName = new brls::Label();
        m_authorName->setText("beiklive");
        m_authorName->setFontSize(28.f);
        m_authorName->setTextColor(GET_THEME_COLOR("brls/text"));
        m_authorName->setMarginBottom(16.f);
        m_authorName->setFocusable(false);

        m_githubLabel = new brls::Label();
        m_githubLabel->setText("GitHub:  https://github.com/beiklive/GBAStation");
        m_githubLabel->setFontSize(18.f);
        m_githubLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_githubLabel->setFocusable(false);

        auto *githubBadge = new brls::Box(brls::Axis::ROW);
        githubBadge->setCornerRadius(8.f);
        githubBadge->setBackgroundColor(nvgRGBA(79, 193, 255, 30));
        githubBadge->setPadding(6.f, 12.f, 6.f, 12.f);
        githubBadge->setMarginBottom(10.f);
        githubBadge->setFocusable(false);
        githubBadge->setHideHighlightBackground(true);
        githubBadge->addView(m_githubLabel);

        m_biliLabel = new brls::Label();
        m_biliLabel->setText("BiliBili:   BEIKLIVE");
        m_biliLabel->setFontSize(18.f);
        m_biliLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_biliLabel->setFocusable(false);

        auto *biliBadge = new brls::Box(brls::Axis::ROW);
        biliBadge->setCornerRadius(8.f);
        biliBadge->setBackgroundColor(nvgRGBA(0, 168, 107, 30));
        biliBadge->setPadding(6.f, 12.f, 6.f, 12.f);
        biliBadge->setFocusable(false);
        biliBadge->setHideHighlightBackground(true);
        biliBadge->addView(m_biliLabel);

        infoBox->addView(m_authorName);
        infoBox->addView(githubBadge);
        infoBox->addView(biliBadge);

        authorCard->addView(m_authorImage);
        authorCard->addView(infoBox);
        contentBox->addView(authorCard);

        auto *sectionHeader = new brls::Header();
        sectionHeader->setTitle("关于本项目");
        sectionHeader->setMarginTop(30.f);
        sectionHeader->setMarginBottom(15.f);
        contentBox->addView(sectionHeader);

        auto *descCard = new brls::Box(brls::Axis::COLUMN);
        descCard->setCornerRadius(16.f);
        descCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
        descCard->setShadowVisibility(true);
        descCard->setShadowType(brls::ShadowType::GENERIC);
        descCard->setPadding(20.f, 24.f, 20.f, 24.f);
        descCard->setFocusable(false);
        descCard->setHideHighlightBackground(true);
        descCard->setHideHighlightBorder(true);
        descCard->setHeight(brls::View::AUTO);

        std::vector<std::string> descLines = {
            "本项目基于 libretro 核心接口构建，目前内置 mGBA 模拟器核心。",
            "",
            "目前已实现功能：",
            "  •  游戏库功能（运行过的游戏会被自动添加到游戏库中）",
            "  •  定时存档功能",
            "  •  键位自定义",
            "  •  金手指功能",
            "  •  封面设置",
            "  •  游戏时间统计",
            "  •  RA 着色器及参数修改支持（还不完善）",
            "  •  遮罩功能",
            "  •  RA 游戏库导入",
            "  •  快进倒带"
        };

        for (const auto &line : descLines)
        {
            auto *label = new brls::Label();
            label->setText(line);
            label->setFontSize(20.f);
            label->setHeight(line.empty() ? 8.f : 26.f);
            label->setWidth(brls::View::AUTO);
            label->setTextColor(GET_THEME_COLOR("brls/text"));
            label->setFocusable(false);
            descCard->addView(label);
        }

        contentBox->addView(descCard);
        contentBox->addView(new brls::Padding());
    }
} // namespace beiklive
