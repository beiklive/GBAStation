#include "StartPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "core/Tools.hpp"
#include "core/ThreadPool.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include <borealis/views/dropdown.hpp>
#include <filesystem>

#ifdef __SWITCH__
#include "platform/switch/NroLauncher.hpp"
#endif

namespace
{

bool deleteGameFileIfExists(const std::string& path)
{
    if (path.empty())
        return true;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return true;

    ec.clear();
    return std::filesystem::remove(path, ec) && !ec;
}

} // namespace

namespace beiklive
{
    namespace
    {
        constexpr long START_PAGE_REFRESH_DEFER_MS = 260;

        bool shouldUseNdsExternalNro(const beiklive::GameEntry& entry)
        {
#ifdef __SWITCH__
            return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
#else
            (void)entry;
            return false;
#endif
        }

        bool shouldUseNdsExternalNro(const beiklive::DirListData& dirItem)
        {
#ifdef __SWITCH__
            return dirItem.itemType == beiklive::enums::FileType::NDS_ROM;
#else
            (void)dirItem;
            return false;
#endif
        }

        void ensureGameDbEntryForFileLaunch(const beiklive::DirListData& dirItem)
        {
            if (!beiklive::GameDB || dirItem.fullPath.empty())
                return;

            auto entryOpt = beiklive::GameDB->findByPath(dirItem.fullPath);
            beiklive::GameEntry entry = entryOpt.value_or(beiklive::GameEntry{});
            bool changed = !entryOpt.has_value();

            const int platform = static_cast<int>(dirItem.itemType);
            const std::string stem = beiklive::tools::getFileNameWithoutExtension(dirItem.fileName);

            if (entry.path.empty()) {
                entry.path = dirItem.fullPath;
                changed = true;
            }
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::NONE)) {
                entry.platform = platform;
                changed = true;
            }
            if (entry.core.empty()) {
                entry.core = beiklive::GetDefaultCoreId(platform);
                changed = true;
            }
            entry.core = beiklive::NormalizeCoreId(entry.platform, entry.core);
            if (entry.title.empty()) {
                entry.title = GET_MAPPING_KEY_STR(stem, stem);
                changed = true;
            }
            if (entry.savePath.empty()) {
                entry.savePath = beiklive::tools::defaultGameSavePath(entry.platform, entry.path);
                changed = true;
            }
            if (entry.logoPath.empty()) {
                entry.logoPath = beiklive::tools::getDefaultLogoPath(
                    static_cast<beiklive::enums::EmuPlatform>(entry.platform));
                changed = true;
            }
            if (entry.screenShotPath.empty()) {
                entry.screenShotPath = beiklive::path::screenshotPath();
                changed = true;
            }

            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                if (entry.ndsScreenLayout.empty()) {
                    entry.ndsScreenLayout = "priority_top";
                    changed = true;
                }
                if (entry.ndsScreenOrientation.empty()) {
                    entry.ndsScreenOrientation = "0";
                    changed = true;
                }
            }

            std::error_code ec;
            std::filesystem::create_directories(entry.savePath, ec);
            beiklive::GameDB->upsertByPath(entry);
            if (changed)
                brls::Logger::info("StartPage: added file launch entry to GameDB: {}", entry.path);
            beiklive::GameDB->flush();
        }

#ifdef __SWITCH__
        bool launchNdsExternalNro(const std::string& romPath, const std::string& title)
        {
            const std::string nroPath = GET_SETTING_KEY_STR("nds.externalNro.path", "/GBAStation/core/GBAStationNDSStub.nro");
            const std::string returnPath = GET_SETTING_KEY_STR("nds.externalNro.returnPath", "sdmc:/switch/GBAStation.nro");

            auto result = beiklive::switch_platform::launchNroOnExit({nroPath, romPath, returnPath});
            if (!result.success)
            {
                brls::Logger::error("NDS external NRO launch failed for {}: {}", title, result.message);
                brls::Application::notify("NDS独立NRO启动失败：" + result.message);
                return false;
            }

            brls::Logger::info("NDS external NRO configured for {}: {}", title, result.message);
            brls::Application::notify("正在启动NDS独立NRO...");
            brls::sync([]() { brls::Application::quit(); });
            return true;
        }
#endif
    }

    StartPage::StartPage()
    {
        brls::Logger::debug("StartPage initialized");
        brls::sync([this]()
                   {
        this->showHeader(false);
        this->hideFooterLine();
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
        _applyRuntimeUiSettings();
        _requestRecentGamesRefresh(true);
    }

    void StartPage::_applyRuntimeUiSettings()
    {
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
    }

    void StartPage::_requestRecentGamesRefresh(bool defer)
    {
        // 每次回到起始页时刷新游戏列表，获取最新的最近玩过的10款游戏
        if (!switchLayout)
            return;

        int gen = ++m_recentRefreshGen;
        auto dispatchRefresh = [this, gen]() {
            if (!m_alive.load() || gen != m_recentRefreshGen.load())
                return;

            ThreadPool::instance().enqueue([this, gen]() {
                if (!m_alive.load() || gen != m_recentRefreshGen.load()) return;

                beiklive::GameList recent = beiklive::GameDB
                    ? beiklive::GameDB->getRecentPlayed(10)
                    : beiklive::GameList{};

                brls::sync([this, gen, recent = std::move(recent)]() {
                    if (!m_alive.load() || gen != m_recentRefreshGen.load() || !switchLayout) return;

                    brls::View* currentFocus = brls::Application::getCurrentFocus();
                    bool needInitialCardFocus = !currentFocus || currentFocus == this || currentFocus->isHidden();
                    switchLayout->refreshGameList(recent);
                    if (m_resetCardFocusOnNextRefresh)
                    {
                        m_resetCardFocusOnNextRefresh = false;
                        switchLayout->resetCardFocusToFirst();
                    }
                    else if (needInitialCardFocus)
                        switchLayout->restoreCardFocus(false);
                });
            });
        };

        if (defer)
            brls::delay(START_PAGE_REFRESH_DEFER_MS, dispatchRefresh);
        else
            dispatchRefresh();
    }

    void StartPage::willAppear(bool resetState)
    {
        brls::Box::willAppear(resetState);
        onResume();
    }

    void StartPage::_pushGameActivity(const beiklive::GameEntry& entry, beiklive::Box* previousPage)
    {
        if (shouldUseNdsExternalNro(entry))
        {
#ifdef __SWITCH__
            launchNdsExternalNro(entry.path, entry.title);
            return;
#endif
        }

        auto* gamePage = new beiklive::GamePage(entry);
        m_gamePage = gamePage;
        auto* frame = new brls::AppletFrame(gamePage);
        HIDE_BRLS_BAR(frame);
        brls::Logger::info("Pushing GamePage activity for: " + entry.title);
        brls::sync([frame, previousPage, gamePage]() {
            beiklive::pushActivity(frame, previousPage, gamePage, [gamePage]() { gamePage->startGame(); });
        });
    }

    void StartPage::_pushGameActivity(const beiklive::DirListData& dirItem, beiklive::Box* previousPage)
    {
        if (shouldUseNdsExternalNro(dirItem))
        {
            ensureGameDbEntryForFileLaunch(dirItem);
#ifdef __SWITCH__
            launchNdsExternalNro(dirItem.fullPath, dirItem.fileName);
            return;
#endif
        }

        auto* gamePage = new beiklive::GamePage(dirItem);
        m_gamePage = gamePage;
        auto* frame = new brls::AppletFrame(gamePage);
        HIDE_BRLS_BAR(frame);
        brls::Logger::info("Pushing GamePage activity for: " + dirItem.fileName);
        brls::sync([frame, previousPage, gamePage]() {
            beiklive::pushActivity(frame, previousPage, gamePage, [gamePage]() { gamePage->startGame(); });
        });
    }

    void StartPage::_useSwitchLayout()
    {
        brls::Logger::debug("Using SWITCH theme layout");
        switchLayout = new beiklive::SwitchLayout();
        switchLayout->setGrow(1.f);
        // TODO: 后续改为从数据库读取数据 参数为  游戏路径、标题、封面路径

        switchLayout->onGameActivated = [this](const beiklive::GameEntry &entry)
        {
            m_resetCardFocusOnNextRefresh = true;
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(entry.path)
                : std::optional<beiklive::GameEntry>{};
            const auto& e = fresh.has_value() ? *fresh : entry;
            brls::Logger::info("Game activated: " + e.title);
            _pushGameActivity(e, this);
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
        _requestRecentGamesRefresh(true);
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

        gameLibraryPage->onGameSelected = [this, gameLibraryPage](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game selected from library: " + entry.title);
            _pushGameActivity(entry, gameLibraryPage);
        };

        HIDE_BRLS_BAR(frame);
        brls::sync([frame, this, gameLibraryPage]()
                   { beiklive::pushActivity(frame, this, gameLibraryPage); });
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
                brls::sync([this]()
                           { beiklive::popActivity(m_fileListPage); });

                return true;
            });
        m_fileListPage->setFliter(beiklive::enums::FilterMode::Whitelist, {"gba", "gbc", "gb", "nes", "fds", "sfc", "smc", "nds", "md", "gen", "bin", "smd", "sms", "gg", "sg", "cue", "png"});

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
            case beiklive::enums::FileType::NDS_ROM:
                brls::Application::notify("启动游戏：" + dirItem.fileName);
                _pushGameActivity(dirItem, this);
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
                       beiklive::pushActivity(frame, this, m_fileListPage);
                       m_fileListPage->showDriveList();
                   });
    }

    void StartPage::_openSettings()
    {
        brls::Logger::debug("Opening Settings Page");
        auto *settingPage = new beiklive::SettingPage();
        auto *frame       = new brls::AppletFrame(settingPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([this, frame, settingPage]()
                   { beiklive::pushActivity(frame, this, settingPage); });
    }

    void StartPage::_openAbout()
    {
        brls::Logger::debug("Opening About Page");
        auto *aboutPage = new beiklive::AboutPage();
        auto *frame     = new brls::AppletFrame(aboutPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([this, frame, aboutPage]()
                   { beiklive::pushActivity(frame, this, aboutPage); });
    }

    void StartPage::_openDataManagement()
    {
        brls::Logger::debug("Opening Data Management Page");
        auto *dataPage = new beiklive::DataManagementPage();
        auto *frame    = new brls::AppletFrame(dataPage);
        HIDE_BRLS_BAR(frame);
        brls::sync([this, frame, dataPage]()
                   { beiklive::pushActivity(frame, this, dataPage); });
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
                std::filesystem::path currentLogo(e.logoPath);
                beiklive::openFilePicker({"png", "jpg"},
                    [this, path](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(path, "logoPath", nlohmann::json(selectedPath));
                            onResume();
                            beiklive::GameDB->flush();
                        }
                    },
                    currentLogo.parent_path().empty() ? beiklive::path::GetRootPath() : currentLogo.parent_path().string(),
                    currentLogo.filename().string());
            });

        // ── 核心选择 ──
        if (beiklive::GetCoreOptions(entry.platform).size() > 1)
        {
            m_gameOptionsSidebar->addButton("核心选择", BK_RES("img/ui/setting/emu.png"),
                [this, path, platform = entry.platform, core = entry.core](const beiklive::GameEntry&) {
                    _hideGameOptionsPanel();

                    const auto options = beiklive::GetCoreOptions(platform);
                    std::vector<std::string> names;
                    names.reserve(options.size());
                    for (const auto& option : options)
                        names.push_back(option.name);

                    auto* dropdown = new brls::Dropdown(
                        "核心选择",
                        names,
                        [this, path, options](int selected) {
                            if (selected < 0 || selected >= static_cast<int>(options.size()))
                                return;
                            if (beiklive::GameDB) {
                                beiklive::GameDB->set(path, "core", nlohmann::json(options[selected].id));
                                beiklive::GameDB->flush();
                                brls::Application::notify("已切换核心：" + options[selected].name);
                                onResume();
                            }
                        },
                        beiklive::GetCoreSelectionIndex(platform, core));
                    brls::Application::pushActivity(new brls::Activity(dropdown));
                });
        }

        // ── 删除游戏 ──
        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, path](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                auto* removeDialog = new brls::Dialog("是否从游戏库移除该游戏？");
                removeDialog->addButton("是", [this, path]() {
                    auto* romDialog = new brls::Dialog("是否删除 ROM 文件？");
                    auto deleteGame = [this, path](bool deleteRomFile) {
                        if (beiklive::GameDB) {
                            bool removedRecord = false;
                            if ((int)beiklive::GameDB->getAll().size() <= 1)
                            {
                                beiklive::GameDB->clearAll();
                                removedRecord = true;
                            }
                            else {
                                removedRecord = beiklive::GameDB->removeByPath(path);
                                if (removedRecord)
                                    beiklive::GameDB->flush();
                            }

                            bool removedFile = true;
                            if (removedRecord && deleteRomFile)
                                removedFile = deleteGameFileIfExists(path);

                            if (!removedRecord)
                                brls::Application::notify("删除失败");
                            else if (deleteRomFile)
                                brls::Application::notify(removedFile ? "已删除游戏" : "已移除记录，ROM 文件删除失败");
                            else
                                brls::Application::notify("已从游戏库移除该游戏");
                            onResume();
                        } else {
                            brls::Application::notify("删除失败");
                        }
                    };
                    romDialog->addButton("是", [deleteGame]() { deleteGame(true); });
                    romDialog->addButton("否", [deleteGame]() { deleteGame(false); });
                    romDialog->addButton("不删了", []() {
                    });
                    romDialog->open();
                });
                removeDialog->addButton("否", []() {
                });
                removeDialog->addButton("不删了", []() {
                });
                removeDialog->open();
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
