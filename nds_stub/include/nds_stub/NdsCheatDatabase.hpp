#pragma once

#include <string>
#include <vector>

#include "nds_stub/NdsMenuLayer.hpp"

namespace beiklive::nds_stub {

struct NdsCheatLoadResult {
    std::vector<NdsCheatItem> items;
    std::string sourcePath;
    std::string gameName;
    int skippedInvalidCodes = 0;
    bool databaseFound = false;
    bool gameMatched = false;
};

NdsCheatLoadResult LoadUsrCheatDatForRom(const std::string& romPath);

} // namespace beiklive::nds_stub
