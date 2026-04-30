#include "StartPage.hpp"

namespace beiklive
{
    StartPage::StartPage()
    {
        brls::Logger::debug("StartPage initialized");
        brls::sync([this]()
                   {
        this->showHeader(true);
        // this->showFooter(false);
        // this->showBackground(true);
        this->showShader(true);
        this->setGradientTheme(GradientTheme::VscodeBlack);
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
    switchLayout->onGameOptions = [this](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game options opened: " + entry.title);
            _showGameOptionsPanel(entry);
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
            _openDataManagement();
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

        gameLibraryPage->onGameSelected = [this](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game selected from library: " + entry.title);
            {
                m_gamePage = new beiklive::GamePage(entry);
                auto *frame = new brls::AppletFrame(m_gamePage);
                HIDE_BRLS_BAR(frame);
                brls::Logger::info("Pushing GamePage activity for: " + entry.title);
                brls::sync([this, frame]()
                           { brls::Application::pushActivity(new brls::Activity(frame)); });
            }
        };

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

    void StartPage::_openDataManagement()
    {
        brls::Logger::debug("Opening Data Management Page");
        auto *dataPage = new beiklive::DataManagementPage();
        auto *frame    = new brls::AppletFrame(dataPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([frame]()
                   { brls::Application::pushActivity(new brls::Activity(frame)); });
    }

    void StartPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        auto* currentFocus = brls::Application::getCurrentFocus();

        _hideGameOptionsPanel();

        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();

        m_gameOptionsSidebar->addButton(
            "修改映射名称", 
            BK_RES("img/ui/setting/emu.png"),
            [](const beiklive::GameEntry& e) 
            { 
                brls::Application::notify("修改映射名称 (待实现)"); 
                // 获取当前游戏文件名
                std::string name = beiklive::tools::getFileNameWithoutExtension(e.path);
                // TODO 打开输入法界面，允许用户输入新的映射名称
                
                // TODO 输入完成后更新NameMappingManager以及数据库中的title字段并刷新界面显示


            });
        m_gameOptionsSidebar->addButton(
            "设置封面图", 
            BK_RES("img/ui/setting/display.png"),
            [](const beiklive::GameEntry& e) 
            { 
                brls::Application::notify("设置封面图 (待实现)"); 
                // TODO 打开FileListPage界面， 后缀白名单设置为 png ,允许用户选择新的封面图文件,选择完成后更新GameDB中logoPath字段并刷新界面显示
            });
        m_gameOptionsSidebar->addButton(
            "从游戏库移除", 
            BK_RES("img/ui/menu/exit.png"),
            [](const beiklive::GameEntry& e) 
            { 
                brls::Application::notify("从游戏库移除 (待实现)"); 
                // TODO 弹出对话框确认是否移除，确认后从GameDB中删除该游戏记录并刷新游戏库显示
            });

        m_gameOptionsSidebar->onClosed = [this, currentFocus]() {
            brls::Application::giveFocus(currentFocus);
        };

        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(entry);
    }

    void StartPage::_hideGameOptionsPanel()
    {
        if (m_gameOptionsSidebar)
        {
            m_gameOptionsSidebar->close();
            m_gameOptionsSidebar->removeFromSuperView(true);
            m_gameOptionsSidebar = nullptr;
        }
    }
}
