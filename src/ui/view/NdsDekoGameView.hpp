#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"

#include <functional>

namespace brls {
class Label;
}

namespace beiklive {

class NdsDekoGameView : public beiklive::Box {
public:
    explicit NdsDekoGameView(beiklive::GameEntry gameEntry);

    void startProbe();
    void setOnOpenMenu(std::function<void()> cb) { m_onOpenMenu = std::move(cb); }

private:
    void _initLayout();
    void _runProbeLevel(int level);

    beiklive::GameEntry m_gameEntry;
    std::function<void()> m_onOpenMenu;
    brls::Label* m_statusLabel = nullptr;
    int m_nextProbeLevel = 1;
};

} // namespace beiklive
