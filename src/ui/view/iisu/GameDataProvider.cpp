#include "GameDataProvider.hpp"

#include "core/common.h"

namespace beiklive
{
    std::optional<GameInfo> GameDbProvider::getGame(const std::string& id) const
    {
        if (id.empty() || !beiklive::GameDB)
            return std::nullopt;

        auto entry = beiklive::GameDB->findByPath(id);
        if (!entry)
            return std::nullopt;

        GameInfo info;
        info.id = id;
        info.title = entry->title;
        info.coverPath = entry->logoPath;
        info.platform = entry->platform;
        info.playTime = entry->playTime;
        info.lastPlayed = entry->lastPlayed;
        return info;
    }
} // namespace beiklive
