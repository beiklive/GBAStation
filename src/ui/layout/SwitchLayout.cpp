#include "SwitchLayout.hpp"

namespace beiklive
{
    SwitchLayout::SwitchLayout() : Layout()
    {
        HIDE_BRLS_BACKGROUND(this);
        this->setAxis(brls::Axis::COLUMN);
        m_frame = new brls::HScrollingFrame();
        m_frame->setGrow(1.f);
        m_frame->setWidth(View::AUTO);
        m_frame->setHeight(400.f);
        m_frame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_frame->setScrollingIndicatorVisible(false);
        m_cardRow = new brls::Box(brls::Axis::ROW);
        m_cardRow->setAlignItems(brls::AlignItems::CENTER);
        m_cardRow->setGrow(0.0f);
        m_cardRow->setHeight(300.f);
        m_frame->setContentView(m_cardRow);

        m_functionArea = new brls::Box(brls::Axis::ROW);
        m_functionArea->setGrow(1.0f);

        addView(m_frame);
        addView(m_functionArea);

        buildFunctionArea();
    }

    void SwitchLayout::refreshGameList(beiklive::GameList *gameList)
    {
        beiklive::GameList copy = *gameList;
        brls::sync([this, copy = std::move(copy)]() mutable
        {
            buildCardRow(&copy);
            brls::Application::giveFocus(m_cardRow->getDefaultFocus());
        });
    }

    void SwitchLayout::buildCardRow(beiklive::GameList *gameList)
    {
        m_cardRow->clearViews(true);
        for (auto &gameEntry : *gameList)
        {
            auto *gameCard = new beiklive::GameCard(beiklive::enums::ThemeLayout::SWITCH_THEME, gameEntry);
            gameCard->applyThemeLayout();
            gameCard->setMarginRight(10.f);
            gameCard->setMarginLeft(10.f);
            gameCard->onCardClicked = [this](beiklive::GameEntry &entry)
            {
                if (onGameActivated)
                    onGameActivated(entry);
            };
            gameCard->registerAction(
                "游戏选项",
                brls::BUTTON_X,
                [this, gameEntry](brls::View *)
                {
                    if (onGameOptions)
                        onGameOptions(gameEntry);
                    return true;
                });

            m_cardRow->addView(gameCard);
        }
    }

    void SwitchLayout::buildFunctionArea()
    {
        m_functionArea->addView(new brls::Padding());
        std::string path_prefix = "img/ui/" +
                                  std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");

        auto GameDataBaseButton = new beiklive::RoundButton(BK_RES(path_prefix + "GameList_64.png"), "游戏库", [this]()
                                                            {if (onGameLibraryOpened) onGameLibraryOpened(); });
        auto FileListButton = new beiklive::RoundButton(BK_RES(path_prefix + "wenjianjia_64.png"), "文件列表", [this]()
                                                        {if (onFileBrowserOpened) onFileBrowserOpened(); });
        auto DataManagementButton = new beiklive::RoundButton(BK_RES(path_prefix + "jifen_64.png"), "数据管理", [this]()
                                                              { if (onDataManagementOpened) onDataManagementOpened(); });
        auto SettingsButton = new beiklive::RoundButton(BK_RES(path_prefix + "shezhi_64.png"), "设置", [this]()
                                                        { if(onSettingsOpened) onSettingsOpened(); });
        auto AboutButton = new beiklive::RoundButton(BK_RES(path_prefix + "bangzhu_64.png"), "关于", [this]()
                                                     { if (onAboutOpened) onAboutOpened(); });
        auto ExitButton = new beiklive::RoundButton(BK_RES(path_prefix + "tuichu_64.png"), "退出", [this]()
                                                    { if (onExitRequested) onExitRequested(); });

        m_functionArea->addView(GameDataBaseButton);
        m_functionArea->addView(FileListButton);
        m_functionArea->addView(DataManagementButton);
        m_functionArea->addView(SettingsButton);
        m_functionArea->addView(AboutButton);
        m_functionArea->addView(ExitButton);

        m_functionArea->addView(new brls::Padding());
    }

} // namespace beiklive
