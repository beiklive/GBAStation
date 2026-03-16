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
    mainbox->setWidthPercentage(100.0f);
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
        m_cheatbox = new brls::Box(brls::Axis::COLUMN);
        m_cheatbox->setVisibility(brls::Visibility::GONE);
        btn2->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
            if (m_cheatbox->getVisibility() == brls::Visibility::GONE) {
                m_cheatbox->setVisibility(brls::Visibility::VISIBLE);
            } else {
                m_cheatbox->setVisibility(brls::Visibility::GONE);
            }
            return true;
        });

        leftBox->addView(btn2);
        rightBox->addView(m_cheatbox);

        auto* btn3 = new brls::Button();
        btn3->setText("退出游戏");
        btn3->registerAction("", brls::BUTTON_A, [](brls::View* v) {
            // 直接退出整个 Activity，返回游戏列表
            brls::Application::popActivity();
            return true;
        });
        leftBox->addView(btn3);
        leftBox->addView(new brls::Padding());

        // ---- 修复焦点越界问题：最顶部按钮按上键循环到最底部按钮，反之亦然 ----
        btn->setCustomNavigationRoute(brls::FocusDirection::UP, btn3);
        btn3->setCustomNavigationRoute(brls::FocusDirection::DOWN, btn);
    }

    addView(mainbox);

    auto* bottomBar = new brls::BottomBar();
    bottomBar->setWidthPercentage(100.0f);
    addView(bottomBar);

    // 初始化 cheatbox：默认提示无金手指
    auto* noCheatLabel = new brls::Label();
    noCheatLabel->setText("无金手指");
    m_cheatbox->addView(noCheatLabel);
}

GameMenu::~GameMenu()
{
    bklog::debug("GameMenu destructor");
}

void GameMenu::setCheats(const std::vector<CheatEntry>& cheats)
{
    m_cheats = cheats;

    if (!m_cheatbox) return;

    // 清空旧内容
    m_cheatbox->clearViews(true);

    if (m_cheats.empty()) {
        // 无金手指：显示提示标签
        auto* label = new brls::Label();
        label->setText("无金手指");
        m_cheatbox->addView(label);
        return;
    }

    // 逐条添加金手指切换按钮
    for (int i = 0; i < static_cast<int>(m_cheats.size()); ++i) {
        auto* toggleBtn = new brls::Button();
        const std::string onText  = m_cheats[i].desc + " (开)";
        const std::string offText = m_cheats[i].desc + " (关)";
        toggleBtn->setText(m_cheats[i].enabled ? onText : offText);

        toggleBtn->registerAction("", brls::BUTTON_A,
            [this, i, onText, offText](brls::View* v) {
                // 切换启用状态
                m_cheats[i].enabled = !m_cheats[i].enabled;
                // 更新按钮文字
                static_cast<brls::Button*>(v)->setText(
                    m_cheats[i].enabled ? onText : offText);
                // 通知 GameView 应用更改
                if (m_cheatToggleCallback)
                    m_cheatToggleCallback(i, m_cheats[i].enabled);
                return true;
            });

        m_cheatbox->addView(toggleBtn);
    }
}

