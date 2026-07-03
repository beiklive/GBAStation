#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"

#include <functional>

namespace beiklive {

class NdsDekoGameMenuView : public beiklive::Box {
public:
    explicit NdsDekoGameMenuView(beiklive::GameEntry gameEntry);

    void onShow();
    void setOnResume(std::function<void()> cb) { m_onResume = std::move(cb); }
    void setOnExit(std::function<void()> cb) { m_onExit = std::move(cb); }

private:
    void _initLayout();

    beiklive::GameEntry m_gameEntry;
    std::function<void()> m_onResume;
    std::function<void()> m_onExit;
};

} // namespace beiklive
