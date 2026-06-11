#include "StartPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "core/Tools.hpp"
#include "core/ThreadPool.hpp"
#include "ui/utils/CheatMatcher.hpp"

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
        // 动态背景由 Box::setupShaderLayer 根据配置初始化
        Init();
        brls::Application::giveFocus(this); });
    }

    StartPage::~StartPage()
    {
        m_alive.store(false);
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
        // 重新读取动态背景配置（设置页面可能已修改）
        bool enableShader = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_SHADER, 1) != 0;
        this->showShader(enableShader);
        if (enableShader) {
            std::string themeStr = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, "VscodeBlack");
            if (themeStr == "Midnight")           this->setGradientTheme(GradientTheme::Midnight);
            else if (themeStr == "LemonYellow")   this->setGradientTheme(GradientTheme::LemonYellow);
            else if (themeStr == "AvocadoGreen")  this->setGradientTheme(GradientTheme::AvocadoGreen);
            else if (themeStr == "StrawberryRed") this->setGradientTheme(GradientTheme::StrawberryRed);
            else if (themeStr == "OceanBlue")     this->setGradientTheme(GradientTheme::OceanBlue);
            else if (themeStr == "SakuraPink")    this->setGradientTheme(GradientTheme::SakuraPink);
            else                                   this->setGradientTheme(GradientTheme::VscodeBlack);
        }
        // 重新读取背景图片配置
        bool enableBg = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        this->showBackground(enableBg);
        if (enableBg) {
            std::string bgPath = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
            if (!bgPath.empty())
                this->setBackgroundImage(bgPath);
        }
        // 每次回到起始页时刷新游戏列表，获取最新的最近玩过的10款游戏
        if (switchLayout)
        {
            ThreadPool::instance().enqueue([this]() {
                if (!m_alive.load()) return;
                beiklive::GameList recent = beiklive::GameDB
                    ? beiklive::GameDB->getRecentPlayed(10)
                    : beiklive::GameList{};
                brls::sync([this, recent = std::move(recent)]() {
                    if (!m_alive.load()) return;
                    switchLayout->refreshGameList(recent);
                    auto& children = switchLayout->getContentBox()->getChildren();
                    if (!children.empty())
                        brls::Application::giveFocus(children[0]->getDefaultFocus());
                });
            });
        }
    }

    void StartPage::willAppear(bool resetState)
    {
        brls::Box::willAppear(resetState);
        onResume();
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
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(entry.path)
                : std::optional<beiklive::GameEntry>{};
            const auto& e = fresh.has_value() ? *fresh : entry;
            brls::Logger::info("Game activated: " + e.title);
            {
                m_gamePage = new beiklive::GamePage(e);
                auto *frame = new brls::AppletFrame(m_gamePage);
                HIDE_BRLS_BAR(frame);
                brls::Logger::info("Pushing GamePage activity for: " + e.title);
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
        if (!beiklive::GameDB || beiklive::GameDB->getAll().empty()) {
            auto* dialog = new brls::Dialog("游戏库为空，请从文件列表选择游戏或者从设置中进行游戏导入");
            dialog->addButton("确定", [](){});
            dialog->open();
            return;
        }

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
        m_fileListPage->setFliter(beiklive::enums::FilterMode::Whitelist, {"gba", "gbc", "gb", "nes", "fds", "sfc", "smc", "md", "gen", "bin", "smd", "sms", "gg", "sg", "cue", "png"});

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
            case beiklive::enums::FileType::NES_ROM:
            case beiklive::enums::FileType::SNES_ROM:
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
        std::string romPath = entry.path;
        std::string path = entry.path;

        _hideGameOptionsPanel();

        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);
        std::string filename = beiklive::tools::getFileNameWithoutExtension(entry.path);

        // ── 修改映射名称 ──
        m_gameOptionsSidebar->addButton("修改映射名称", BK_RES("img/ui/setting/emu.png"),
            [this, path, title = entry.title, filename](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) return;
                ime->openForText(
                    [this, path, filename](std::string text) {
                        if (!text.empty() && beiklive::GameDB) {
                            beiklive::GameDB->set(path, "title", nlohmann::json(text));
                            onResume();
                            beiklive::NameMappingManager->Set(filename, text, true);
                            beiklive::GameDB->flush();
                            beiklive::NameMappingManager->Save();
                        }
                    },
                    "编辑游戏名称",     // header
                    "",                  // subText
                    128,                 // maxLength
                    title,               // initialText (当前名称)
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            });

        // ── 设置封面图 ──
        m_gameOptionsSidebar->addButton("设置封面图", BK_RES("img/ui/setting/display.png"),
            [this, path, romPath](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                beiklive::openFilePicker({"png", "jpg"},
                    [this, path](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(path, "logoPath", nlohmann::json(selectedPath));
                            onResume();
                            beiklive::GameDB->flush();
                        }
                    },
                    beiklive::path::GetRootPath());
            });

        // ── 删除游戏 ──
        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, path](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                auto* dialog = new brls::Dialog("确定要删除该游戏吗？\n此操作将清除游戏记录与存档数据。");
                dialog->addButton("确认删除", [this, path]() {
                    _hideGameOptionsPanel();
                    if (beiklive::GameDB) {
                        if ((int)beiklive::GameDB->getAll().size() <= 1)
                            beiklive::GameDB->clearAll();
                        else {
                            beiklive::GameDB->removeByPath(path);
                            beiklive::GameDB->flush();
                        }
                        brls::Application::notify("已删除游戏");
                        onResume();
                    } else {
                        brls::Application::notify("删除失败");
                    }
                });
                dialog->addButton("取消", [this]() {

                });
                dialog->open();
            });

        // ── 收藏 ──
        m_gameOptionsSidebar->addButton(
            entry.favourite ? "取消收藏" : "加入收藏",
            BK_RES("img/ui/setting/emu.png"),
            [this, path, fav = entry.favourite](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                std::string msg = fav ? "确定要取消收藏吗？" : "确定要加入收藏吗？";
                auto* dlg = new brls::Dialog(msg);
                dlg->addButton("确认", [this, path, fav]() {
                    if (beiklive::GameDB) {
                        beiklive::GameDB->set(path, "favourite", nlohmann::json(!fav));
                        beiklive::GameDB->flush();
                        onResume();
                    }
                });
                dlg->addButton("取消", [](){});
                dlg->open();
            });

        // ── 自动匹配金手指 ──
        // m_gameOptionsSidebar->addButton("自动匹配金手指", BK_RES("img/ui/setting/emu.png"),
        //     [this, path, platform = entry.platform, romPath = entry.path](const beiklive::GameEntry&) {
        //         _hideGameOptionsPanel();
        //         auto* dlg = new brls::Dialog("此功能目前处于测试阶段，\n使用前请慎重考虑");
        //         dlg->addButton("取消", []() {});
        //         dlg->addButton("我先试试", [this, path, platform, romPath]() {
        //             beiklive::startCheatMatching(platform, romPath,
        //                 [this, path](const std::string& cheatPath) {
        //                     if (!cheatPath.empty() && beiklive::GameDB) {
        //                         beiklive::GameDB->set(path, "cheatPath", nlohmann::json(cheatPath));
        //                         beiklive::GameDB->flush();
        //                         onResume();
        //                     }
        //                 });
        //         });
        //         dlg->open();
        //     });

        m_gameOptionsSidebar->onClosed = [this, currentFocus]() {
            brls::Application::giveFocus(currentFocus);
        this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);

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
        this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);

        }
    }
}
