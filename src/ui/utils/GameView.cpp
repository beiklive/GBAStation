#include "GameView.hpp"

namespace beiklive
{
    GameView::GameView(beiklive::GameEntry gameData) : m_gameEntry(std::move(gameData))
    {
        _brls_inputLocked = false;               // 初始化输入锁定状态
        GameInputManager::instance().sayHello(); // 测试输入管理器是否正常工作
        HIDE_BRLS_HIGHLIGHT(this);               // 隐藏Borealis的默认高亮效果，避免与游戏视图的交互冲突

        _registerGameInput(); // 注册游戏相关的输入
        _registerGameRuntime();
    }

    GameView::~GameView()
    {
        GameInputManager::instance().clearEmuFunctionKeys(); // 清除所有注册的热键，确保在游戏视图销毁时不会留下无效的回调
        GameInputManager::instance().dropInput(); // 清除当前输入状态，防止在游戏视图销毁后继续处理输入
    }

    void GameView::onFocusGained()
    {
        Box::onFocusGained();
        brls::Logger::debug("GameView gained focus");

        GameInputManager::instance().setInputEnabled(true); // 当游戏视图获得焦点时启用输入处理
        // 当游戏视图获得焦点时的处理逻辑，如暂停其他活动、调整输入等

        if (!_brls_inputLocked)
        {
            _brls_inputLocked = true;
            brls::Application::blockInputs(true); // 锁定输入，防止其他视图处理输入
        }

        // 锁定鼠标指针
        // brls::Application::getPlatform()->getInputManager()->setPointerLock(true);
    }

    void GameView::onFocusLost()
    {
        Box::onFocusLost();
        // 当游戏视图失去焦点时的处理逻辑，如恢复其他活动、调整输入等
        brls::Logger::debug("GameView lost focus");

        GameInputManager::instance().setInputEnabled(false); // 当游戏视图失去焦点时禁用输入处理
        GameInputManager::instance().dropInput();            // 清除当前输入状态，防止在游戏视图失去焦点时继续处理输入

        if (_brls_inputLocked)
        {
            _brls_inputLocked = false;
            brls::Application::unblockInputs(); // 解锁输入，允许其他视图处理输入
        }

        // 解锁鼠标指针
        // brls::Application::getPlatform()->getInputManager()->setPointerLock(false);
    }

    void GameView::draw(NVGcontext *vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext *ctx)
    {
        // 游戏视图的绘制逻辑，如渲染游戏画面等
        Box::draw(vg, x, y, width, height, style, ctx);

        GameInputManager::instance().handleInput(); // 在绘制时处理输入，确保游戏逻辑能够及时响应输入事件
    }

    void GameView::_registerGameInput()
    {
                GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_OPEN_MENU,
            {{brls::BUTTON_LB, brls::BUTTON_START}, {brls::BUTTON_RT, brls::BUTTON_LT}},
            []()
            {
                brls::Logger::debug("打开菜单热键触发！");
            },
            TriggerType::LONG_PRESS,
            2.5f // 按住2.5秒
        );
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_FAST_FORWARD,
            {{brls::BUTTON_LSB}},
            []()
            {
                brls::Logger::debug("快进触发！");
            }
        );
        GameInputManager::instance().registerEmuFunctionKey(
            EmuFunctionKey::EMU_REWIND,
            {{brls::BUTTON_RSB}},
            []()
            {
               brls::Logger::debug("倒带触发！");
            },
            TriggerType::HOLD);
    }

    void GameView::_registerGameRuntime()
    {
        // 判断游戏平台
        if (m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuGBA ||
            m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuGB || 
            m_gameEntry.platform == (int)beiklive::enums::EmuPlatform::EmuGBC)
        {

            m_gba_core = new beiklive::gba::CoreMgba();
            if(m_gba_core->SetupGame(m_gameEntry)) // 替换为实际的核心路径
            {
                brls::Logger::debug("GBA 核心已初始化，游戏路径：{}", m_gameEntry.path);
            }
        }
        else
        {
            brls::Logger::warning("不支持的平台：{}", m_gameEntry.platform);
        }

    }
}