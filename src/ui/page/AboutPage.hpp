#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"
#include "ui/utils/TabFrame.hpp"

namespace beiklive {

class AboutPage : public beiklive::Box {
public:
    AboutPage();

private:
    beiklive::TabFrame* m_tabFrame = nullptr;

    brls::View* _buildInfoTab();
    brls::View* _buildUpdateTab();
    brls::View* _buildSupportTab();
    void _checkUpdate();
    void _updateCheatDatabase();
};

} // namespace beiklive
