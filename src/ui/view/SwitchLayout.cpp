#include "SwitchLayout.hpp"
#include "core/ThreadPool.hpp"
#include "ui/widget/GameCard.hpp"
#include "ui/widget/RoundButton.hpp"

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

        auto* emptyCard = new brls::Box(brls::Axis::ROW);
        emptyCard->setHeight(60.f);
        addView(emptyCard);



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

    SwitchLayout::~SwitchLayout()
    {
        ++m_loadGen;
    }

    void SwitchLayout::_buildEmptyCards()
    {
        m_cardRow->clearViews(true);
        m_cardRow->setDefaultFocusedIndex(m_cardFocusIndex);
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
        int thisGen = ++m_loadGen;
        bool restoreFocus = isCardRowFocusActive();

        // UI 线程：构建卡片框架（无封面图片）
        buildCardRow(gameList);
        if (restoreFocus)
            restoreCardFocus(false);

        // 后台线程池：逐张加载封面图片
        auto& children = m_cardRow->getChildren();
        for (size_t i = 0; i < children.size() && i < gameList.size(); ++i)
        {
            GameCard* card = dynamic_cast<GameCard*>(children[i]);
            if (!card) continue;

            std::string logoPath = gameList[i].logoPath;
            if (logoPath.empty()) continue;

            ThreadPool::instance().enqueue([card, logoPath, thisGen, this, i]() {
                if (thisGen != m_loadGen.load()) return;
                brls::delay(static_cast<long>(i) * 18, [card, logoPath, thisGen, this]() {
                    if (thisGen != m_loadGen.load()) return;
                    card->loadCoverImage(logoPath);
                });
            });
        }
    }

    void SwitchLayout::buildCardRow(beiklive::GameList gameList)
    {
        m_cardRow->clearViews(true);

        size_t totalSlots = std::max(static_cast<size_t>(DEFAULT_EMPTY_CARDS), gameList.size());
        if (m_cardFocusIndex < 0)
            m_cardFocusIndex = 0;
        if (totalSlots > 0 && m_cardFocusIndex >= static_cast<int>(totalSlots))
            m_cardFocusIndex = static_cast<int>(totalSlots) - 1;
        m_cardRow->setDefaultFocusedIndex(m_cardFocusIndex);

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
                gameCard->onCardClicked = [this](beiklive::GameEntry &entry)
                {
                    if (onGameActivated)
                        onGameActivated(entry);
                };
                gameCard->onFavouriteToggled = [this](beiklive::GameEntry &) {
                    brls::Logger::debug("favourite toggled, refreshing game list");
                    ThreadPool::instance().enqueue([this]() {
                        beiklive::GameList recent = beiklive::GameDB
                            ? beiklive::GameDB->getRecentPlayed(10)
                            : beiklive::GameList{};
                        brls::sync([this, recent = std::move(recent)]() {
                            refreshGameList(recent);
                        });
                    });
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


        // functionbox
        auto *functionBox = new brls::Box(brls::Axis::ROW);
        m_functionArea->addView(functionBox);

        #undef ABSOLUTE
        functionBox->setPositionType(brls::PositionType::ABSOLUTE);
        functionBox->setPositionLeft(310.f);
        functionBox->setPositionTop(1.5f);
        functionBox->setFocusable(false);
        functionBox->setHeight(95.f);
        functionBox->setWidth(600.f);
        functionBox->setBackgroundColor(nvgRGBA(0, 0, 0, 10));
        functionBox->setClipsToBounds(false);
        functionBox->setCornerRadius(45.0f);
        functionBox->setShadowVisibility(true);
        functionBox->setShadowType(brls::ShadowType::GENERIC);
        functionBox->setBorderColor(nvgRGBA(125, 125, 125, 95));
        functionBox->setBorderThickness(1.5f);

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

        m_functionArea->addView(new brls::Padding());

        m_functionArea->addView(GameDataBaseButton);
        m_functionArea->addView(FileListButton);
        m_functionArea->addView(DataManagementButton);
        m_functionArea->addView(SettingsButton);
        m_functionArea->addView(AboutButton);
        m_functionArea->addView(ExitButton);

        m_functionArea->addView(new brls::Padding());
    }

    bool SwitchLayout::isCardRowFocusActive() const
    {
        return getCardIndexForFocus(brls::Application::getCurrentFocus()) >= 0;
    }

    int SwitchLayout::getCardIndexForFocus(brls::View* focusedView) const
    {
        if (!focusedView || !m_cardRow)
            return -1;

        brls::View* view = focusedView;
        while (view && view->getParent() != m_cardRow)
            view = view->getParent();

        if (!view)
            return -1;

        auto& children = m_cardRow->getChildren();
        for (size_t i = 0; i < children.size(); ++i)
        {
            if (children[i] == view)
                return static_cast<int>(i);
        }
        return -1;
    }

    void SwitchLayout::restoreCardFocus(bool animated)
    {
        auto& children = m_cardRow->getChildren();
        if (children.empty())
            return;

        if (m_cardFocusIndex < 0)
            m_cardFocusIndex = 0;
        if (m_cardFocusIndex >= static_cast<int>(children.size()))
            m_cardFocusIndex = static_cast<int>(children.size()) - 1;

        m_cardRow->setDefaultFocusedIndex(m_cardFocusIndex);
        brls::View* target = children[m_cardFocusIndex]->getDefaultFocus();
        if (!target)
            return;

        brls::Application::giveFocus(target);
        if (!animated && m_frame)
        {
            float cardCenter = children[m_cardFocusIndex]->getLocalX() + children[m_cardFocusIndex]->getWidth() * 0.5f;
            float newOffset = cardCenter - m_frame->getWidth() * 0.5f;
            m_frame->setContentOffsetX(newOffset, false);
        }
    }

    void SwitchLayout::onChildFocusGained(brls::View* directChild, brls::View* focusedView)
    {
        int index = getCardIndexForFocus(focusedView);
        if (index >= 0)
        {
            m_cardFocusIndex = index;
            m_cardRow->setDefaultFocusedIndex(index);
        }
        Layout::onChildFocusGained(directChild, focusedView);
    }

} // namespace beiklive
