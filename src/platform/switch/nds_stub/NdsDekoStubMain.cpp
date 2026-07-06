#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <switch.h>

#include "platform/switch/nds_stub/NdsDekoRuntime.hpp"
#include "platform/switch/nds_stub/NdsStubMelonPlatform.hpp"

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

std::optional<nlohmann::json> loadNdsGameDbRecord(const std::string& romPath)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_NDS.json",
        "/GBAStation/data/GameData_NDS.json",
    };

    for (const char* dbPath : paths)
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko try GameDB path=%s", dbPath);
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
                const std::string itemPath = normalizePathForCompare(jsonString(item, "path"));
                if (itemPath == normalizedRom)
                {
                    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko GameDB match path=%s", itemPath.c_str());
                    return item;
                }
            }
        }
        catch (const std::exception& e)
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko GameDB exception %s", e.what());
        }
    }

    return std::nullopt;
}

std::string titleFromPath(const std::string& romPath)
{
    std::string title = std::filesystem::path(romPath).stem().string();
    return title.empty() ? "NDS Game" : title;
}

} // namespace

int main(int argc, char* argv[])
{
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko-only start argc=%d", argc);
    for (int i = 0; i < argc; ++i)
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");

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
            break;
        }
    }

    beiklive::nds_stub::DekoRunOptions options;
    options.romPath = romPath ? romPath : "";
    options.returnNroPath = returnNro && returnNro[0] ? returnNro : "sdmc:/switch/GBAStation.nro";
    options.title = titleFromPath(options.romPath);

    if (!options.romPath.empty())
    {
        std::optional<nlohmann::json> record = loadNdsGameDbRecord(options.romPath);
        if (record.has_value())
        {
            options.title = jsonString(*record, "title");
            options.savePath = jsonString(*record, "savePath");
            if (options.title.empty())
                options.title = titleFromPath(options.romPath);
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko gameDb.found=1 title=%s savePath=%s",
                                             options.title.c_str(),
                                             options.savePath.c_str());
        }
        else
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko gameDb.found=0");
        }
    }

    return beiklive::nds_stub::RunDekoRuntime(options);
}
