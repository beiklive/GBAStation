#pragma once

#include "core/enums.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace beiklive::steamgriddb
{
    enum class AssetType : int
    {
        Grids = 0,
        Heroes,
        Logos,
        Icons,
        Count,
    };

    struct SearchGame
    {
        std::int64_t id = 0;
        std::string name;
        bool verified = false;
    };

    struct Asset
    {
        std::int64_t id = 0;
        AssetType type = AssetType::Grids;
        std::string url;
        std::string thumbnailUrl;
        std::string localPath;
        int width = 0;
        int height = 0;
        std::string style;
        std::string mime;
        std::string language;
        bool nsfw = false;
        bool humor = false;
        float score = 0.f;
    };

    struct Filters
    {
        int width = 0;
        int height = 0;
        std::string style;
        std::string mime;
        std::string language;
        bool allowHumor = false;
    };

    template <typename T>
    struct Result
    {
        bool ok = false;
        bool networkError = false;
        std::string error;
        T value{};
    };

    using AssetGroups = std::array<std::vector<Asset>,
        static_cast<size_t>(AssetType::Count)>;

    std::string rootDirectory();
    std::string apiKeyPath();
    std::string cacheDirectory();
    bool hasApiKey();
    std::string loadApiKey();
    bool saveApiKey(const std::string& key, std::string* error = nullptr);
    Result<bool> validateApiKey(const std::string& key);
    bool clearCache(std::string* error = nullptr);

    Result<std::vector<SearchGame>> searchGames(
        const std::vector<std::string>& terms);
    Result<AssetGroups> fetchAllAssets(
        const std::vector<SearchGame>& games, int page = 0);
    std::vector<Asset> applyFilters(
        const std::vector<Asset>& source, const Filters& filters);

    bool ensureAssetCached(Asset& asset, bool thumbnail,
                           std::string* error = nullptr);
    bool saveAssetAsCover(const Asset& asset, const GameEntry& entry,
                          std::string& outputPath,
                          std::string* error = nullptr);

    const char* assetTypeName(AssetType type);
}
