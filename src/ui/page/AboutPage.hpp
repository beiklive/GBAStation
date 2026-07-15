#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"

namespace beiklive {

class AboutPage : public beiklive::Box {
public:
    AboutPage();

private:
    brls::View* m_aboutCanvas = nullptr;

    brls::View* _buildInfoTab();
    brls::View* _buildUpdateTab();
    brls::View* _buildSupportTab();
    void _checkUpdate();
    void _updateCheatDatabase();
    void _downloadNdsFirmware();
    void _downloadNdsCheatDatabase();
};

} // namespace beiklive
