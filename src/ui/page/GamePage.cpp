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
            _setupGame();
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

            _setupGame();
        }
    }

    GamePage::~GamePage()
    {
        brls::Logger::debug("GamePage destructor called for game: " + m_gameEntry.title);
    }

    void GamePage::updateGameCount()
    {
        auto &db = beiklive::GameDB; // 获取全局游戏数据库实例
        // 更新运行时间戳
        m_gameEntry.lastPlayed = beiklive::tools::getTimestampString();
        m_gameEntry.playCount += 1; // 玩过的次数加1
        brls::Logger::debug("GamePage 更新游戏条目：lastPlayed={}, playCount={}", m_gameEntry.lastPlayed, m_gameEntry.playCount);
        // 提交一次数据库更改，确保在游戏过程中数据被保存，即使中途崩溃也不会丢失
        db->upsert(m_gameEntry);
        db->flush();
    }

    // 使用 #166 新增的通用字段访问接口（setDefault/set/get）处理数据库数据
    void GamePage::GameEntryInitialize()
    {
        auto &db = beiklive::GameDB;
        auto dcrc32 = tools::crc32(m_gameData.fullPath);
        std::string baseName    = beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName);
        std::string mappedTitle = GET_MAPPING_KEY_STR(baseName, baseName);

        brls::Logger::debug("GamePage 初始化游戏条目，路径: {}, CRC32: {}", m_gameData.fullPath, dcrc32);

        bool isNew = !db->findByCrc32(dcrc32).has_value();
        if (isNew)
        {
            // 新游戏：创建基础条目并插入数据库，后续 setDefault 补充其余字段
            brls::Logger::debug("GamePage 数据库中没有此游戏记录，创建新条目: {}", m_gameData.fullPath);
            GameEntry newEntry;
            newEntry.path     = m_gameData.fullPath;
            newEntry.crc32    = dcrc32;
            newEntry.platform = (int)m_gameData.itemType;
            newEntry.title    = mappedTitle;
            newEntry.logoPath = beiklive::tools::getDefaultLogoPath(
                (beiklive::enums::EmuPlatform)m_gameData.itemType);
            db->upsert(newEntry);
        }
        else
        {
            brls::Logger::debug("GamePage 数据库中已有此游戏记录: {}", m_gameData.fullPath);
        }

        // 使用 setDefault 补全缺失字段（不覆盖已有值）
        // 适用于旧版数据迁移或字段新增后的向后兼容
        db->setDefault(dcrc32, "platform",  (int)m_gameData.itemType);
        db->setDefault(dcrc32, "crc32",     dcrc32);
        db->setDefault(dcrc32, "playCount", 0);
        db->setDefault(dcrc32, "playTime",  0);
        db->setDefault(dcrc32, "logoPath",
            beiklive::tools::getDefaultLogoPath((beiklive::enums::EmuPlatform)m_gameData.itemType));

        // 读取最新完整条目
        if (auto found = db->findByCrc32(dcrc32); found.has_value())
        {
            m_gameEntry = found.value();
            // 若标题为空（旧数据缺失映射名），使用 set 写入映射名称
            if (m_gameEntry.title.empty())
            {
                m_gameEntry.title = mappedTitle;
                db->set(dcrc32, "title", mappedTitle);
            }
        }
        else
        {
            // 极端情况：数据库操作失败，手动构建基础条目
            m_gameEntry.path     = m_gameData.fullPath;
            m_gameEntry.crc32    = dcrc32;
            m_gameEntry.platform = (int)m_gameData.itemType;
            m_gameEntry.title    = mappedTitle;
        }

        // 初始化路径字段（从配置读取默认值填充空字段）
        _initGameEntryPaths();
        updateGameCount();
        brls::Logger::debug("GamePage 游戏条目初始化完成: title={}, platform={}", m_gameEntry.title, m_gameEntry.platform);
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
            std::string sramDir = GET_SETTING_KEY_STR("save.sramDir", "");
            m_gameEntry.savePath = sramDir.empty() ? beiklive::path::savePath() : sramDir;
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
            std::string overlayKey;
            switch (static_cast<beiklive::enums::EmuPlatform>(m_gameEntry.platform))
            {
                case beiklive::enums::EmuPlatform::EmuGBA: overlayKey = sk::KEY_DISPLAY_OVERLAY_GBA_PATH; break;
                case beiklive::enums::EmuPlatform::EmuGBC: overlayKey = sk::KEY_DISPLAY_OVERLAY_GBC_PATH; break;
                case beiklive::enums::EmuPlatform::EmuGB:  overlayKey = sk::KEY_DISPLAY_OVERLAY_GB_PATH;  break;
                default: break;
            }
            if (!overlayKey.empty())
                m_gameEntry.overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");
        }

        // logoPath：优先使用已有值（包括自定义封面），否则使用平台默认图标
        if (m_gameEntry.logoPath.empty())
        {
            m_gameEntry.logoPath = beiklive::tools::getDefaultLogoPath(
                static_cast<beiklive::enums::EmuPlatform>(m_gameEntry.platform));
        }

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
        this->addView(m_gameView);
    }

    void GamePage::GameMenuInitialize()
    {
        #undef ABSOLUTE
        m_gameMenuView = new GameMenuView(m_gameEntry);
        m_gameMenuView->setWidthPercentage(100.f);
        m_gameMenuView->setHeightPercentage(100.f);
        m_gameMenuView->setFocusable(false);
        m_gameMenuView->setPositionType(brls::PositionType::ABSOLUTE);
        m_gameMenuView->setPositionTop(0);
        m_gameMenuView->setPositionLeft(0);
        m_gameMenuView->setVisibility(brls::Visibility::GONE); // 初始隐藏


        // setOnResume和setOnExit回调由GamePage注入，触发时分别执行对应的动画和操作
        m_gameMenuView->setOnResume([this]() {
            brls::sync([this]() {
                m_gameView->setFocusable(true);
                AnimationHelper::slideOutToBottom(m_gameMenuView, MENU_FADE_OUT_MS, 460.f,true, [this]() {
                    brls::Application::giveFocus(m_gameView);
                });
            });
        });

        // "退出游戏"回调：触发退出信号
        m_gameMenuView->setOnExit([this]() {
            brls::sync([this]() {
                AnimationHelper::slideOutToBottom(m_gameMenuView, MENU_EXIT_FADE_MS, 460.f,true, [this]() {
                    GameSignal::instance().requestExit();
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

        // 注入槽位信息查询回调：供菜单面板异步扫描存档目录
        m_gameMenuView->setStateInfoCallback([this](int slot) -> beiklive::StateSlotInfo {
            beiklive::StateSlotInfo info;
            if (!m_gameView) return info;
            std::string statePath = m_gameView->getStatePath(slot);
            std::string thumbPath = m_gameView->getStateThumbPath(slot);
            std::error_code ec;
            info.exists = std::filesystem::exists(statePath, ec);
            if (info.exists) {
                if (std::filesystem::exists(thumbPath, ec))
                    info.thumbPath = thumbPath;
                // 使用公共工具函数读取文件修改时间字符串
                info.timeStr = beiklive::tools::getFileModTimeStr(statePath);
            }
            return info;
        });

        this->addView(m_gameMenuView);
    }

    void GamePage::RewindSelectorInitialize()
    {
        #undef ABSOLUTE
        // 仅当设置中启用了可视化倒带界面时才创建
        bool showUI = GET_SETTING_KEY_INT("rewind.showUI", 0) != 0;
        if (!showUI || !m_gameView) return;

        m_rewindSelectorView = new RewindSelectorView(m_gameView);
        // 注册"选择某帧恢复"回调：通知游戏线程恢复到对应帧
        m_rewindSelectorView->setOnSelectFrame([this](int frameIdx) {
            // frameIdx 是采样结果中的索引，实际恢复由 GameView 的倒带机制处理
            // 这里仅关闭倒带信号，GameView 的 _stepRewind 会处理实际恢复
            brls::Logger::debug("RewindSelector: 用户选择帧索引 {}", frameIdx);
        });

        m_rewindSelectorView->setPositionType(brls::PositionType::ABSOLUTE);
        m_rewindSelectorView->setPositionLeft(0);
        m_rewindSelectorView->setPositionBottom(0);
        m_rewindSelectorView->setWidthPercentage(100.f);
        m_rewindSelectorView->setHeight(220.f);
        m_rewindSelectorView->setVisibility(brls::Visibility::GONE); // 初始隐藏
        this->addView(m_rewindSelectorView);
    }

    void GamePage::_setupGame()
    {
        PageInit();
        GameViewInitialize();
        GameMenuInitialize();
        RewindSelectorInitialize();

        // 将菜单视图引用注入 GameView，以便菜单热键触发时可打开菜单
        if (m_gameView && m_gameMenuView)
            m_gameView->setGameMenuView(m_gameMenuView);

        // 将可视化倒带视图引用注入 GameView，以便倒带热键触发时可显示界面
        if (m_gameView && m_rewindSelectorView)
            m_gameView->setRewindSelectorView(m_rewindSelectorView);

        brls::sync([this]()
                   { brls::Application::giveFocus(m_gameView); }); // 游戏视图获得焦点，准备接受输入
    }
}