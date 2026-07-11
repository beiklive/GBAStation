#pragma once

#include <borealis.hpp>
#include "core/common.h"
#include "ui/widget/ButtonBox.hpp"
#include <functional>
#include <string>
#include <vector>

namespace beiklive
{

    /// 通用侧边栏组件：半透明遮罩 + 右侧面板 + 动态按钮列表
    ///
    /// 用法:
    ///   auto* sidebar = new GameOptionsSidebar();
    ///   contentBox->addView(sidebar);
    ///   sidebar->addButton("选项A", BK_RES("icon.png"), [](const GameEntry& e) { ... });
    ///   sidebar->addButton("选项B", BK_RES("icon.png"), [](const GameEntry& e) { ... });
    ///   sidebar->open(entry);
    class GameOptionsSidebar : public brls::Box
    {
    public:
        GameOptionsSidebar();
        ~GameOptionsSidebar() = default;

        /// 打开侧边栏，传入游戏条目信息（标题和图标显示在面板顶部）
        void open(const beiklive::GameEntry& entry);

        /// 关闭侧边栏
        void close();

        /// 是否正在显示
        bool isOpen() const { return m_isOpen; }

        /// 添加一个操作按钮
        /// @param text     按钮文字
        /// @param iconCodepoint Material Icons 字形码点
        /// @param callback A 键点击回调，参数为 open() 时传入的 GameEntry
        void addButton(const std::string& text,
                       char32_t iconCodepoint,
                       std::function<void(const beiklive::GameEntry&)> callback);

        /// 清空所有按钮
        void clearButtons();

        /// 面板关闭后回调
        std::function<void()> onClosed;

    private:
        void _buildUI(const beiklive::GameEntry& entry);
        void _destroyUI();

        bool m_isOpen = false;

        // 按钮配置
        struct ButtonConfig
        {
            std::string text;
            char32_t iconCodepoint;
            std::function<void(const beiklive::GameEntry&)> callback;
        };
        std::vector<ButtonConfig> m_buttons;

        // 运行时 UI 控件
        std::vector<beiklive::ButtonBox*> m_btnInstances;
        brls::Box*   m_panel      = nullptr;
        brls::Label* m_titleLabel = nullptr;
        brls::Image* m_iconImage  = nullptr;

        beiklive::GameEntry m_entry;
    };

} // namespace beiklive
