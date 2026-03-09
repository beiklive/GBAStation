#include "UI/game_view.hpp"
#ifdef __SWITCH__

#include <borealis.hpp>
#include <cmath>


GameView::GameView(std::string gameName) : GameView()
{
    brls::Logger::debug("GameView constructor with gameName called: {}", gameName);
    m_gameName = std::move(gameName);
    initialize();
}

GameView::GameView()
{
    brls::Logger::debug("GameView constructor called");
    setFocusable(true);
    setHideHighlight(true);

    // Block all Borealis navigation and face-button actions while the game
    // has focus.  The game reads input directly via libnx padGetButtons(), so
    // we never want Borealis to intercept these presses.
    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_B);
    beiklive::swallow(this, brls::BUTTON_X);
    beiklive::swallow(this, brls::BUTTON_Y);
    beiklive::swallow(this, brls::BUTTON_UP);
    beiklive::swallow(this, brls::BUTTON_DOWN);
    beiklive::swallow(this, brls::BUTTON_LEFT);
    beiklive::swallow(this, brls::BUTTON_RIGHT);
    beiklive::swallow(this, brls::BUTTON_NAV_UP);
    beiklive::swallow(this, brls::BUTTON_NAV_DOWN);
    beiklive::swallow(this, brls::BUTTON_NAV_LEFT);
    beiklive::swallow(this, brls::BUTTON_NAV_RIGHT);
    beiklive::swallow(this, brls::BUTTON_LB);
    beiklive::swallow(this, brls::BUTTON_RB);
    beiklive::swallow(this, brls::BUTTON_LT);
    beiklive::swallow(this, brls::BUTTON_RT);

    brls::Logger::debug("GameView constructed, initialization deferred to first draw");
}

GameView::~GameView()
{
    brls::Logger::debug("GameView destructor called");
}

void GameView::initialize()
{
    brls::Logger::debug("GameView::initialize() - GameView created for game: {}", m_gameName);
    this->setHideHighlight(true);
    this->setHideClickAnimation(true);
    this->setHideHighlightBackground(true);
    this->setHideHighlightBorder(true);

}

void GameView::draw(NVGcontext* vg, float x, float y, float width, float height,
                    brls::Style style, brls::FrameContext* ctx)
{

    this->invalidate();
}

void GameView::onFocusGained()
{
    brls::Logger::debug("GameView::onFocusGained() called");
    Box::onFocusGained();
}

void GameView::onFocusLost()
{
    brls::Logger::debug("GameView::onFocusLost() called");
    Box::onFocusLost();
}

void GameView::onLayout()
{
    brls::Logger::debug("GameView::onLayout() called");
    Box::onLayout();
}

#endif