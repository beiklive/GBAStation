#include "mgba_stub/MgbaStubRuntime.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <switch.h>

namespace {

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool endsWithNoCase(const std::string& value, const char* suffix)
{
    const std::string s(suffix);
    if (value.size() < s.size())
        return false;
    return lower(value.substr(value.size() - s.size())) == lower(s);
}

std::string normalizePath(std::string path)
{
    for (char& c : path)
    {
        if (c == '\\')
            c = '/';
    }
    if (path.rfind("sdmc:", 0) == 0)
        path.erase(0, 5);
    return path;
}

int platformFromPath(const std::string& path)
{
    const std::string p = lower(path);
    if (endsWithNoCase(p, ".gbc"))
        return 2;
    if (endsWithNoCase(p, ".gb"))
        return 3;
    return 1;
}

std::string titleFromPath(const std::string& path)
{
    std::string title = std::filesystem::path(path).stem().string();
    return title.empty() ? "Game" : title;
}

int jsonInt(const nlohmann::json& item, const char* key, int fallback)
{
    const auto it = item.find(key);
    if (it == item.end())
        return fallback;
    if (it->is_number_integer())
        return it->get<int>();
    if (it->is_number_float())
        return static_cast<int>(it->get<float>());
    return fallback;
}

float jsonFloat(const nlohmann::json& item, const char* key, float fallback)
{
    const auto it = item.find(key);
    if (it == item.end())
        return fallback;
    if (it->is_number())
        return it->get<float>();
    return fallback;
}

bool jsonBool(const nlohmann::json& item, const char* key, bool fallback)
{
    const auto it = item.find(key);
    return it != item.end() && it->is_boolean() ? it->get<bool>() : fallback;
}

std::string jsonString(const nlohmann::json& item, const char* key)
{
    const auto it = item.find(key);
    return it != item.end() && it->is_string() ? it->get<std::string>() : std::string {};
}

std::optional<nlohmann::json> loadGameDbRecord(const std::string& romPath)
{
    static constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_GBA.json",
        "sdmc:/GBAStation/data/GameData_GBC.json",
        "sdmc:/GBAStation/data/GameData_GB.json",
        "/GBAStation/data/GameData_GBA.json",
        "/GBAStation/data/GameData_GBC.json",
        "/GBAStation/data/GameData_GB.json",
    };

    const std::string normalized = normalizePath(romPath);
    for (const char* path : paths)
    {
        try
        {
            std::ifstream in(path);
            if (!in)
                continue;
            nlohmann::json data;
            in >> data;
            if (!data.is_array())
                continue;
            for (const auto& item : data)
            {
                if (item.is_object() && normalizePath(item.value("path", std::string{})) == normalized)
                    return item;
            }
        }
        catch (...)
        {
        }
    }
    return std::nullopt;
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
            returnNro = argv[++i];
            continue;
        }
        if (!romPath[0] && !endsWithNoCase(argv[i], ".nro"))
            romPath = argv[i];
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
            if (!title.empty())
                options.title = title;
            options.savePath = jsonString(*record, "savePath");
            options.platform = jsonInt(*record, "platform", options.platform);
            options.hasDisplaySettings = true;
            options.displayMode = jsonInt(*record, "displayMode", options.displayMode);
            options.integerScaleMultiplier = std::clamp(jsonInt(*record, "integerAspectRatio", options.integerScaleMultiplier), 0, 8);
            options.customScale = jsonFloat(*record, "customScale", options.customScale);
            options.customOffsetX = jsonFloat(*record, "customOffsetX", options.customOffsetX);
            options.customOffsetY = jsonFloat(*record, "customOffsetY", options.customOffsetY);
            options.overlayEnabled = jsonBool(*record, "overlayEnabled", false);
            options.overlayPath = jsonString(*record, "overlayPath");
            options.shaderEnabled = jsonBool(*record, "shaderEnabled", false);
            options.shaderType = jsonString(*record, "MgbaShaderType");
            if (options.shaderType.empty())
                options.shaderType = jsonString(*record, "shaderType");
            if (options.shaderType.empty())
                options.shaderType = jsonString(*record, "NdsShaderType");
            options.shaderPath = jsonString(*record, "shaderPath");
            if (options.shaderPath.empty())
                options.shaderPath = jsonString(*record, "shaderParaPath");
        }
    }

    return beiklive::mgba_stub::RunRuntime(options);
}
