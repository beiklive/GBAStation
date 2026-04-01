#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"

namespace beiklive
{
    /// 设置页面：使用 TabFrame 展示多分类设置
    class SettingPage : public beiklive::Box
    {
    public:
        SettingPage();
        ~SettingPage();

    private:
        brls::TabFrame *m_tabframe = nullptr;

        // 构建各标签页
        brls::ScrollingFrame *buildUITab();
        brls::ScrollingFrame *buildGameTab();
        brls::ScrollingFrame *buildDisplayTab();
        brls::ScrollingFrame *buildAudioTab();
        brls::ScrollingFrame *buildKeyBindTab();
        brls::ScrollingFrame *buildDebugTab();

        void init();
    };
} // namespace beiklive
