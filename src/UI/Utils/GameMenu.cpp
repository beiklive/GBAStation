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

    auto* label = new brls::Label();
    label->setText("Game Menu");
    addView(label);
    addView(new brls::Padding());

    auto* btn = new brls::Button();
    btn->setText("返回游戏");
    btn->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
        // 隐藏整个菜单（this 为 GameMenu，v 为按钮本身）
        setVisibility(brls::Visibility::GONE);
        if (m_closeCallback) m_closeCallback();
        return true;
    });
    addView(btn);

    addView(new brls::Padding());
    addView(new brls::BottomBar());

}

GameMenu::~GameMenu()
{
    bklog::debug("GameMenu destructor");
}

