#include "StartPage.hpp"
#include "ui/utils/HintsBar.hpp"

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
        // 保存当前焦点状态，确保关闭面板后能正确恢复
        View* currentFocus = brls::Application::getCurrentFocus();
        // 如果已存在，先清理
        _hideGameOptionsPanel();


        // ── 半透明遮罩层 ──
        m_optionsOverlay = new brls::Box(brls::Axis::COLUMN);
        m_optionsOverlay->setFocusable(false);
        m_optionsOverlay->setHideHighlight(true);
        m_optionsOverlay->detach();
        m_optionsOverlay->setDetachedPosition(0, 0);
        m_optionsOverlay->setWidth(1280);
        m_optionsOverlay->setHeight(720);
        m_optionsOverlay->setBackgroundColor(nvgRGBA(0, 0, 0, 120));

        // ── 右侧选项面板 ──
        m_optionsPanel = new brls::Box(brls::Axis::COLUMN);
        m_optionsPanel->setFocusable(false);
        m_optionsPanel->setWidth(320.f);
        m_optionsPanel->setHeightPercentage(100);
        m_optionsPanel->setBackgroundColor(nvgRGBA(30, 30, 32, 255));
        m_optionsPanel->setCornerRadius(12.f);
        m_optionsPanel->setPadding(24.f, 20.f, 0.f, 20.f);
        m_optionsPanel->setAlignItems(brls::AlignItems::STRETCH);

        // 游戏图标和标题
        auto* iconImg = new brls::Image();
        iconImg->setWidth(48.f);
        iconImg->setHeight(48.f);
        iconImg->setCornerRadius(8.f);
        iconImg->setScalingType(brls::ImageScalingType::FIT);
        iconImg->setMarginBottom(10.f);
        iconImg->setFocusable(false);
        if (!entry.logoPath.empty())
            iconImg->setImageFromFile(entry.logoPath);
        m_optionsPanel->addView(iconImg);

        m_optionsTitle = new brls::Label();
        m_optionsTitle->setText(entry.title.empty() ? "未知游戏" : entry.title);
        m_optionsTitle->setFontSize(22.f);
        m_optionsTitle->setTextColor(GET_THEME_COLOR("brls/text"));
        m_optionsTitle->setFocusable(false);
        m_optionsTitle->setSingleLine(true);
        m_optionsTitle->setAnimated(true);
        m_optionsTitle->setMarginBottom(20.f);
        m_optionsPanel->addView(m_optionsTitle);

        // 分隔线
        auto* divider = new brls::Rectangle(nvgRGBA(255, 255, 255, 50));
        divider->setWidthPercentage(100);
        divider->setHeight(1.f);
        divider->setMarginBottom(20.f);
        m_optionsPanel->addView(divider);


        // 在每个按钮上注册 B 键关闭，确保 BottomBar 始终显示提示
        auto closeAction = [this, currentFocus](brls::View*) {
            _hideGameOptionsPanel();
            brls::Application::giveFocus(currentFocus);
            return true;
        };

        // ── 修改映射名称 ──
        m_renameBtn = new beiklive::ButtonBox();
        m_renameBtn->setText("修改映射名称");
        m_renameBtn->setIcon(BK_RES("img/ui/setting/emu.png"));
        m_renameBtn->setMarginBottom(4.f);
        m_renameBtn->registerClickAction([this](brls::View*) {
            brls::Application::notify("修改映射名称 (待实现)");
            return true;
        });
        m_renameBtn->setCustomNavigationRoute(brls::FocusDirection::LEFT,  m_renameBtn);
        m_renameBtn->setCustomNavigationRoute(brls::FocusDirection::RIGHT, m_renameBtn);
        m_optionsPanel->addView(m_renameBtn);

        // ── 设置封面图 ──
        m_coverBtn = new beiklive::ButtonBox();
        m_coverBtn->setText("设置封面图");
        m_coverBtn->setIcon(BK_RES("img/ui/setting/display.png"));
        m_coverBtn->setMarginBottom(4.f);
        m_coverBtn->registerClickAction([this](brls::View*) {
            brls::Application::notify("设置封面图 (待实现)");
            return true;
        });
        m_coverBtn->setCustomNavigationRoute(brls::FocusDirection::LEFT,  m_coverBtn);
        m_coverBtn->setCustomNavigationRoute(brls::FocusDirection::RIGHT, m_coverBtn);
        m_optionsPanel->addView(m_coverBtn);

        // ── 删除游戏 ──
        m_deleteBtn = new beiklive::ButtonBox();
        m_deleteBtn->setText("删除游戏");
        m_deleteBtn->setIcon(BK_RES("img/ui/menu/exit.png"));
        m_deleteBtn->registerClickAction([this](brls::View*) {
            brls::Application::notify("删除游戏 (待实现)");
            return true;
        });
        m_deleteBtn->setCustomNavigationRoute(brls::FocusDirection::LEFT,  m_deleteBtn);
        m_deleteBtn->setCustomNavigationRoute(brls::FocusDirection::RIGHT, m_deleteBtn);
        m_optionsPanel->addView(m_deleteBtn);

        // 弹性空间
        m_optionsPanel->addView(new brls::Padding());

        // 把面板添加到遮罩右侧
        auto* panelWrapper = new brls::Box(brls::Axis::ROW);
        panelWrapper->setFocusable(false);
        panelWrapper->setWidthPercentage(100);
        panelWrapper->setHeightPercentage(100);
        panelWrapper->setJustifyContent(brls::JustifyContent::FLEX_END);
        panelWrapper->addView(m_optionsPanel);

        m_optionsOverlay->addView(panelWrapper);
        m_optionsPanel->registerAction("关闭", brls::BUTTON_B, closeAction);

        // ── HintsBar 按钮提示栏（仅按键提示，无时间和系统图标）──
        auto* hintsBar = new beiklive::HintsBar();
        m_optionsPanel->addView(hintsBar);

        // ── 焦点循环导航：首尾互连 ──
        m_renameBtn->setCustomNavigationRoute(brls::FocusDirection::UP,    m_deleteBtn);
        m_deleteBtn->setCustomNavigationRoute(brls::FocusDirection::DOWN,  m_renameBtn);

        this->addView(m_optionsOverlay);
        brls::Application::giveFocus(m_renameBtn);
    }

    void StartPage::_hideGameOptionsPanel()
    {
        if (m_optionsOverlay)
        {
            m_optionsOverlay->removeFromSuperView(true);
            m_optionsOverlay = nullptr;
            m_optionsPanel   = nullptr;
            m_renameBtn      = nullptr;
            m_coverBtn       = nullptr;
            m_deleteBtn      = nullptr;
            m_optionsTitle   = nullptr;
        }
    }
}
