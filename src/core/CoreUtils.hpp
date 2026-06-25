#pragma once

#include <string>
#include <vector>
#include "game/retro/LibretroLoader.hpp"
#include "core/enums.h"

namespace beiklive::core_utils {

bool loadSram(LibretroLoader& core, const std::string& savePath, const std::string& romName);

bool saveSram(LibretroLoader& core, const std::string& savePath, const std::string& romName);

bool loadCheats(LibretroLoader& core, const std::string& cheatPath, std::vector<CheatEntry>& out);

void updateCheats(LibretroLoader& core, const std::vector<CheatEntry>& cheats);

} // namespace beiklive::core_utils
