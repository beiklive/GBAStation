#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"

namespace beiklive
{
    /// 设置页面：使用顶部分类轮播展示多分类设置
    class SettingPage : public beiklive::Box
    {
    public:
        SettingPage();
        ~SettingPage();

    private:
        brls::View *m_settingsFrame = nullptr;


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
