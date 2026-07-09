#pragma once

#include "core/common.h"

#include <string>
#include <vector>

namespace beiklive::mgba_native::cheats
{
    struct LoadResult
    {
        std::vector<beiklive::CheatEntry> entries;
        bool editable = true;
        bool loaded = false;
    };

    bool IsMgbaPlatform(int platform);
    std::string NormalizeCodeType(std::string value);
    std::string DetectCodeType(const std::string& code);
    LoadResult LoadCheats(const std::string& path);
    bool SaveChtFile(const std::string& path, const std::vector<beiklive::CheatEntry>& entries);
}
