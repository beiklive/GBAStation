#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"
#include "ui/widget/TabFrame.hpp"

namespace beiklive
{
    /// 设置页面：使用 TabFrame 展示多分类设置
    class SettingPage : public beiklive::Box
    {
    public:
        SettingPage();
        ~SettingPage();

    private:
        beiklive::TabFrame *m_tabframe = nullptr;


        // 构建各标签页
        brls::View *buildUITab();
        brls::View *buildGameTab();
        brls::View *buildDisplayTab();
        brls::View *buildAudioTab();
        brls::View *buildKeyBindTab();
        brls::View *buildDebugTab();

        void init();
    };
} // namespace beiklive
