#include "XMLUI/StartPageView.hpp"

#include "UI/game_view.hpp"

#undef ABSOLUTE // avoid conflict with brls::PositionType::ABSOLUTE

using namespace brls::literals; // for _i18n

// ─────────────────────────────────────────────────────────────────────────────
//  Default ROM root (same as List_view.cpp)
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__SWITCH__)
#define ROOT_PATH "/"
#elif defined(WIN32)
#define ROOT_PATH "F:/games/GBA"
#else
#define ROOT_PATH "/Users/beiklive"
#endif

// ─────────────────────────────────────────────────────────────────────────────

StartPageView::StartPageView()
{
    setAxis(brls::Axis::COLUMN);

    // Background image (absolute positioning, does not participate in layout)
    m_bgImage = new brls::Image();
    m_bgImage->setFocusable(false);
    m_bgImage->setPositionType(brls::PositionType::ABSOLUTE);
    m_bgImage->setPositionTop(0);
    m_bgImage->setPositionLeft(0);
    m_bgImage->setWidthPercentage(100);
    m_bgImage->setHeightPercentage(100);
    m_bgImage->setScalingType(brls::ImageScalingType::FIT);
    m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_bgImage->setImageFromFile(BK_APP_DEFAULT_BG);
    addView(m_bgImage);
}

void StartPageView::Init()
{
    setBackground(brls::ViewBackground::NONE);

    if (!gameRunner)
        return;

    // Swallow BUTTON_B so it never triggers a back-navigation on this page.
    beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_B);

    // Quit action
    gameRunner->uiParams->StartPageframe->registerAction(
        "beiklive/hints/exit"_i18n,
        brls::BUTTON_START,
        [](brls::View*) {
            bklog::debug("Exit app");
            brls::Application::quit();
            return true;
        });

    // Choose initial page based on the start_page_index setting
    int startPageIndex = 0;
    if (SettingManager && SettingManager->Contains(KEY_UI_START_PAGE))
        startPageIndex = *gameRunner->settingConfig->Get(KEY_UI_START_PAGE)->AsInt();

    if (startPageIndex == 0)
    {
        createAppPage();
        showAppPage();
    }
    else
    {
        createFileListPage();
        showFileListPage();
    }

    bklog::debug("Startup Page: {}", startPageIndex);
}

// ─────────── Page creation ───────────────────────────────────────────────────

void StartPageView::createAppPage()
{
    if (m_appPage)
        return;

    m_appPage = new AppPage();
    m_appPage->addGame({ "/mGBA/roms/haogou.gba",          "好狗狗星系",   BK_RES("img/thumb/209.png") });
    m_appPage->addGame({ "./1.gba",                         "win测试",      BK_RES("img/thumb/209.png") });
    m_appPage->addGame({ "/mGBA/roms/gba/seaglass.gba",    "海镜",         BK_RES("img/thumb/210.png") });
    m_appPage->addGame({ "./2.gbc",                         "gbc口袋要管",  "" });
    m_appPage->addGame({ "/mGBA/roms/gba/Mother 3.gba",    "地球冒险3",    BK_RES("img/thumb/212.png") });
    m_appPage->addGame({ "/mGBA/roms/gba/MuChangWuYu.gba", "牧场物语",     "" });
    m_appPage->onGameSelected = [](const GameEntry& e) {
        auto* frame = new brls::AppletFrame(new GameView(e.path));
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
    };
}

void StartPageView::createFileListPage()
{
    if (m_fileListPage)
        return;

    m_fileListPage = new FileListPage();

    // Set default filter (GBA / GB / GBC roms)
    m_fileListPage->setFilter({ "gba", "gb", "gbc", "zip" }, FileListPage::FilterMode::Whitelist);
    m_fileListPage->setFilterEnabled(true);

    // Default file callback: launch the game
    m_fileListPage->setDefaultFileCallback([](const FileListItem& item) {
#ifdef __SWITCH__
        auto* frame = new brls::AppletFrame(new GameView(item.fullPath));
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
#endif
    });

    m_fileListPage->navigateTo(ROOT_PATH);
}

// ─────────── Page switching ──────────────────────────────────────────────────

void StartPageView::showAppPage()
{
    // Hide FileListPage if visible
    if (m_fileListPage)
        m_fileListPage->setVisibility(brls::Visibility::GONE);

    // Create AppPage if needed and add to view tree
    createAppPage();
    if (m_appPage->getParent() == nullptr)
        addView(m_appPage);
    m_appPage->setVisibility(brls::Visibility::VISIBLE);
    m_activeIndex = 0;

    // Bind LT → switch to FileListPage
    registerAction("beiklive/hints/FILE"_i18n,
                   brls::ControllerButton::BUTTON_LT,
                   [this](brls::View*) {
                       bklog::debug("Switching to FileListPage");
                       createFileListPage();
                       showFileListPage();
                       return true;
                   },
                   /*hidden=*/false);
}

void StartPageView::showFileListPage()
{
    // Hide AppPage if visible
    if (m_appPage)
        m_appPage->setVisibility(brls::Visibility::GONE);

    // Create FileListPage if needed and add to view tree
    createFileListPage();
    if (m_fileListPage->getParent() == nullptr)
        addView(m_fileListPage);
    m_fileListPage->setVisibility(brls::Visibility::VISIBLE);
    m_activeIndex = 1;

    // Bind RT → switch to AppPage
    registerAction("beiklive/hints/APP"_i18n,
                   brls::ControllerButton::BUTTON_RT,
                   [this](brls::View*) {
                       bklog::debug("Switching to AppPage");
                       createAppPage();
                       showAppPage();
                       return true;
                   },
                   /*hidden=*/false);
}

// ─────────────────────────────────────────────────────────────────────────────

StartPageView::~StartPageView()
{
}

void StartPageView::onFocusGained()
{
    Box::onFocusGained();
}

void StartPageView::onFocusLost()
{
    Box::onFocusLost();
}

void StartPageView::onLayout()
{
    Box::onLayout();
}

brls::View* StartPageView::create()
{
    return new StartPageView();
}