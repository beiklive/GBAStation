#pragma once

#include "core/enums.h"

#include <cstdint>
#include <string>
#include <vector>

namespace beiklive::cheat {

struct LoadRequest
{
    std::string path;
    std::string romPath;
    int platform = 0;
};

struct LoadResult
{
    std::vector<beiklive::CheatEntry> entries;
    beiklive::CheatSourceFormat format = beiklive::CheatSourceFormat::Unknown;
    bool editable = true;
    std::string message;
};

LoadResult loadCheats(const LoadRequest& request);

std::vector<beiklive::CheatEntry> loadChtFile(const std::string& path);
std::vector<beiklive::CheatEntry> loadNdsUsrCheatDat(const std::string& datPath,
                                                     const std::string& romPath);

bool saveChtFile(const std::string& path,
                 const std::vector<beiklive::CheatEntry>& entries);

std::vector<beiklive::CheatEntry> filterRunnableCheats(
    const std::vector<beiklive::CheatEntry>& entries);

std::vector<std::string> extractHexTokens(const std::string& code);
bool parseU32Hex(const std::string& text, uint32_t& out);
std::string stateKey(const beiklive::CheatEntry& cheat);
std::string lowerExtension(const std::string& path);
bool isNdsUsrCheatDat(const std::string& path, int platform);

} // namespace beiklive::cheat
