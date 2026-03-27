#include "GamePage.hpp"
#include "core/Tools.hpp"

namespace beiklive
{
    GamePage::GamePage(beiklive::DirListData gameData)
    {
        m_gameData = std::move(gameData);
        // 检查文件是否存在
        if (!beiklive::tools::isFileExists(m_gameData.fullPath)) {
            brls::Application::notify("文件不存在: " + m_gameData.fileName);
            // 这里可以选择返回上一级或显示错误界面
            brls::sync([this]()
            {
                brls::Application::popActivity();
            });
        }else{
            PageInit();
        }
    }

    GamePage::~GamePage()
    {

    }

    void GamePage::PageInit()
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setFocusable(true);
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
            }, /*hidden=*/false, /*repeat=*/false, brls::SOUND_BACK);

    }

    void GamePage::GameViewInitialize()
    {
        
    }

    void GamePage::GameMenuInitialize()
    {
        // 游戏菜单初始化逻辑，如设置菜单选项、绑定按键等
    }



}