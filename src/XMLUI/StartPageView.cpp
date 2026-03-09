#include "XMLUI/StartPageView.hpp"

#include "UI/game_view.hpp"
#include "XMLUI/Pages/AppPage.hpp"

#undef ABSOLUTE // 避免系统头文件宏与 brls::PositionType::ABSOLUTE 冲突
StartPageView::StartPageView()
{
    // 背景图片（绝对定位，不参与布局流）
    m_bgImage = new brls::Image();
    m_bgImage->setFocusable(false);
    m_bgImage->setPositionType(brls::PositionType::ABSOLUTE);
    m_bgImage->setPositionTop(0);
    m_bgImage->setPositionLeft(0);
    m_bgImage->setWidthPercentage(100);
    m_bgImage->setHeightPercentage(100);
    m_bgImage->setScalingType(brls::ImageScalingType::FIT);
    m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_bgImage->setImageFromFile(BK_APP_DEFAULT_BG);
    addView(m_bgImage);
}

void StartPageView::Init()
{
    setBackground(brls::ViewBackground::NONE);



    if (!gameRunner)
    {
        return;
    }

	beiklive::swallow(gameRunner->uiParams->StartPageframe, brls::BUTTON_B);
	gameRunner->uiParams->StartPageframe->registerAction("beiklive/hints/exit"_i18n, brls::BUTTON_START, [](brls::View*) {
		bklog::debug("Exit app");
		brls::Application::quit();
		return true;
	});


    if (SettingManager->Contains(KEY_UI_START_PAGE))
    {
        int start_page_index = *gameRunner->settingConfig->Get(KEY_UI_START_PAGE)->AsInt();
        if (start_page_index == 0) // APP 页面
        {
            AppPage* app_page = new AppPage();
            app_page->addGame({ "/mGBA/roms/haogou.gba", "好狗狗星系", BK_RES("img/thumb/209.png") });
            app_page->addGame({ "/mGBA/roms/gba/jiezou.gba", "节奏天国", BK_RES("img/thumb/209.png") });
            app_page->addGame({ "/mGBA/roms/gba/Mother 3.gba", "地球冒险3", BK_RES("img/thumb/210.png") });
            app_page->addGame({ "/mGBA/roms/gba/MuChangWuYu.gba", "牧场物语", "" });
            app_page->addGame({ "/mGBA/roms/gba/Mother 3.gba", "地球冒险3", BK_RES("img/thumb/212.png") });
            app_page->addGame({ "/mGBA/roms/gba/MuChangWuYu.gba", "牧场物语", "" });
            app_page->addGame({ "/mGBA/roms/gba/Mother 3.gba", "地球冒险3", BK_RES("img/thumb/214.png") });
            app_page->addGame({ "/mGBA/roms/gba/MuChangWuYu.gba", "牧场物语", "" });
            app_page->addGame({ "/mGBA/roms/gba/Mother 3.gba", "地球冒险3", BK_RES("img/thumb/214.png") });
            app_page->addGame({ "/mGBA/roms/gba/MuChangWuYu.gba", "牧场物语", "" });
            app_page->onGameSelected = [](const GameEntry& e)
            {
                auto* frame = new brls::AppletFrame(new GameView(e.path));
                frame->setHeaderVisibility(brls::Visibility::GONE);
                frame->setFooterVisibility(brls::Visibility::GONE);
                frame->setBackground(brls::ViewBackground::NONE);
                brls::Application::pushActivity(new brls::Activity(frame));
            };
            addView(app_page);
            registerAction("beiklive/hints/FILE"_i18n, brls::ControllerButton::BUTTON_LT, [this](brls::View*) {
                bklog::debug("Switch to File List View");
                return true;
            });
        }
        else if (start_page_index == 1) // 文件列表界面
        {
    
    
            registerAction("beiklive/hints/APP"_i18n, brls::ControllerButton::BUTTON_LT, [this](brls::View*) {
                bklog::debug("Switch to APP View");
                return true;
            });
        }
        bklog::debug("Startup Page: {}", start_page_index);
    }

}


StartPageView::~StartPageView()
{
}

void StartPageView::onFocusGained()
{
    Box::onFocusGained();
}

void StartPageView::onFocusLost()
{
    Box::onFocusLost();
}

void StartPageView::onLayout()
{
    Box::onLayout();
}

brls::View* StartPageView::create()
{
    return new StartPageView();
}