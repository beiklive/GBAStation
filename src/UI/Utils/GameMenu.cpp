#include "UI/Utils/GameMenu.hpp"

GameMenu::GameMenu()
{
    bklog::debug("GameMenu constructor");
    #undef ABSOLUTE
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setJustifyContent(brls::JustifyContent::CENTER);
    setPositionType(brls::PositionType::ABSOLUTE);
    setPositionTop(0);
    setPositionLeft(0);
    setWidthPercentage(100);
    setHeightPercentage(100);
    setBackgroundColor(nvgRGBA(0, 0, 0, 200));

    setHideHighlight(true);
    setHideClickAnimation(true);
    setHideHighlightBorder(true);
    setHideHighlightBackground(true);

    auto* mainbox = new brls::Box(brls::Axis::ROW);
    auto* leftBox = new brls::Box(brls::Axis::COLUMN);
    auto* rightBox = new brls::Box(brls::Axis::COLUMN);
    mainbox->addView(leftBox);
    mainbox->addView(rightBox);
    // 左边栏
    {
        auto* label = new brls::Label();
        label->setText("Game Menu");
        leftBox->addView(label);
        leftBox->addView(new brls::Padding());

        auto* btn = new brls::Button();
        btn->setText("返回游戏");
        btn->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
            // 隐藏整个菜单（this 为 GameMenu，v 为按钮本身）
            setVisibility(brls::Visibility::GONE);
            if (m_closeCallback) m_closeCallback();
            return true;
        });
        leftBox->addView(btn);

        auto* btn2 = new brls::Button();
        btn2->setText("金手指");
        auto* cheatbox = new brls::Box(brls::Axis::COLUMN);
        cheatbox->setVisibility(brls::Visibility::GONE);
        btn2->registerAction("", brls::BUTTON_A, [cheatbox](brls::View* v) {
            if (cheatbox->getVisibility() == brls::Visibility::GONE) {
                cheatbox->setVisibility(brls::Visibility::VISIBLE);
            } else {
                cheatbox->setVisibility(brls::Visibility::GONE);
            }
            return true;
        });

        leftBox->addView(btn2);
        rightBox->addView(cheatbox);

        auto* btn3 = new brls::Button();
        btn3->setText("退出游戏");
        btn3->registerAction("", brls::BUTTON_A, [](brls::View* v) {
            // 直接退出整个 Activity，返回游戏列表
            brls::Application::popActivity();
            return true;
        });
        leftBox->addView(btn3);
        leftBox->addView(new brls::Padding());
    }

    addView(mainbox);

    addView(new brls::BottomBar());

}

GameMenu::~GameMenu()
{
    bklog::debug("GameMenu destructor");
}

