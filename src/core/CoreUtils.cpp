#include "CoreUtils.hpp"
#include "core/Tools.hpp"
#include "core/cheat/CheatSystem.hpp"
#include "core/constexpr.h"
#include <filesystem>
#include <fstream>
#include <cstring>

namespace beiklive::core_utils {

namespace {

std::filesystem::path sramPath(const std::string& savePath,
                               const std::string& romName)
{
    if (savePath.empty() || romName.empty())
        return {};
    return std::filesystem::path(savePath) / (romName + ".sav");
}

}

bool loadSram(LibretroLoader& core, const std::string& savePath, const std::string& romName)
{
    size_t sz = core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0)
        return true;

    const auto path = sramPath(savePath, romName);
    if (path.empty())
        return true;

    if (!std::filesystem::exists(path))
        return true;

    std::ifstream f(path, std::ios::binary);
    if (!f)
        return true;

    std::vector<uint8_t> buf(sz, 0);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
    std::streamsize got = f.gcount();

    void* sramPtr = core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
    if (sramPtr)
        std::memcpy(sramPtr, buf.data(), static_cast<size_t>(got));

    return true;
}

bool saveSram(LibretroLoader& core, const std::string& savePath, const std::string& romName)
{
    size_t sz = core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0)
        return true;

    const void* sramPtr = core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
    if (!sramPtr)
        return true;

    const auto path = sramPath(savePath, romName);
    if (path.empty())
        return true;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
        return false;

    std::ofstream f(path, std::ios::binary);
    if (!f)
        return true;

    f.write(reinterpret_cast<const char*>(sramPtr), static_cast<std::streamsize>(sz));
    return !!f;
}

bool loadCheats(LibretroLoader& core, const std::string& cheatPath, std::vector<CheatEntry>& out)
{
    if (cheatPath.empty())
        return true;

    out = beiklive::cheat::loadChtFile(cheatPath);
    if (out.empty())
        return true;

    core.cheatReset();
    unsigned cheatIndex = 0;
    for (size_t i = 0; i < out.size(); ++i)
    {
        if (out[i].enabled && out[i].valid &&
            out[i].payloadType == beiklive::CheatPayloadType::LibretroRaw)
            core.cheatSet(cheatIndex++, true, out[i].code);
    }
    return true;
}

void updateCheats(LibretroLoader& core, const std::vector<CheatEntry>& cheats)
{
    core.cheatReset();
    unsigned cheatIndex = 0;
    for (size_t i = 0; i < cheats.size(); ++i)
    {
        if (cheats[i].enabled && cheats[i].valid &&
            cheats[i].payloadType == beiklive::CheatPayloadType::LibretroRaw)
            core.cheatSet(cheatIndex++, true, cheats[i].code);
    }
}

} // namespace beiklive::core_utils
