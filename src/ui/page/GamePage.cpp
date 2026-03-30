#include "GamePage.hpp"
#include "core/Tools.hpp"

namespace beiklive
{
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

        this->registerAction(
            "退出游戏",
            brls::BUTTON_START,
            [this](brls::View *)
            {
                // 此处设置按键功能
                brls::sync([this]()
                           { brls::Application::popActivity(); });
                return true;
            },
            /*hidden=*/false, /*repeat=*/false, brls::SOUND_BACK);
    }

    void GamePage::GameViewInitialize()
    {
        #undef ABSOLUTE
        m_gameView = new GameView(m_gameEntry);
        m_gameView->setWidthPercentage(100.f);
        m_gameView->setHeightPercentage(100.f);
        m_gameView->setBackgroundColor(NVGcolor{20, 133, 238, 255}); // 设置游戏视图背景为黑色
        m_gameView->setFocusable(true);
        m_gameView->setPositionType(brls::PositionType::ABSOLUTE);
        m_gameView->setPositionTop(0);
        m_gameView->setPositionLeft(0);
        this->addView(m_gameView);
    }

    void GamePage::GameMenuInitialize()
    {
        // 游戏菜单初始化逻辑，如设置菜单选项、绑定按键等
        // #undef ABSOLUTE
        // m_gameMenuView = new GameMenuView();
        // m_gameMenuView->setWidthPercentage(100.f);
        // m_gameMenuView->setHeightPercentage(100.f);
        // m_gameMenuView->setFocusable(false);
        // m_gameMenuView->setPositionType(brls::PositionType::ABSOLUTE);
        // m_gameMenuView->setPositionTop(0);
        // m_gameMenuView->setPositionLeft(0);
        // this->addView(m_gameMenuView);
        // m_gameView->setVisibility(brls::Visibility::GONE); // 初始隐藏，加载完成后显示
    }

    void GamePage::_setupGame()
    {
        PageInit();
        GameViewInitialize();
        GameMenuInitialize();

        brls::Application::giveFocus(m_gameView); // 游戏视图获得焦点，准备接受输入
    }
}