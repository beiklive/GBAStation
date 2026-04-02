#include "GamePage.hpp"
#include "core/Tools.hpp"
#include "core/GameSignal.hpp"
#include "ui/utils/AnimationHelper.hpp"

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

    // TODO: 此处还要获取游戏的映射名称、 处理游戏独立设置以及路径等信息，后续完善
    void GamePage::GameEntryInitialize()
    {
        auto &db = beiklive::GameDB;                     // 获取全局游戏数据库实例
        auto dcrc32 = tools::crc32(m_gameData.fullPath); // 计算 CRC32 校验值
        // 记录日志：开始处理游戏条目
        brls::Logger::debug("GamePage 开始处理游戏条目，路径: {}, CRC32: {}", m_gameData.fullPath, dcrc32);
        auto foundByCrc = db->findByCrc32(dcrc32);
        // 如果数据库中没有此游戏的记录 ,插入一条新记录，等到游戏结束时再插入一次
        if (!foundByCrc.has_value())
        {
            brls::Logger::debug("GamePage 数据库中没有此游戏的记录，插入新记录: {}", m_gameData.fullPath);
            // 数据库中没有此游戏的记录，创建一个新的 GameEntry 并插入数据库
            m_gameEntry.path = m_gameData.fullPath;
            m_gameEntry.title = GET_MAPPING_KEY_STR(beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName), beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName));
            m_gameEntry.platform = (int)m_gameData.itemType;                                                                // 设置平台类型
            m_gameEntry.crc32 = dcrc32;                                                                                     // 设置 CRC32 校验值
            m_gameEntry.logoPath = beiklive::tools::getDefaultLogoPath((beiklive::enums::EmuPlatform)m_gameEntry.platform); // 设置默认封面路径
        }
        else
        {
            brls::Logger::debug("GamePage 数据库中已存在此游戏记录: {}", m_gameData.fullPath);
            // 数据库中已存在此游戏记录，使用数据库中的数据初始化 GameEntry
            m_gameEntry = foundByCrc.value();
        }

        updateGameCount();
        brls::Logger::debug("GamePage 游戏条目已保存到数据库: {}", m_gameData.fullPath);
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

        this->addView(m_gameMenuView);
    }

    void GamePage::_setupGame()
    {
        PageInit();
        GameViewInitialize();
        GameMenuInitialize();

        // 将菜单视图引用注入 GameView，以便菜单热键触发时可打开菜单
        if (m_gameView && m_gameMenuView)
            m_gameView->setGameMenuView(m_gameMenuView);

        brls::sync([this]()
                   { brls::Application::giveFocus(m_gameView); }); // 游戏视图获得焦点，准备接受输入
    }
}