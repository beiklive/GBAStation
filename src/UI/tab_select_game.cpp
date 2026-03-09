#include "UI/tab_select_game.hpp"
#include "UI/List_view.hpp"
using namespace brls::literals;  // for _i18n

SelectGameTab::SelectGameTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/mgba_xml/tabs/select_game.xml");

    // notify->registerClickAction([](...){
    //     std::string notification = NOTIFICATIONS[std::rand() % NOTIFICATIONS.size()];
    //     brls::Application::notify(notification);
    //     return true;
    // });

    auto dismissAction = [](View* view) {
        brls::Logger::info("Clicked on btn");
        // ListView* lv = dynamic_cast<ListView*>(view);
        // // view->present(lv);
        // this->present(lv);

        return true;
    };

    select_file->registerClickAction(dismissAction);



    // select_recent->setImage("img/tiles.png");
    // select_recent->setTitle("beiklive/select/recent"_i18n);

    // select_favorites->setImage("img/tiles.png");
    // select_favorites->setTitle("beiklive/select/favorites"_i18n);
    applyBackTheme(brls::Application::getPlatform()->getThemeVariant());

}

brls::View* SelectGameTab::create()
{
    return new SelectGameTab();
}


void SelectGameTab::applyBackTheme(brls::ThemeVariant theme)
{

}
