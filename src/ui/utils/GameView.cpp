#include "GameView.hpp"

namespace beiklive
{
    GameView::GameView(beiklive::GameEntry gameData)
    {
        // 游戏视图初始化逻辑，如设置背景、输入处理等
        GameInputManager::instance().sayHello(); // 测试输入管理器是否正常工作
    }

    GameView::~GameView()
    {
        // 游戏视图清理逻辑，如释放资源等
    }


    void GameView::onFocusGained()
    {
        Box::onFocusGained();
        brls::Logger::debug("GameView gained focus");

        GameInputManager::instance().setInputEnabled(true); // 当游戏视图获得焦点时启用输入处理
        // 当游戏视图获得焦点时的处理逻辑，如暂停其他活动、调整输入等


        if(!_brls_inputLocked)
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
        GameInputManager::instance().dropInput(); // 清除当前输入状态，防止在游戏视图失去焦点时继续处理输入

        if(_brls_inputLocked)
        {
            _brls_inputLocked = false;
            brls::Application::unblockInputs(); // 解锁输入，允许其他视图处理输入
        }
    
    
        // 解锁鼠标指针
        // brls::Application::getPlatform()->getInputManager()->setPointerLock(false);
    
    
    }


    void GameView::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx)
    {
        // 游戏视图的绘制逻辑，如渲染游戏画面等
        Box::draw(vg, x, y, width, height, style, ctx);
        
    
        GameInputManager::instance().handleInput(); // 在绘制时处理输入，确保游戏逻辑能够及时响应输入事件
    
    }


}