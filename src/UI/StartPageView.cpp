#include "UI/StartPageView.hpp"

#include "Game/game_view.hpp"
#include "UI/Pages/ImageView.hpp"

#undef ABSOLUTE // avoid conflict with brls::PositionType::ABSOLUTE

using namespace brls::literals; // for _i18n

// ─────────────────────────────────────────────────────────────────────────────
//  Default ROM root (same as List_view.cpp)
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__SWITCH__)
#define ROOT_PATH "/"
#elif defined(WIN32)
#define ROOT_PATH "F:\\games\\GBA"
#else
#define ROOT_PATH "/Users/beiklive"
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  FileSettingsPanel
// ─────────────────────────────────────────────────────────────────────────────

static constexpr float PANEL_OPTION_HEIGHT = 54.f;
static constexpr float PANEL_TITLE_HEIGHT  = 48.f;

FileSettingsPanel::FileSettingsPanel()
{
    // Absolute-positioned overlay – added to / removed from tree on demand
    setPositionType(brls::PositionType::ABSOLUTE);
    setBackgroundColor(nvgRGBA(20, 20, 20, 255));
    setAxis(brls::Axis::COLUMN);
    setFocusable(true);

    // ── Title bar ──────────────────────────────────────────────────────────
    m_titleBar = new brls::Box(brls::Axis::ROW);
    m_titleBar->setHeight(PANEL_TITLE_HEIGHT);
    m_titleBar->setWidth(brls::View::AUTO);
    m_titleBar->setBackgroundColor(nvgRGBA(40, 40, 40, 255));
    m_titleBar->setPadding(8.f, 16.f, 8.f, 16.f);
    m_titleBar->setAlignItems(brls::AlignItems::CENTER);

    m_titleLabel = new brls::Label();
    m_titleLabel->setFontSize(24.f);
    m_titleLabel->setTextColor(nvgRGBA(220, 220, 220, 255));
    m_titleLabel->setGrow(1.0f);
    m_titleBar->addView(m_titleLabel);
    addView(m_titleBar);

    // ── Option buttons container ───────────────────────────────────────────
    m_optionsBox = new brls::Box(brls::Axis::COLUMN);
    m_optionsBox->setWidth(brls::View::AUTO);
    m_optionsBox->setGrow(1.0f);
    m_optionsBox->setPadding(8.f, 0.f, 8.f, 0.f);
    addView(m_optionsBox);
}

void FileSettingsPanel::clearOptions()
{
    // Remove all option buttons from the options box
    m_optionsBox->clearViews(true);
}

void FileSettingsPanel::addOptionButton(const std::string& label,
                                         std::function<void()> action)
{
    auto* btn = new brls::Box(brls::Axis::ROW);
    btn->setFocusable(true);
    btn->setHeight(PANEL_OPTION_HEIGHT);
    btn->setWidth(brls::View::AUTO);
    btn->setAlignItems(brls::AlignItems::CENTER);
    btn->setPadding(0.f, 20.f, 0.f, 20.f);
    btn->setHideHighlightBackground(false);

    auto* lbl = new brls::Label();
    lbl->setText(label);
    lbl->setFontSize(22.f);
    lbl->setTextColor(nvgRGBA(220, 220, 220, 255));
    lbl->setGrow(1.0f);
    btn->addView(lbl);

    // Capture action by value so it lives past this function's scope
    btn->registerAction("beiklive/hints/confirm"_i18n,
                        brls::BUTTON_A,
                        [action](brls::View*) {
                            action();
                            return true;
                        },
                        false, false, brls::SOUND_CLICK);

    m_optionsBox->addView(btn);
}

void FileSettingsPanel::showForItem(const FileListItem& item,
                                     int itemIndex,
                                     FileListPage* page)
{
    m_fileListPage   = page;
    m_currentItemIdx = itemIndex;

    m_titleLabel->setText(item.displayName());

    clearOptions();

    // ── Rename (Switch only) ───────────────────────────────────────────────
#ifdef __SWITCH__
    addOptionButton("beiklive/sidebar/rename"_i18n, [this]() {
        close();
        if (m_fileListPage)
            m_fileListPage->doRenamePublic(m_currentItemIdx);
    });
#endif

    // ── Set mapping ────────────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/set_mapping"_i18n, [this]() {
        close();
        if (m_fileListPage)
            m_fileListPage->doSetMappingPublic(m_currentItemIdx);
    });

    // ── Cut ────────────────────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/cut"_i18n, [this]() {
        if (m_fileListPage)
            m_fileListPage->doCutPublic(m_currentItemIdx);
        close();
    });

    // ── Paste (only when clipboard has an item) ────────────────────────────
    if (page && page->hasClipboardItem())
    {
        addOptionButton("beiklive/sidebar/paste"_i18n, [this]() {
            if (m_fileListPage)
                m_fileListPage->doPastePublic();
            close();
        });
    }

    // ── Delete ────────────────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/delete"_i18n, [this]() {
        close();
        if (m_fileListPage)
            m_fileListPage->doDeletePublic(m_currentItemIdx);
    });

    // ── New folder (directory operation) ─────────────────────────────────
    addOptionButton("beiklive/sidebar/new_folder"_i18n, [this]() {
        close();
        if (m_fileListPage)
            m_fileListPage->doNewFolder();
    });

    setVisibility(brls::Visibility::VISIBLE);

    // Give focus to the first option button
    if (!m_optionsBox->getChildren().empty())
        brls::Application::giveFocus(m_optionsBox->getChildren()[0]);

    // Wrap vertical focus navigation so it cannot escape the panel.
    // Without this, pressing UP from the first button would let Borealis
    // traverse the parent (StartPageView) and land on the file list.
    const auto& opts = m_optionsBox->getChildren();
    if (opts.size() > 1)
    {
        opts.front()->setCustomNavigationRoute(brls::FocusDirection::UP,   opts.back());
        opts.back()->setCustomNavigationRoute(brls::FocusDirection::DOWN,  opts.front());
    }

    registerAction("beiklive/hints/close"_i18n,
                   brls::BUTTON_B,
                   [this](brls::View*) {
                       close();
                       return true;
                   },
                   false, false, brls::SOUND_CLICK);
}

void FileSettingsPanel::close()
{
    // Remove from view tree instead of hiding.
    // Guard against close() being called before the panel was ever added.
    if (getParent())
        getParent()->removeView(this, false);

    // Return focus to the file list
    if (m_fileListPage)
        brls::Application::giveFocus(m_fileListPage->getDefaultFocus());
}

// ─────────────────────────────────────────────────────────────────────────────
//  StartPageView
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
    // addView(m_bgImage);

    // Settings overlay panel (absolute positioning, added/removed on demand)
    m_settingsPanel = new FileSettingsPanel();
    float panelW = 1280 * 0.70f;
    float panelH = 720 * 0.70f;
    m_settingsPanel->setWidth(panelW);
    m_settingsPanel->setHeight(panelH);
    m_settingsPanel->setPositionLeft((1280 - panelW) * 0.5f);
    m_settingsPanel->setPositionTop((720 - panelH) * 0.5f);
    // Do NOT addView here; it will be added when X is pressed
}

void StartPageView::ActionInit()
{
    // 移除默认操作
    beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_X);
    beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_Y);
    beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_B);
    beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_LT);
    beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_RT);

    // Quit action
    gameRunner->uiParams->StartPageframe->registerAction(
        "beiklive/hints/exit"_i18n,
        brls::BUTTON_START,
        [](brls::View*) {
            auto dialog = new brls::Dialog("hints/exit_hint"_i18n);
            dialog->addButton("hints/cancel"_i18n, []() {});
            dialog->addButton("hints/ok"_i18n, []()
                { brls::Application::quit(); });
            dialog->open();
            return true;
        });

}

void StartPageView::Init()
{
    setBackground(brls::ViewBackground::NONE);

    if (!gameRunner)
        return;

    ActionInit();

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
    std::vector<std::string> IMAGE_EXTENSIONS = {"png", "jpg", "jpeg", "bmp", "gif",};
    std::vector<std::string> ROM_EXTENSIONS = {"zip", "gba", "gbc", "gb"};
    // Set default filter (GBA / GB / GBC roms)
    // m_fileListPage->setFilter(ROM_EXTENSIONS, FileListPage::FilterMode::Whitelist);
    // m_fileListPage->setFilterEnabled(true);

    // Register settings panel callback
    m_fileListPage->onOpenSettings = [this](const FileListItem& item, int idx) {
        onFileSettingsRequested(item, idx);
    };

    for (const auto& ext : IMAGE_EXTENSIONS)
    {
        m_fileListPage->setFileCallback(ext, [](const FileListItem& item) {
            auto* imageView = new ImageView(item.fullPath);
            auto* frame = new brls::AppletFrame(imageView);
            frame->setHeaderVisibility(brls::Visibility::GONE);
            // frame->setFooterVisibility(brls::Visibility::GONE);
            frame->setBackground(brls::ViewBackground::NONE);
            brls::Application::pushActivity(new brls::Activity(frame));
        });
    }
    // Default file callback: launch the game
    for (const auto& ext : ROM_EXTENSIONS)
    {
        m_fileListPage->setFileCallback(ext, [](const FileListItem& item) {
            auto* frame = new brls::AppletFrame(new GameView(item.fullPath));
            frame->setHeaderVisibility(brls::Visibility::GONE);
            frame->setFooterVisibility(brls::Visibility::GONE);
            frame->setBackground(brls::ViewBackground::NONE);
            brls::Application::pushActivity(new brls::Activity(frame));
        });
    }

    SettingManager->Contains("last_game_path") ? m_fileListPage->navigateTo(*gameRunner->settingConfig->Get("last_game_path")->AsString()) : m_fileListPage->navigateTo(ROOT_PATH);
}

// ─────────── Settings panel ──────────────────────────────────────────────────

void StartPageView::onFileSettingsRequested(const FileListItem& item, int itemIndex)
{
    if (m_settingsPanel)
    {
        // Remove first (no-op if not already in tree), then add so it renders on top
        removeView(m_settingsPanel, false);
        addView(m_settingsPanel);
        m_settingsPanel->showForItem(item, itemIndex, m_fileListPage);
    }
}

// ─────────── Page switching ──────────────────────────────────────────────────

void StartPageView::showAppPage()
{
    // Create AppPage if needed and add to view tree
    createAppPage();
    addView(m_appPage);
    m_appPage->setVisibility(brls::Visibility::VISIBLE);
    m_activeIndex = 0;

    gameRunner->uiParams->StartPageframe->setHeaderVisibility(brls::Visibility::VISIBLE);
    gameRunner->uiParams->StartPageframe->setFooterVisibility(brls::Visibility::VISIBLE);



    // Transfer focus to AppPage's first focusable child so it doesn't linger
    // on the FileListPage that was just removed from the tree.
    brls::Application::giveFocus(m_appPage->getDefaultFocus());

    beiklive::swallow(this, brls::BUTTON_RT);
    beiklive::swallow(this, brls::BUTTON_A);
    // Bind LT → switch to FileListPage
    registerAction("beiklive/hints/FILE"_i18n,
                   brls::ControllerButton::BUTTON_LT,
                   [this](brls::View*) {
                       bklog::debug("Switching to FileListPage");
                       removeView(m_appPage, false); // keep AppPage alive but detached
                       createFileListPage();
                       showFileListPage();
                       return true;
                   },
                   /*hidden=*/false);

}

void StartPageView::showFileListPage()
{
    // Create FileListPage if needed and add to view tree
    createFileListPage();
    addView(m_fileListPage);
    m_fileListPage->setVisibility(brls::Visibility::VISIBLE);
    m_activeIndex = 1;

    gameRunner->uiParams->StartPageframe->setHeaderVisibility(brls::Visibility::GONE);
    gameRunner->uiParams->StartPageframe->setFooterVisibility(brls::Visibility::GONE);


    // Reset focus to the first item in the file list so it never lingers at
    // the position from the previous visit (or on the removed AppPage).
    m_fileListPage->resetFocusToTop();

    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_LT);
    // Bind RT → switch to AppPage
    registerAction("beiklive/hints/APP"_i18n,
                   brls::ControllerButton::BUTTON_RT,
                   [this](brls::View*) {
                       bklog::debug("Switching to AppPage");
                       removeView(m_fileListPage, false); // keep FileListPage alive but detached
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

brls::View* StartPageView::create()
{
    return new StartPageView();
}
