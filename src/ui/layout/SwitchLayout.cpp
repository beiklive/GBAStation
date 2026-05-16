#include "SwitchLayout.hpp"

namespace beiklive
{
    static constexpr int DEFAULT_EMPTY_CARDS = 10;

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
        m_cardRow->setGrow(1.0f);
        m_cardRow->setPaddingRight(20.f);
        m_cardRow->setPaddingLeft(20.f);
        m_frame->setContentView(m_cardRow);

        m_functionArea = new brls::Box(brls::Axis::ROW);
        m_functionArea->setHeight(150.f);

        addView(m_frame);
        addView(m_functionArea);

        buildFunctionArea();

        _buildEmptyCards();
    }

    void SwitchLayout::_buildEmptyCards()
    {
        m_cardRow->clearViews(true);
        for (int i = 0; i < DEFAULT_EMPTY_CARDS; ++i)
        {
            beiklive::GameEntry emptyEntry;
            auto *gameCard = new beiklive::GameCard(
                beiklive::enums::ThemeLayout::SWITCH_THEME, emptyEntry, i);
            gameCard->applyThemeLayout();
            gameCard->setMarginRight(10.f);
            gameCard->setMarginLeft(10.f);
            m_cardRow->addView(gameCard);
        }
    }

    void SwitchLayout::refreshGameList(beiklive::GameList gameList)
    {
        _updateCardData(gameList);
    }

    void SwitchLayout::_updateCardData(const beiklive::GameList& gameList)
    {
        size_t totalSlots = std::max(static_cast<size_t>(DEFAULT_EMPTY_CARDS), gameList.size());

        auto& children = m_cardRow->getChildren();

        for (size_t i = 0; i < totalSlots; ++i)
        {
            GameCard* card = nullptr;
            if (i < children.size())
                card = dynamic_cast<GameCard*>(children[i]);
            else
                continue;

            if (!card) continue;

            if (i < gameList.size())
            {
                card->updateGameEntry(gameList[i]);
                if (!card->onCardClicked)
                {
                    card->onCardClicked = [this](beiklive::GameEntry &entry)
                    {
                        if (onGameActivated)
                            onGameActivated(entry);
                    };
                }
                card->updateLogo(gameList[i].logoPath);
            }
            else
            {
                beiklive::GameEntry emptyEntry;
                card->updateGameEntry(emptyEntry);
            }
        }
    }

    void SwitchLayout::buildCardRow(beiklive::GameList gameList)
    {
        m_cardRow->clearViews(true);

        size_t totalSlots = std::max(static_cast<size_t>(DEFAULT_EMPTY_CARDS), gameList.size());

        for (size_t i = 0; i < totalSlots; ++i)
        {
            if (i < gameList.size())
            {
                auto gameEntry = gameList[i];
                auto *gameCard = new beiklive::GameCard(
                    beiklive::enums::ThemeLayout::SWITCH_THEME, gameEntry, static_cast<int>(i));
                gameCard->applyThemeLayout();
                gameCard->setMarginRight(10.f);
                gameCard->setMarginLeft(10.f);
                gameCard->updateLogo(gameEntry.logoPath);
                gameCard->onCardClicked = [this](beiklive::GameEntry &entry)
                {
                    if (onGameActivated)
                        onGameActivated(entry);
                };
                gameCard->onFavouriteToggled = [this, i](beiklive::GameEntry &toggled) {
                    // 收藏变更后只更新对应卡片的数据
                    if (beiklive::GameDB) {
                        auto recent = beiklive::GameDB->getRecentPlayed(10);
                        brls::sync([this, recent]() {
                            _updateCardData(recent);
                        });
                    }
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
            else
            {
                beiklive::GameEntry emptyEntry;
                auto *gameCard = new beiklive::GameCard(
                    beiklive::enums::ThemeLayout::SWITCH_THEME, emptyEntry, static_cast<int>(i));
                gameCard->applyThemeLayout();
                gameCard->setMarginRight(10.f);
                gameCard->setMarginLeft(10.f);
                m_cardRow->addView(gameCard);
            }
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
