#include "StartPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "core/Tools.hpp"
#include "core/ThreadPool.hpp"
#include "core/forwarder/ForwarderInstaller.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/utils/NdsEnvironment.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include <borealis/views/dropdown.hpp>
#include <algorithm>
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
        // Leave the first frame free, then prepare the complete library snapshot.
        constexpr long START_PAGE_REFRESH_DEFER_MS = 16;

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
        this->showFooter(false);
        // this->showBackground(true);
        // 动态背景由 Box::setupShaderLayer 根据配置初始化
        Init();
        brls::Application::giveFocus(this); });
    }

    StartPage::~StartPage()
    {
        m_alive.store(false);
        m_aliveToken->store(false);
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

    void StartPage::onActivityResume()
    {
        if (switchLayout)
            switchLayout->playEntranceAnimation();
        onResume();
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
        if (!switchLayout || m_homeDeletePending)
            return;

        int gen = ++m_recentRefreshGen;
        auto dispatchRefresh = [this, gen]() {
            if (!m_alive.load() || gen != m_recentRefreshGen.load())
                return;

            ThreadPool::instance().enqueuePriority([this, gen]() {
                if (!m_alive.load() || gen != m_recentRefreshGen.load()) return;

                auto prepared = beiklive::GameLibraryPage::prepareInitialData();
                beiklive::GameList recent;
                const size_t recentCount = std::min<size_t>(
                    10, prepared.entries.size());
                recent.reserve(recentCount);
                recent.insert(recent.end(), prepared.entries.begin(),
                              prepared.entries.begin() + recentCount);

                brls::sync([this, gen, recent = std::move(recent),
                            prepared = std::move(prepared)]() mutable {
                    if (!m_alive.load() || gen != m_recentRefreshGen.load() || !switchLayout) return;

                    m_libraryPreparedData = std::move(prepared);
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
        onActivityResume();
    }

    bool StartPage::_pushGameActivity(const beiklive::GameEntry& entry,
                                      beiklive::Box* previousPage)
    {
        if (!beiklive::tools::isFileExists(entry.path)) {
            brls::Application::notify("文件不存在: " + entry.title);
            return false;
        }
        if (shouldUseNdsExternalNro(entry))
        {
#ifdef __SWITCH__
            if (!beiklive::ensureNdsEnvironmentReady())
                return false;
            return launchNdsExternalNro(entry.path, entry.title);
#endif
        }

        auto* gamePage = new beiklive::GamePage(entry);
        m_gamePage = gamePage;
        auto* frame = new brls::AppletFrame(gamePage);
        HIDE_BRLS_BAR(frame);
        brls::Logger::info("Pushing GamePage activity for: " + entry.title);
        // GamePage 退出仍使用 beiklive::popActivity，因此保留其返回页记录，
        // 但推入过程直接交给 Borealis，避免再次播放页面级隐藏动画。
        beiklive::g_beiklive_boxes.push_back(previousPage);
        brls::Application::pushActivity(
            new brls::Activity(frame), brls::TransitionAnimation::NONE);
        if (auto* library = dynamic_cast<beiklive::GameLibraryPage*>(previousPage))
            library->resetLaunchOverlay();
        gamePage->startGame();
        return true;
    }

    void StartPage::_pushGameActivity(const beiklive::DirListData& dirItem, beiklive::Box* previousPage)
    {
        if (!beiklive::tools::isFileExists(dirItem.fullPath)) {
            brls::Application::notify("文件不存在: " + dirItem.fileName);
            return;
        }
        if (shouldUseNdsExternalNro(dirItem))
        {
            if (!beiklive::ensureNdsEnvironmentReady())
                return;
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
        beiklive::g_beiklive_boxes.push_back(previousPage);
        brls::Application::pushActivity(
            new brls::Activity(frame), brls::TransitionAnimation::NONE);
        gamePage->startGame();
    }

    void StartPage::_useSwitchLayout()
    {
        brls::Logger::debug("Using SWITCH theme layout");
        switchLayout = new beiklive::SwitchLayout();
        switchLayout->setGrow(1.f);
        switchLayout->onGameActivated = [this](const beiklive::GameEntry &entry)
        {
            m_resetCardFocusOnNextRefresh = true;
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(entry.path)
                : std::optional<beiklive::GameEntry>{};
            const auto& e = fresh.has_value() ? *fresh : entry;
            brls::Logger::info("Game activated: " + e.title);
            if (!_pushGameActivity(e, this)) {
                m_gameLaunchPending = false;
                if (switchLayout) {
                    switchLayout->playEntranceAnimation();
                    switchLayout->restoreCardFocus(false);
                }
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
        switchLayout->onPico8Opened = [this]()
        {
            brls::Logger::info("PICO-8 shortcut opened");
            _openPico8Page();
        };
        switchLayout->onExitRequested = [this]()
        {
            brls::Logger::info("Exit requested");
            if (switchLayout) {
                switchLayout->playExitAnimation([]() {
                    brls::Application::quit();
                });
            } else {
                brls::Application::quit();
            }
        };
        this->getContentBox()->addView(switchLayout);
        _requestRecentGamesRefresh(true);
    }

    void StartPage::_openPico8Page()
    {
        if (!switchLayout) {
            brls::Application::unblockInputs();
            return;
        }
        auto* pico8Page = new beiklive::Pico8Page(switchLayout);
        brls::Application::pushActivity(
            new brls::Activity(pico8Page),
            brls::TransitionAnimation::NONE);
        brls::Application::unblockInputs();
    }

    void StartPage::_openGameLibrary()
    {
        if (!beiklive::GameDB ||
            (m_libraryPreparedData.ready &&
             m_libraryPreparedData.entries.empty())) {
            auto* dialog = new brls::Dialog("游戏库为空，请从文件列表选择游戏或者从设置中进行游戏导入");
            dialog->addButton("确定", [](){});
            dialog->open();
            return;
        }

        brls::Logger::debug("Opening Game Library Page");
        auto prepared = std::move(m_libraryPreparedData);
        m_libraryPreparedData = {};
        auto pushLibrary = [this, prepared = std::move(prepared)]() mutable {
            auto* gameLibraryPage =
                new beiklive::GameLibraryPage(std::move(prepared));
            auto* frame = new brls::AppletFrame(gameLibraryPage);
            gameLibraryPage->onGameSelected =
                [this, gameLibraryPage](const beiklive::GameEntry& entry) {
                    brls::Logger::info(
                        "Game selected from library: " + entry.title);
                    _pushGameActivity(entry, gameLibraryPage);
                };
            HIDE_BRLS_BAR(frame);
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushLibrary));
        else
            pushLibrary();
    }

    void StartPage::_openFileList()
    {
        brls::Logger::debug("Opening File List Page");
        m_fileListPage = new beiklive::FileListPage();
        m_fileListPage->onRequestClose = [this]() {
            beiklive::popActivity(m_fileListPage);
        };

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
        auto pushPage = [this, frame]() {
            brls::Logger::info("Pushing FileListPage activity");
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
            m_fileListPage->showDriveList();
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_openSettings()
    {
        brls::Logger::debug("Opening Settings Page");
        auto *settingPage = new beiklive::SettingPage();
        auto *frame       = new brls::AppletFrame(settingPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_openAbout()
    {
        brls::Logger::debug("Opening About Page");
        auto *aboutPage = new beiklive::AboutPage();
        auto *frame     = new brls::AppletFrame(aboutPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_openDataManagement()
    {
        brls::Logger::debug("Opening Data Management Page");
        auto *dataPage = new beiklive::DataManagementPage();
        auto *frame    = new brls::AppletFrame(dataPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        m_gameOptionsSidebar->setNanoVgMenu(true);
        m_gameOptionsSidebar->setLaunchFadeToBlack(true);
        if (switchLayout) {
            m_gameOptionsSidebar->setNanoVgPreviewImageHandle(
                switchLayout->acquireSelectedCoverTexture());
        }
        this->getBottomBar()->setVisibility(brls::Visibility::GONE);

        const std::string path = entry.path;
        const std::string filename =
            beiklive::tools::getFileNameWithoutExtension(entry.path);

        m_gameOptionsSidebar->addButton(
            "启动游戏", beiklive::material::PLAY_ARROW,
            [this, entry](const beiklive::GameEntry&) {
                if (entry.platform == static_cast<int>(
                        beiklive::enums::EmuPlatform::EmuNDS) &&
                    !beiklive::ensureNdsEnvironmentReady())
                    return;
                m_gameLaunchPending = true;
                if (switchLayout)
                    switchLayout->playExitAnimation();
                _closeGameOptionsPanelAnimated([this, entry]() {
                    if (switchLayout && switchLayout->onGameActivated)
                        switchLayout->onGameActivated(entry);
                    m_gameLaunchPending = false;
                }, true);
            });

        m_gameOptionsSidebar->addButton(
            entry.favourite ? "取消收藏" : "加入收藏",
            entry.favourite ? beiklive::material::FAVORITE : beiklive::material::FAVORITE_BORDER,
            [this, path, fav = entry.favourite](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, path, fav]() {
                    const std::string msg = fav
                        ? "确定要取消收藏吗？"
                        : "确定要加入收藏吗？";
                    auto* dlg = new brls::Dialog(msg);
                    dlg->addButton("确认", [this, path, fav]() {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(
                                path, "favourite", nlohmann::json(!fav));
                            beiklive::GameDB->flush();
                            _requestRecentGamesRefresh(false);
                        }
                    });
                    dlg->addButton("取消", []() {});
                    dlg->open();
                });
            });

        const int operationsMenu = m_gameOptionsSidebar->addSubmenu(
            "游戏操作", beiklive::material::SETTINGS);

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, "修改映射名称", beiklive::material::EDIT,
            [this, path, title = entry.title, filename](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated(
                    [this, path, title, filename]() {
                        auto* ime = brls::Application::getPlatform()->getImeManager();
                        if (!ime)
                            return;
                        ime->openForText(
                            [this, path, filename](std::string text) {
                                if (text.empty() || !beiklive::GameDB)
                                    return;
                                beiklive::GameDB->set(
                                    path, "title", nlohmann::json(text));
                                beiklive::NameMappingManager->Set(
                                    filename, text, true);
                                beiklive::GameDB->flush();
                                beiklive::NameMappingManager->Save();
                                _requestRecentGamesRefresh(false);
                            },
                            "编辑游戏名称", "", 128, title,
                            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                    });
            });

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, "修改封面", beiklive::material::IMAGE,
            [this, path](const beiklive::GameEntry& game) {
                const auto pickerLocation = beiklive::getGameCoverPickerLocation(game);
                _closeGameOptionsPanelAnimated(
                    [this, path, pickerLocation]() {
                        beiklive::openFilePicker(
                            {"png", "jpg"},
                            [this, path](const std::string& selectedPath) {
                                if (!beiklive::GameDB)
                                    return;
                                beiklive::GameDB->set(
                                    path, "logoPath", nlohmann::json(selectedPath));
                                beiklive::GameDB->flush();
                                _requestRecentGamesRefresh(false);
                            },
                            pickerLocation.startPath,
                            pickerLocation.filename);
                    });
            });

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, "安装到 Switch 桌面",
            beiklive::material::INSTALL_APP,
            [this](const beiklive::GameEntry& game) {
                _closeGameOptionsPanelAnimated([game]() {
                    beiklive::forwarder::showInstallDialog(game);
                });
            });

        if (beiklive::GetCoreOptions(entry.platform).size() > 1) {
            m_gameOptionsSidebar->addSubmenuButton(
                operationsMenu, "核心切换", beiklive::material::MEMORY,
                [this, path, platform = entry.platform,
                 core = entry.core](const beiklive::GameEntry&) {
                    _closeGameOptionsPanelAnimated(
                        [this, path, platform, core]() {
                            const auto options =
                                beiklive::GetCoreOptions(platform);
                            std::vector<std::string> names;
                            names.reserve(options.size());
                            for (const auto& option : options)
                                names.push_back(option.name);
                            auto* dropdown = new brls::Dropdown(
                                "核心切换", names,
                                [this, path, options](int selected) {
                                    if (selected < 0 ||
                                        selected >= static_cast<int>(options.size()))
                                        return;
                                    if (beiklive::GameDB) {
                                        beiklive::GameDB->set(
                                            path, "core",
                                            nlohmann::json(options[selected].id));
                                        beiklive::GameDB->flush();
                                        brls::Application::notify(
                                            "已切换核心：" + options[selected].name);
                                        _requestRecentGamesRefresh(false);
                                    }
                                },
                                beiklive::GetCoreSelectionIndex(platform, core));
                            brls::Application::pushActivity(
                                new brls::Activity(dropdown));
                        });
                });
        }

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, "删除游戏", beiklive::material::DELETE_ICON,
            [this, path](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, path]() {
                    auto* removeDialog =
                        new brls::Dialog("是否从游戏库移除该游戏？");
                    removeDialog->addButton("是", [this, path]() {
                        auto* romDialog =
                            new brls::Dialog("是否删除 ROM 文件？");
                        auto deleteGame = [this, path](bool deleteRomFile) {
                            if (!beiklive::GameDB) {
                                brls::Application::notify("删除失败");
                                return;
                            }
                            m_homeDeletePending = true;
                            ++m_recentRefreshGen;
                            if (switchLayout)
                                switchLayout->removeGameByPath(path);

                            auto alive = m_aliveToken;
                            ThreadPool::instance().enqueue([
                                this, alive, path, deleteRomFile]() {
                                bool removedRecord = false;
                                if (alive->load() && beiklive::GameDB) {
                                    if (beiklive::GameDB->getAll().size() <= 1) {
                                        beiklive::GameDB->clearAll();
                                        removedRecord = true;
                                    } else {
                                        removedRecord =
                                            beiklive::GameDB->removeByPath(path);
                                        if (removedRecord)
                                            beiklive::GameDB->flush();
                                    }
                                }

                                bool removedFile = true;
                                if (removedRecord && deleteRomFile)
                                    removedFile = deleteGameFileIfExists(path);
                                if (!alive->load())
                                    return;

                                brls::sync([
                                    this, alive, removedRecord, removedFile,
                                    deleteRomFile]() {
                                    if (!alive->load())
                                        return;
                                    if (!removedRecord) {
                                        m_homeDeletePending = false;
                                        if (switchLayout)
                                            switchLayout->cancelGameRemoval();
                                        brls::Application::notify("删除失败");
                                        _requestRecentGamesRefresh(false);
                                        return;
                                    }

                                    auto finish = [this, alive, removedFile,
                                                   deleteRomFile]() {
                                        if (!alive->load())
                                            return;
                                        m_homeDeletePending = false;
                                        if (deleteRomFile)
                                            brls::Application::notify(removedFile
                                                ? "已删除游戏"
                                                : "已移除记录，ROM 文件删除失败");
                                        else
                                            brls::Application::notify(
                                                "已从游戏库移除该游戏");
                                        _requestRecentGamesRefresh(false);
                                    };
                                    if (switchLayout)
                                        switchLayout->completeGameRemoval(
                                            std::move(finish));
                                    else
                                        finish();
                                });
                            });
                        };
                        romDialog->addButton(
                            "是", [deleteGame]() { deleteGame(true); });
                        romDialog->addButton(
                            "否", [deleteGame]() { deleteGame(false); });
                        romDialog->addButton("不删了", []() {});
                        romDialog->open();
                    });
                    removeDialog->addButton("否", []() {});
                    removeDialog->addButton("不删了", []() {});
                    removeDialog->open();
                });
            });

        m_gameOptionsSidebar->onClosed = [this]() {
            if (switchLayout)
                switchLayout->releaseSelectedCoverTexture();
            if (switchLayout)
                switchLayout->restoreCardFocus(false);
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        };
        m_gameOptionsSidebar->onCloseRequested = [this]() {
            _closeGameOptionsPanelAnimated({});
        };

        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(entry);
    }

    void StartPage::_hideGameOptionsPanel()
    {
        if (!m_gameOptionsSidebar)
            return;
        auto* stale = m_gameOptionsSidebar;
        m_gameOptionsSidebar = nullptr;
        if (switchLayout)
            switchLayout->releaseSelectedCoverTexture();
        stale->removeFromSuperView(true);
        this->getBottomBar()->setVisibility(brls::Visibility::GONE);
    }

    void StartPage::_closeGameOptionsPanelAnimated(
        std::function<void()> completion, bool launchTransition)
    {
        if (!m_gameOptionsSidebar) {
            if (completion)
                completion();
            return;
        }
        auto* sidebar = m_gameOptionsSidebar;
        auto alive = m_aliveToken;
        auto finishClose = [this, alive, sidebar,
                            completion = std::move(completion)]() mutable {
            brls::sync([this, alive, sidebar,
                        completion = std::move(completion)]() mutable {
                if (!alive->load())
                    return;
                if (m_gameOptionsSidebar == sidebar)
                    m_gameOptionsSidebar = nullptr;
                sidebar->removeFromSuperView(true);
                this->getBottomBar()->setVisibility(brls::Visibility::GONE);
                if (completion)
                    completion();
            });
        };
        if (launchTransition)
            sidebar->closeForLaunch(std::move(finishClose));
        else
            sidebar->close(std::move(finishClose));
    }
}
