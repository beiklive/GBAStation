#include "UI/StartPageView.hpp"

#include "Game/game_view.hpp"
#include "UI/Pages/ImageView.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>

#undef ABSOLUTE // avoid conflict with brls::PositionType::ABSOLUTE

using namespace brls::literals; // for _i18n

// ─────────────────────────────────────────────────────────────────────────────
//  默认 ROM 根路径
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__SWITCH__)
#define ROOT_PATH "/"
#elif defined(WIN32)
#define ROOT_PATH "F:\\games\\GBA"
#else
#define ROOT_PATH "/Users/beiklive"
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  内部辅助函数
// ─────────────────────────────────────────────────────────────────────────────

/// 将当前本地时间记录为 fileName 的"最近打开"时间戳
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

/// 清理 UI 图像缓存，并为 romPath 推送 GameView 活动。
/// 所有启动游戏的路径均应经过此函数以保持一致。
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

    // 背景图片（绝对定位，不参与布局）
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

    // 退出操作
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

// ─────────── 页面创建 ────────────────────────────────────────────────────────

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
        // 优先从 NameMappingManager 获取显示名，否则取文件名（不含扩展名）
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

    // ── 从 SettingManager 加载最近游戏 ──────────────────────────────────
    refreshRecentGames();

    m_appPage->onGameSelected = [](const GameEntry& e) {
        bklog::info("StartPageView: launching game '{}'", e.path);
        std::string fileName = std::filesystem::path(e.path).filename().string();
        // 记录最近打开时间并加入最近游戏队列
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

    // ── 绑定游戏卡片 X 键回调 ─────────────────────────────────────────────
    // 使用 Dropdown 展示游戏卡片选项（设置映射名、选择封面、从列表移除）
    m_appPage->onGameOptions = [this](const GameEntry& entry) {
        AppPage* capturePage = m_appPage;

        struct Option {
            std::string label;
            std::function<void()> action;
        };
        std::vector<Option> opts;

        // 设置显示名
        opts.push_back({"beiklive/sidebar/set_mapping"_i18n, [entry, capturePage]() {
            std::string fileName = std::filesystem::path(entry.path).filename().string();
            std::string key = gamedataKeyPrefix(fileName);
            std::string currentMapped;
            if (NameMappingManager) {
                auto mv = NameMappingManager->Get(key);
                if (mv && mv->AsString() && !mv->AsString()->empty())
                    currentMapped = *mv->AsString();
            }
            brls::Application::getPlatform()->getImeManager()->openForText(
                [key, entry, capturePage](const std::string& mapped) {
                    if (!NameMappingManager) return;
                    if (mapped.empty())
                        NameMappingManager->Remove(key);
                    else
                        NameMappingManager->Set(key, mapped);
                    NameMappingManager->Save();
                    // 立即更新卡片显示的标题
                    if (capturePage) {
                        std::string newTitle = mapped.empty()
                            ? std::filesystem::path(entry.path).stem().string()
                            : mapped;
                        capturePage->updateGameTitle(entry.path, newTitle);
                    }
                },
                "beiklive/sidebar/set_mapping"_i18n,
                "",
                128,
                currentMapped);
        }});

        // 选择封面
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
            flPage->setLayoutMode(FileListPage::LayoutMode::ListOnly);
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

        // 从最近列表移除
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
        // 修复：将 opts 动作传给 dismissCb（第5参数），确保在 Dropdown 弹出后再执行。
        // 若传给 cb（第3参数），会导致 popActivity 误关刚推入的活动。
        auto* dropdown = new brls::Dropdown(
            title,
            labels,
            [](int) {},   // cb: 无操作（动作在关闭后执行）
            -1,
            [opts](int sel) {
                if (sel >= 0 && sel < static_cast<int>(opts.size()))
                    opts[sel].action();
            });
        brls::Application::pushActivity(new brls::Activity(dropdown));
    };

}

// ─────────── 页面切换 ────────────────────────────────────────────────────────

void StartPageView::showAppPage()
{
    // 将 AppPage 加入视图树
    createAppPage();
    addView(m_appPage);
    m_appPage->setVisibility(brls::Visibility::VISIBLE);

    gameRunner->uiParams->StartPageframe->setHeaderVisibility(brls::Visibility::VISIBLE);
    gameRunner->uiParams->StartPageframe->setFooterVisibility(brls::Visibility::VISIBLE);

    // 将焦点转移到 AppPage 的第一个可聚焦子节点
    brls::Application::giveFocus(m_appPage->getDefaultFocus());
}

void StartPageView::openFileListPage()
{
    static const std::vector<std::string> IMAGE_EXTENSIONS = {"png", "jpg", "jpeg", "bmp"};
    static const std::vector<std::string> ROM_EXTENSIONS   = {"zip", "gba", "gbc", "gb"};

    // ── 创建 FileListPage ──────────────────────────────────────────────────
    auto* fileListPage = new FileListPage();
    fileListPage->setFilter({"png", "gba", "gbc", "gb"}, FileListPage::FilterMode::Whitelist);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setBackground(brls::ViewBackground::NONE);

    // ── 绑定文件打开回调 ──────────────────────────────────────────────────────
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
            // 更新 gamedataManager：gamepath、lastopen、platform
            beiklive::EmuPlatform platform = FileListPage::detectPlatform(item.fileName);
            initGameData(item.fileName, platform);
            setGameDataStr(item.fileName, GAMEDATA_FIELD_GAMEPATH, item.fullPath);
            // 记录最近打开时间并加入最近游戏队列
            recordGameOpenTime(item.fileName);
            pushRecentGame(item.fileName);
            launchGameActivity(item.fullPath);
        });
    }

    // ── 导航到上次（或默认）路径 ─────────────────────────────────────────────
    if (SettingManager->Contains("last_game_path"))
        fileListPage->navigateTo(*gameRunner->settingConfig->Get("last_game_path")->AsString());
    else
        fileListPage->navigateTo(ROOT_PATH);

    // ── 将 fileListPage 放入 container ───────────────────────────────────────
    container->addView(fileListPage);

    // ── 绑定 +（BUTTON_START）→ 弹出当前活动 ─────────────────────────────────
    container->registerAction(
        "beiklive/hints/close"_i18n,
        brls::BUTTON_START,
        [this](brls::View*) {
            bklog::debug("StartPageView: BUTTON_START pressed, closing file list");
            brls::Application::popActivity();
            // 显式恢复 AppPage 游戏卡片的焦点，防止 borealis 尝试恢复已销毁的焦点导致崩溃
            if (m_appPage) {
                auto* focus = m_appPage->getDefaultFocus();
                if (focus)
                    brls::Application::giveFocus(focus);
            }
            return true;
        },
        /*hidden=*/false, /*repeat=*/false, brls::SOUND_CLICK);

    // ── 作为新 Activity 推入 ──────────────────────────────────────────────────
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
    // 始终将 borealis 焦点引导到 AppPage 游戏卡片，
    // 确保从任何子活动（游戏、文件列表、设置）返回时焦点落在有效视图上
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
    // 每帧检查最近游戏队列是否更新（如从文件列表启动游戏后返回）
    if (g_recentGamesDirty && m_appPage) {
        g_recentGamesDirty = false;
        bklog::debug("StartPageView::draw: refreshing recent games list");
        refreshRecentGames();
        // 重建了卡片行，将焦点转给新的第一张卡片，防止 borealis 尝试恢复已删除卡片的焦点
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
