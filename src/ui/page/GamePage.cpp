#include "GamePage.hpp"
#include "core/Tools.hpp"
#include "core/GameSignal.hpp"
#include "ui/utils/AnimationHelper.hpp"

#include <filesystem>

namespace beiklive
{
    // 菜单动画时长常量（毫秒）
    static constexpr int MENU_SLIDE_IN_MS  = 220; ///< 菜单入场滑动动画时长
    static constexpr int MENU_FADE_OUT_MS  = 180; ///< 菜单关闭淡出动画时长
    static constexpr int MENU_EXIT_FADE_MS = 150; ///< 退出游戏淡出动画时长
    GamePage::GamePage(beiklive::DirListData gameData)
    {

        m_gameData = std::move(gameData);
        // 检查文件是否存在
        if (!beiklive::tools::isFileExists(m_gameData.fullPath))
        {
            brls::Application::notify("文件不存在: " + m_gameData.fileName);
            // 这里可以选择返回上一级或显示错误界面
            brls::sync([this]()
                       { brls::Application::popActivity(); });
        }
        else
        {
            // 此处将 DirListData 处理为 GameEntry 以供游戏使用
            GameEntryInitialize();
        }
    }

    GamePage::GamePage(beiklive::GameEntry gameEntry)
    {
        m_gameEntry = std::move(gameEntry);
        // 检查文件是否存在
        if (!beiklive::tools::isFileExists(m_gameEntry.path))
        {
            brls::Application::notify("文件不存在: " + m_gameEntry.title);
            // 这里可以选择返回上一级或显示错误界面
            brls::sync([this]()
                       { brls::Application::popActivity(); });
        }
        else
        {
            // GameEntry 已经包含了游戏的完整信息，可以直接使用，无需再处理一次
            // 仍需检查并补全可能为空的路径字段
            _initGameEntryPaths();
            updateGameCount();
        }
    }

    GamePage::~GamePage()
    {
        brls::Logger::debug("GamePage destructor called for game: " + m_gameEntry.title);
    }

    void GamePage::updateGameCount()
    {
        auto &db = beiklive::GameDB; // 获取全局游戏数据库实例
        // 使用通用字段接口更新运行时间戳和启动次数
        m_gameEntry.lastPlayed = beiklive::tools::getTimestampString();
        m_gameEntry.playCount += 1;
        brls::Logger::debug("GamePage 更新游戏条目：lastPlayed={}, playCount={}", m_gameEntry.lastPlayed, m_gameEntry.playCount);
        db->set(m_gameEntry.path, "lastPlayed", m_gameEntry.lastPlayed);
        db->set(m_gameEntry.path, "playCount", m_gameEntry.playCount);
        db->flush();
    }

    void GamePage::GameEntryInitialize()
    {
        auto &db = beiklive::GameDB;                     // 获取全局游戏数据库实例
        brls::Logger::debug("GamePage 开始处理游戏条目，路径: {}", m_gameData.fullPath);
        
        // 若数据库中不存在此游戏记录，先插入含必要字段的最小条目
        if (!db->findByPath(m_gameData.fullPath).has_value())
        {
            auto dcrc32 = tools::crc32(m_gameData.fullPath); // 计算 CRC32 校验值
            brls::Logger::debug("GamePage 数据库中没有此游戏的记录，插入新记录: {}", m_gameData.fullPath);
            GameEntry minimal;
            minimal.path     = m_gameData.fullPath;
            minimal.crc32    = dcrc32;
            minimal.platform = (int)m_gameData.itemType;
            minimal.title    = GET_MAPPING_KEY_STR(
                beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName),
                beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName));
            db->upsertByPath(minimal);
        }
        else
        {
            brls::Logger::debug("GamePage 数据库中已存在此游戏记录: {}", m_gameData.fullPath);
        }

        // 使用 setDefault 为可选字段设置首次默认值（已有值时不覆盖）
        std::string defaultLogo = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>((int)m_gameData.itemType));

        auto& path = m_gameData.fullPath;
        db->setDefault(path, "logoPath", defaultLogo);

        namespace sk = beiklive::SettingKey;
        db->setDefault(path, "overlayEnabled",
                       GET_SETTING_KEY_INT(sk::KEY_DISPLAY_OVERLAY_ENABLED, 0));
        db->setDefault(path, "shaderEnabled",
                       GET_SETTING_KEY_INT(sk::KEY_DISPLAY_SHADER_ENABLED, 0));

        // 画面模式：全局配置为字符串，DB 存整数 ScreenMode 枚举值
        {
            std::string dmStr = GET_SETTING_KEY_STR("display.mode", "original");
            int dm = 0; // Fit
            if (dmStr == "fill") dm = 1;          // Fill
            else if (dmStr == "integer") dm = 2;   // IntegerScale
            else if (dmStr == "custom") dm = 3;    // FreeScale
            db->setDefault(path, "displayMode", dm);
        }
        // 整数倍缩放
        db->setDefault(path, "integerAspectRatio",
                       GET_SETTING_KEY_INT("display.integer_scale_mult", 0));

        m_gameEntry = db->findByPath(path).value();

        // 初始化路径字段（优先使用已有记录，若为空则从配置中读取默认值）
        _initGameEntryPaths();

        updateGameCount();
        brls::Logger::debug("GamePage 游戏条目已处理完成: {}", m_gameData.fullPath);
    }

    void GamePage::_initGameEntryPaths()
    {
        namespace sk = beiklive::SettingKey;
        std::filesystem::path gamePath(m_gameEntry.path);
        std::string baseName = gamePath.stem().string(); // 游戏文件名（不含扩展名）
        std::string gameDir  = gamePath.parent_path().string();

        // savePath：优先使用已有值，否则从设置读取 save.sramDir，为空时使用全局 saves 目录
        if (m_gameEntry.savePath.empty())
        {
            // sramDir为空表示rom目录， 不为空为模拟器目录
            std::string sramDir = GET_SETTING_KEY_STR("save.sramDir", "");
            if(!sramDir.empty())
            {
                // 如果没有单独的 sramDir 设置，则使用全局 saves 目录
                sramDir = beiklive::path::savePath() + beiklive::path::SPLIT_CHAR + baseName;
            }else
            {
                sramDir = gameDir;
            }
            std::filesystem::create_directories(sramDir);
            m_gameEntry.savePath = sramDir;
        }

        // cheatPath：优先使用已有值，否则构建为 <cheat目录>/<游戏名>.cht
        if (m_gameEntry.cheatPath.empty())
        {
            std::string cheatDir = GET_SETTING_KEY_STR("cheat.dir", "");
            if (cheatDir.empty())
                cheatDir = beiklive::path::cheatPath();
            // 使用 std::filesystem::path 拼接，自动处理平台路径分隔符
            m_gameEntry.cheatPath = (std::filesystem::path(cheatDir) / (baseName + ".cht")).string();
        }

        // overlayPath：优先使用已有值，否则从设置读取平台对应的遮罩路径
        if (m_gameEntry.overlayPath.empty())
        {
            std::string overlayKey = beiklive::tools::platformOverlayKey(m_gameEntry.platform);
            if (!overlayKey.empty())
                m_gameEntry.overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");
        }

        // shaderPath：优先使用已有值，否则从设置读取平台对应的着色器路径（平台路径为空时回退到全局路径）
        if (m_gameEntry.shaderPath.empty())
        {
            std::string shaderKey = beiklive::tools::platformShaderKey(m_gameEntry.platform);
            if (!shaderKey.empty())
                m_gameEntry.shaderPath = GET_SETTING_KEY_STR(shaderKey.c_str(), "");
            if (m_gameEntry.shaderPath.empty())
                m_gameEntry.shaderPath = GET_SETTING_KEY_STR(sk::KEY_DISPLAY_SHADER_PATH, "");
        }

        // // overlayEnabled：优先使用已有值，新游戏使用全局设置初始化
        // if (!m_gameEntry.overlayEnabled)
        //     m_gameEntry.overlayEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_OVERLAY_ENABLED, 0) != 0;

        // // shaderEnabled：优先使用已有值，新游戏使用全局设置初始化
        // if (!m_gameEntry.shaderEnabled)
        //     m_gameEntry.shaderEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_SHADER_ENABLED, 0) != 0;

        // logoPath：优先使用已有值（包括自定义封面），否则使用平台默认图标。若为默认图标则尝试替换为存档截图
        if (m_gameEntry.logoPath.empty())
        {
            m_gameEntry.logoPath = beiklive::tools::getDefaultLogoPath(
                static_cast<beiklive::enums::EmuPlatform>(m_gameEntry.platform));
        }
        // 检查一次 封面是否需要替换
        _tryUpdateLogoFromThumbnail();

        // screenShotPath：优先使用已有值，否则使用全局截图目录
        if (m_gameEntry.screenShotPath.empty())
        {
            m_gameEntry.screenShotPath = beiklive::path::screenshotPath();
        }

        brls::Logger::debug("GamePage 路径初始化完成: savePath={}, cheatPath={}, overlayPath={}, logoPath={}, screenShotPath={}",
            m_gameEntry.savePath, m_gameEntry.cheatPath, m_gameEntry.overlayPath,
            m_gameEntry.logoPath, m_gameEntry.screenShotPath);
    }

    void GamePage::PageInit()
    {
        this->showFooter(false);
        this->showHeader(false);
        this->showBackground(false);
        this->showShader(false);

        this->setAxis(brls::Axis::COLUMN);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setFocusable(false);
        this->setHideHighlightBackground(true);
        this->setHideHighlightBorder(true);
        this->setHideClickAnimation(true);
        this->setBackground(brls::ViewBackground::NONE);
        this->setWidthPercentage(100.f);
        this->setHeightPercentage(100.f);

        this->getContentBox()->setMarginRight(0.f);
        this->getContentBox()->setMarginLeft(0.f);
    }

    void GamePage::GameViewInitialize()
    {
        #undef ABSOLUTE
        m_gameView = new GameView(m_gameEntry);
        m_gameView->setWidthPercentage(100.f);
        m_gameView->setHeightPercentage(100.f);
        // m_gameView->setBackgroundColor(nvgRGBA(114, 187, 255, 255)); // 设置游戏视图背景为黑色
        m_gameView->setBackground(brls::ViewBackground::NONE);
        m_gameView->setFocusable(true);
        m_gameView->setPositionType(brls::PositionType::ABSOLUTE);
        m_gameView->setPositionTop(0);
        m_gameView->setPositionLeft(0);
        this->getContentBox()->addView(m_gameView);
    }

    void GamePage::GameMenuInitialize()
    {
        #undef ABSOLUTE
        m_gameMenuView = new GameMenuView(m_gameEntry);
        m_gameMenuView->setWidthPercentage(100.f);
        m_gameMenuView->setHeightPercentage(100.f);
        m_gameMenuView->setFocusable(true);
        m_gameMenuView->setPositionType(brls::PositionType::ABSOLUTE);
        m_gameMenuView->setPositionTop(0);
        m_gameMenuView->setPositionLeft(0);
        this->getContentBox()->addView(m_gameMenuView);
        m_gameMenuView->setVisibility(brls::Visibility::GONE); // 初始隐藏


        // setOnResume和setOnExit回调由GamePage注入，触发时分别执行对应的动画和操作
        m_gameMenuView->setOnResume([this]() {
            brls::sync([this]() {
                GameSignal::instance().requestReloadCheats();
                beiklive::GameDB->flush();
                m_gameView->setFocusable(true);
                AnimationHelper::slideOutToBottom(m_gameMenuView, MENU_FADE_OUT_MS, 120.f,true, [this]() {
                    brls::Application::giveFocus(m_gameView);
                });
            });
        });

        // "重置游戏"回调：触发重置信号
        m_gameMenuView->setOnReset([this]() {
            brls::sync([this]() {
                m_gameView->setFocusable(true);
                AnimationHelper::slideOutToBottom(m_gameMenuView, MENU_FADE_OUT_MS, 120.f,true, [this]() {
                    GameSignal::instance().requestReset();
                    brls::Application::giveFocus(m_gameView);

                });
            });
        });


        // "退出游戏"回调：触发退出信号
        m_gameMenuView->setOnExit([this]() {
            brls::sync([this]() {
                _tryUpdateLogoFromThumbnail();
                beiklive::GameDB->flush();
                AnimationHelper::slideOutToBottom(m_gameMenuView, MENU_EXIT_FADE_MS, 120.f,true, [this]() {
                    int exitSlot = GET_SETTING_KEY_INT("save.autoSaveOnExit", 0);
                    if (exitSlot > 0 && exitSlot <= 10)
                        GameSignal::instance().requestAutoSave(exitSlot - 1);
                    brls::sync([this]() {
                        beiklive::popActivity(this);
                    });
                });
            });
        });

        // 注入保存状态回调：通过 GameSignal 在游戏线程中执行实际存档
        m_gameMenuView->setSaveStateCallback([this](int slot) {
            GameSignal::instance().requestQuickSave(slot);
        });

        // 注入读取状态回调：通过 GameSignal 在游戏线程中执行实际读档
        m_gameMenuView->setLoadStateCallback([this](int slot) {
            GameSignal::instance().requestQuickLoad(slot);
        });

        // 注入金手指切换回调：通过 GameSignal 在游戏线程中执行实际切换
        m_gameMenuView->setCheatToggleCallback([this](int idx, bool enabled) {
            GameSignal::instance().requestCheatToggle(idx, enabled);
        });

        // 注入金手指文件变更回调：更新 GameEntry 的 cheatPath 并持久化
        m_gameMenuView->setCheatPathCallback([this](const std::string& path) {
            m_gameEntry.cheatPath = path;
            if (beiklive::GameDB)
                beiklive::GameDB->set(m_gameEntry.path, "cheatPath", nlohmann::json(path));
            if (m_gameView)
                m_gameView->requestCheatPathUpdate(path);
        });

        // 注入画面设置回调
        m_gameMenuView->setDisplayModeCallback([this](const std::string& mode) {
            if (m_gameView) m_gameView->_onDisplayModeChange(mode);
        });
        m_gameMenuView->setIntegerScaleCallback([this](float scale) {
            if (m_gameView) m_gameView->_onIntegerScaleChange(scale);
        });
        m_gameMenuView->setCustomScaleCallback([this](float x, float y, float scale) {
            if (m_gameView) m_gameView->_onCustomValuesChanged(x, y, scale);
        });
        m_gameMenuView->setOverlayToggleCallback([this](bool enabled) {
            if (m_gameView) m_gameView->_onOverlayToggle(enabled);
        });
        m_gameMenuView->setOverlayPathCallback([this](const std::string& path) {
            if (m_gameView) m_gameView->_onOverlayPathChange(path);
        });
        m_gameMenuView->setFilterCallback([this](const std::string& filter) {
            if (m_gameView) m_gameView->_onFilterChange(filter);
        });
        m_gameMenuView->setShaderToggleCallback([this](bool on) {
            if (m_gameView) m_gameView->_onShaderToggle(on);
        });
        m_gameMenuView->setShaderPathCallback([this](const std::string& path) {
            if (m_gameView) m_gameView->_onShaderPathChange(path);
        });
        // 注入着色器参数回调
        m_gameMenuView->setShaderParamsCallback([this]() -> std::vector<ShaderParamInfo> {
            if (m_gameView) return m_gameView->_getShaderParams();
            return {};
        });
        m_gameMenuView->setShaderParamCallback([this](const std::string& name, float val) {
            if (m_gameView) m_gameView->_setShaderParam(name, val);
        });

        // 注入 GB 配色回调：写入配置后，核心在 retro_run() 中自动重读并应用
        m_gameMenuView->setGbColorCallback([this](const std::string& color) {
            if (m_gameView) m_gameView->_onConfigUpdated();
        });

        // 注入槽位信息查询回调：供菜单面板异步扫描存档目录
        // 预先在UI线程计算所有槽位路径（仅字符串操作），避免后台线程持有 GameView 原始指针，
        // 防止游戏退出后 GameView 被销毁时后台线程仍访问其成员导致崩溃。
        // 槽位数量 10 与 GameMenuView 内部的 _createSaveStatePanel/_createLoadStatePanel 保持一致
        {
            std::vector<std::string> statePaths, thumbPaths;
            statePaths.reserve(10);
            thumbPaths.reserve(10);
            for (int slot = 0; slot < 10; ++slot) {
                statePaths.push_back(m_gameView->getStatePath(slot));
                thumbPaths.push_back(m_gameView->getStateThumbPath(slot));
            }
            m_gameMenuView->setStateInfoCallback(
                [statePaths = std::move(statePaths), thumbPaths = std::move(thumbPaths)](int slot) -> beiklive::StateSlotInfo {
                    beiklive::StateSlotInfo info;
                    if (slot < 0 || slot >= static_cast<int>(statePaths.size())) return info;
                    const std::string& statePath = statePaths[slot];
                    const std::string& thumbPath = thumbPaths[slot];
                    if (statePath.empty()) return info;
                    std::error_code ec;
                    info.exists = std::filesystem::exists(statePath, ec);
                    if (info.exists) {
                        if (std::filesystem::exists(thumbPath, ec))
                            info.thumbPath = thumbPath;
                        info.timeStr = beiklive::tools::getFileModTimeStr(statePath);
                    }
                    return info;
                });

            // 注入删除存档回调：删除指定槽位的存档文件和缩略图，然后刷新网格显示
            m_gameMenuView->setDeleteStateCallback([this](int slot) {
                if (!m_gameView) return;
                std::string statePath = m_gameView->getStatePath(slot);
                std::string thumbPath = m_gameView->getStateThumbPath(slot);
                std::error_code ec;
                std::filesystem::remove(statePath, ec);
                std::filesystem::remove(thumbPath, ec);
                m_gameMenuView->refreshSlotState(slot);
            });
        }

    }

    void GamePage::RewindSelectorViewInitialize()
    {
        #undef ABSOLUTE
        m_rewindSelectorView = new RewindSelectorView();
        m_rewindSelectorView->setWidthPercentage(100.f);
        m_rewindSelectorView->setHeightPercentage(100.f);
        // 不让容器自身接受焦点，由 RewindSelectorView::getDefaultFocus() 将焦点导向具体卡片
        m_rewindSelectorView->setFocusable(false);
        m_rewindSelectorView->setPositionType(brls::PositionType::ABSOLUTE);
        m_rewindSelectorView->setPositionTop(0);
        m_rewindSelectorView->setPositionLeft(0);
        m_rewindSelectorView->setVisibility(brls::Visibility::GONE); // 初始隐藏

        // 选择帧后恢复状态：通过 GameView 向游戏线程发送恢复请求，然后关闭界面
        m_rewindSelectorView->setOnFrameSelected([this](int frameIndex) {
            if (m_gameView)
                m_gameView->requestRestoreRewindFrame(frameIndex);
            // 关闭倒带界面并恢复游戏
            brls::sync([this]() {
                AnimationHelper::slideOutToBottom(m_rewindSelectorView, 80.f, 180, true, [this]() {
                    m_gameView->setFocusable(true);
                    GameSignal::instance().requestPause(false);
                    brls::Application::giveFocus(m_gameView);
                });
            });
        });

        // B 键取消：直接关闭倒带界面并恢复游戏
        m_rewindSelectorView->setOnClose([this]() {
            brls::sync([this]() {
                AnimationHelper::slideOutToBottom(m_rewindSelectorView, 80.f, 180, true, [this]() {
                    m_gameView->setFocusable(true);
                    GameSignal::instance().requestPause(false);
                    GameSignal::instance().requestRewind(false);
                    brls::Application::giveFocus(m_gameView);
                });
            });
        });

        this->getContentBox()->addView(m_rewindSelectorView);
    }

    void GamePage::_setupGame()
    {
        PageInit();
        GameViewInitialize();
        GameMenuInitialize();
        RewindSelectorViewInitialize();

        // 将菜单视图引用注入 GameView，以便菜单热键触发时可打开菜单
        if (m_gameView && m_gameMenuView)
            m_gameView->setGameMenuView(m_gameMenuView);

        // 将倒带选择视图引用注入 GameView，以便倒带键触发时可打开可视化倒带界面
        if (m_gameView && m_rewindSelectorView)
            m_gameView->setRewindSelectorView(m_rewindSelectorView);

        brls::sync([this]()
                   { brls::Application::giveFocus(m_gameView); }); // 游戏视图获得焦点，准备接受输入
    }

    void GamePage::startGame()
    {
        if (!m_gameView)
            _setupGame();
    }

    void GamePage::_tryUpdateLogoFromThumbnail()
    {
        brls::Logger::debug("当前logopath -> {}", m_gameEntry.logoPath);

        // 打开了使用截图作为默认封面时才执行下面的逻辑
        if (GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_USE_SAVESTATE_THUMB, 0) == 0)
            return;

        // 获取当前平台默认图标路径
        std::string defaultLogo = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(m_gameEntry.platform));

        // 如果当前封面不是默认图标，已自定义则不用替换
        if (m_gameEntry.logoPath != defaultLogo)
            return;

        // 构建即时存档 0 截图路径
        std::string gamename = beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path);
        std::string thumbPath = m_gameEntry.savePath +  beiklive::path::SPLIT_CHAR +
                                gamename + ".ss0.png";

        // 检测文件是否存在
        if (std::filesystem::exists(thumbPath))
        {
            m_gameEntry.logoPath = thumbPath;
            if (beiklive::GameDB)
            {
                beiklive::GameDB->set(m_gameEntry.path, "logoPath", nlohmann::json(thumbPath));
            }
        }

        // 检查一下是否成功变更
        // brls::Logger::debug("变更GameDB logopath -> {}", beiklive::GameDB->get(m_gameEntry.crc32, "logoPath").value());

    }
}
