#include "UI/Pages/SettingPage.hpp"


SettingPage::SettingPage()
{
    m_tabframe = new brls::TabFrame();
    Init();
    addView(m_tabframe);
}



SettingPage::~SettingPage()
{

}

void SettingPage::Init()
{
    m_tabframe->addTab("界面设置", []() {
        auto* page = new brls::Box();

        return page;
    });
    m_tabframe->addTab("游戏设置", []() {
        auto* page = new brls::Box();

        return page;
    });
    m_tabframe->addTab("画面设置", []() {
        auto* page = new brls::Box();

        return page;
    });
    m_tabframe->addTab("声音设置", []() {
        auto* page = new brls::Box();

        return page;
    });
    m_tabframe->addTab("按键映射", []() {
        auto* page = new brls::Box();

        return page;
    });
    m_tabframe->addTab("调试工具", []() {
        auto* page = new brls::Box();

        return page;
    });
}