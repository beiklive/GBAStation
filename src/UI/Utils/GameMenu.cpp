#include "UI/Utils/GameMenu.hpp"
#include "UI/Pages/FileListPage.hpp"

using beiklive::cfgGetBool;
using beiklive::cfgGetStr;
using beiklive::cfgSetStr;
using beiklive::cfgSetBool;

/// 金手指滚动面板最大高度（像素）。
static constexpr float CHEAT_SCROLL_HEIGHT   = 400.0f;
/// 画面设置面板最大高度（像素）。
static constexpr float DISPLAY_SCROLL_HEIGHT = 400.0f;

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

    // Button_B：隐藏菜单，返回游戏
    registerAction("返回游戏", brls::BUTTON_B, [this](brls::View* v) {
        setVisibility(brls::Visibility::GONE);
        if (m_closeCallback) m_closeCallback();
        return true;
    });

    auto* mainbox = new brls::Box(brls::Axis::ROW);
    mainbox->setWidthPercentage(100.0f);
    auto* leftBox = new brls::Box(brls::Axis::COLUMN);
    leftBox->setWidthPercentage(30.0f);
    auto* rightBox = new brls::Box(brls::Axis::COLUMN);
    rightBox->setWidthPercentage(70.0f);
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
        // 金手指面板：ScrollingFrame 限高，避免内容溢出后焦点丢失
        m_cheatScrollFrame = new brls::ScrollingFrame();
        m_cheatScrollFrame->setVisibility(brls::Visibility::GONE);
        m_cheatScrollFrame->setHeight(CHEAT_SCROLL_HEIGHT);
        m_cheatScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_cheatItemBox = new brls::Box(brls::Axis::COLUMN);
        m_cheatScrollFrame->setContentView(m_cheatItemBox);
        btn2->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
            if (m_cheatScrollFrame->getVisibility() == brls::Visibility::GONE) {
                // 关闭画面设置面板，打开金手指面板
                if (m_displayScrollFrame)
                    m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
                m_cheatScrollFrame->setVisibility(brls::Visibility::VISIBLE);
            } else {
                m_cheatScrollFrame->setVisibility(brls::Visibility::GONE);
            }
            return true;
        });

        leftBox->addView(btn2);
        rightBox->addView(m_cheatScrollFrame);

        // ---- 画面设置按钮 ----
        auto* btnDisplay = new brls::Button();
        btnDisplay->setText("画面设置");

        // ---- 构建画面设置面板 ----
        m_displayScrollFrame = new brls::ScrollingFrame();
        m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
        m_displayScrollFrame->setHeight(DISPLAY_SCROLL_HEIGHT);
        m_displayScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

        auto* displayBox = new brls::Box(brls::Axis::COLUMN);
        displayBox->setPadding(10.f, 20.f, 10.f, 20.f);

        // --- 遮罩设置 header ---
        auto* overlayHeader = new brls::Header();
        overlayHeader->setTitle("beiklive/settings/display/header_overlay"_i18n);
        displayBox->addView(overlayHeader);

        // --- 遮罩开关 ---
        auto* overlayEnCell = new brls::BooleanCell();
        overlayEnCell->init("beiklive/settings/display/overlay_enable"_i18n,
                            cfgGetBool(KEY_DISPLAY_OVERLAY_ENABLED, false),
                            [](bool v) { cfgSetBool(KEY_DISPLAY_OVERLAY_ENABLED, v); });
        displayBox->addView(overlayEnCell);

        // --- GBA 遮罩路径选择（平台为 GBA 时显示） ---
        m_overlayGbaPathCell = new brls::DetailCell();
        m_overlayGbaPathCell->setText("beiklive/settings/display/overlay_gba_path"_i18n);
        m_overlayGbaPathCell->setDetailText(beiklive::string::extractFileName(
            cfgGetStr(KEY_DISPLAY_OVERLAY_GBA_PATH,
                      "beiklive/settings/display/overlay_not_set"_i18n)));
        {
            auto* cell    = m_overlayGbaPathCell;
            const std::string cfgKey = KEY_DISPLAY_OVERLAY_GBA_PATH;
            cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
                [cell, cfgKey](brls::View*) {
                    auto* flPage = new FileListPage();
                    flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
                    flPage->setDefaultFileCallback([cell, cfgKey](const FileListItem& item) {
                        cfgSetStr(cfgKey, item.fullPath);
                        cell->setDetailText(beiklive::string::extractFileName(item.fullPath));
                        brls::Application::popActivity();
                    });
                    std::string startPath = cfgGetStr(cfgKey, "");
                    if (!startPath.empty()) {
                        auto pos = startPath.rfind('/');
#ifdef _WIN32
                        auto posW = startPath.rfind('\\');
                        if (posW != std::string::npos &&
                            (pos == std::string::npos || posW > pos))
                            pos = posW;
#endif
                        if (pos != std::string::npos)
                            startPath = startPath.substr(0, pos);
                    }
                    if (startPath.empty()) startPath = "/";
                    flPage->navigateTo(startPath);
                    auto* container = new brls::Box(brls::Axis::COLUMN);
                    container->setGrow(1.0f);
                    container->addView(flPage);
                    container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                        [](brls::View*) { brls::Application::popActivity(); return true; });
                    auto* frame = new brls::AppletFrame(container);
                    frame->setHeaderVisibility(brls::Visibility::GONE);
                    frame->setFooterVisibility(brls::Visibility::GONE);
                    frame->setBackground(brls::ViewBackground::NONE);
                    brls::Application::pushActivity(new brls::Activity(frame));
                    return true;
                }, false, false, brls::SOUND_CLICK);
        }
        displayBox->addView(m_overlayGbaPathCell);

        // --- GBC 遮罩路径选择（平台为 GB 时显示） ---
        m_overlayGbcPathCell = new brls::DetailCell();
        m_overlayGbcPathCell->setText("beiklive/settings/display/overlay_gbc_path"_i18n);
        m_overlayGbcPathCell->setDetailText(beiklive::string::extractFileName(
            cfgGetStr(KEY_DISPLAY_OVERLAY_GBC_PATH,
                      "beiklive/settings/display/overlay_not_set"_i18n)));
        {
            auto* cell    = m_overlayGbcPathCell;
            const std::string cfgKey = KEY_DISPLAY_OVERLAY_GBC_PATH;
            cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
                [cell, cfgKey](brls::View*) {
                    auto* flPage = new FileListPage();
                    flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
                    flPage->setDefaultFileCallback([cell, cfgKey](const FileListItem& item) {
                        cfgSetStr(cfgKey, item.fullPath);
                        cell->setDetailText(beiklive::string::extractFileName(item.fullPath));
                        brls::Application::popActivity();
                    });
                    std::string startPath = cfgGetStr(cfgKey, "");
                    if (!startPath.empty()) {
                        auto pos = startPath.rfind('/');
#ifdef _WIN32
                        auto posW = startPath.rfind('\\');
                        if (posW != std::string::npos &&
                            (pos == std::string::npos || posW > pos))
                            pos = posW;
#endif
                        if (pos != std::string::npos)
                            startPath = startPath.substr(0, pos);
                    }
                    if (startPath.empty()) startPath = "/";
                    flPage->navigateTo(startPath);
                    auto* container = new brls::Box(brls::Axis::COLUMN);
                    container->setGrow(1.0f);
                    container->addView(flPage);
                    container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                        [](brls::View*) { brls::Application::popActivity(); return true; });
                    auto* frame = new brls::AppletFrame(container);
                    frame->setHeaderVisibility(brls::Visibility::GONE);
                    frame->setFooterVisibility(brls::Visibility::GONE);
                    frame->setBackground(brls::ViewBackground::NONE);
                    brls::Application::pushActivity(new brls::Activity(frame));
                    return true;
                }, false, false, brls::SOUND_CLICK);
        }
        displayBox->addView(m_overlayGbcPathCell);

        // --- 着色器设置 header ---
        auto* shaderHeader = new brls::Header();
        shaderHeader->setTitle("beiklive/settings/display/header_shader"_i18n);
        displayBox->addView(shaderHeader);

        // --- 着色器开关（占位） ---
        auto* shaderEnCell = new brls::BooleanCell();
        shaderEnCell->init("beiklive/settings/display/shader_enable"_i18n, false,
                           [](bool) { /* 功能待实现 */ });
        displayBox->addView(shaderEnCell);

        // --- 着色器选择（占位） ---
        auto* shaderSelectCell = new brls::DetailCell();
        shaderSelectCell->setText("beiklive/settings/display/shader_select"_i18n);
        shaderSelectCell->setDetailText("beiklive/settings/display/not_implemented"_i18n);
        displayBox->addView(shaderSelectCell);

        // --- 着色器参数设置（占位） ---
        auto* shaderParamsBtn = new brls::Button();
        shaderParamsBtn->setText("beiklive/settings/display/shader_params"_i18n);
        displayBox->addView(shaderParamsBtn);

        m_displayScrollFrame->setContentView(displayBox);
        rightBox->addView(m_displayScrollFrame);

        // 绑定"画面设置"按钮动作
        btnDisplay->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
            if (m_displayScrollFrame->getVisibility() == brls::Visibility::GONE) {
                // 关闭金手指面板，打开画面设置面板
                if (m_cheatScrollFrame)
                    m_cheatScrollFrame->setVisibility(brls::Visibility::GONE);
                m_displayScrollFrame->setVisibility(brls::Visibility::VISIBLE);
            } else {
                m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
            }
            return true;
        });
        leftBox->addView(btnDisplay);

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

    // 初始化 cheatItemBox：默认提示无金手指
    auto* noCheatLabel = new brls::Label();
    noCheatLabel->setText("无金手指");
    m_cheatItemBox->addView(noCheatLabel);
}

GameMenu::~GameMenu()
{
    bklog::debug("GameMenu destructor");
}

void GameMenu::setPlatform(beiklive::EmuPlatform platform)
{
    m_platform = platform;
    if (!m_overlayGbaPathCell || !m_overlayGbcPathCell) return;

    // 根据游戏平台显示对应遮罩路径选择项
    switch (m_platform) {
        case beiklive::EmuPlatform::GBA:
            m_overlayGbaPathCell->setVisibility(brls::Visibility::VISIBLE);
            m_overlayGbcPathCell->setVisibility(brls::Visibility::GONE);
            break;
        case beiklive::EmuPlatform::GB:
            m_overlayGbaPathCell->setVisibility(brls::Visibility::GONE);
            m_overlayGbcPathCell->setVisibility(brls::Visibility::VISIBLE);
            break;
        default:
            // 平台未知时两者均不显示
            m_overlayGbaPathCell->setVisibility(brls::Visibility::GONE);
            m_overlayGbcPathCell->setVisibility(brls::Visibility::GONE);
            break;
    }
}

void GameMenu::setCheats(const std::vector<CheatEntry>& cheats)
{
    m_cheats = cheats;

    if (!m_cheatItemBox) return;

    // 清空旧内容
    m_cheatItemBox->clearViews(true);

    if (m_cheats.empty()) {
        // 无金手指：显示提示标签
        auto* label = new brls::Label();
        label->setText("无金手指");
        m_cheatItemBox->addView(label);
        return;
    }

    // 逐条添加金手指 BooleanCell
    brls::BooleanCell* firstCell = nullptr;
    brls::BooleanCell* lastCell  = nullptr;

    for (int i = 0; i < static_cast<int>(m_cheats.size()); ++i) {
        auto* cell = new brls::BooleanCell();
        cell->init(m_cheats[i].desc, m_cheats[i].enabled,
            [this, i](bool v) {
                m_cheats[i].enabled = v;
                if (m_cheatToggleCallback)
                    m_cheatToggleCallback(i, v);
            });
        m_cheatItemBox->addView(cell);
        if (i == 0) firstCell = cell;
        lastCell = cell;
    }

    // 循环导航：首条按上键到末条，末条按下键到首条
    if (firstCell && lastCell && firstCell != lastCell) {
        firstCell->setCustomNavigationRoute(brls::FocusDirection::UP, lastCell);
        lastCell->setCustomNavigationRoute(brls::FocusDirection::DOWN, firstCell);
    }
}

