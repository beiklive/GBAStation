#pragma once

#include "core/common.h"
#include "ui/view/NdsDekoGameMenuView.hpp"
#include "ui/view/NdsDekoGameView.hpp"
#include "ui/widget/Box.hpp"

namespace beiklive {

class NdsDekoGamePage : public beiklive::Box {
public:
    explicit NdsDekoGamePage(beiklive::DirListData gameData);
    explicit NdsDekoGamePage(beiklive::GameEntry gameEntry);
    ~NdsDekoGamePage();

    void startGame();

private:
    void _pageInit();
    void _setupGame();
    void _initGameEntryFromDir();
    void _initGameEntryPaths();
    void _updateGameCount();
    void _openMenu();
    void _closeMenu();
    void _exitToPreviousPage();

    beiklive::DirListData m_gameData;
    beiklive::GameEntry m_gameEntry;
    NdsDekoGameView* m_gameView = nullptr;
    NdsDekoGameMenuView* m_gameMenuView = nullptr;
    bool m_started = false;
    bool m_exitRequested = false;
};

} // namespace beiklive
