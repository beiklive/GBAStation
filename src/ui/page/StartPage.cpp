#include "StartPage.hpp"

namespace beiklive
{
    StartPage::StartPage()
    {
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

    void StartPage::onResume()
    {
        brls::Logger::debug("StartPage onResume called");
        // 每次回到起始页时刷新游戏列表，获取最新的最近玩过的10款游戏
        if (switchLayout)
        {
            beiklive::GameList recent = beiklive::GameDB->getRecentPlayed(10);
            switchLayout->refreshGameList(recent);
        }
    }

    void StartPage::_useSwitchLayout()
    {
        brls::Logger::debug("Using SWITCH theme layout");
        switchLayout = new beiklive::SwitchLayout();
        switchLayout->setGrow(1.f);
        // TODO: 后续改为从数据库读取数据 参数为  游戏路径、标题、封面路径

        onResume(); // 刷新游戏列表显示

        switchLayout->onGameActivated = [this](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game activated: " + entry.title);
            {
                m_gamePage = new beiklive::GamePage(entry);
                auto *frame = new brls::AppletFrame(m_gamePage);
                HIDE_BRLS_BAR(frame);
                brls::Logger::info("Pushing GamePage activity for: " + entry.title);
                brls::sync([this, frame]()
                           { brls::Application::pushActivity(new brls::Activity(frame)); });
            }
        };
        switchLayout->onGameOptions = [](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game options opened: " + entry.title);
            static int i = 0;
            i++;
            switch (i % 4)
            {
            case 0:
                brls::Application::getAudioPlayer()->play(brls::Sound::SOUND_FOCUS_CHANGE);
                brls::Application::notify("Sound::SOUND_FOCUS_CHANGE");
                break;
            case 1:
                brls::Application::getAudioPlayer()->play(brls::Sound::SOUND_FOCUS_CHANGE);
                brls::Application::notify("Sound::SOUND_FOCUS_LOST");
                break;
            case 2:
                brls::Application::getAudioPlayer()->play(brls::Sound::SOUND_FOCUS_ERROR);
                brls::Application::notify("Sound::SOUND_FOCUS_ERROR");
                break;
            case 3:
                brls::Application::getAudioPlayer()->play(brls::Sound::SOUND_SLIDER_TICK);
                brls::Application::notify("Sound::SOUND_SLIDER_TICK");
                break;

            default:
                break;
            }

            
            


        };

        switchLayout->onGameLibraryOpened = [this]()
        {
            brls::Logger::info("Game Library opened");
            _openGameLibrary();
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
            _openSettings();
        };
        switchLayout->onAboutOpened = [this]()
        {
            brls::Logger::info("About opened");
            _openAbout();
        };
        switchLayout->onExitRequested = [this]()
        {
            brls::Logger::info("Exit requested");
            brls::sync([this]()
                       { brls::Application::quit(); });
        };
        this->getContentBox()->addView(switchLayout);
    }

    void StartPage::_openGameLibrary()
    {
        brls::Logger::debug("Opening Game Library Page");
        auto *gameLibraryPage = new beiklive::GameLibraryPage();
        auto *frame           = new brls::AppletFrame(gameLibraryPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([frame]()
                   { brls::Application::pushActivity(new brls::Activity(frame)); });
    }

    void StartPage::_openFileList()
    {
        brls::Logger::debug("Opening File List Page");
        m_fileListPage = new beiklive::FileListPage();

        m_fileListPage->registerAction(
            "关闭列表",
            brls::BUTTON_START,
            [this](brls::View *)
            {
                // 此处设置按键功能
                brls::sync([this]()
                           { brls::Application::popActivity(); });

                return true;
            });
        m_fileListPage->setFliter(beiklive::enums::FilterMode::None, {".gba", ".gbc", ".gb"});

        m_fileListPage->onFileSelected = [this](beiklive::DirListData dirItem)
        {
            switch (dirItem.itemType)
            {
            case beiklive::enums::FileType::IMAGE_FILE:
                brls::Application::notify("查看图片：" + dirItem.fileName);
                break;
            case beiklive::enums::FileType::GBA_ROM:
            case beiklive::enums::FileType::GBC_ROM:
            case beiklive::enums::FileType::GB_ROM:
                brls::Application::notify("启动游戏：" + dirItem.fileName);
                {
                    m_gamePage = new beiklive::GamePage(dirItem);
                    auto *frame = new brls::AppletFrame(m_gamePage);
                    HIDE_BRLS_BAR(frame);
                    brls::Logger::info("Pushing GamePage activity for: " + dirItem.fileName);
                    brls::sync([this, frame]()
                               { brls::Application::pushActivity(new brls::Activity(frame)); });
                }
                break;
            default:
                brls::Logger::debug("Selected item: " + dirItem.fileName + ", type: " + std::to_string((int)dirItem.itemType));
                break;
            }
        };

        auto *frame = new brls::AppletFrame(m_fileListPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([this, frame]()
                   {
                       brls::Logger::info("Pushing FileListPage activity");
                       brls::Application::pushActivity(new brls::Activity(frame));
                       m_fileListPage->showDriveList(); // Activity 入栈后再加载，确保 recycler 已在视图树中
                   });
    }

    void StartPage::_openSettings()
    {
        brls::Logger::debug("Opening Settings Page");
        auto *settingPage = new beiklive::SettingPage();
        auto *frame       = new brls::AppletFrame(settingPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([frame]()
                   { brls::Application::pushActivity(new brls::Activity(frame)); });
    }

    void StartPage::_openAbout()
    {
        brls::Logger::debug("Opening About Page");
        auto *aboutPage = new beiklive::AboutPage();
        auto *frame     = new brls::AppletFrame(aboutPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([frame]()
                   { brls::Application::pushActivity(new brls::Activity(frame)); });
    }
}
