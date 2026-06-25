#pragma once

#include "core/common.h"
#include <functional>
#include "ButtonBox.hpp"

namespace beiklive
{
    class TabFrame : public brls::Box
    {
        public:
            TabFrame();
            ~TabFrame();
            void draw(NVGcontext* vg, float x, float y, float w, float h,
                brls::Style style, brls::FrameContext* ctx) override;
                
            void addDivider();

            void addTab(
                /* 按钮文字 */ const std::string& text,
                /* 按钮图标路径 */ const std::string& iconPath,
                /* 点击回调 */ std::function<void()> onClick = nullptr,
                /* 聚焦回调 */ std::function<void()> onFocus = nullptr,
                /* 失焦回调 */ std::function<void()> onBlur = nullptr,
                /* 关联的内容视图 */ brls::View* contentView = nullptr,
                /* 内容视图默认焦点 */ brls::View* contentDefaultFocus = nullptr
            );
            void addFinish();

            void onShow();

            /// 开关 Tab 切换滑入动画（默认开启）
            void setAnimationEnabled(bool enabled) { m_animationEnabled = enabled; }
            bool isAnimationEnabled() const { return m_animationEnabled; }
        private:
            void _initLayout();
            brls::Box* m_tabBar = nullptr; ///< 标签栏容器
            brls::Box* m_contentArea = nullptr; ///< 内容区域容器
            bool m_animationEnabled = true; ///< Tab 切换滑入动画开关
    };
}
