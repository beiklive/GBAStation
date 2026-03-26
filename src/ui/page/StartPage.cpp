#include "StartPage.hpp"

StartPage::StartPage()
{
    auto *audioPlayer = brls::Application::getAudioPlayer();
    brls::Application::setAudioPlayer(nullptr); // 切换音频播放器以停止当前播放的音乐
    brls::Logger::debug("StartPage initialized");
    brls::sync([this]()
               {
        this->showHeader(false);
        // this->showFooter(false);
        // this->showBackground(false);`
        // this->showShader(false);
        Init();
        brls::Application::giveFocus(this); });
}

StartPage::~StartPage()
{
}

void StartPage::Init()
{
    // 读取主题配置
    if (!CHECK_KEY("theme"))
    {
        SET_SETTING_KEY_INT("theme", (int)beiklive::enums::ThemeLayout::SWITCH_THEME);
    }
    int theme = GET_SETTING_KEY_INT("theme", (int)beiklive::enums::ThemeLayout::SWITCH_THEME);
    brls::Logger::debug("Current theme: " + std::to_string(theme));

    if (theme == (int)beiklive::enums::ThemeLayout::SWITCH_THEME)
    {
        _useSwitchLayout();
    }
}

void StartPage::_useSwitchLayout()
{
    brls::Logger::debug("Using SWITCH theme layout");
    switchLayout = new beiklive::SwitchLayout();
    switchLayout->setGrow(1.f);
    // TODO: 后续改为从数据库读取数据 参数为  游戏路径、标题、封面路径
    beiklive::GameList gameList = {
        {"path/to/game1.exe", "Game 1", BK_RES("img/test/test.png")},
        {"path/to/game2.exe", "Game 2", BK_RES("img/test/test.png")},
        {"path/to/game3.exe", "Game 3", BK_RES("img/test/test.png")},
        {"path/to/game4.exe", "Game 4", BK_RES("img/test/test.png")},
        {"path/to/game5.exe", "Game 5", BK_RES("img/test/test.png")},
        {"path/to/game6.exe", "Game 6", BK_RES("img/test/test.png")},
        {"path/to/game7.exe", "Game 7", BK_RES("img/test/test.png")},
        {"path/to/game8.exe", "Game 8", BK_RES("img/test/test.png")},
        {"path/to/game9.exe", "Game 9", BK_RES("img/test/test.png")},
        {"path/to/game10.exe", "Game 10", BK_RES("img/test/test.png")}};
    switchLayout->refreshGameList(&gameList);
    switchLayout->onGameActivated = [](const beiklive::GameEntry &entry)
    {
        brls::Logger::info("Game activated: " + entry.title);
    };
    switchLayout->onGameOptions = [](const beiklive::GameEntry &entry)
    {
        brls::Logger::info("Game options opened: " + entry.title);
    };

    switchLayout->onGameLibraryOpened = [this]()
    {
        brls::Logger::info("Game Library opened");
        beiklive::GameList gameList2 = {
            {"path/to/game1.exe", "subGame 1", BK_RES("img/test/test2.png")},
            {"path/to/game2.exe", "subGame 2", BK_RES("img/test/test2.png")},
            {"path/to/game3.exe", "subGame 3", BK_RES("img/test/test2.png")},
            {"path/to/game4.exe", "subGame 4", BK_RES("img/test/test2.png")},
            {"path/to/game5.exe", "subGame 5", BK_RES("img/test/test2.png")},
            {"path/to/game6.exe", "subGame 6", BK_RES("img/test/test2.png")},
            {"path/to/game7.exe", "subGame 7", BK_RES("img/test/test2.png")},
            {"path/to/game8.exe", "subGame 8", BK_RES("img/test/test2.png")},
            {"path/to/game9.exe", "subGame 9", BK_RES("img/test/test2.png")},
            {"path/to/game10.exe", "subGame 10", BK_RES("img/test/test2.png")}};
        switchLayout->refreshGameList(&gameList2);
    };

    switchLayout->onFileBrowserOpened = [this]()
    {
        brls::Logger::info("File Browser opened");
        _openFileList();
    };
    switchLayout->onDataManagementOpened = [this]()
    {
        brls::Logger::info("Data Management opened");
    };
    switchLayout->onSettingsOpened = [this]()
    {
        brls::Logger::info("Settings opened");
    };
    switchLayout->onAboutOpened = [this]()
    {
        brls::Logger::info("About opened");
    };
    switchLayout->onExitRequested = [this]()
    {
        brls::Logger::info("Exit requested");
        brls::sync([this]()
                   { brls::Application::quit(); });
    };
    this->getContentBox()->addView(switchLayout);

    brls::Application::setAudioPlayer(audioPlayer);
}

void StartPage::_openFileList()
{
    brls::Logger::debug("Opening File List Page");
    m_fileListPage = new beiklive::FileListPage();

    m_fileListPage->registerAction(
        "关闭列表",
        brls::BUTTON_X,
        [this](brls::View *)
        {
            // 此处设置按键功能
            brls::sync([this]()
                       { brls::Application::popActivity(); });

            return true;
        });
    m_fileListPage->setFliter(beiklive::enums::FilterMode::None, {".gba", ".gbc", ".gb"});
    auto *frame = new brls::AppletFrame(m_fileListPage);
    HIDE_BRLS_BAR(frame);
    brls::sync([this, frame]()
               {
                   brls::Logger::info("Pushing FileListPage activity");
                   brls::Application::pushActivity(new brls::Activity(frame));
                   m_fileListPage->showDriveList(); // Activity 入栈后再加载，确保 recycler 已在视图树中
               });
}
