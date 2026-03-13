#include "UI/StartPageView.hpp"

#include "Game/game_view.hpp"
#include "UI/Pages/ImageView.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>

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

static constexpr float PANEL_OPTION_HEIGHT = 64.f;
static constexpr float PANEL_TITLE_HEIGHT  = 64.f;

FileSettingsPanel::FileSettingsPanel()
{
    // Absolute-positioned overlay – added to / removed from tree on demand
    setPositionType(brls::PositionType::ABSOLUTE);
    setBackgroundColor(GET_THEME_COLOR("beiklive/sidePanel"));
    setAxis(brls::Axis::COLUMN);
    setFocusable(true);

    // ── Title bar ──────────────────────────────────────────────────────────
    m_titleBar = new brls::Box(brls::Axis::ROW);
    m_titleBar->setHeight(PANEL_TITLE_HEIGHT);
    m_titleBar->setWidth(brls::View::AUTO);
    // m_titleBar->setPadding(8.f, 16.f, 8.f, 16.f);
    m_titleBar->setAlignItems(brls::AlignItems::CENTER);

    m_titleLabel = new brls::Label();
    m_titleLabel->setFontSize(24.f);
    m_titleLabel->setSingleLine(true);
    m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    // Scroll (marquee) the label when the item is focused and text overflows.
    m_titleLabel->setAutoAnimate(true);
    m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
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
    btn->setHideHighlightBackground(true);
    btn->setHideClickAnimation(true);

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

    // ── Set mapping ────────────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/set_mapping"_i18n, [this]() {
        close();
        if (m_fileListPage)
            m_fileListPage->doSetMappingPublic(m_currentItemIdx);
    });
#endif

    // ── Select logo (game files only) ─────────────────────────────────────
    if (!item.isDir && FileListPage::detectPlatform(item.fileName) != beiklive::EmuPlatform::None)
    {
        std::string captureFileName = item.fileName;
        std::string captureFullPath = item.fullPath;
        addOptionButton("beiklive/sidebar/select_logo"_i18n, [captureFileName, captureFullPath]() {
            auto* flPage = new FileListPage();
            flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
            flPage->setDefaultFileCallback([captureFileName](const FileListItem& imgItem) {
                // Save logo path to gamedataManager
                setGameDataStr(captureFileName, GAMEDATA_FIELD_LOGOPATH, imgItem.fullPath);
                brls::Application::popActivity();
            });
            // Start at the game file's parent directory
            std::string startPath = beiklive::file::getParentPath(captureFullPath);
            if (startPath.empty() ||
                beiklive::file::getPathType(startPath) != beiklive::file::PathType::Directory) {
                startPath = "/";
#ifdef _WIN32
                startPath = "C:\\";
#endif
            }
            flPage->navigateTo(startPath);

            auto* container = new brls::Box(brls::Axis::COLUMN);
            container->setGrow(1.0f);
            container->addView(flPage);
            container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START, [](brls::View*) {
                brls::Application::popActivity();
                return true;
            });
            auto* frame = new brls::AppletFrame(container);
            frame->setHeaderVisibility(brls::Visibility::GONE);
            frame->setFooterVisibility(brls::Visibility::GONE);
            frame->setBackground(brls::ViewBackground::NONE);
            brls::Application::pushActivity(new brls::Activity(frame));
        });
    }

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
//  AppGameSettingsPanel
// ─────────────────────────────────────────────────────────────────────────────

AppGameSettingsPanel::AppGameSettingsPanel()
{
    setPositionType(brls::PositionType::ABSOLUTE);
    setBackgroundColor(GET_THEME_COLOR("beiklive/sidePanel"));
    setAxis(brls::Axis::COLUMN);
    setFocusable(true);

    // ── Title bar ──────────────────────────────────────────────────────────
    m_titleBar = new brls::Box(brls::Axis::ROW);
    m_titleBar->setHeight(PANEL_TITLE_HEIGHT);
    m_titleBar->setWidth(brls::View::AUTO);
    m_titleBar->setAlignItems(brls::AlignItems::CENTER);

    m_titleLabel = new brls::Label();
    m_titleLabel->setFontSize(24.f);
    m_titleLabel->setSingleLine(true);
    m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_titleLabel->setAutoAnimate(true);
    m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
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

void AppGameSettingsPanel::clearOptions()
{
    m_optionsBox->clearViews(true);
}

void AppGameSettingsPanel::addOptionButton(const std::string& label,
                                            std::function<void()> action)
{
    auto* btn = new brls::Box(brls::Axis::ROW);
    btn->setFocusable(true);
    btn->setHeight(PANEL_OPTION_HEIGHT);
    btn->setWidth(brls::View::AUTO);
    btn->setAlignItems(brls::AlignItems::CENTER);
    btn->setPadding(0.f, 20.f, 0.f, 20.f);
    btn->setHideHighlightBackground(true);
    btn->setHideClickAnimation(true);

    auto* lbl = new brls::Label();
    lbl->setText(label);
    lbl->setFontSize(22.f);
    lbl->setTextColor(nvgRGBA(220, 220, 220, 255));
    lbl->setGrow(1.0f);
    btn->addView(lbl);

    btn->registerAction("beiklive/hints/confirm"_i18n,
                        brls::BUTTON_A,
                        [action](brls::View*) {
                            action();
                            return true;
                        },
                        false, false, brls::SOUND_CLICK);

    m_optionsBox->addView(btn);
}

void AppGameSettingsPanel::showForEntry(const GameEntry& entry, AppPage* page)
{
    m_appPage = page;
    m_entry   = entry;

    m_titleLabel->setText(entry.title.empty() ? entry.path : entry.title);
    clearOptions();

    // ── Set Mapping Name ───────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/set_mapping"_i18n, [this]() {
        std::string fileName = std::filesystem::path(m_entry.path).filename().string();
        std::string key = gamedataKeyPrefix(fileName);
        std::string currentMapped;
        if (NameMappingManager) {
            auto mv = NameMappingManager->Get(key);
            if (mv && mv->AsString() && !mv->AsString()->empty())
                currentMapped = *mv->AsString();
        }
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this, key](const std::string& mapped) {
                if (!NameMappingManager) return;
                if (mapped.empty())
                    NameMappingManager->Remove(key);
                else
                    NameMappingManager->Set(key, mapped);
                NameMappingManager->Save();
                // Update the title in the entry if AppPage is available
                if (m_appPage) {
                    std::string newTitle = mapped.empty()
                        ? std::filesystem::path(m_entry.path).stem().string()
                        : mapped;
                    m_entry.title = newTitle;
                    m_titleLabel->setText(newTitle);
                }
            },
            "beiklive/sidebar/set_mapping"_i18n,
            "",
            128,
            currentMapped);
    });

    // ── Set Logo ───────────────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/select_logo"_i18n, [this]() {
        GameEntry captureEntry = m_entry;
        AppPage*  capturePage  = m_appPage;

        // Determine start path: parent directory of the game file
        std::string startPath = beiklive::file::getParentPath(captureEntry.path);
        if (startPath.empty() ||
            beiklive::file::getPathType(startPath) != beiklive::file::PathType::Directory) {
            startPath = "/";
#ifdef _WIN32
            startPath = "C:\\";
#endif
        }

        auto* flPage = new FileListPage();
        flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
        flPage->setDefaultFileCallback([captureEntry, capturePage](const FileListItem& imgItem) {
            std::string fileName = std::filesystem::path(captureEntry.path).filename().string();
            setGameDataStr(fileName, GAMEDATA_FIELD_LOGOPATH, imgItem.fullPath);
            if (capturePage)
                capturePage->updateGameLogo(captureEntry.path, imgItem.fullPath);
            brls::Application::popActivity();
        });
        flPage->navigateTo(startPath);

        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->addView(flPage);
        container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                                  [](brls::View*) {
                                      brls::Application::popActivity();
                                      return true;
                                  });
        auto* frame = new brls::AppletFrame(container);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
    });

    // ── Remove from list ───────────────────────────────────────────────────
    addOptionButton("beiklive/sidebar/remove_from_list"_i18n, [this]() {
        std::string fileName = std::filesystem::path(m_entry.path).filename().string();
        removeRecentGame(fileName);
        if (m_appPage)
            m_appPage->removeGame(m_entry.path);
        close();
    });

    setVisibility(brls::Visibility::VISIBLE);

    // Give focus to the first option button
    if (!m_optionsBox->getChildren().empty())
        brls::Application::giveFocus(m_optionsBox->getChildren()[0]);

    // Wrap vertical focus navigation so it cannot escape the panel
    const auto& opts = m_optionsBox->getChildren();
    if (opts.size() > 1) {
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

void AppGameSettingsPanel::close()
{
    if (getParent())
        getParent()->removeView(this, false);

    if (m_appPage) {
        auto* focus = m_appPage->getDefaultFocus();
        if (focus)
            brls::Application::giveFocus(focus);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  StartPageView
// ─────────────────────────────────────────────────────────────────────────────

StartPageView::StartPageView()
{
    setAxis(brls::Axis::COLUMN);

    // Background image (absolute positioning, does not participate in layout)
    // beiklive::InsertBackground(this);

    
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
    // setBackground(brls::ViewBackground::NONE);

    if (!gameRunner)
        return;

    ActionInit();

    // 启动页面始终显示 AppPage
    createAppPage();
    showAppPage();

    addView(new brls::BottomBar());
    bklog::debug("Startup Page: AppPage");
}

// ─────────── Page creation ───────────────────────────────────────────────────

void StartPageView::refreshRecentGames()
{
    if (!m_appPage || !SettingManager) return;

    std::vector<GameEntry> games;
    for (int i = 0; i < RECENT_GAME_COUNT; ++i) {
        std::string key = std::string(RECENT_GAME_KEY_PREFIX) + std::to_string(i);
        auto v = SettingManager->Get(key);
        if (!v || !v->AsString() || v->AsString()->empty())
            continue;
        std::string fileName = *v->AsString();
        std::string gamePath = getGameDataStr(fileName, GAMEDATA_FIELD_GAMEPATH, "");
        if (gamePath.empty())
            continue;
        std::string logoPath = getGameDataStr(fileName, GAMEDATA_FIELD_LOGOPATH, "");
        // Prefer mapped display name from NameMappingManager, fall back to file stem
        std::string title;
        if (NameMappingManager) {
            std::string mapKey = gamedataKeyPrefix(fileName);
            auto mv = NameMappingManager->Get(mapKey);
            if (mv && mv->AsString() && !mv->AsString()->empty())
                title = *mv->AsString();
        }
        if (title.empty())
            title = std::filesystem::path(fileName).stem().string();
        games.push_back({ gamePath, title, logoPath });
    }
    m_appPage->setGames(games);
}

void StartPageView::createAppPage()
{
    if (m_appPage)
        return;

    m_appPage = new AppPage();

    // ── Load recent games from SettingManager ─────────────────────────────
    refreshRecentGames();

    m_appPage->onGameSelected = [](const GameEntry& e) {
        // Update game metadata before launching
        std::string fileName = std::filesystem::path(e.path).filename().string();
        // Record last opened time
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char timeBuf[64] = "";
        struct tm tmInfo;
#ifdef _WIN32
        if (localtime_s(&tmInfo, &now) == 0)
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tmInfo);
#else
        if (localtime_r(&now, &tmInfo))
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tmInfo);
#endif
        if (timeBuf[0] != '\0')
            setGameDataStr(fileName, GAMEDATA_FIELD_LASTOPEN, timeBuf);
        // Push to recent games queue
        pushRecentGame(fileName);
        // Free UI image cache before launching the emulator to reclaim memory.
        beiklive::clearUIImageCache();
        auto* frame = new brls::AppletFrame(new GameView(e.path));
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame), brls::TransitionAnimation::LINEAR);
    };
    // 文件列表按钮回调：打开文件列表页
    m_appPage->onOpenFileList = [this]() {
        openFileListPage();
    };

    m_appPage->onOpenSettings = [this]() {
        openSettingsPage();
    };

    // ── Create settings panel for game cards ──────────────────────────────
    m_appSettingsPanel = new AppGameSettingsPanel();
    m_appSettingsPanel->setWidthPercentage(30.f);
    m_appSettingsPanel->setHeightPercentage(100.f);
    m_appSettingsPanel->setPositionRight(0.f);
    m_appSettingsPanel->setPositionTop(0.f);
    m_appSettingsPanel->setVisibility(brls::Visibility::GONE);
    // Add to this (StartPageView) so it is always owned and freed with us.
    addView(m_appSettingsPanel);

    // ── Wire X-button callback on game cards ──────────────────────────────
    m_appPage->onGameOptions = [this](const GameEntry& entry) {
        // Bring panel to top by re-inserting it
        if (m_appSettingsPanel->getParent())
            m_appSettingsPanel->getParent()->removeView(m_appSettingsPanel, false);
        addView(m_appSettingsPanel);
        m_appSettingsPanel->showForEntry(entry, m_appPage);
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
    static const std::vector<std::string> IMAGE_EXTENSIONS = {"png", "jpg", "jpeg", "bmp"};
    static const std::vector<std::string> ROM_EXTENSIONS   = {"zip", "gba", "gbc", "gb"};

    // ── Create a fresh FileListPage ───────────────────────────────────────────
    auto* fileListPage = new FileListPage();
    fileListPage->setFilter({"png", "gba", "gbc", "gb"}, FileListPage::FilterMode::Whitelist);

    // ── Create a fresh settings panel and a container that holds both ─────────
    // The container is absolute-positioned so the panel can overlay the list.
    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setBackground(brls::ViewBackground::NONE);

    


    auto* settingsPanel = new FileSettingsPanel();
    settingsPanel->setWidthPercentage(30.f);
    settingsPanel->setHeightPercentage(100.f);
    settingsPanel->setPositionRight(0.f);
    settingsPanel->setPositionTop (0.f);
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
            // Update gamedataManager: gamepath, lastopen
            beiklive::EmuPlatform platform = FileListPage::detectPlatform(item.fileName);
            initGameData(item.fileName, platform);
            setGameDataStr(item.fileName, GAMEDATA_FIELD_GAMEPATH, item.fullPath);
            // Record last opened time
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            char timeBuf[64] = "未知时间";
            struct tm tmInfo;
#ifdef _WIN32
            if (localtime_s(&tmInfo, &now) == 0)
                std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tmInfo);
#else
            if (localtime_r(&now, &tmInfo))
                std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tmInfo);
#endif
            setGameDataStr(item.fileName, GAMEDATA_FIELD_LASTOPEN, timeBuf);
            // Push to recent games queue
            pushRecentGame(item.fileName);
            // Free UI image cache before launching the emulator.
            beiklive::clearUIImageCache();
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

void StartPageView::openSettingsPage()
{
    auto* m_SettingPage = new SettingPage();
    auto* frame = new brls::AppletFrame(m_SettingPage);
    frame->setBackground(brls::ViewBackground::NONE);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setTitle("设置");
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

void StartPageView::draw(NVGcontext* vg, float x, float y, float width, float height,
                          brls::Style style, brls::FrameContext* ctx)
{
    // Refresh recent games list whenever the queue has been updated
    // (e.g. after returning from a game launched via the file list).
    if (g_recentGamesDirty && m_appPage) {
        g_recentGamesDirty = false;
        refreshRecentGames();
    }
    Box::draw(vg, x, y, width, height, style, ctx);
}

brls::View* StartPageView::create()
{
    return new StartPageView();
}
