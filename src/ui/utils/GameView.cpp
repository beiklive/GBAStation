#include "GameView.hpp"

namespace beiklive
{
    GameView::GameView(beiklive::GameEntry gameData)
    {
        // 游戏视图初始化逻辑，如设置背景、输入处理等
        _brls_inputLocked = false;               // 初始化输入锁定状态
        GameInputManager::instance().sayHello(); // 测试输入管理器是否正常工作
        HIDE_BRLS_HIGHLIGHT(this);               // 隐藏Borealis的默认高亮效果，避免与游戏视图的交互冲突

        _registerGameInput(); // 注册游戏相关的输入
    }

    GameView::~GameView()
    {
        // 游戏视图清理逻辑，如释放资源等
        GameInputManager::instance().clearEmuFunctionKeys(); // 清除所有注册的热键，确保在游戏视图销毁时不会留下无效的回调
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
                // brls::Application::notify("打开菜单热键触发！"); // 这里可以替换为实际的打开菜单逻辑
                // brls::sync([]()
                //            {
                //                brls::Application::popActivity(brls::TransitionAnimation::FADE); // 示例：触发热键时关闭当前活动，返回上一级菜单
                //            });
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
}