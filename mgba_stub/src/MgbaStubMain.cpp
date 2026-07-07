#include "mgba_stub/MgbaStubRuntime.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <switch.h>

namespace {

bool endsWithNoCase(const std::string& value, const char* suffix)
{
    const size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;

    const size_t offset = value.size() - suffixLen;
    for (size_t i = 0; i < suffixLen; ++i)
    {
        const char a = value[offset + i];
        const char b = suffix[i];
        if (std::tolower(static_cast<unsigned char>(a)) !=
            std::tolower(static_cast<unsigned char>(b)))
            return false;
    }
    return true;
}

std::string normalizePathForCompare(std::string path)
{
    for (char& c : path)
    {
        if (c == '\\')
            c = '/';
    }
    if (path.rfind("sdmc:", 0) == 0)
        path.erase(0, 5);
    while (path.size() > 1 && path[0] == '/' && path[1] == '/')
        path.erase(0, 1);
    return path;
}

bool fileExists(const std::string& path)
{
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

std::string jsonString(const nlohmann::json& item, const char* key)
{
    if (!item.contains(key) || !item.at(key).is_string())
        return {};
    return item.at(key).get<std::string>();
}

int jsonInt(const nlohmann::json& item, const char* key, int fallback)
{
    if (!item.contains(key) || !item.at(key).is_number_integer())
        return fallback;
    return item.at(key).get<int>();
}

std::optional<nlohmann::json> loadGameDbRecord(const std::string& romPath)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_GBA.json",
        "sdmc:/GBAStation/data/GameData_GBC.json",
        "sdmc:/GBAStation/data/GameData_GB.json",
        "/GBAStation/data/GameData_GBA.json",
        "/GBAStation/data/GameData_GBC.json",
        "/GBAStation/data/GameData_GB.json",
    };

    for (const char* dbPath : paths)
    {
        if (!fileExists(dbPath))
            continue;
        try
        {
            std::ifstream file(dbPath);
            nlohmann::json data;
            file >> data;
            if (!data.is_array())
                continue;
            for (const auto& item : data)
            {
                if (normalizePathForCompare(jsonString(item, "path")) == normalizedRom)
                    return item;
            }
        }
        catch (...)
        {
        }
    }
    return std::nullopt;
}

int platformFromPath(const std::string& path)
{
    if (endsWithNoCase(path, ".gba"))
        return 1;
    if (endsWithNoCase(path, ".gbc"))
        return 2;
    return 3;
}

std::string titleFromPath(const std::string& romPath)
{
    std::string title = std::filesystem::path(romPath).stem().string();
    return title.empty() ? "mGBA Game" : title;
}

} // namespace

int main(int argc, char* argv[])
{
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);

    const char* romPath = "";
    const char* returnNro = "";
    for (int i = 1; i < argc; ++i)
    {
        if (!argv[i])
            continue;
        if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc)
        {
            returnNro = argv[i + 1];
            ++i;
            continue;
        }
        if (!romPath[0] && !endsWithNoCase(argv[i], ".nro"))
        {
            romPath = argv[i];
            continue;
        }
    }

    beiklive::mgba_stub::RunOptions options;
    options.romPath = romPath ? romPath : "";
    options.returnNroPath = returnNro && returnNro[0] ? returnNro : "sdmc:/switch/GBAStation.nro";
    options.title = titleFromPath(options.romPath);
    options.platform = platformFromPath(options.romPath);

    if (!options.romPath.empty())
    {
        auto record = loadGameDbRecord(options.romPath);
        if (record)
        {
            const std::string title = jsonString(*record, "title");
            const std::string savePath = jsonString(*record, "savePath");
            if (!title.empty())
                options.title = title;
            options.savePath = savePath;
            options.platform = jsonInt(*record, "platform", options.platform);
        }
    }

    return beiklive::mgba_stub::RunRuntime(options);
}
