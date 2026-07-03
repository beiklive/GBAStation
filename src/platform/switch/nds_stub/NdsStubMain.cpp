#include <cstdarg>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <switch.h>

namespace {

void appendLog(const char* format, ...)
{
    char line[1024] = {};

    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    constexpr const char* paths[] = {
        "sdmc:/GBAStation/log/GBAStationNDSStub.log",
        "/GBAStation/log/GBAStationNDSStub.log",
        "sdmc:/GBAStationNDSStub.log",
        "/GBAStationNDSStub.log",
    };

    for (const char* path : paths)
    {
        FILE* fp = std::fopen(path, "a");
        if (!fp)
            continue;

        std::fprintf(fp, "%s\n", line);
        std::fflush(fp);
        std::fclose(fp);
    }
}

std::string quoteArg(const std::string& value)
{
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char c : value)
    {
        if (c == '"' || c == '\\')
            out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

bool fileExists(const std::string& path)
{
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
        return false;
    std::fclose(fp);
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

std::string jsonString(const nlohmann::json& item, const char* key)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_string())
        return "";
    return it->get<std::string>();
}

int jsonInt(const nlohmann::json& item, const char* key, int fallback)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_number_integer())
        return fallback;
    return it->get<int>();
}

bool jsonBool(const nlohmann::json& item, const char* key, bool fallback)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_boolean())
        return fallback;
    return it->get<bool>();
}

std::optional<nlohmann::json> loadNdsGameDbRecord(const std::string& romPath)
{
    constexpr const char* dbPaths[] = {
        "/GBAStation/data/GameData_NDS.json",
        "sdmc:/GBAStation/data/GameData_NDS.json",
    };

    const std::string normalizedRomPath = normalizePathForCompare(romPath);

    for (const char* dbPath : dbPaths)
    {
        appendLog("GBAStationNDSStub: try GameDB path=%s", dbPath);
        if (!fileExists(dbPath))
        {
            appendLog("GBAStationNDSStub: GameDB file missing path=%s", dbPath);
            continue;
        }

        try
        {
            std::ifstream file(dbPath, std::ios::binary);
            if (!file.is_open())
            {
                appendLog("GBAStationNDSStub: GameDB open failed path=%s", dbPath);
                continue;
            }

            nlohmann::json data;
            file >> data;
            if (!data.is_array())
            {
                appendLog("GBAStationNDSStub: GameDB root is not array path=%s", dbPath);
                continue;
            }

            appendLog("GBAStationNDSStub: GameDB loaded path=%s count=%zu", dbPath, data.size());
            for (const auto& item : data)
            {
                if (!item.is_object())
                    continue;

                const std::string itemPath = jsonString(item, "path");
                if (itemPath == romPath || normalizePathForCompare(itemPath) == normalizedRomPath)
                {
                    appendLog("GBAStationNDSStub: GameDB match path=%s", itemPath.c_str());
                    return item;
                }
            }

            appendLog("GBAStationNDSStub: GameDB no match romPath=%s normalized=%s",
                romPath.c_str(), normalizedRomPath.c_str());
        }
        catch (const std::exception& e)
        {
            appendLog("GBAStationNDSStub: GameDB exception path=%s error=%s", dbPath, e.what());
        }
        catch (...)
        {
            appendLog("GBAStationNDSStub: GameDB unknown exception path=%s", dbPath);
        }
    }

    return std::nullopt;
}

void logGameDbRecord(const nlohmann::json& item)
{
    appendLog("GBAStationNDSStub: gameDb.found=1");
    appendLog("GBAStationNDSStub: gameDb.title=%s", jsonString(item, "title").c_str());
    appendLog("GBAStationNDSStub: gameDb.path=%s", jsonString(item, "path").c_str());
    appendLog("GBAStationNDSStub: gameDb.savePath=%s", jsonString(item, "savePath").c_str());
    appendLog("GBAStationNDSStub: gameDb.cheatPath=%s", jsonString(item, "cheatPath").c_str());
    appendLog("GBAStationNDSStub: gameDb.screenShotPath=%s", jsonString(item, "screenShotPath").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsInternalResolution=%d", jsonInt(item, "ndsInternalResolution", 1));
    appendLog("GBAStationNDSStub: gameDb.ndsScreenLayout=%s", jsonString(item, "ndsScreenLayout").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsScreenOrientation=%s", jsonString(item, "ndsScreenOrientation").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsIntegerScale=%d", jsonBool(item, "ndsIntegerScale", false) ? 1 : 0);
}

} // namespace

int main(int argc, char* argv[])
{
    appendLog("GBAStationNDSStub: start argc=%d", argc);
    for (int i = 0; i < argc; ++i)
        appendLog("GBAStationNDSStub: argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");

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

    appendLog("GBAStationNDSStub: romPath=%s", romPath && romPath[0] ? romPath : "(empty)");
    appendLog("GBAStationNDSStub: returnNro=%s", returnNro && returnNro[0] ? returnNro : "(empty)");

    if (romPath && romPath[0])
    {
        const auto record = loadNdsGameDbRecord(romPath);
        if (record.has_value())
            logGameDbRecord(*record);
        else
            appendLog("GBAStationNDSStub: gameDb.found=0");
    }

    if (returnNro && returnNro[0])
    {
        if (envHasNextLoad())
        {
            const std::string args = quoteArg(returnNro);
            const Result rc = envSetNextLoad(returnNro, args.c_str());
            appendLog("GBAStationNDSStub: envSetNextLoad return rc=0x%x", rc);
        }
        else
        {
            appendLog("GBAStationNDSStub: envHasNextLoad=false, cannot return automatically");
        }
    }

    appendLog("GBAStationNDSStub: exit");
    svcSleepThread(1000 * 1000 * 1000);
    return 0;
}
