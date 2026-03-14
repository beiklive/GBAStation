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
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Record the current local time as the "last opened" timestamp for @p fileName
/// in gamedataManager.
static void recordGameOpenTime(const std::string& fileName)
{
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
}

/// Clear the UI image cache and push a GameView activity for @p romPath.
/// All game launch paths should go through this helper to stay consistent.
static void launchGameActivity(const std::string& romPath)
{
    beiklive::clearUIImageCache();
    auto* frame = new brls::AppletFrame(new GameView(romPath));
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::LINEAR);
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

    bklog::debug("StartPageView::Init: initializing");
    ActionInit();

    // 启动页面始终显示 AppPage
    createAppPage();
    showAppPage();

    addView(new brls::BottomBar());
    bklog::debug("StartPageView::Init: done, AppPage visible");
}

// ─────────── Page creation ───────────────────────────────────────────────────

void StartPageView::refreshRecentGames()
{
    if (!m_appPage || !SettingManager) return;

    bklog::debug("StartPageView::refreshRecentGames: rebuilding game list");
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
        bklog::debug("StartPageView::refreshRecentGames: adding '{}' -> '{}'", fileName, gamePath);
        games.push_back({ gamePath, title, logoPath });
    }
    bklog::debug("StartPageView::refreshRecentGames: total {} game(s)", static_cast<int>(games.size()));
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
        bklog::info("StartPageView: launching game '{}'", e.path);
        std::string fileName = std::filesystem::path(e.path).filename().string();
        // Record last opened time and push to recent games queue
        recordGameOpenTime(fileName);
        pushRecentGame(fileName);
        bklog::info("StartPageView: pushing GameView activity for '{}'", fileName);
        launchGameActivity(e.path);
    };
    // 文件列表按钮回调：打开文件列表页
    m_appPage->onOpenFileList = [this]() {
        openFileListPage();
    };

    m_appPage->onOpenSettings = [this]() {
        openSettingsPage();
    };

    // ── Wire X-button callback on game cards ──────────────────────────────
    // Use a Dropdown to present game-card options (set mapping, select logo,
    // remove from list).
    m_appPage->onGameOptions = [this](const GameEntry& entry) {
        AppPage* capturePage = m_appPage;

        struct Option {
            std::string label;
            std::function<void()> action;
        };
        std::vector<Option> opts;

        // Set display name
        opts.push_back({"beiklive/sidebar/set_mapping"_i18n, [entry]() {
            std::string fileName = std::filesystem::path(entry.path).filename().string();
            std::string key = gamedataKeyPrefix(fileName);
            std::string currentMapped;
            if (NameMappingManager) {
                auto mv = NameMappingManager->Get(key);
                if (mv && mv->AsString() && !mv->AsString()->empty())
                    currentMapped = *mv->AsString();
            }
            brls::Application::getPlatform()->getImeManager()->openForText(
                [key, entry](const std::string& mapped) {
                    if (!NameMappingManager) return;
                    if (mapped.empty())
                        NameMappingManager->Remove(key);
                    else
                        NameMappingManager->Set(key, mapped);
                    NameMappingManager->Save();
                },
                "beiklive/sidebar/set_mapping"_i18n,
                "",
                128,
                currentMapped);
        }});

        // Select logo
        opts.push_back({"beiklive/sidebar/select_logo"_i18n, [entry, capturePage]() {
            std::string startPath = beiklive::file::getParentPath(entry.path);
            if (startPath.empty() ||
                beiklive::file::getPathType(startPath) != beiklive::file::PathType::Directory) {
                startPath = "/";
#ifdef _WIN32
                startPath = "C:\\";
#endif
            }
            auto* flPage = new FileListPage();
            flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
            flPage->setDefaultFileCallback([entry, capturePage](const FileListItem& imgItem) {
                std::string fileName = std::filesystem::path(entry.path).filename().string();
                setGameDataStr(fileName, GAMEDATA_FIELD_LOGOPATH, imgItem.fullPath);
                if (capturePage)
                    capturePage->updateGameLogo(entry.path, imgItem.fullPath);
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
        }});

        // Remove from recent list
        opts.push_back({"beiklive/sidebar/remove_from_list"_i18n, [entry, capturePage]() {
            std::string fileName = std::filesystem::path(entry.path).filename().string();
            removeRecentGame(fileName);
            if (capturePage)
                capturePage->removeGame(entry.path);
        }});

        std::vector<std::string> labels;
        for (const auto& o : opts)
            labels.push_back(o.label);

        std::string title = entry.title.empty() ? std::filesystem::path(entry.path).stem().string() : entry.title;
        auto* dropdown = new brls::Dropdown(
            title,
            labels,
            [opts](int sel) {
                if (sel >= 0 && sel < static_cast<int>(opts.size()))
                    opts[sel].action();
            });
        brls::Application::pushActivity(new brls::Activity(dropdown));
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

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setBackground(brls::ViewBackground::NONE);

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
            // Update gamedataManager: gamepath, lastopen, platform
            beiklive::EmuPlatform platform = FileListPage::detectPlatform(item.fileName);
            initGameData(item.fileName, platform);
            setGameDataStr(item.fileName, GAMEDATA_FIELD_GAMEPATH, item.fullPath);
            // Record last opened time and push to recent games queue
            recordGameOpenTime(item.fileName);
            pushRecentGame(item.fileName);
            launchGameActivity(item.fullPath);
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
        [this](brls::View*) {
            bklog::debug("StartPageView: BUTTON_START pressed, closing file list");
            brls::Application::popActivity();
            // Explicitly restore focus to the AppPage game cards.
            // Without this, borealis may try to restore a stale focus that
            // points into the now-destroyed file-list activity, causing a crash.
            if (m_appPage) {
                auto* focus = m_appPage->getDefaultFocus();
                if (focus)
                    brls::Application::giveFocus(focus);
            }
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

brls::View* StartPageView::getDefaultFocus()
{
    // Always guide borealis focus back to the AppPage game cards so that
    // returning from any child activity (game, file list, settings) lands
    // on a valid, focusable view instead of whatever was last focused.
    if (m_appPage) {
        auto* focus = m_appPage->getDefaultFocus();
        if (focus) {
            bklog::debug("StartPageView::getDefaultFocus -> AppPage card");
            return focus;
        }
    }
    return Box::getDefaultFocus();
}

void StartPageView::draw(NVGcontext* vg, float x, float y, float width, float height,
                          brls::Style style, brls::FrameContext* ctx)
{
    // Refresh recent games list whenever the queue has been updated
    // (e.g. after returning from a game launched via the file list).
    if (g_recentGamesDirty && m_appPage) {
        g_recentGamesDirty = false;
        bklog::debug("StartPageView::draw: refreshing recent games list");
        refreshRecentGames();
        // The card row was rebuilt – give focus to the new first card so
        // borealis doesn't try to restore the now-deleted old card focus.
        auto* focus = m_appPage->getDefaultFocus();
        if (focus)
            brls::Application::giveFocus(focus);
    }
    Box::draw(vg, x, y, width, height, style, ctx);
}

brls::View* StartPageView::create()
{
    return new StartPageView();
}
