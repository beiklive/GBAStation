#include "SwitchLayout.hpp"
#include "core/ThreadPool.hpp"
#include "ui/widget/GameCard.hpp"
#include "ui/widget/RoundButton.hpp"

namespace beiklive
{
    static constexpr int DEFAULT_EMPTY_CARDS = 10;
    static constexpr long CARD_REFRESH_STAGGER_MS = 35;
    static constexpr int INTRO_CARD_FADE_MS = 220;
    static constexpr int INTRO_FUNCTION_MS = 260;

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
            configureGameCard(gameCard);
            m_cardRow->addView(gameCard);
        }
    }

    void SwitchLayout::refreshGameList(beiklive::GameList gameList)
    {
        int thisGen = ++m_loadGen;
        bool restoreFocus = isCardRowFocusActive();

        // UI 线程：保证卡片槽位存在，但不清空整行，避免刷新时出现全空闪烁
        buildCardRow(gameList);
        if (!m_introAnimationPlayed && !gameList.empty())
            playInitialIntroAnimation();
        if (restoreFocus)
            restoreCardFocus(false);

        // 逐张错峰替换内容和加载封面，刷新效果更平滑
        auto& children = m_cardRow->getChildren();
        for (size_t i = 0; i < children.size(); ++i)
        {
            GameCard* card = dynamic_cast<GameCard*>(children[i]);
            if (!card) continue;

            beiklive::GameEntry entry = (i < gameList.size()) ? gameList[i] : beiklive::GameEntry{};

            brls::delay(static_cast<long>(i) * CARD_REFRESH_STAGGER_MS, [card, entry = std::move(entry), thisGen, this]() mutable {
                if (thisGen != m_loadGen.load()) return;
                card->setGameEntry(std::move(entry), true);
            });
        }
    }

    void SwitchLayout::buildCardRow(beiklive::GameList gameList)
    {
        size_t totalSlots = std::max(static_cast<size_t>(DEFAULT_EMPTY_CARDS), gameList.size());
        if (m_cardFocusIndex < 0)
            m_cardFocusIndex = 0;
        if (totalSlots > 0 && m_cardFocusIndex >= static_cast<int>(totalSlots))
            m_cardFocusIndex = static_cast<int>(totalSlots) - 1;
        m_cardRow->setDefaultFocusedIndex(m_cardFocusIndex);

        auto& children = m_cardRow->getChildren();
        for (size_t i = children.size(); i < totalSlots; ++i)
        {
            beiklive::GameEntry emptyEntry;
            auto *gameCard = new beiklive::GameCard(
                beiklive::enums::ThemeLayout::SWITCH_THEME, emptyEntry, static_cast<int>(i));
            gameCard->applyThemeLayout();
            gameCard->setMarginRight(10.f);
            gameCard->setMarginLeft(10.f);
            configureGameCard(gameCard);
            m_cardRow->addView(gameCard);
        }
    }

    void SwitchLayout::configureGameCard(GameCard* gameCard)
    {
        if (!gameCard)
            return;

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
            [this](brls::View *view)
            {
                auto* card = dynamic_cast<beiklive::GameCard*>(view);
                if (!card)
                {
                    brls::View* parent = view ? view->getParent() : nullptr;
                    while (parent && !(card = dynamic_cast<beiklive::GameCard*>(parent)))
                        parent = parent->getParent();
                }
                if (card && !card->isEmpty() && onGameOptions)
                    onGameOptions(card->getGameEntry());
                return true;
            });
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

    brls::View* SwitchLayout::getParentNavigationDecision(brls::View* from, brls::View* newFocus, brls::FocusDirection direction)
    {
        if (from == m_cardRow && !newFocus && (direction == brls::FocusDirection::LEFT || direction == brls::FocusDirection::RIGHT))
        {
            auto& children = m_cardRow->getChildren();
            int targetIndex = (direction == brls::FocusDirection::LEFT)
                ? static_cast<int>(children.size()) - 1
                : 0;
            if (targetIndex >= 0 && targetIndex < static_cast<int>(children.size()))
            {
                m_cardFocusIndex = targetIndex;
                m_cardRow->setDefaultFocusedIndex(targetIndex);
                brls::View* target = children[targetIndex]->getDefaultFocus();
                if (target && m_frame)
                {
                    float cardCenter = children[targetIndex]->getLocalX() + children[targetIndex]->getWidth() * 0.5f;
                    float newOffset = cardCenter - m_frame->getWidth() * 0.5f;
                    m_frame->setContentOffsetX(newOffset, false);
                }
                return target;
            }
        }

        return Layout::getParentNavigationDecision(from, newFocus, direction);
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

    void SwitchLayout::resetCardFocusToFirst()
    {
        m_cardFocusIndex = 0;
        m_cardRow->setDefaultFocusedIndex(0);
        restoreCardFocus(false);
    }

    void SwitchLayout::playInitialIntroAnimation()
    {
        if (m_introAnimationPlayed)
            return;

        m_introAnimationPlayed = true;
        m_introAnimating = true;

        m_introCardAlpha.stop();
        m_introFunctionAlpha.stop();
        m_introFunctionY.stop();

        m_introCardAlpha.reset(0.0f);
        m_introFunctionAlpha.reset(0.0f);
        m_introFunctionY.reset(26.0f);

        if (m_cardRow)
            m_cardRow->setAlpha(0.0f);
        if (m_functionArea)
        {
            m_functionArea->setAlpha(0.0f);
            m_functionArea->setTranslationY(26.0f);
        }

        m_introCardAlpha.addStep(1.0f, INTRO_CARD_FADE_MS, brls::EasingFunction::quadraticOut);
        m_introCardAlpha.start();

        brls::delay(70, [this]() {
            if (!m_introAnimating)
                return;

            m_introFunctionAlpha.addStep(1.0f, INTRO_FUNCTION_MS, brls::EasingFunction::quadraticOut);
            m_introFunctionY.addStep(0.0f, INTRO_FUNCTION_MS, brls::EasingFunction::quadraticOut);
            m_introFunctionY.setEndCallback([this](bool finished) {
                if (!finished)
                    return;
                m_introAnimating = false;
                if (m_cardRow)
                    m_cardRow->setAlpha(1.0f);
                if (m_functionArea)
                {
                    m_functionArea->setAlpha(1.0f);
                    m_functionArea->setTranslationY(0.0f);
                }
            });
            m_introFunctionAlpha.start();
            m_introFunctionY.start();
        });
    }

    void SwitchLayout::frame(brls::FrameContext* ctx)
    {
        Layout::frame(ctx);

        if (!m_introAnimating)
            return;

        if (m_cardRow)
            m_cardRow->setAlpha(m_introCardAlpha);
        if (m_functionArea)
        {
            m_functionArea->setAlpha(m_introFunctionAlpha);
            m_functionArea->setTranslationY(m_introFunctionY);
        }
        invalidate();
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
