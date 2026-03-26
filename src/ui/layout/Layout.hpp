#pragma once
#include <functional>

#include <borealis.hpp>
#include "core/common.h"


namespace beiklive
{
    class Layout : public brls::Box
    {
    public:
        Layout();
        ~Layout() = default;
        virtual void refreshGameList(GameList* gameList) = 0; // 刷新游戏列表显示



        // ===================================================================
        // 游戏打开接口, 触发打开游戏时调用, 由布局内部实现来触发
        std::function<void(const beiklive::GameEntry&)> onGameActivated;
        // 游戏设置接口, 触发打开游戏卡片设置功能时调用, 由布局内部实现来触发
        std::function<void(const beiklive::GameEntry&)> onGameOptions;
        // ===================================================================
        // 游戏库接口, 触发打开游戏库时调用, 由布局内部实现来触发
        std::function<void()> onGameLibraryOpened;
        // 文件浏览器接口, 触发打开文件浏览器时调用, 由布局内部实现来触发
        std::function<void()> onFileBrowserOpened;
        // 数据管理接口, 触发打开数据管理时调用, 由布局内部实现来触发
        std::function<void()> onDataManagementOpened;
        // 设置界面接口, 触发打开设置界面时调用, 由布局内部实现来触发
        std::function<void()> onSettingsOpened;
        // 关于界面接口, 触发打开关于界面时调用, 由布局内部实现来触发
        std::function<void()> onAboutOpened;
        // 退出应用接口, 触发退出应用时调用, 由布局内部实现来触发
        std::function<void()> onExitRequested;
        // ===================================================================

    };
} // namespace beiklive