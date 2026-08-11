#include "FolderDataProvider.hpp"

#include <cstdlib>

#include "core/Tools.hpp"
#include "core/common.h"

namespace beiklive
{
    namespace
    {
        bool parsePlatformId(const std::string& id, int& platform)
        {
            const std::string prefix = "platform:";
            if (id.rfind(prefix, 0) != 0)
                return false;
            platform = std::atoi(id.c_str() + prefix.size());
            return platform > 0;
        }
    } // namespace

    std::optional<FolderInfo> GameDbFolderProvider::getFolder(
        const std::string& id) const
    {
        int platform = 0;
        if (!parsePlatformId(id, platform) || !beiklive::GameDB)
            return std::nullopt;

        const auto games = beiklive::GameDB->getByPlatform(
            static_cast<beiklive::enums::EmuPlatform>(platform));

        FolderInfo info;
        info.id = id;
        info.title = beiklive::tools::platformBadgeName(platform);
        info.childCount = static_cast<int>(games.size());
        return info;
    }

    std::vector<FolderItemDescriptor> GameDbFolderProvider::getFolderItems(
        const std::string& id) const
    {
        int platform = 0;
        if (!parsePlatformId(id, platform) || !beiklive::GameDB)
            return {};

        const auto games = beiklive::GameDB->getByPlatform(
            static_cast<beiklive::enums::EmuPlatform>(platform));

        std::vector<FolderItemDescriptor> items;
        const size_t count = games.size();
        for (size_t i = 0; i < count; ++i) {
            FolderItemDescriptor desc;
            desc.type = WidgetType::GameCover;
            desc.id = games[i].path;
            // 文件夹浮层固定三行、按列向右扩展，由 GridSystem 横向滚动。
            desc.x = static_cast<int>(i / 3);
            desc.y = static_cast<int>(i % 3);
            desc.w = 1;
            desc.h = 1;
            items.push_back(std::move(desc));
        }
        return items;
    }
} // namespace beiklive
