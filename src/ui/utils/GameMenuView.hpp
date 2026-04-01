#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include <functional>

namespace beiklive
{
    /// 游戏菜单视图
    ///
    /// 以半透明覆盖层形式显示在游戏画面之上，提供：
    ///   - 返回游戏按钮（将焦点交还给 GameView）
    ///   - 退出游戏按钮（触发退出信号）
    ///
    /// 由 GamePage 创建并注入回调后使用。
    class GameMenuView : public brls::Box
    {
        public:
            GameMenuView(beiklive::GameEntry gameData);
            ~GameMenuView();

            void draw(NVGcontext* vg, float x, float y, float w, float h,
                      brls::Style style, brls::FrameContext* ctx) override;

            /// 设置"返回游戏"回调（由 GamePage 注入）
            void setOnResume(std::function<void()> cb) { m_onResume = std::move(cb); }
            /// 设置"退出游戏"回调（由 GamePage 注入）
            void setOnExit(std::function<void()> cb) { m_onExit = std::move(cb); }

        private:
            beiklive::GameEntry m_gameEntry;
            std::function<void()> m_onResume;
            std::function<void()> m_onExit;

            brls::Box*   m_panel     = nullptr; ///< 居中面板容器
            brls::Label* m_title     = nullptr; ///< 标题文字
            brls::Box*   m_btnResume = nullptr; ///< "返回游戏"按钮
            brls::Box*   m_btnExit   = nullptr; ///< "退出游戏"按钮

            void _initLayout();
            brls::Box* _createMenuButton(const std::string& text, std::function<void()> onClick);
    };

}
