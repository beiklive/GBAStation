#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Widget.hpp"

namespace beiklive
{
    /// 文件夹信息
    struct FolderInfo
    {
        std::string id;
        std::string title;
        std::string iconPath;
        int childCount = 0;
    };

    /// 子布局条目描述（布局 JSON 阶段由文件直接映射）
    struct FolderItemDescriptor
    {
        WidgetType type = WidgetType::Empty;
        std::string id;   // GameCover: gameId；Folder: folderId
        std::string path; // Image: 图片路径
        int x = 0;
        int y = 0;
        int w = 1;
        int h = 1;
        bool focusable = true;
        float speedMul = 1.f; // Image(GIF) 播放速度倍率
    };

    /// 文件夹数据提供者抽象（子布局来源可替换）
    class FolderDataProvider
    {
    public:
        virtual ~FolderDataProvider() = default;
        virtual std::optional<FolderInfo> getFolder(const std::string& id) const = 0;
        virtual std::vector<FolderItemDescriptor> getFolderItems(
            const std::string& id) const = 0;
    };

    /// 默认实现：folder id = "platform:<int>"，子项为该平台全部游戏（GameDB）
    class GameDbFolderProvider : public FolderDataProvider
    {
    public:
        std::optional<FolderInfo> getFolder(const std::string& id) const override;
        std::vector<FolderItemDescriptor> getFolderItems(
            const std::string& id) const override;
    };
} // namespace beiklive
