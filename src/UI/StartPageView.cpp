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
    m_bgImage = new beiklive::UI::ProImage();
    m_bgImage->setFocusable(false);
    m_bgImage->setPositionType(brls::PositionType::ABSOLUTE);
    m_bgImage->setPositionTop(0);
    m_bgImage->setPositionLeft(0);
    m_bgImage->setWidthPercentage(100);
    m_bgImage->setHeightPercentage(100);
    m_bgImage->setScalingType(brls::ImageScalingType::FIT);
    m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    // m_bgImage->setImageFromFile(BK_APP_DEFAULT_BG);
    m_bgImage->setImageFromGif(BK_RES("img/test.gif"));
    m_bgImage->setShaderAnimation(beiklive::UI::ShaderAnimationType::PSP_XMB_RIPPLE);
    addView(m_bgImage);
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

    // 启动页面始终显示 AppPage
    createAppPage();
    showAppPage();

    bklog::debug("Startup Page: AppPage");
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
    // 文件列表按钮回调：打开文件列表页
    m_appPage->onOpenFileList = [this]() {
        openFileListPage();
    };
}

// ─────────── Page switching ──────────────────────────────────────────────────

void StartPageView::showAppPage()
{
    // Add AppPage to view tree
    createAppPage();
    addView(m_appPage);
    m_appPage->setVisibility(brls::Visibility::VISIBLE);

    gameRunner->uiParams->StartPageframe->setHeaderVisibility(brls::Visibility::VISIBLE);
    gameRunner->uiParams->StartPageframe->setFooterVisibility(brls::Visibility::VISIBLE);

    // Transfer focus to AppPage's first focusable child
    brls::Application::giveFocus(m_appPage->getDefaultFocus());
}

void StartPageView::openFileListPage()
{
    static const std::vector<std::string> IMAGE_EXTENSIONS = {"png", "jpg", "jpeg", "bmp", "gif"};
    static const std::vector<std::string> ROM_EXTENSIONS   = {"zip", "gba", "gbc", "gb"};

    // ── Create a fresh FileListPage ───────────────────────────────────────────
    auto* fileListPage = new FileListPage();

    // ── Create a fresh settings panel and a container that holds both ─────────
    // The container is absolute-positioned so the panel can overlay the list.
    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setBackground(brls::ViewBackground::NONE);

    auto* settingsPanel = new FileSettingsPanel();
    float panelW = 1280 * 0.70f;
    float panelH = 720  * 0.70f;
    settingsPanel->setWidth(panelW);
    settingsPanel->setHeight(panelH);
    settingsPanel->setPositionLeft((1280 - panelW) * 0.5f);
    settingsPanel->setPositionTop ((720  - panelH) * 0.5f);
    settingsPanel->setVisibility(brls::Visibility::GONE);
    // Add to container now (GONE) so it is always owned and freed with the Activity
    container->addView(settingsPanel);

    // ── Wire the settings-panel callback ─────────────────────────────────────
    // Safety: This lambda is stored in fileListPage->onOpenSettings, which is a
    // member of fileListPage.  fileListPage is a child of container, so all
    // three objects (container, settingsPanel, fileListPage) share the same
    // lifetime.  The lambda cannot outlive the objects it captures.
    fileListPage->onOpenSettings = [container, settingsPanel, fileListPage]
        (const FileListItem& item, int idx)
    {
        // Bring panel to top by re-inserting it (addView while GONE keeps it hidden)
        container->removeView(settingsPanel, /*free=*/false);
        container->addView(settingsPanel);
        settingsPanel->showForItem(item, idx, fileListPage);
    };

    // ── Wire file-open callbacks ──────────────────────────────────────────────
    for (const auto& ext : IMAGE_EXTENSIONS)
    {
        fileListPage->setFileCallback(ext, [](const FileListItem& item) {
            auto* imageView = new ImageView(item.fullPath);
            auto* frame = new brls::AppletFrame(imageView);
            frame->setBackground(brls::ViewBackground::NONE);
            brls::Application::pushActivity(new brls::Activity(frame));
        });
    }
    for (const auto& ext : ROM_EXTENSIONS)
    {
        fileListPage->setFileCallback(ext, [](const FileListItem& item) {
            auto* frame = new brls::AppletFrame(new GameView(item.fullPath));
            frame->setHeaderVisibility(brls::Visibility::GONE);
            frame->setFooterVisibility(brls::Visibility::GONE);
            frame->setBackground(brls::ViewBackground::NONE);
            brls::Application::pushActivity(new brls::Activity(frame));
        });
    }

    // ── Navigate to last (or default) path ───────────────────────────────────
    if (SettingManager->Contains("last_game_path"))
        fileListPage->navigateTo(*gameRunner->settingConfig->Get("last_game_path")->AsString());
    else
        fileListPage->navigateTo(ROOT_PATH);

    // ── Assemble: fileListPage inside container ───────────────────────────────
    container->addView(fileListPage);

    // ── Bind + (BUTTON_START) → pop this activity ─────────────────────────────
    container->registerAction(
        "beiklive/hints/close"_i18n,
        brls::BUTTON_START,
        [](brls::View*) {
            brls::Application::popActivity();
            return true;
        },
        /*hidden=*/false, /*repeat=*/false, brls::SOUND_CLICK);

    // ── Push as a new Activity ────────────────────────────────────────────────
    auto* frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame));
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
