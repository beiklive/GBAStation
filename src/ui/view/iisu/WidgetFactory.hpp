#pragma once

#include <memory>
#include <string>

#include "ClockWidget.hpp"
#include "ColorWidget.hpp"
#include "FolderWidget.hpp"
#include "GameCoverWidget.hpp"
#include "ImageWidget.hpp"
#include "RecentGamesWidget.hpp"
#include "SystemInfoWidget.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// Widget 工厂：按类型创建组件（替代 LayoutManager 里的 if(type==...) 分支）
    class WidgetFactory
    {
    public:
        static std::shared_ptr<Widget> create(WidgetType type);

        /// 调试用：纯色块组件（临时验证用）
        static std::shared_ptr<Widget> createColor(NVGcolor color)
        {
            return std::make_shared<ColorWidget>(color);
        }

        /// 图片组件
        static std::shared_ptr<Widget> createImage(std::string path)
        {
            return std::make_shared<ImageWidget>(std::move(path));
        }

        /// 游戏封面组件（gameId = 游戏文件路径，数据由 GameDataProvider 提供）
        static std::shared_ptr<Widget> createGameCover(std::string gameId)
        {
            return std::make_shared<GameCoverWidget>(std::move(gameId));
        }

        /// 文件夹组件（folderId 由 FolderDataProvider 解析子布局）
        static std::shared_ptr<Widget> createFolder(std::string folderId)
        {
            return std::make_shared<FolderWidget>(std::move(folderId));
        }

        /// 动态组件（widgetId: recent_games / system_info / clock）
        static std::shared_ptr<Widget> createLive(const std::string& widgetId)
        {
            if (widgetId == "recent_games")
                return std::make_shared<RecentGamesWidget>();
            if (widgetId == "system_info")
                return std::make_shared<SystemInfoWidget>();
            if (widgetId == "clock")
                return std::make_shared<ClockWidget>();
            return nullptr;
        }
    };
} // namespace beiklive
