#include <algorithm>
#include <cstdarg>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <sys/stat.h>

#include <nlohmann/json.hpp>
#include <switch.h>

#include "nds_stub/NdsDekoRuntime.hpp"
#include "nds_stub/StubLog.hpp"

namespace beiklive::nds_stub {

namespace {

constexpr const char* kStubLogPaths[] = {
    "sdmc:/GBAStation/log/NdsDekoStub.log",
    "/GBAStation/log/NdsDekoStub.log",
};

std::mutex g_stubLogMutex;
std::uint64_t g_stubLogSequence = 0;

FILE* openStubLog(const char* mode)
{
    mkdir("sdmc:/GBAStation", 0777);
    mkdir("sdmc:/GBAStation/log", 0777);
    mkdir("/GBAStation", 0777);
    mkdir("/GBAStation/log", 0777);
    for (const char* path : kStubLogPaths)
    {
        if (FILE* fp = std::fopen(path, mode))
            return fp;
    }
    return nullptr;
}

} // namespace

void initializeStubLog()
{
    std::lock_guard<std::mutex> lock(g_stubLogMutex);
    g_stubLogSequence = 0;
    if (FILE* fp = openStubLog("wb"))
    {
        std::fputs("000000 GBAStationNDSStub: log session begin\n", fp);
        std::fflush(fp);
        std::fclose(fp);
    }
}

void appendStubLog(const char* format, ...)
{
    if (!format)
        return;

    char line[1024] = {};
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(g_stubLogMutex);
    if (FILE* fp = openStubLog("ab"))
    {
        std::fprintf(fp, "%06llu %s\n",
                     static_cast<unsigned long long>(++g_stubLogSequence),
                     line);
        std::fflush(fp);
        std::fclose(fp);
    }
}

} // namespace beiklive::nds_stub

extern "C" void GBAStationNDSStubLogLine(const char* line)
{
    beiklive::nds_stub::appendStubLog("%s", line ? line : "(null)");
}

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

float jsonFloat(const nlohmann::json& item, const char* key, float fallback)
{
    if (!item.contains(key) || !item.at(key).is_number())
        return fallback;
    return item.at(key).get<float>();
}

int jsonInt(const nlohmann::json& item, const char* key, int fallback)
{
    if (!item.contains(key) || !item.at(key).is_number_integer())
        return fallback;
    return item.at(key).get<int>();
}

bool jsonBool(const nlohmann::json& item, const char* key, bool fallback)
{
    if (!item.contains(key) || !item.at(key).is_boolean())
        return fallback;
    return item.at(key).get<bool>();
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
    beiklive::nds_stub::initializeStubLog();
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: main entered");
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: svcSetThreadCoreMask begin");
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: svcSetThreadCoreMask ok");
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko-only start argc=%d", argc);
    for (int i = 0; i < argc; ++i)
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");

    const char* romPath = "";
    const char* returnNro = "";
    bool exitToHome = false;

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

        if (std::strcmp(argv[i], "--exit-to-home") == 0)
        {
            exitToHome = true;
            continue;
        }

        if (!romPath[0] && !endsWithNoCase(argv[i], ".nro"))
        {
            romPath = argv[i];
            continue;
        }
    }

    beiklive::nds_stub::DekoRunOptions options;
    options.romPath = romPath ? romPath : "";
    options.returnNroPath = returnNro && returnNro[0] ? returnNro : "sdmc:/switch/GBAStation.nro";
    options.returnToNroOnExit = !exitToHome;
    options.title = titleFromPath(options.romPath);
    beiklive::nds_stub::appendStubLog(
        "GBAStationNDSStub: exit target=%s returnNro=%s",
        options.returnToNroOnExit ? "nro" : "home",
        options.returnNroPath.c_str());

    if (!options.romPath.empty())
    {
        std::optional<nlohmann::json> record = loadNdsGameDbRecord(options.romPath);
        if (record.has_value())
        {
            options.title = jsonString(*record, "title");
            options.savePath = jsonString(*record, "savePath");
            options.screenLayout = jsonString(*record, "ndsScreenLayout");
            options.screenOrientation = jsonString(*record, "ndsScreenOrientation");
            options.integerScale = jsonBool(*record, "ndsIntegerScale", true);
            options.screenGap = std::clamp(jsonInt(*record, "ndsScreenGap", 0), -256, 256);
            options.overlayEnabled = jsonBool(*record, "overlayEnabled", false);
            options.overlayPath = jsonString(*record, "overlayPath");
            options.shaderEnabled = jsonBool(*record, "shaderEnabled", false);
            options.ndsShaderType = jsonString(*record, "NdsShaderType");
            if (options.ndsShaderType.empty())
                options.ndsShaderType = jsonString(*record, "shaderParaPath");
            options.customLayout.topScale = jsonFloat(*record, "ndsTopScale", 1.0f);
            options.customLayout.topOffsetX = jsonFloat(*record, "ndsTopOffsetX", 0.0f);
            options.customLayout.topOffsetY = jsonFloat(*record, "ndsTopOffsetY", 0.0f);
            options.customLayout.bottomScale = jsonFloat(*record, "ndsBottomScale", 1.0f);
            options.customLayout.bottomOffsetX = jsonFloat(*record, "ndsBottomOffsetX", 0.0f);
            options.customLayout.bottomOffsetY = jsonFloat(*record, "ndsBottomOffsetY", 0.0f);
            if (options.title.empty())
                options.title = titleFromPath(options.romPath);
            if (options.screenLayout.empty())
                options.screenLayout = "priority_top";
            if (options.screenOrientation.empty())
                options.screenOrientation = "0";
            if (options.ndsShaderType.empty())
                options.ndsShaderType = "RetroArch_dot";
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko gameDb.found=1 title=%s savePath=%s layout=%s orientation=%s integer=%d gap=%d overlay=%d overlayPath=%s shader=%d shaderType=%s customTop=%.2f/%.1f/%.1f customBottom=%.2f/%.1f/%.1f",
                                             options.title.c_str(),
                                             options.savePath.c_str(),
                                             options.screenLayout.c_str(),
                                             options.screenOrientation.c_str(),
                                             options.integerScale ? 1 : 0,
                                             options.screenGap,
                                             options.overlayEnabled ? 1 : 0,
                                             options.overlayPath.c_str(),
                                             options.shaderEnabled ? 1 : 0,
                                             options.ndsShaderType.c_str(),
                                             options.customLayout.topScale,
                                             options.customLayout.topOffsetX,
                                             options.customLayout.topOffsetY,
                                             options.customLayout.bottomScale,
                                             options.customLayout.bottomOffsetX,
                                             options.customLayout.bottomOffsetY);
        }
        else
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: Deko gameDb.found=0");
        }
    }

    return beiklive::nds_stub::RunDekoRuntime(options);
}
