#pragma once

#include <optional>
#include <string>

namespace beiklive
{
    /// 游戏信息（GameCoverWidget 展示所需，数据来源可替换）
    struct GameInfo
    {
        std::string id;
        std::string title;
        std::string coverPath;
        int platform = 0;
        int playTime = 0;
        std::string lastPlayed;
    };

    /// 游戏数据提供者抽象（以后可接 SQLite/JSON/文件扫描）
    class GameDataProvider
    {
    public:
        virtual ~GameDataProvider() = default;
        virtual std::optional<GameInfo> getGame(const std::string& id) const = 0;
    };

    /// 默认实现：从项目 GameDB 查询（id = 游戏文件路径）
    class GameDbProvider : public GameDataProvider
    {
    public:
        std::optional<GameInfo> getGame(const std::string& id) const override;
    };
} // namespace beiklive
