#include "mgba_stub/MgbaStubRuntime.hpp"

#include "mgba_stub/MgbaGameLayer.hpp"
#include "mgba_stub/MgbaMenuLayer.hpp"
#include "mgba_stub/MgbaShaderCatalog.hpp"
#include "mgba_stub/ui/UiComponents.hpp"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include <mgba-util/vfs.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/interface.h>
#include <mgba/core/rewind.h>
#include <mgba/core/serialize.h>
#include <mgba/gba/interface.h>
#include <mgba/internal/gba/audio.h>
#include <mgba/internal/gba/input.h>
#include <mgba/internal/gb/overrides.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace {

constexpr unsigned kMaxVideoWidth = 256;
constexpr unsigned kMaxVideoHeight = 224;

void appendMgbaStubLog(const char*, ...)
{
}

std::string trim(std::string value)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string lower(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

std::string upper(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

std::string pathStem(const std::string& path)
{
    std::string stem = std::filesystem::path(path).stem().string();
    return stem.empty() ? "game" : stem;
}

std::string joinPath(const std::string& dir, const std::string& name)
{
    if (dir.empty())
        return name;
    return (std::filesystem::path(dir) / name).string();
}

bool fileExists(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return static_cast<bool>(file);
}

bool writeJsonFileChecked(const std::string& path, const nlohmann::json& data)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return false;
    out << data.dump(4) << '\n';
    out.close();
    return out.good();
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

std::string platformName(int platform)
{
    switch (platform)
    {
    case 2: return "GBC";
    case 3: return "GB";
    case 1:
    default:
        return "GBA";
    }
}

std::string platformSuffix(int platform)
{
    switch (platform)
    {
    case 2: return "gbc";
    case 3: return "gb";
    case 1:
    default:
        return "gba";
    }
}

std::string defaultSaveDir(const beiklive::mgba_stub::RunOptions& options)
{
    return joinPath(joinPath("sdmc:/GBAStation/saves/GBA", platformName(options.platform)),
                    pathStem(options.romPath));
}

std::string resolveSaveDir(const beiklive::mgba_stub::RunOptions& options)
{
    std::string dir = options.savePath.empty() ? defaultSaveDir(options) : options.savePath;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::string saveFilePath(const std::string& saveDir, const std::string& romPath)
{
    return joinPath(saveDir, pathStem(romPath) + ".sav");
}

std::string stateDir(const std::string& saveDir)
{
    const std::string dir = joinPath(saveDir, "state");
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::string statePath(const std::string& dir, const std::string& romPath, int slot)
{
    return joinPath(dir, pathStem(romPath) + ".ss" + std::to_string(std::clamp(slot, 0, 9)));
}

std::string configValuePayload(std::string value)
{
    if (value.size() >= 2 && value[1] == '|')
        value.erase(0, 2);
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\\' && i + 1 < value.size())
        {
            out.push_back(value[i + 1]);
            ++i;
        }
        else
        {
            out.push_back(value[i]);
        }
    }
    return trim(out);
}

constexpr const char* kConfigPaths[] = {
    "sdmc:/GBAStation/config/config.cfg",
    "/GBAStation/config/config.cfg",
};

std::map<std::string, std::string> loadConfigValues()
{
    std::map<std::string, std::string> values;
    for (const char* path : kConfigPaths)
    {
        std::ifstream file(path);
        if (!file)
            continue;
        std::string line;
        while (std::getline(file, line))
        {
            const size_t eq = line.find('=');
            if (eq == std::string::npos)
                continue;
            const std::string key = trim(line.substr(0, eq));
            if (!key.empty())
                values[key] = configValuePayload(line.substr(eq + 1));
        }
        break;
    }
    return values;
}

std::string configString(const std::map<std::string, std::string>& values,
                         const std::string& key,
                         const std::string& fallback)
{
    const auto it = values.find(key);
    return it == values.end() || it->second.empty() ? fallback : it->second;
}

int configInt(const std::map<std::string, std::string>& values,
              const std::string& key,
              int fallback)
{
    const auto text = configString(values, key, {});
    if (text.empty())
        return fallback;
    try { return std::stoi(text); }
    catch (...) { return fallback; }
}

int jsonIntValue(const nlohmann::json& item, const char* key, int fallback)
{
    const auto it = item.find(key);
    if (it == item.end())
        return fallback;
    if (it->is_number_integer())
        return it->get<int>();
    if (it->is_number_float())
        return static_cast<int>(it->get<float>());
    if (it->is_string())
    {
        try { return std::stoi(it->get<std::string>()); }
        catch (...) { return fallback; }
    }
    return fallback;
}

float configFloat(const std::map<std::string, std::string>& values,
                  const std::string& key,
                  float fallback)
{
    const auto text = configString(values, key, {});
    if (text.empty())
        return fallback;
    try { return std::stof(text); }
    catch (...) { return fallback; }
}

bool configBoolLike(const std::string& value, bool fallback)
{
    const std::string text = lower(trim(value));
    if (text == "1" || text == "true" || text == "yes" || text == "on" || text == "enabled")
        return true;
    if (text == "0" || text == "false" || text == "no" || text == "off" || text == "disabled")
        return false;
    return fallback;
}

bool configBool(const std::map<std::string, std::string>& values,
                const std::string& key,
                bool fallback)
{
    return configBoolLike(configString(values, key, {}), fallback);
}

std::vector<std::string> split(const std::string& value, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream stream(value);
    std::string item;
    while (std::getline(stream, item, delimiter))
    {
        item = trim(item);
        if (!item.empty())
            result.push_back(item);
    }
    return result;
}

bool writeConfigValue(const std::string& key, const std::string& typedValue)
{
    std::string path;
    for (const char* candidate : kConfigPaths)
    {
        if (fileExists(candidate))
        {
            path = candidate;
            break;
        }
    }
    if (path.empty())
        path = kConfigPaths[0];

    std::vector<std::string> lines;
    {
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line))
            lines.push_back(line);
    }

    bool replaced = false;
    for (std::string& line : lines)
    {
        const size_t eq = line.find('=');
        if (eq != std::string::npos && trim(line.substr(0, eq)) == key)
        {
            line = key + "=" + typedValue;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        lines.push_back(key + "=" + typedValue);

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return false;
    for (const std::string& line : lines)
        out << line << '\n';
    return true;
}

std::string overlayConfigKey(int platform)
{
    return "display.overlay." + platformSuffix(platform) + "Path";
}

std::string shaderConfigKey(int platform)
{
    return "display.shader." + platformSuffix(platform);
}

int uiLayoutFromConfigMode(const std::string& mode)
{
    if (mode == "fit") return 0;
    if (mode == "fill") return 1;
    if (mode == "original") return 2;
    if (mode == "four_three" || mode == "4:3") return 3;
    if (mode == "integer") return 4;
    if (mode == "custom") return 5;
    return 4;
}

int uiLayoutFromScreenMode(int mode)
{
    switch (mode)
    {
    case 0: return 0;
    case 1: return 1;
    case 2: return 4;
    case 3: return 5;
    case 4: return 3;
    default:
        return 0;
    }
}

int screenModeFromUiLayout(int layout)
{
    switch (layout)
    {
    case 1: return 1;
    case 3: return 4;
    case 4: return 2;
    case 5: return 3;
    case 7: return 3;
    default: return 0;
    }
}

uint32_t makeRGBA8888(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint32_t>(r) |
           (static_cast<uint32_t>(g) << 8) |
           (static_cast<uint32_t>(b) << 16) |
           0xFF000000u;
}

uint32_t nativeColorToRgba(color_t px)
{
#if defined(COLOR_16_BIT) && defined(COLOR_5_6_5)
    const uint8_t r5 = static_cast<uint8_t>((px >> 11) & 0x1F);
    const uint8_t g6 = static_cast<uint8_t>((px >> 5) & 0x3F);
    const uint8_t b5 = static_cast<uint8_t>(px & 0x1F);
    return makeRGBA8888(static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
                        static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
                        static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
#elif defined(COLOR_16_BIT)
    const uint8_t r5 = static_cast<uint8_t>(px & 0x1F);
    const uint8_t g5 = static_cast<uint8_t>((px >> 5) & 0x1F);
    const uint8_t b5 = static_cast<uint8_t>((px >> 10) & 0x1F);
    return makeRGBA8888(static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
                        static_cast<uint8_t>((g5 << 3) | (g5 >> 2)),
                        static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
#else
    return static_cast<uint32_t>(px) | 0xFF000000u;
#endif
}

std::string formatFileTime(const std::string& path)
{
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec)
        return {};
    const auto sysTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t tt = std::chrono::system_clock::to_time_t(sysTime);
    std::tm* tm = std::localtime(&tt);
    if (!tm)
        return {};
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm);
    return buffer;
}

std::array<beiklive::mgba_stub::MgbaStateSlotInfo, 10>
loadStateSlots(const std::string& dir, const std::string& romPath)
{
    std::array<beiklive::mgba_stub::MgbaStateSlotInfo, 10> slots {};
    for (int slot = 0; slot < static_cast<int>(slots.size()); ++slot)
    {
        auto& info = slots[slot];
        info.statePath = statePath(dir, romPath, slot);
        info.stateFileAvailable = std::filesystem::exists(info.statePath);
        info.exists = info.stateFileAvailable;
        info.loadable = info.stateFileAvailable;
        if (info.stateFileAvailable)
            info.modifiedTime = formatFileTime(info.statePath);
    }
    return slots;
}

std::string timestampString()
{
    const std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buffer[32] = {};
    if (tm)
        std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", tm);
    return buffer[0] ? buffer : "unknown_time";
}

std::string currentLastPlayedTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&tt);
    char buffer[64] = {};
    if (tm)
        std::strftime(buffer, sizeof(buffer), "%y-%m-%d %H-%M-%S", tm);
    return buffer;
}

constexpr const char* kGameDataPaths[] = {
    "sdmc:/GBAStation/data/GameData_GBA.json",
    "sdmc:/GBAStation/data/GameData_GBC.json",
    "sdmc:/GBAStation/data/GameData_GB.json",
    "/GBAStation/data/GameData_GBA.json",
    "/GBAStation/data/GameData_GBC.json",
    "/GBAStation/data/GameData_GB.json",
};

std::string gameDataPathForPlatform(int platform)
{
    const char* primary = "sdmc:/GBAStation/data/GameData_GBA.json";
    const char* fallback = "/GBAStation/data/GameData_GBA.json";
    if (platform == 2)
    {
        primary = "sdmc:/GBAStation/data/GameData_GBC.json";
        fallback = "/GBAStation/data/GameData_GBC.json";
    }
    else if (platform == 3)
    {
        primary = "sdmc:/GBAStation/data/GameData_GB.json";
        fallback = "/GBAStation/data/GameData_GB.json";
    }
    return fileExists(primary) || !fileExists(fallback) ? primary : fallback;
}

std::optional<std::string> findGameDataPathForRom(const std::string& romPath)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    for (const char* dbPath : kGameDataPaths)
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
                if (item.is_object() &&
                    normalizePathForCompare(item.value("path", std::string{})) == normalizedRom)
                    return std::string(dbPath);
            }
        }
        catch (...) {}
    }
    return std::nullopt;
}

template <typename Updater>
bool updateGameDataRecord(const beiklive::mgba_stub::RunOptions& options, Updater&& updater)
{
    const std::string dbPath = findGameDataPathForRom(options.romPath).value_or(
        gameDataPathForPlatform(options.platform));
    try
    {
        nlohmann::json data = nlohmann::json::array();
        {
            std::ifstream in(dbPath);
            if (in)
                in >> data;
        }
        if (!data.is_array())
            data = nlohmann::json::array();

        const std::string normalizedRom = normalizePathForCompare(options.romPath);
        nlohmann::json* target = nullptr;
        for (auto& item : data)
        {
            if (item.is_object() &&
                normalizePathForCompare(item.value("path", std::string{})) == normalizedRom)
            {
                target = &item;
                break;
            }
        }
        if (!target)
        {
            nlohmann::json item;
            item["path"] = options.romPath;
            item["title"] = options.title.empty() ? pathStem(options.romPath) : options.title;
            item["platform"] = options.platform;
            item["savePath"] = options.savePath;
            item["playCount"] = 0;
            item["playTime"] = 0;
            data.push_back(std::move(item));
            target = &data.back();
        }

        updater(*target);
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(dbPath).parent_path(), ec);
        return writeJsonFileChecked(dbPath, data);
    }
    catch (...)
    {
        return false;
    }
}

struct PlayStats {
    int playCount = 0;
    int playTime = 0;
};

PlayStats loadAndIncrementPlayCount(const beiklive::mgba_stub::RunOptions& options)
{
    PlayStats stats {};
    updateGameDataRecord(options, [&](nlohmann::json& target) {
        stats.playCount = target.value("playCount", 0);
        stats.playTime = target.value("playTime", 0);
        ++stats.playCount;
        target["playCount"] = stats.playCount;
        target["playTime"] = stats.playTime;
        target["lastPlayed"] = currentLastPlayedTimestamp();
    });
    return stats;
}

void saveDisplaySettingsToGameData(const beiklive::mgba_stub::RunOptions& options,
                                   const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    updateGameDataRecord(options, [&](nlohmann::json& target) {
        const std::string shaderType = beiklive::mgba_stub::normalizeMgbaShaderType(display.mgbaShaderType);
        target["displayMode"] = screenModeFromUiLayout(display.layout);
        target["integerAspectRatio"] = display.layout == 4 ? display.integerScaleMultiplier : 0;
        target["customScale"] = display.customLayout.topScale;
        target["customOffsetX"] = display.customLayout.topOffsetX;
        target["customOffsetY"] = display.customLayout.topOffsetY;
        target["overlayEnabled"] = display.overlayEnabled;
        target["overlayPath"] = display.overlayPath;
        target["shaderEnabled"] = display.shaderEnabled;
        target["MgbaShaderType"] = shaderType;
    });
}

void savePlayStats(const beiklive::mgba_stub::RunOptions& options,
                   int playCount,
                   int playTime)
{
    updateGameDataRecord(options, [&](nlohmann::json& target) {
        target["playCount"] = std::max(0, playCount);
        target["playTime"] = std::max(0, playTime);
        target["lastPlayed"] = currentLastPlayedTimestamp();
    });
}

int syncDisplaySettingsToGameData(const std::string& romPath,
                                  const beiklive::mgba_stub::MgbaDisplaySettings& display,
                                  int platform,
                                  int mode)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    const std::string dbPath = findGameDataPathForRom(romPath).value_or(
        gameDataPathForPlatform(platform));
    int count = 0;
    if (!fileExists(dbPath))
        return 0;
    try
    {
        std::ifstream in(dbPath);
        nlohmann::json data;
        in >> data;
        in.close();
        if (!data.is_array())
            return 0;

        const std::string shaderType = beiklive::mgba_stub::normalizeMgbaShaderType(display.mgbaShaderType);

        bool changed = false;
        for (auto& item : data)
        {
            if (!item.is_object())
                continue;
            const std::string itemPath = normalizePathForCompare(item.value("path", std::string{}));
            if (itemPath.empty() || itemPath == normalizedRom)
                continue;
            if (jsonIntValue(item, "platform", platform) != platform)
                continue;
            if (mode == 0)
            {
                item["displayMode"] = screenModeFromUiLayout(display.layout);
                item["integerAspectRatio"] = display.layout == 4 ? display.integerScaleMultiplier : 0;
                item["customScale"] = display.customLayout.topScale;
                item["customOffsetX"] = display.customLayout.topOffsetX;
                item["customOffsetY"] = display.customLayout.topOffsetY;
            }
            else if (mode == 1)
            {
                item["overlayEnabled"] = display.overlayEnabled;
                item["overlayPath"] = display.overlayPath;
            }
            else
            {
                item["shaderEnabled"] = display.shaderEnabled;
                item["MgbaShaderType"] = shaderType;
            }
            changed = true;
            ++count;
        }
        if (changed)
        {
            if (!writeJsonFileChecked(dbPath, data))
                return -1;
        }
    }
    catch (...) { return -1; }
    return count;
}

void saveRuntimeDisplaySettings(const beiklive::mgba_stub::RunOptions& options,
                                const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    writeConfigValue("fastforward.multiplier", "f|" + std::to_string(display.fastForwardMultiplier));
    writeConfigValue("display.filter", std::string("s|") + (display.linearFiltering ? "linear" : "nearest"));
    writeConfigValue("display.overlay.enabled", std::string("i|") + (display.overlayEnabled ? "1" : "0"));
    writeConfigValue(overlayConfigKey(options.platform), "s|" + display.overlayPath);
    writeConfigValue("display.shader.enabled", std::string("i|") + (display.shaderEnabled ? "1" : "0"));
    writeConfigValue("display.shader.mgbaType", "s|" + display.mgbaShaderType);
    saveDisplaySettingsToGameData(options, display);
}

std::vector<beiklive::mgba_stub::MgbaShaderParam> defaultShaderParams(const std::string& type)
{
    using beiklive::mgba_stub::MgbaShaderParam;
    const std::string shader = beiklive::mgba_stub::normalizeMgbaShaderType(type);
    if (shader == "RetroArch_dot")
    {
        return {
            {"gamma", "Gamma", 2.4f, 2.4f, 0.5f, 6.0f, 0.1f, 1},
            {"shine", "Shine", 0.05f, 0.05f, 0.0f, 0.5f, 0.01f, 2},
            {"blend", "Blend", 0.65f, 0.65f, 0.0f, 1.0f, 0.05f, 2},
        };
    }
    if (shader == "RetroArch_dot-clear")
    {
        return {
            {"screen_gamma", "Screen Gamma", 2.2f, 2.2f, 0.5f, 4.0f, 0.1f, 1},
            {"dot_gamma", "Dot Gamma", 2.2f, 2.2f, 0.5f, 4.0f, 0.1f, 1},
            {"dot_scale_x", "Dot Scale X", 1.1f, 1.1f, 0.5f, 3.0f, 0.1f, 1},
            {"dot_scale_y", "Dot Scale Y", 1.1f, 1.1f, 0.5f, 3.0f, 0.1f, 1},
            {"dot_opacity", "Dot Opacity", 0.7f, 0.7f, 0.0f, 1.0f, 0.05f, 2},
            {"halftone_strength", "Halftone", 0.7f, 0.7f, 0.0f, 1.0f, 0.05f, 2},
        };
    }
    if (shader == "RetroArch_lcd-grid-v2-nds-color")
    {
        return {
            {"gain", "Gain", 1.5f, 1.5f, 0.5f, 2.0f, 0.05f, 2},
            {"gamma", "LCD Gamma", 2.2f, 2.2f, 0.5f, 5.0f, 0.1f, 1},
            {"blacklevel", "Black Level", 0.0f, 0.0f, 0.0f, 0.5f, 0.01f, 2},
            {"ambient", "Ambient", 0.0f, 0.0f, 0.0f, 0.5f, 0.01f, 2},
            {"bgr", "BGR", 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0},
            {"nds_color", "NDS Color", 1.0f, 1.0f, 0.0f, 1.0f, 0.05f, 2},
        };
    }
    return {};
}

std::string shaderConfigPath(const std::string& type)
{
    return joinPath("sdmc:/GBAStation/config/mgbashaderconfig",
                    beiklive::mgba_stub::normalizeMgbaShaderType(type) + ".ini");
}

std::vector<beiklive::mgba_stub::MgbaShaderParam> loadShaderParams(const std::string& type)
{
    auto params = defaultShaderParams(type);
    std::ifstream in(shaderConfigPath(type));
    if (!in)
        return params;
    std::map<std::string, float> values;
    std::string line;
    while (std::getline(in, line))
    {
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        try { values[trim(line.substr(0, eq))] = std::stof(trim(line.substr(eq + 1))); }
        catch (...) {}
    }
    for (auto& param : params)
    {
        const auto it = values.find(param.name);
        if (it != values.end())
            param.value = std::clamp(it->second, param.minValue, param.maxValue);
    }
    return params;
}

void saveShaderParams(const std::string& type,
                      const std::vector<beiklive::mgba_stub::MgbaShaderParam>& params)
{
    const std::string path = shaderConfigPath(type);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return;
    out << "shader=" << beiklive::mgba_stub::normalizeMgbaShaderType(type) << "\n";
    for (const auto& param : params)
        out << param.name << '=' << param.value << "\n";
}

bool endsWithNoCase(const std::string& value, const char* suffix)
{
    const size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;
    const size_t offset = value.size() - suffixLen;
    for (size_t i = 0; i < suffixLen; ++i)
    {
        if (std::tolower(static_cast<unsigned char>(value[offset + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i])))
            return false;
    }
    return true;
}

std::vector<std::string> pathCandidates(const std::string& source)
{
    std::vector<std::string> paths;
    if (source.empty())
        return paths;
    paths.push_back(source);
    if (source.rfind("sdmc:", 0) == 0)
        paths.push_back(source.substr(5));
    else if (!source.empty() && source[0] == '/')
        paths.push_back("sdmc:" + source);
    return paths;
}

bool loadPngTextureFromFile(const std::string& path, std::uint32_t& texture, int& width, int& height)
{
    texture = 0;
    width = 0;
    height = 0;
    if (path.empty() || !endsWithNoCase(path, ".png"))
        return false;

    std::string openedPath;
    for (const std::string& candidate : pathCandidates(path))
    {
        FILE* fp = std::fopen(candidate.c_str(), "rb");
        if (!fp)
            continue;
        std::fclose(fp);
        openedPath = candidate;
        break;
    }
    if (openedPath.empty())
        return false;

    int comp = 0;
    unsigned char* pixels = stbi_load(openedPath.c_str(), &width, &height, &comp, 4);
    if (!pixels || width <= 0 || height <= 0 || width > 4096 || height > 4096)
    {
        if (pixels)
            stbi_image_free(pixels);
        width = 0;
        height = 0;
        return false;
    }

    std::vector<unsigned char> scaled;
    constexpr int kMaxUploadPixels = beiklive::mgba_stub::kScreenWidth *
                                     beiklive::mgba_stub::kScreenHeight;
    if (static_cast<long long>(width) * static_cast<long long>(height) > kMaxUploadPixels)
    {
        const float scale = std::sqrt(static_cast<float>(kMaxUploadPixels) /
                                      static_cast<float>(width * height));
        const int outW = std::max(1, static_cast<int>(std::floor(width * scale)));
        const int outH = std::max(1, static_cast<int>(std::floor(height * scale)));
        scaled.resize(static_cast<std::size_t>(outW) * outH * 4);
        for (int y = 0; y < outH; ++y)
        {
            const int sy = std::min(height - 1, static_cast<int>((static_cast<long long>(y) * height) / outH));
            for (int x = 0; x < outW; ++x)
            {
                const int sx = std::min(width - 1, static_cast<int>((static_cast<long long>(x) * width) / outW));
                const unsigned char* src = pixels + (static_cast<std::size_t>(sy) * width + sx) * 4;
                unsigned char* dst = scaled.data() + (static_cast<std::size_t>(y) * outW + x) * 4;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = src[3];
            }
        }
        width = outW;
        height = outH;
    }

    const unsigned char* uploadPixels = scaled.empty() ? pixels : scaled.data();
    const std::size_t uploadBytes = static_cast<std::size_t>(width) * height * 4;
    if (uploadBytes > 7 * 1024 * 1024)
    {
        stbi_image_free(pixels);
        width = 0;
        height = 0;
        return false;
    }

    texture = Gfx::TextureCreate(static_cast<u32>(width),
                                 static_cast<u32>(height),
                                 DkImageFormat_RGBA8_Unorm);
    if (texture != 0)
    {
        Gfx::TextureUpload(texture, 0, 0, width, height, const_cast<unsigned char*>(uploadPixels), width * 4);
    }
    stbi_image_free(pixels);
    return texture != 0;
}

std::string shaderTypeFromPath(const std::string& path)
{
    const std::string key = lower(std::filesystem::path(path).filename().string());
    if (key.find("dot-clear") != std::string::npos || key.find("dot_clear") != std::string::npos)
        return "RetroArch_dot-clear";
    if (key.find("lcd-grid") != std::string::npos || key.find("lcd_grid") != std::string::npos)
        return "RetroArch_lcd-grid-v2-nds-color";
    if (key.find("xbr") != std::string::npos || key.find("sabr") != std::string::npos ||
        key.find("hq") != std::string::npos || key.find("scale2x") != std::string::npos)
        return "RetroArch_xbrz-freescale";
    return "RetroArch_dot";
}

bool setReturnNro(const std::string& returnNro)
{
    if (returnNro.empty())
        return false;
    std::string quoted = "\"";
    for (char c : returnNro)
    {
        if (c == '"' || c == '\\')
            quoted.push_back('\\');
        quoted.push_back(c);
    }
    quoted.push_back('"');
    return R_SUCCEEDED(envSetNextLoad(returnNro.c_str(), quoted.c_str()));
}

// Input
struct InputCombo {
    uint64_t hid = 0;
    std::uint32_t virtualBits = 0;
    bool empty() const { return hid == 0 && virtualBits == 0; }
};

enum VirtualInputBit : std::uint32_t {
    VirtualLeftStickUp = 1u << 0,
    VirtualLeftStickDown = 1u << 1,
    VirtualLeftStickLeft = 1u << 2,
    VirtualLeftStickRight = 1u << 3,
    VirtualRightStickUp = 1u << 4,
    VirtualRightStickDown = 1u << 5,
    VirtualRightStickLeft = 1u << 6,
    VirtualRightStickRight = 1u << 7,
};

struct InputSnapshot {
    uint64_t held = 0;
    uint64_t down = 0;
    std::uint32_t virtualHeld = 0;
    std::uint32_t virtualDown = 0;
    HidAnalogStickState leftStick {};
    HidAnalogStickState rightStick {};
};

uint64_t hidFromToken(const std::string& token)
{
    const std::string t = upper(trim(token));
    if (t == "PAD_A") return HidNpadButton_A;
    if (t == "PAD_B") return HidNpadButton_B;
    if (t == "PAD_X") return HidNpadButton_X;
    if (t == "PAD_Y") return HidNpadButton_Y;
    if (t == "PAD_LB" || t == "LB") return HidNpadButton_L;
    if (t == "PAD_RB" || t == "RB") return HidNpadButton_R;
    if (t == "PAD_LT" || t == "LT") return HidNpadButton_ZL;
    if (t == "PAD_RT" || t == "RT") return HidNpadButton_ZR;
    if (t == "PAD_LSB" || t == "LSB") return HidNpadButton_StickL;
    if (t == "PAD_RSB" || t == "RSB") return HidNpadButton_StickR;
    if (t == "PAD_START" || t == "START") return HidNpadButton_Plus;
    if (t == "PAD_BACK" || t == "BACK" || t == "SELECT") return HidNpadButton_Minus;
    if (t == "PAD_UP") return HidNpadButton_Up;
    if (t == "PAD_DOWN") return HidNpadButton_Down;
    if (t == "PAD_LEFT") return HidNpadButton_Left;
    if (t == "PAD_RIGHT") return HidNpadButton_Right;
    return 0;
}

std::uint32_t virtualFromToken(const std::string& token)
{
    const std::string t = upper(trim(token));
    if (t == "PAD_LEFTSTICKUP") return VirtualLeftStickUp;
    if (t == "PAD_LEFTSTICKDOWN") return VirtualLeftStickDown;
    if (t == "PAD_LEFTSTICKLEFT") return VirtualLeftStickLeft;
    if (t == "PAD_LEFTSTICKRIGHT") return VirtualLeftStickRight;
    if (t == "PAD_RIGHTSTICKUP") return VirtualRightStickUp;
    if (t == "PAD_RIGHTSTICKDOWN") return VirtualRightStickDown;
    if (t == "PAD_RIGHTSTICKLEFT") return VirtualRightStickLeft;
    if (t == "PAD_RIGHTSTICKRIGHT") return VirtualRightStickRight;
    return 0;
}

InputCombo parseCombo(const std::string& comboText)
{
    InputCombo combo;
    for (const std::string& part : split(comboText, '+'))
    {
        const std::string token = upper(part);
        if (token == "NONE")
            return {};
        combo.hid |= hidFromToken(token);
        combo.virtualBits |= virtualFromToken(token);
    }
    return combo;
}

std::vector<InputCombo> parseCombos(const std::string& value)
{
    std::vector<InputCombo> combos;
    for (const std::string& comboText : split(value, '|'))
    {
        InputCombo combo = parseCombo(comboText);
        if (!combo.empty())
            combos.push_back(combo);
    }
    return combos;
}

bool comboHeld(const InputCombo& combo, const InputSnapshot& input)
{
    return (input.held & combo.hid) == combo.hid &&
           (input.virtualHeld & combo.virtualBits) == combo.virtualBits;
}

bool comboDown(const InputCombo& combo, const InputSnapshot& input)
{
    return comboHeld(combo, input) &&
           (((input.down & combo.hid) != 0) || ((input.virtualDown & combo.virtualBits) != 0));
}

bool anyComboHeld(const std::vector<InputCombo>& combos, const InputSnapshot& input)
{
    for (const InputCombo& combo : combos)
    {
        if (comboHeld(combo, input))
            return true;
    }
    return false;
}

bool anyComboDown(const std::vector<InputCombo>& combos, const InputSnapshot& input)
{
    for (const InputCombo& combo : combos)
    {
        if (comboDown(combo, input))
            return true;
    }
    return false;
}

InputSnapshot makeInputSnapshot(PadState& pad, std::uint32_t& previousVirtualHeld)
{
    InputSnapshot input;
    input.held = padGetButtons(&pad);
    input.down = padGetButtonsDown(&pad);
    input.leftStick = padGetStickPos(&pad, 0);
    input.rightStick = padGetStickPos(&pad, 1);
    constexpr int kStickThreshold = 12000;
    auto addStick = [&](const HidAnalogStickState& stick,
                        VirtualInputBit up,
                        VirtualInputBit down,
                        VirtualInputBit left,
                        VirtualInputBit right) {
        if (stick.y > kStickThreshold) input.virtualHeld |= up;
        if (stick.y < -kStickThreshold) input.virtualHeld |= down;
        if (stick.x < -kStickThreshold) input.virtualHeld |= left;
        if (stick.x > kStickThreshold) input.virtualHeld |= right;
    };
    addStick(input.leftStick, VirtualLeftStickUp, VirtualLeftStickDown,
             VirtualLeftStickLeft, VirtualLeftStickRight);
    addStick(input.rightStick, VirtualRightStickUp, VirtualRightStickDown,
             VirtualRightStickLeft, VirtualRightStickRight);
    input.virtualDown = input.virtualHeld & ~previousVirtualHeld;
    previousVirtualHeld = input.virtualHeld;
    return input;
}

std::string inputPrefixForPlatform(int platform)
{
    if (platform == 2)
        return "gbc.";
    if (platform == 3)
        return "gb.";
    return "";
}

class InputConfig {
public:
    explicit InputConfig(int platform)
        : m_prefix(inputPrefixForPlatform(platform))
    {
    }

    void load()
    {
        m_values = loadConfigValues();
        buildMappings();
    }

    bool menuDown(const InputSnapshot& input) const
    {
        return anyComboDown(button(prefixedKey("hotkey.menu.pad")), input);
    }

    bool comboHeldFor(const char* key, const InputSnapshot& input) const
    {
        return anyComboHeld(button(prefixedKey(key)), input);
    }

    bool comboDownFor(const char* key, const InputSnapshot& input) const
    {
        return anyComboDown(button(prefixedKey(key)), input);
    }

    bool fastForwardHeld(const InputSnapshot& input) const
    {
        return anyComboHeld(button(handleKey("fastforward")), input);
    }

    bool fastForwardDown(const InputSnapshot& input) const
    {
        return anyComboDown(button(handleKey("fastforward")), input);
    }

    bool fastForwardEnabled() const { return intValue("fastforward.enabled", 1) != 0; }
    bool fastForwardToggleMode() const { return lower(value("fastforward.mode", "hold")) == "toggle"; }
    bool fastForwardMute() const { return intValue("fastforward.mute", 0) != 0; }
    bool rewindEnabled() const { return intValue("rewind.enabled", 0) != 0; }
    bool rewindToggleMode() const { return lower(value("rewind.mode", "hold")) == "toggle"; }
    bool rewindMute() const { return intValue("rewind.mute", 0) != 0; }
    int rewindSaveInterval() const { return std::clamp(intValue("rewind.saveInterval", 1), 1, 120); }
    int rewindBufferSize() const { return std::clamp(intValue("rewind.bufferSize", 300), 10, 1800); }
    int rewindStep() const { return std::clamp(intValue("rewind.step", 1), 1, 10); }
    bool showFps() const { return intValue("display.showFps", 0) != 0; }
    bool showFfOverlay() const { return intValue("display.showFfOverlay", 1) != 0; }
    bool showRewindOverlay() const { return intValue("display.showRewindOverlay", 1) != 0; }
    int turboIntervalFrames() const
    {
        const float turboHz = std::clamp(floatValue("turbo.rate", 10.0f), 1.0f, 30.0f);
        return std::max(1, static_cast<int>(60.0f / (turboHz * 2.0f)));
    }

    uint32_t keyMask(const InputSnapshot& input, bool turboAOn, bool turboBOn) const
    {
        uint32_t keys = 0;
        for (const auto& binding : kButtonBindings)
        {
            if (anyComboHeld(button(handleKey(binding.suffix)), input))
                keys |= binding.mgbaBit;
        }
        if (turboAOn)
            keys |= 1u << GBA_KEY_A;
        if (turboBOn)
            keys |= 1u << GBA_KEY_B;
        return keys;
    }

private:
    struct ButtonBinding {
        const char* suffix;
        uint32_t mgbaBit;
        const char* fallback;
    };

    const std::vector<InputCombo>& button(const std::string& key) const
    {
        static const std::vector<InputCombo> empty;
        const auto it = m_comboValues.find(key);
        return it == m_comboValues.end() ? empty : it->second;
    }

    std::string value(const std::string& key, const std::string& fallback) const
    {
        const auto it = m_values.find(key);
        return it == m_values.end() || it->second.empty() ? fallback : it->second;
    }

    int intValue(const std::string& key, int fallback) const
    {
        try { return std::stoi(value(key, std::to_string(fallback))); }
        catch (...) { return fallback; }
    }

    float floatValue(const std::string& key, float fallback) const
    {
        try { return std::stof(value(key, std::to_string(fallback))); }
        catch (...) { return fallback; }
    }

    std::string handleKey(const char* suffix) const
    {
        return m_prefix + "handle." + suffix;
    }

    std::string prefixedKey(const char* key) const
    {
        const std::string text(key ? key : "");
        if (text.rfind("handle.", 0) == 0 || text.rfind("hotkey.", 0) == 0)
            return m_prefix + text;
        return text;
    }

    void buildMappings()
    {
        for (const auto& binding : kButtonBindings)
        {
            const std::string key = handleKey(binding.suffix);
            m_comboValues[key] = parseCombos(value(key, binding.fallback));
        }
        const std::string menuKey = prefixedKey("hotkey.menu.pad");
        m_comboValues[menuKey] = parseCombos(value(menuKey, value("hotkey.menu.pad", "PAD_LT+PAD_RT")));
        const std::pair<const char*, const char*> extraMappings[] = {
            {"handle.fastforward", "PAD_LSB"},
            {"handle.a_turbo", "none"},
            {"handle.b_turbo", "none"},
            {"handle.rewind", "none"},
            {"hotkey.quicksave.pad", "none"},
            {"hotkey.quickload.pad", "none"},
            {"hotkey.screenshot.pad", "none"},
            {"hotkey.mute.pad", "none"},
            {"hotkey.pause.pad", "none"},
        };
        for (const auto& mapping : extraMappings)
        {
            const std::string key = prefixedKey(mapping.first);
            m_comboValues[key] = parseCombos(value(key, value(mapping.first, mapping.second)));
        }
    }

    static constexpr ButtonBinding kButtonBindings[] = {
        {"a", 1u << GBA_KEY_A, "PAD_A"},
        {"b", 1u << GBA_KEY_B, "PAD_B"},
        {"select", 1u << GBA_KEY_SELECT, "PAD_BACK"},
        {"start", 1u << GBA_KEY_START, "PAD_START"},
        {"right", 1u << GBA_KEY_RIGHT, "PAD_RIGHT"},
        {"left", 1u << GBA_KEY_LEFT, "PAD_LEFT"},
        {"up", 1u << GBA_KEY_UP, "PAD_UP"},
        {"down", 1u << GBA_KEY_DOWN, "PAD_DOWN"},
        {"r", 1u << GBA_KEY_R, "PAD_RB"},
        {"l", 1u << GBA_KEY_L, "PAD_LB"},
    };

    std::string m_prefix;
    std::map<std::string, std::string> m_values;
    std::map<std::string, std::vector<InputCombo>> m_comboValues;
};

std::uint16_t readU16LE(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint32_t readU32LE(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

struct WavData {
    std::vector<std::int16_t> samples;
    int sampleRate = 44100;
    int channels = 2;
    bool loaded = false;
};

bool loadWavFile(const char* path, WavData& out)
{
    FILE* file = std::fopen(path, "rb");
    if (!file)
        return false;
    if (std::fseek(file, 0, SEEK_END) != 0)
    {
        std::fclose(file);
        return false;
    }
    const long fileSize = std::ftell(file);
    if (fileSize < 44)
    {
        std::fclose(file);
        return false;
    }
    std::rewind(file);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(fileSize));
    const std::size_t read = std::fread(bytes.data(), 1, bytes.size(), file);
    std::fclose(file);
    if (read != bytes.size() ||
        std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
        return false;

    std::uint16_t fmtTag = 0;
    std::uint16_t channels = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint32_t sampleRate = 0;
    const std::uint8_t* pcmStart = nullptr;
    std::size_t pcmBytes = 0;
    std::size_t pos = 12;
    while (pos + 8 <= bytes.size())
    {
        const std::uint32_t chunkSize = readU32LE(bytes.data() + pos + 4);
        if (pos + 8 + chunkSize > bytes.size())
            break;
        if (std::memcmp(bytes.data() + pos, "fmt ", 4) == 0 && chunkSize >= 16)
        {
            fmtTag = readU16LE(bytes.data() + pos + 8);
            channels = readU16LE(bytes.data() + pos + 10);
            sampleRate = readU32LE(bytes.data() + pos + 12);
            bitsPerSample = readU16LE(bytes.data() + pos + 22);
        }
        else if (std::memcmp(bytes.data() + pos, "data", 4) == 0)
        {
            pcmStart = bytes.data() + pos + 8;
            pcmBytes = chunkSize;
            break;
        }
        pos += 8 + chunkSize + (chunkSize & 1u);
    }
    if (fmtTag != 1 || bitsPerSample != 16 || channels == 0 ||
        sampleRate == 0 || !pcmStart || pcmBytes == 0)
        return false;
    out.sampleRate = static_cast<int>(sampleRate);
    out.channels = static_cast<int>(channels);
    const std::size_t sampleCount = pcmBytes / sizeof(std::int16_t);
    out.samples.resize(sampleCount);
    std::memcpy(out.samples.data(), pcmStart, sampleCount * sizeof(std::int16_t));
    if (channels == 1)
    {
        std::vector<std::int16_t> stereo(sampleCount * 2);
        for (std::size_t i = 0; i < sampleCount; ++i)
        {
            stereo[i * 2] = out.samples[i];
            stereo[i * 2 + 1] = out.samples[i];
        }
        out.samples = std::move(stereo);
        out.channels = 2;
    }
    out.loaded = true;
    return true;
}

const char* menuSoundFileName(beiklive::mgba_stub::MgbaMenuSound sound)
{
    using beiklive::mgba_stub::MgbaMenuSound;
    switch (sound)
    {
    case MgbaMenuSound::Focus: return "SeNaviFocus.wav";
    case MgbaMenuSound::Click: return "SeBtnDecide.wav";
    case MgbaMenuSound::Back: return "SeFooterDecideFinish.wav";
    case MgbaMenuSound::Error: return "SeKeyErrorCursor.wav";
    case MgbaMenuSound::Slider: return "SeSliderTickOver.wav";
    default: return "";
    }
}

std::size_t menuSoundIndex(beiklive::mgba_stub::MgbaMenuSound sound)
{
    return static_cast<std::size_t>(sound);
}

class SwitchAudio {
public:
    bool init(mCore* core, double fps, bool lowPassEnabled, int lowPassRangePercent)
    {
        if (!core)
            return false;
        m_core = core;
        m_fps = std::max(1.0, fps);
        Result rc = audoutInitialize();
        if (R_FAILED(rc))
            return false;
        m_initialized = true;
        rc = audoutStartAudioOut();
        if (R_FAILED(rc))
            return false;
        m_started = true;
        m_stream.owner = this;
        m_stream.stream.postAudioBuffer = &SwitchAudio::postAudioBufferThunk;
        for (std::size_t i = 0; i < m_outBuffers.size(); ++i)
        {
            m_outBuffers[i].next = nullptr;
            m_outBuffers[i].buffer = m_audioBuffers[i].data();
            m_outBuffers[i].buffer_size = kAudioBufferSize;
            m_outBuffers[i].data_size = kAudioSamples * sizeof(mStereoSample);
            m_outBuffers[i].data_offset = 0;
        }
        core->setAudioBufferSize(core, kAudioSamples);
        m_sampleRate = static_cast<int>(audoutGetSampleRate());
        setLowPass(lowPassEnabled, lowPassRangePercent);
        applyCoreAudioRates();
        core->setAVStream(core, &m_stream.stream);
        return true;
    }

    void setFrameLimiter(bool limit)
    {
        if (!m_frameLimiter && limit)
        {
            while (m_enqueuedBuffers > 3)
                audioWait(100000000);
        }
        m_frameLimiter = limit;
    }

    void setMuted(bool muted) { m_muted = muted; }

    void setSpeed(float speed)
    {
        const float next = std::clamp(speed, 0.1f, 10.0f);
        if (std::fabs(next - m_speed) < 0.001f)
            return;
        m_speed = next;
        applyCoreAudioRates();
    }

    void setLowPass(bool enabled, int rangePercent)
    {
        const int fixedRange = (std::clamp(rangePercent, 0, 100) * 0x10000) / 100;
        if (enabled != m_lowPassEnabled || fixedRange != m_lowPassRange)
        {
            m_lowPassLeftPrev = 0;
            m_lowPassRightPrev = 0;
        }
        m_lowPassEnabled = enabled;
        m_lowPassRange = fixedRange;
    }

    void playSound(beiklive::mgba_stub::MgbaMenuSound sound, float pitch = 1.0f)
    {
        const std::size_t index = menuSoundIndex(sound);
        if (index >= m_sounds.size())
            return;
        if (!m_sounds[index].loaded)
        {
            char path[128] = {};
            std::snprintf(path, sizeof(path), "romfs:/sounds/switch/%s", menuSoundFileName(sound));
            loadWavFile(path, m_sounds[index]);
        }
        if (!m_sounds[index].loaded || m_sounds[index].samples.empty())
            return;
        ActiveSound active;
        active.wav = &m_sounds[index];
        active.step = (static_cast<double>(m_sounds[index].sampleRate) *
                       std::max(0.1, static_cast<double>(pitch))) /
                      static_cast<double>(std::max(1, m_sampleRate));
        active.gain = sound == beiklive::mgba_stub::MgbaMenuSound::Slider ? 0.55f : 0.75f;
        if (m_activeSounds.size() >= kMaxActiveSounds)
            m_activeSounds.pop_front();
        m_activeSounds.push_back(active);
    }

    void pumpUiAudio()
    {
        if (!m_started || m_activeSounds.empty())
            return;
        audioWait(0);
        while (m_enqueuedBuffers >= kAudioBufferCount - 1)
            audioWait(10000000);

        auto* samples = reinterpret_cast<mStereoSample*>(m_audioBuffers[m_activeBuffer].data());
        std::memset(samples, 0, kAudioSamples * sizeof(mStereoSample));
        mixActiveSounds(samples, kAudioSamples);
        applyLowPass(samples, kAudioSamples);
        armDCacheFlush(samples, kAudioSamples * sizeof(mStereoSample));
        if (R_SUCCEEDED(audoutAppendAudioOutBuffer(&m_outBuffers[m_activeBuffer])))
        {
            m_activeBuffer = (m_activeBuffer + 1) % kAudioBufferCount;
            ++m_enqueuedBuffers;
        }
    }

    void deinit()
    {
        if (m_started)
        {
            audoutStopAudioOut();
            m_started = false;
        }
        if (m_initialized)
        {
            audoutExit();
            m_initialized = false;
        }
        m_enqueuedBuffers = 0;
    }

private:
    struct StreamWrapper {
        mAVStream stream {};
        SwitchAudio* owner = nullptr;
    };
    struct ActiveSound {
        const WavData* wav = nullptr;
        double position = 0.0;
        double step = 1.0;
        float gain = 0.75f;
    };

    static constexpr int kAudioSamples = 0x400;
    static constexpr int kAudioBufferCount = 6;
    static constexpr int kMaxActiveSounds = 6;
    static constexpr std::size_t kAudioBufferSize =
        (kAudioSamples * sizeof(mStereoSample)) < 0x1000 ? 0x1000 : (kAudioSamples * sizeof(mStereoSample));

    void applyCoreAudioRates()
    {
        if (!m_core || !m_core->getAudioChannel)
            return;
        const double ratio = GBAAudioCalculateRatio(1.0f, static_cast<float>(m_fps), 1.0f);
        const double effectiveRate = (static_cast<double>(m_sampleRate) * ratio) / std::max(0.1f, m_speed);
        if (blip_t* left = m_core->getAudioChannel(m_core, 0))
            blip_set_rates(left, m_core->frequency(m_core), effectiveRate);
        if (blip_t* right = m_core->getAudioChannel(m_core, 1))
            blip_set_rates(right, m_core->frequency(m_core), effectiveRate);
    }

    int audioWait(uint64_t timeout)
    {
        AudioOutBuffer* releasedBuffers = nullptr;
        uint32_t releasedCount = 0;
        const Result rc = timeout != 0
                              ? audoutWaitPlayFinish(&releasedBuffers, &releasedCount, timeout)
                              : audoutGetReleasedAudioOutBuffer(&releasedBuffers, &releasedCount);
        if (R_FAILED(rc))
            return 0;
        m_enqueuedBuffers = std::max(0, m_enqueuedBuffers - static_cast<int>(releasedCount));
        return static_cast<int>(releasedCount);
    }

    static void postAudioBufferThunk(mAVStream* stream, blip_t* left, blip_t* right)
    {
        auto* wrapper = reinterpret_cast<StreamWrapper*>(stream);
        if (wrapper && wrapper->owner)
            wrapper->owner->postAudioBuffer(left, right);
    }

    static std::int16_t clampI16(int value)
    {
        return static_cast<std::int16_t>(std::clamp(value, -32768, 32767));
    }

    void mixActiveSounds(mStereoSample* samples, int frames)
    {
        if (!samples || frames <= 0 || m_activeSounds.empty())
            return;
        for (auto it = m_activeSounds.begin(); it != m_activeSounds.end();)
        {
            const WavData* wav = it->wav;
            if (!wav || wav->samples.empty() || wav->channels <= 0)
            {
                it = m_activeSounds.erase(it);
                continue;
            }
            const std::size_t inFrames = wav->samples.size() / static_cast<std::size_t>(wav->channels);
            for (int frame = 0; frame < frames && it->position < static_cast<double>(inFrames); ++frame)
            {
                std::size_t srcFrame = static_cast<std::size_t>(it->position);
                srcFrame = std::min(srcFrame, inFrames - 1);
                const std::size_t src = srcFrame * static_cast<std::size_t>(wav->channels);
                const int left = static_cast<int>(static_cast<float>(wav->samples[src]) * it->gain);
                const int right = static_cast<int>(static_cast<float>(wav->samples[src + 1]) * it->gain);
                samples[frame].left = clampI16(static_cast<int>(samples[frame].left) + left);
                samples[frame].right = clampI16(static_cast<int>(samples[frame].right) + right);
                it->position += it->step;
            }
            if (it->position >= static_cast<double>(inFrames))
                it = m_activeSounds.erase(it);
            else
                ++it;
        }
    }

    void applyLowPass(mStereoSample* samples, int frames)
    {
        if (!m_lowPassEnabled || !samples || frames <= 0)
            return;
        int32_t left = m_lowPassLeftPrev;
        int32_t right = m_lowPassRightPrev;
        const int32_t factorA = m_lowPassRange;
        const int32_t factorB = 0x10000 - factorA;
        for (int frame = 0; frame < frames; ++frame)
        {
            left = (left * factorA) + (static_cast<int32_t>(samples[frame].left) * factorB);
            right = (right * factorA) + (static_cast<int32_t>(samples[frame].right) * factorB);
            left >>= 16;
            right >>= 16;
            samples[frame].left = static_cast<std::int16_t>(left);
            samples[frame].right = static_cast<std::int16_t>(right);
        }
        m_lowPassLeftPrev = left;
        m_lowPassRightPrev = right;
    }

    void postAudioBuffer(blip_t* left, blip_t* right)
    {
        if (!m_started || !left || !right)
            return;
        audioWait(0);
        while (m_enqueuedBuffers >= kAudioBufferCount - 1)
        {
            if (!m_frameLimiter)
            {
                blip_read_samples(left, &m_dropSamples[0].left, kAudioSamples, true);
                blip_read_samples(right, &m_dropSamples[0].right, kAudioSamples, true);
                return;
            }
            audioWait(10000000);
        }
        auto* samples = reinterpret_cast<mStereoSample*>(m_audioBuffers[m_activeBuffer].data());
        blip_read_samples(left, &samples[0].left, kAudioSamples, true);
        blip_read_samples(right, &samples[0].right, kAudioSamples, true);
        if (m_muted)
            std::memset(samples, 0, kAudioSamples * sizeof(mStereoSample));
        mixActiveSounds(samples, kAudioSamples);
        applyLowPass(samples, kAudioSamples);
        armDCacheFlush(samples, kAudioSamples * sizeof(mStereoSample));
        if (R_SUCCEEDED(audoutAppendAudioOutBuffer(&m_outBuffers[m_activeBuffer])))
        {
            m_activeBuffer = (m_activeBuffer + 1) % kAudioBufferCount;
            ++m_enqueuedBuffers;
        }
    }

    StreamWrapper m_stream {};
    std::array<AudioOutBuffer, kAudioBufferCount> m_outBuffers {};
    alignas(0x1000) std::array<std::array<std::uint8_t, kAudioBufferSize>, kAudioBufferCount> m_audioBuffers {};
    std::array<mStereoSample, kAudioSamples> m_dropSamples {};
    std::array<WavData, 5> m_sounds {};
    std::deque<ActiveSound> m_activeSounds;
    int m_activeBuffer = 0;
    int m_enqueuedBuffers = 0;
    int m_sampleRate = 48000;
    mCore* m_core = nullptr;
    double m_fps = 60.0;
    float m_speed = 1.0f;
    bool m_initialized = false;
    bool m_started = false;
    bool m_frameLimiter = true;
    bool m_muted = false;
    bool m_lowPassEnabled = false;
    int m_lowPassRange = (60 * 0x10000) / 100;
    int32_t m_lowPassLeftPrev = 0;
    int32_t m_lowPassRightPrev = 0;
};

class SwitchRumble {
public:
    bool init()
    {
        m_rumble.owner = this;
        m_rumble.rumble.setRumble = &SwitchRumble::setRumbleThunk;
        m_value.freq_low = 120.0f;
        m_value.freq_high = 180.0f;
        m_stop.freq_low = 160.0f;
        m_stop.freq_high = 320.0f;
        hidInitializeVibrationDevices(&m_handles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
        hidInitializeVibrationDevices(&m_handles[2], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);
        m_initialized = true;
        return true;
    }
    mRumble* peripheral() { return &m_rumble.rumble; }
    void flush()
    {
        if (!m_initialized)
            return;
        HidVibrationValue values[4] {};
        if (m_up > 0)
        {
            const float amp = static_cast<float>(m_up) / static_cast<float>(m_up + m_down);
            m_value.amp_low = amp;
            m_value.amp_high = amp;
            for (auto& value : values)
                value = m_value;
        }
        else
        {
            for (auto& value : values)
                value = m_stop;
        }
        hidSendVibrationValues(m_handles.data(), values, 4);
        m_up = 0;
        m_down = 0;
    }
    void stop()
    {
        if (!m_initialized)
            return;
        HidVibrationValue values[4] {m_stop, m_stop, m_stop, m_stop};
        hidSendVibrationValues(m_handles.data(), values, 4);
        m_initialized = false;
    }

private:
    struct RumbleWrapper {
        mRumble rumble {};
        SwitchRumble* owner = nullptr;
    };
    static void setRumbleThunk(mRumble* rumble, int enable)
    {
        auto* wrapper = reinterpret_cast<RumbleWrapper*>(rumble);
        if (!wrapper || !wrapper->owner)
            return;
        if (enable) ++wrapper->owner->m_up;
        else ++wrapper->owner->m_down;
    }
    RumbleWrapper m_rumble {};
    std::array<HidVibrationDeviceHandle, 4> m_handles {};
    HidVibrationValue m_value {};
    HidVibrationValue m_stop {};
    int m_up = 0;
    int m_down = 0;
    bool m_initialized = false;
};

class SwitchLuminance {
public:
    void configure(int level)
    {
        m_levelIndex = std::clamp(level, 0, 10);
        m_luxLevel = 0x16;
        if (m_levelIndex > 0)
            m_luxLevel = static_cast<std::uint8_t>(m_luxLevel + GBA_LUX_LEVELS[m_levelIndex - 1]);
    }
    GBALuminanceSource* peripheral()
    {
        m_source.owner = this;
        m_source.source.sample = &SwitchLuminance::sampleThunk;
        m_source.source.readLuminance = &SwitchLuminance::readThunk;
        return &m_source.source;
    }
private:
    struct SourceWrapper {
        GBALuminanceSource source {};
        SwitchLuminance* owner = nullptr;
    };
    static void sampleThunk(GBALuminanceSource*) {}
    static std::uint8_t readThunk(GBALuminanceSource* source)
    {
        auto* wrapper = reinterpret_cast<SourceWrapper*>(source);
        if (!wrapper || !wrapper->owner)
            return static_cast<std::uint8_t>(0xFF - 0x16);
        return static_cast<std::uint8_t>(0xFF - wrapper->owner->m_luxLevel);
    }
    SourceWrapper m_source {};
    int m_levelIndex = 5;
    std::uint8_t m_luxLevel = 0x16;
};

class SystemRtc {
public:
    mRTCSource* source()
    {
        m_source.owner = this;
        m_source.source.sample = &SystemRtc::sampleThunk;
        m_source.source.unixTime = &SystemRtc::unixTimeThunk;
        m_source.source.serialize = nullptr;
        m_source.source.deserialize = nullptr;
        return &m_source.source;
    }
private:
    struct SourceWrapper {
        mRTCSource source {};
        SystemRtc* owner = nullptr;
    };
    static void sampleThunk(mRTCSource*) {}
    static std::time_t unixTimeThunk(mRTCSource*) { return std::time(nullptr); }
    SourceWrapper m_source {};
};

const char* mgbaGbModelName(const std::string& value)
{
    const std::string text = lower(trim(value));
    if (text == "game boy" || text == "dmg") return "DMG";
    if (text == "super game boy" || text == "sgb") return "SGB";
    if (text == "game boy color" || text == "cgb") return "CGB";
    if (text == "game boy advance" || text == "agb") return "AGB";
    return nullptr;
}

const char* mgbaIdleOptimizationName(const std::string& value)
{
    const std::string text = lower(trim(value));
    if (text == "don't remove" || text == "dont remove" || text == "ignore") return "ignore";
    if (text == "detect and remove" || text == "detect") return "detect";
    return "remove";
}

void applyMgbaGbPalette(mCoreConfig* config, const std::map<std::string, std::string>& values)
{
    const std::string selected = configString(values, "core.mgba_gb_colors", {});
    if (selected.empty())
        return;
    const GBColorPreset* presets = nullptr;
    const size_t count = GBColorPresetList(&presets);
    for (size_t i = 0; i < count; ++i)
    {
        if (selected != presets[i].name)
            continue;
        for (size_t color = 0; color < 12; ++color)
        {
            const std::string key = "gb.pal[" + std::to_string(color) + "]";
            mCoreConfigSetDefaultUIntValue(config, key.c_str(), presets[i].colors[color] & 0xFFFFFFu);
        }
        return;
    }
}

void applyMgbaCoreSettings(mCoreConfig* config,
                           const std::map<std::string, std::string>& values,
                           int platform)
{
    if (!config)
        return;
    mCoreConfigSetDefaultIntValue(config, "useBios",
                                  configBoolLike(configString(values, "core.mgba_use_bios", "OFF"), false) ? 1 : 0);
    mCoreConfigSetDefaultIntValue(config, "skipBios",
                                  configBoolLike(configString(values, "core.mgba_skip_bios", "ON"), true) ? 1 : 0);
    mCoreConfigSetDefaultIntValue(config, "allowOpposingDirections",
                                  configBoolLike(configString(values, "core.mgba_allow_opposing_directions", "yes"), true) ? 1 : 0);
    mCoreConfigSetDefaultIntValue(config, "frameskip",
                                  std::clamp(configInt(values, "core.mgba_frameskip", 0), 0, 10));
    mCoreConfigSetDefaultValue(config, "idleOptimization",
                               mgbaIdleOptimizationName(configString(values, "core.mgba_idle_optimization", "Remove Known")));
    if (platform == 1)
    {
        mCoreConfigSetDefaultIntValue(config, "gba.forceGbp",
                                      configBoolLike(configString(values, "core.mgba_force_gbp", "OFF"), false) ? 1 : 0);
    }
    else
    {
        if (const char* model = mgbaGbModelName(configString(values, "core.mgba_gb_model", "Autodetect")))
        {
            mCoreConfigSetDefaultValue(config, "gb.model", model);
            mCoreConfigSetDefaultValue(config, "sgb.model", model);
            mCoreConfigSetDefaultValue(config, "cgb.model", model);
        }
        mCoreConfigSetDefaultIntValue(config, "sgb.borders",
                                      configBoolLike(configString(values, "core.mgba_sgb_borders", "OFF"), false) ? 1 : 0);
        mCoreConfigSetDefaultIntValue(config, "gb.colors",
                                      std::clamp(configInt(values, "core.mgba_gb_colors_preset", 0), 0, 31));
        applyMgbaGbPalette(config, values);
    }
}

class RuntimeCore {
public:
    ~RuntimeCore() { release(); }
    bool load(const beiklive::mgba_stub::RunOptions& options,
              const std::string& savePath,
              const std::map<std::string, std::string>& configValues)
    {
        release();
        const mPlatform platform = options.platform == 1 ? mPLATFORM_GBA : mPLATFORM_GB;
        m_core = mCoreCreate(platform);
        if (!m_core || !m_core->init(m_core))
        {
            release();
            return false;
        }
        m_coreInitialized = true;
        mCoreInitConfig(m_core, "GBAStationMgbaStub");
        m_configInitialized = true;
        mCoreConfigSetDefaultIntValue(&m_core->config, "useBios", 0);
        mCoreConfigSetDefaultIntValue(&m_core->config, "skipBios", 1);
        mCoreConfigSetDefaultIntValue(&m_core->config, "mute", 0);
        mCoreConfigSetDefaultIntValue(&m_core->config, "volume", 0x100);
        mCoreConfigSetDefaultIntValue(&m_core->config, "sampleRate", 48000);
        mCoreConfigSetDefaultUIntValue(&m_core->config, "audioBuffers", 0x400);
        applyMgbaCoreSettings(&m_core->config, configValues, options.platform);
        mCoreLoadForeignConfig(m_core, &m_core->config);
        if (!mCoreLoadFile(m_core, options.romPath.c_str()))
        {
            release();
            return false;
        }
        m_savePath = savePath;
        mCoreLoadSaveFile(m_core, m_savePath.c_str(), false);
        unsigned desiredW = 0;
        unsigned desiredH = 0;
        m_core->desiredVideoDimensions(m_core, &desiredW, &desiredH);
        m_width = desiredW > 0 ? desiredW : (options.platform == 1 ? 240u : 160u);
        m_height = desiredH > 0 ? desiredH : (options.platform == 1 ? 160u : 144u);
        m_bufferWidth = std::max(m_width, kMaxVideoWidth);
        m_bufferHeight = std::max(m_height, kMaxVideoHeight);
        m_videoBuffer.assign(static_cast<size_t>(m_bufferWidth) * m_bufferHeight, 0);
        m_rgbaBuffer.assign(static_cast<size_t>(m_width) * m_height, 0);
        m_core->setVideoBuffer(m_core, m_videoBuffer.data(), m_bufferWidth);
        const int32_t cycles = m_core->frameCycles(m_core);
        const int32_t frequency = m_core->frequency(m_core);
        if (cycles > 0 && frequency > 0)
            m_fps = static_cast<double>(frequency) / static_cast<double>(cycles);
        m_ready = true;
        m_core->reset(m_core);
        return true;
    }

    void release()
    {
        if (m_ready)
            saveSram();
        m_ready = false;
        m_videoBuffer.clear();
        m_rgbaBuffer.clear();
        m_width = 0;
        m_height = 0;
        m_bufferWidth = 0;
        m_bufferHeight = 0;
        m_keyMask = 0;
        m_fps = 60.0;
        if (!m_core)
            return;
        if (m_coreInitialized)
        {
            m_core->unloadROM(m_core);
            if (m_configInitialized)
                mCoreConfigDeinit(&m_core->config);
            m_core->deinit(m_core);
        }
        else
        {
            std::free(m_core);
        }
        m_core = nullptr;
        m_coreInitialized = false;
        m_configInitialized = false;
    }

    void reset() { if (m_core) m_core->reset(m_core); }
    void runFrame(uint32_t keys)
    {
        if (!m_ready || !m_core)
            return;
        if (keys != m_keyMask)
        {
            m_keyMask = keys;
            m_core->setKeys(m_core, keys);
        }
        m_core->runFrame(m_core);
    }
    bool captureFrame()
    {
        if (!m_ready || !m_core || m_width == 0 || m_height == 0)
            return false;
        const void* pixels = nullptr;
        size_t stride = 0;
        m_core->getPixels(m_core, &pixels, &stride);
        if (!pixels)
            return false;
        const auto* src = static_cast<const color_t*>(pixels);
        for (unsigned y = 0; y < m_height; ++y)
        {
            const color_t* srcRow = src + static_cast<size_t>(y) * stride;
            uint32_t* dstRow = m_rgbaBuffer.data() + static_cast<size_t>(y) * m_width;
            for (unsigned x = 0; x < m_width; ++x)
                dstRow[x] = nativeColorToRgba(srcRow[x]);
        }
        return true;
    }
    bool saveState(const std::string& path)
    {
        if (!m_ready || !m_core)
            return false;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        VFile* vf = VFileOpen(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
        if (!vf)
            return false;
        const bool ok = mCoreSaveStateNamed(m_core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC);
        vf->close(vf);
        saveSram();
        return ok;
    }
    bool loadState(const std::string& path)
    {
        if (!m_ready || !m_core)
            return false;
        VFile* vf = VFileOpen(path.c_str(), O_RDONLY);
        if (!vf)
            return false;
        const bool ok = mCoreLoadStateNamed(m_core, vf, SAVESTATE_RTC);
        vf->close(vf);
        return ok;
    }
    bool saveSram()
    {
        if (!m_core || !m_core->savedataClone || m_savePath.empty())
            return true;
        void* data = nullptr;
        const size_t size = m_core->savedataClone(m_core, &data);
        if (!data || size == 0)
            return true;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(m_savePath).parent_path(), ec);
        std::ofstream out(m_savePath, std::ios::binary | std::ios::trunc);
        if (out)
            out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        std::free(data);
        return true;
    }
    unsigned width() const { return m_width; }
    unsigned height() const { return m_height; }
    double fps() const { return m_fps; }
    mCore* nativeCore() const { return m_core; }
    std::vector<uint32_t>& rgbaBuffer() { return m_rgbaBuffer; }

private:
    mCore* m_core = nullptr;
    bool m_coreInitialized = false;
    bool m_configInitialized = false;
    bool m_ready = false;
    std::string m_savePath;
    unsigned m_width = 0;
    unsigned m_height = 0;
    unsigned m_bufferWidth = 0;
    unsigned m_bufferHeight = 0;
    double m_fps = 60.0;
    uint32_t m_keyMask = 0;
    std::vector<color_t> m_videoBuffer;
    std::vector<uint32_t> m_rgbaBuffer;
};

bool writeScreenshot(const std::vector<uint32_t>& rgba,
                     unsigned width,
                     unsigned height,
                     const std::string& saveDir)
{
    if (rgba.empty() || width == 0 || height == 0)
        return false;
    std::error_code ec;
    std::filesystem::create_directories(saveDir, ec);
    if (ec)
        return false;
    std::filesystem::path out = std::filesystem::path(saveDir) / ("screenshot_" + timestampString() + ".png");
    return stbi_write_png(out.string().c_str(),
                          static_cast<int>(width),
                          static_cast<int>(height),
                          4,
                          rgba.data(),
                          static_cast<int>(width * sizeof(uint32_t))) != 0;
}

} // namespace

namespace beiklive::mgba_stub {

int RunRuntime(const RunOptions& options)
{
    if (options.romPath.empty())
        return 1;

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    hidInitializeTouchScreen();
    PadState pad;
    padInitializeDefault(&pad);
    std::uint32_t previousVirtualHeld = 0;
    InputConfig inputConfig(options.platform);
    inputConfig.load();
    const auto configValues = loadConfigValues();

    if (R_FAILED(romfsInit()))
        return 1;
    Gfx::Init();

    const std::string saveDir = resolveSaveDir(options);
    const std::string savePath = saveFilePath(saveDir, options.romPath);
    const std::string states = stateDir(saveDir);

    RuntimeCore core;
    const bool loaded = core.load(options, savePath, configValues);
    if (!loaded)
    {
        Gfx::DeInit();
        romfsExit();
        return 1;
    }

    MgbaDisplaySettings display;
    display.fastForwardMultiplier = std::clamp(configFloat(configValues, "fastforward.multiplier", 4.0f), 0.1f, 10.0f);
    display.linearFiltering = configString(configValues, "display.filter", "nearest") == "linear";
    display.layout = options.hasDisplaySettings
                         ? uiLayoutFromScreenMode(options.displayMode)
                         : uiLayoutFromConfigMode(configString(configValues, "display.mode", "integer"));
    display.integerScale = display.layout == 4 || configBool(configValues, "display.integer_scale", true);
    display.integerScaleMultiplier = options.hasDisplaySettings
                                         ? std::clamp(options.integerScaleMultiplier, 0, 8)
                                         : std::clamp(configInt(configValues, "display.integer_scale_mult", 0), 0, 8);
    display.customLayout.topScale = std::clamp(options.hasDisplaySettings ? options.customScale : 1.0f, 1.0f, 15.0f);
    display.customLayout.topOffsetX = std::clamp(options.hasDisplaySettings ? options.customOffsetX : 0.0f, -1024.0f, 1024.0f);
    display.customLayout.topOffsetY = std::clamp(options.hasDisplaySettings ? options.customOffsetY : 0.0f, -1024.0f, 1024.0f);
    display.customLayout.bottomScale = display.customLayout.topScale;
    display.customLayout.bottomOffsetX = display.customLayout.topOffsetX;
    display.customLayout.bottomOffsetY = display.customLayout.topOffsetY;
    display.overlayEnabled = options.hasDisplaySettings
                                 ? options.overlayEnabled
                                 : configBool(configValues, "display.overlay.enabled", false);
    display.overlayPath = !options.overlayPath.empty()
                              ? options.overlayPath
                              : configString(configValues, overlayConfigKey(options.platform), {});
    display.shaderEnabled = options.hasDisplaySettings
                                ? options.shaderEnabled
                                : configBool(configValues, "display.shader.enabled", false);
    std::string shaderTypeCandidate = options.shaderType;
    if (shaderTypeCandidate.empty())
        shaderTypeCandidate = configString(configValues, "display.shader.mgbaType", {});
    if (shaderTypeCandidate.empty())
        shaderTypeCandidate = configString(configValues, "display.shader.ndsType", {});
    if (shaderTypeCandidate.empty() && !options.shaderPath.empty())
        shaderTypeCandidate = options.shaderPath;
    if (shaderTypeCandidate.empty())
        shaderTypeCandidate = shaderTypeFromPath(configString(configValues, shaderConfigKey(options.platform), {}));
    display.mgbaShaderType = normalizeMgbaShaderType(shaderTypeCandidate);
    display.shaderParams = loadShaderParams(display.mgbaShaderType);

    MgbaGameLayer gameLayer;
    gameLayer.setDisplaySettings(display);
    MgbaMenuLayer menuLayer;
    menuLayer.setDisplaySettings(display);
    auto stateSlots = loadStateSlots(states, options.romPath);
    menuLayer.setStateSlots(stateSlots);

    uint32_t overlayTexture = 0;
    int overlayWidth = 0;
    int overlayHeight = 0;
    bool appliedOverlayValid = false;
    bool appliedOverlayEnabled = false;
    std::string appliedOverlayPath;
    auto reloadOverlay = [&]() {
        const MgbaDisplaySettings& d = menuLayer.displaySettings();
        const bool wantOverlay = d.overlayEnabled && !d.overlayPath.empty();
        const std::string wantPath = wantOverlay ? d.overlayPath : std::string();
        if (appliedOverlayValid &&
            appliedOverlayEnabled == wantOverlay &&
            appliedOverlayPath == wantPath)
            return false;

        if (!wantOverlay)
        {
            const uint32_t oldTexture = overlayTexture;
            overlayTexture = 0;
            overlayWidth = 0;
            overlayHeight = 0;
            gameLayer.clearOverlay();
            if (oldTexture != 0)
            {
                Gfx::PresentQueue.waitIdle();
                Gfx::TextureDelete(oldTexture);
            }
            appliedOverlayValid = true;
            appliedOverlayEnabled = false;
            appliedOverlayPath.clear();
            return true;
        }

        uint32_t newTexture = 0;
        int newWidth = 0;
        int newHeight = 0;
        if (!loadPngTextureFromFile(wantPath, newTexture, newWidth, newHeight))
        {
            const uint32_t oldTexture = overlayTexture;
            overlayTexture = 0;
            overlayWidth = 0;
            overlayHeight = 0;
            gameLayer.clearOverlay();
            if (oldTexture != 0)
            {
                Gfx::PresentQueue.waitIdle();
                Gfx::TextureDelete(oldTexture);
            }
            appliedOverlayValid = true;
            appliedOverlayEnabled = false;
            appliedOverlayPath.clear();
            return true;
        }

        const uint32_t oldTexture = overlayTexture;
        overlayTexture = newTexture;
        overlayWidth = newWidth;
        overlayHeight = newHeight;
        gameLayer.setOverlay(overlayTexture, overlayWidth, overlayHeight);
        if (oldTexture != 0)
        {
            Gfx::PresentQueue.waitIdle();
            Gfx::TextureDelete(oldTexture);
        }
        appliedOverlayValid = true;
        appliedOverlayEnabled = true;
        appliedOverlayPath = wantPath;
        return true;
    };
    reloadOverlay();

    SwitchAudio gameAudio;
    SwitchRumble gameRumble;
    SwitchLuminance luminance;
    SystemRtc systemRtc;
    if (core.nativeCore())
    {
        gameRumble.init();
        core.nativeCore()->setPeripheral(core.nativeCore(), mPERIPH_RUMBLE, gameRumble.peripheral());
        if (core.nativeCore()->platform(core.nativeCore()) == mPLATFORM_GBA)
        {
            luminance.configure(configInt(configValues, "core.mgba_solar_sensor_level", 5));
            core.nativeCore()->setPeripheral(core.nativeCore(), mPERIPH_GBA_LUMINANCE, luminance.peripheral());
        }
        if (lower(configString(configValues, "core.mgba_rtc_mode", "persist")) == "system")
            mCoreSetRTC(core.nativeCore(), systemRtc.source());
        else
            core.nativeCore()->rtc.override = RTC_NO_OVERRIDE;
        gameAudio.init(core.nativeCore(),
                       core.fps(),
                       lower(configString(configValues, "core.mgba_audio_low_pass_filter", "disabled")) == "enabled",
                       configInt(configValues, "core.mgba_audio_low_pass_range", 60));
    }

    auto refreshSlots = [&]() {
        stateSlots = loadStateSlots(states, options.romPath);
        menuLayer.setStateSlots(stateSlots);
    };
    auto saveState = [&](int slot) {
        const bool ok = core.saveState(statePath(states, options.romPath, slot));
        refreshSlots();
        menuLayer.showToast(ok ? "保存状态完成" : "保存状态失败");
        return ok;
    };
    auto loadState = [&](int slot) {
        const bool ok = core.loadState(statePath(states, options.romPath, slot));
        menuLayer.showToast(ok ? "读取状态完成" : "读取状态失败");
        return ok;
    };
    auto takeScreenshot = [&]() {
        core.captureFrame();
        const bool ok = writeScreenshot(core.rgbaBuffer(), core.width(), core.height(), options.savePath.empty() ? saveDir : options.savePath);
        menuLayer.showToast(ok ? "截图已保存" : "截图失败");
        return ok;
    };
    auto applyDisplay = [&]() {
        MgbaDisplaySettings d = menuLayer.displaySettings();
        d.mgbaShaderType = normalizeMgbaShaderType(d.mgbaShaderType);
        if (d.shaderParams.size() != defaultShaderParams(d.mgbaShaderType).size())
            d.shaderParams = loadShaderParams(d.mgbaShaderType);
        menuLayer.setDisplaySettings(d);
        gameLayer.setDisplaySettings(menuLayer.displaySettings());
    };
    bool displayDirty = false;
    auto markDisplayDirty = [&]() {
        displayDirty = true;
    };
    auto flushDisplay = [&]() {
        if (!displayDirty)
            return;
        saveRuntimeDisplaySettings(options, menuLayer.displaySettings());
        displayDirty = false;
    };
    auto uploadCurrentFrame = [&]() {
        if (core.captureFrame())
        {
            gameLayer.uploadFrame(core.rgbaBuffer().data(),
                                  core.width(),
                                  core.height(),
                                  core.width() * sizeof(uint32_t));
        }
    };

    PlayStats playStats = loadAndIncrementPlayCount(options);
    int sessionPlaySeconds = 0;
    double playTimeFraction = 0.0;
    auto playTimeLast = std::chrono::steady_clock::now();
    constexpr double kPlayTimeSuspendGapSec = 5.0;

    mCoreRewindContext rewindContext {};
    bool rewindContextInitialized = false;
    const int rewindSaveInterval = inputConfig.rewindSaveInterval();
    const int rewindStep = inputConfig.rewindStep();
    int rewindFrameCounter = 0;
    if (inputConfig.rewindEnabled())
    {
        const size_t rewindEntries = static_cast<size_t>(
            std::max(1, inputConfig.rewindBufferSize() / std::max(1, rewindSaveInterval)));
        mCoreRewindContextInit(&rewindContext, rewindEntries, false);
        rewindContextInitialized = true;
    }

    bool running = true;
    bool pendingReturn = false;
    bool runtimePaused = false;
    bool muted = false;
    bool fastForwardToggle = false;
    bool fastForwardActive = false;
    double fastForwardAccumulator = 0.0;
    bool rewindToggle = false;
    bool rewindActive = false;
    bool turboAHeld = false;
    bool turboBHeld = false;
    bool turboAOn = false;
    bool turboBOn = false;
    int turboFrameCount = 0;
    const int turboIntervalFrames = inputConfig.turboIntervalFrames();
    double fps = 0.0;
    int fpsFrames = 0;
    auto fpsStart = std::chrono::steady_clock::now();
    bool menuWasActive = menuLayer.active();

    while (appletMainLoop() && running)
    {
        const auto frameBegin = std::chrono::steady_clock::now();
        padUpdate(&pad);
        const InputSnapshot input = makeInputSnapshot(pad, previousVirtualHeld);
        const uint64_t buttonsDown = padGetButtonsDown(&pad);
        const uint64_t buttonsHeld = padGetButtons(&pad);

        if (inputConfig.menuDown(input))
        {
            if (menuLayer.visible())
            {
                menuLayer.close();
                flushDisplay();
            }
            else
            {
                menuLayer.open();
            }
        }

        const MgbaMenuResult result = menuLayer.update(buttonsDown, buttonsHeld);
        for (const MgbaMenuSound sound : menuLayer.consumeSounds())
            gameAudio.playSound(sound);

        switch (result.action)
        {
        case MgbaMenuAction::DisplaySettingsChanged:
        case MgbaMenuAction::CustomLayoutChanged:
            applyDisplay();
            markDisplayDirty();
            break;
        case MgbaMenuAction::CustomLayoutCommitted:
            applyDisplay();
            markDisplayDirty();
            flushDisplay();
            break;
        case MgbaMenuAction::OverlaySettingsChanged:
            applyDisplay();
            reloadOverlay();
            markDisplayDirty();
            break;
        case MgbaMenuAction::OverlaySettingsCommitted:
            applyDisplay();
            reloadOverlay();
            markDisplayDirty();
            flushDisplay();
            break;
        case MgbaMenuAction::OverlayPathSelected:
        {
            MgbaDisplaySettings d = menuLayer.displaySettings();
            d.overlayPath = result.path;
            d.overlayEnabled = !result.path.empty();
            menuLayer.setDisplaySettings(d);
            applyDisplay();
            reloadOverlay();
            markDisplayDirty();
            flushDisplay();
            break;
        }
        case MgbaMenuAction::ShaderSettingsChanged:
            applyDisplay();
            markDisplayDirty();
            break;
        case MgbaMenuAction::ShaderSettingsCommitted:
            saveShaderParams(menuLayer.displaySettings().mgbaShaderType,
                             menuLayer.displaySettings().shaderParams);
            applyDisplay();
            markDisplayDirty();
            flushDisplay();
            break;
        case MgbaMenuAction::SyncDisplaySettings:
            flushDisplay();
            menuLayer.showSyncResult(MgbaMenuAction::SyncDisplaySettings,
                                     syncDisplaySettingsToGameData(options.romPath,
                                                                   menuLayer.displaySettings(),
                                                                   options.platform,
                                                                   0));
            break;
        case MgbaMenuAction::SyncOverlaySettings:
            flushDisplay();
            menuLayer.showSyncResult(MgbaMenuAction::SyncOverlaySettings,
                                     syncDisplaySettingsToGameData(options.romPath,
                                                                   menuLayer.displaySettings(),
                                                                   options.platform,
                                                                   1));
            break;
        case MgbaMenuAction::SyncShaderSettings:
            flushDisplay();
            saveShaderParams(menuLayer.displaySettings().mgbaShaderType,
                             menuLayer.displaySettings().shaderParams);
            menuLayer.showSyncResult(MgbaMenuAction::SyncShaderSettings,
                                     syncDisplaySettingsToGameData(options.romPath,
                                                                   menuLayer.displaySettings(),
                                                                   options.platform,
                                                                   2));
            break;
        case MgbaMenuAction::SaveState:
            saveState(result.slot);
            break;
        case MgbaMenuAction::LoadState:
            if (loadState(result.slot))
                menuLayer.close();
            break;
        case MgbaMenuAction::DeleteState:
        {
            std::error_code ec;
            std::filesystem::remove(statePath(states, options.romPath, result.slot), ec);
            refreshSlots();
            menuLayer.showToast("状态已删除");
            break;
        }
        case MgbaMenuAction::ResetGame:
            core.reset();
            menuLayer.close();
            break;
        case MgbaMenuAction::ExitGame:
            flushDisplay();
            pendingReturn = true;
            running = false;
            break;
        default:
            break;
        }
        const bool menuActiveAfterUpdate = menuLayer.active();
        if (menuWasActive && !menuActiveAfterUpdate)
            flushDisplay();
        menuWasActive = menuActiveAfterUpdate;

        if (!menuLayer.active())
        {
            if (inputConfig.fastForwardEnabled() &&
                inputConfig.fastForwardToggleMode() &&
                inputConfig.fastForwardDown(input))
                fastForwardToggle = !fastForwardToggle;
            if (inputConfig.rewindEnabled() &&
                inputConfig.rewindToggleMode() &&
                inputConfig.comboDownFor("handle.rewind", input))
                rewindToggle = !rewindToggle;
            if (inputConfig.comboDownFor("hotkey.pause.pad", input))
                runtimePaused = !runtimePaused;
            if (inputConfig.comboDownFor("hotkey.mute.pad", input))
            {
                muted = !muted;
                menuLayer.showToast(muted ? "已静音" : "已取消静音");
            }
            if (inputConfig.comboDownFor("hotkey.quicksave.pad", input))
                saveState(0);
            if (inputConfig.comboDownFor("hotkey.quickload.pad", input))
                loadState(0);
            if (inputConfig.comboDownFor("hotkey.screenshot.pad", input))
                takeScreenshot();
        }

        int framesRan = 0;
        fastForwardActive = false;
        rewindActive = false;
        const bool menuActive = menuLayer.active();
        const bool suppressGameInput = menuActive || runtimePaused;
        const bool fastForwardHeld = inputConfig.fastForwardToggleMode()
                                         ? fastForwardToggle
                                         : inputConfig.fastForwardHeld(input);
        const float multiplier =
            (!suppressGameInput && inputConfig.fastForwardEnabled() && fastForwardHeld)
                ? std::clamp(menuLayer.fastForwardMultiplier(), 0.1f, 10.0f)
                : 1.0f;
        fastForwardActive =
            !suppressGameInput &&
            inputConfig.fastForwardEnabled() &&
            fastForwardHeld &&
            std::fabs(multiplier - 1.0f) > 0.001f;
        rewindActive =
            !menuActive &&
            !runtimePaused &&
            inputConfig.rewindEnabled() &&
            rewindContextInitialized &&
            (inputConfig.rewindToggleMode()
                 ? rewindToggle
                 : inputConfig.comboHeldFor("handle.rewind", input));
        gameAudio.setFrameLimiter(!fastForwardActive);
        gameAudio.setSpeed(fastForwardActive && !inputConfig.fastForwardMute() ? multiplier : 1.0f);
        gameAudio.setMuted(muted ||
                           (fastForwardActive && inputConfig.fastForwardMute()) ||
                           (rewindActive && inputConfig.rewindMute()));

        if (!suppressGameInput)
        {
            turboAHeld = inputConfig.comboHeldFor("handle.a_turbo", input);
            turboBHeld = inputConfig.comboHeldFor("handle.b_turbo", input);
            ++turboFrameCount;
            if (turboFrameCount >= turboIntervalFrames)
            {
                turboFrameCount = 0;
                if (turboAHeld) turboAOn = !turboAOn;
                if (turboBHeld) turboBOn = !turboBOn;
            }
            if (!turboAHeld) turboAOn = false;
            if (!turboBHeld) turboBOn = false;
        }
        else
        {
            turboAOn = false;
            turboBOn = false;
        }

        if (rewindActive)
        {
            for (int i = 0; i < rewindStep; ++i)
            {
                if (!mCoreRewindRestore(&rewindContext, core.nativeCore()))
                {
                    rewindToggle = false;
                    rewindActive = false;
                    break;
                }
                core.runFrame(inputConfig.keyMask(input, false, false));
                ++framesRan;
                uploadCurrentFrame();
            }
            fastForwardAccumulator = 0.0;
        }
        else if (!suppressGameInput)
        {
            const uint32_t keyMask = inputConfig.keyMask(input, turboAOn, turboBOn);
            if (!fastForwardActive)
                fastForwardAccumulator = 0.0;
            if (multiplier >= 1.0f)
            {
                fastForwardAccumulator += static_cast<double>(multiplier);
                int framesToRun = static_cast<int>(std::floor(fastForwardAccumulator));
                fastForwardAccumulator -= static_cast<double>(framesToRun);
                framesToRun = std::clamp(framesToRun, 1, 10);
                for (int i = 0; i < framesToRun; ++i)
                    core.runFrame(keyMask);
                framesRan = framesToRun;
            }
            else
            {
                fastForwardAccumulator += static_cast<double>(multiplier);
                if (fastForwardAccumulator >= 1.0)
                {
                    fastForwardAccumulator -= 1.0;
                    core.runFrame(keyMask);
                    framesRan = 1;
                }
            }
            if (rewindContextInitialized && framesRan > 0)
            {
                rewindFrameCounter += framesRan;
                while (rewindFrameCounter >= rewindSaveInterval)
                {
                    rewindFrameCounter -= rewindSaveInterval;
                    mCoreRewindAppend(&rewindContext, core.nativeCore());
                }
            }
        }
        else
        {
            gameAudio.setFrameLimiter(true);
            gameAudio.setSpeed(1.0f);
            fastForwardAccumulator = 0.0;
        }
        if ((menuActive || runtimePaused || framesRan == 0) && !rewindActive)
            gameAudio.pumpUiAudio();

        const bool emulationPaused = menuActive || runtimePaused;
        if (!emulationPaused && !rewindActive && framesRan > 0)
        {
            const auto nowForPlayTime = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double>(nowForPlayTime - playTimeLast).count();
            playTimeLast = nowForPlayTime;
            if (elapsed > 0.0 && elapsed <= kPlayTimeSuspendGapSec)
            {
                playTimeFraction += elapsed;
                if (playTimeFraction >= 1.0)
                {
                    const int wholeSeconds = static_cast<int>(playTimeFraction);
                    sessionPlaySeconds += wholeSeconds;
                    playTimeFraction -= static_cast<double>(wholeSeconds);
                }
            }
        }
        else
        {
            playTimeLast = std::chrono::steady_clock::now();
        }

        if (!rewindActive)
            uploadCurrentFrame();

        Gfx::StartFrame();
        Gfx::PushScissor(0, 0, kScreenWidth, kScreenHeight);
        gameLayer.setDisplaySettings(menuLayer.displaySettings());
        gameLayer.draw();
        if (!menuLayer.active())
            ui::drawGameStatusBadges(fps,
                                     inputConfig.showFps(),
                                     fastForwardActive,
                                     inputConfig.showFfOverlay(),
                                     rewindActive,
                                     inputConfig.showRewindOverlay(),
                                     runtimePaused);
        menuLayer.draw();
        gameRumble.flush();
        Gfx::PopScissor();
        Gfx::EndFrame({0.015f, 0.020f, 0.026f, 1.0f}, 0);

        fpsFrames += framesRan;
        const auto now = std::chrono::steady_clock::now();
        const auto fpsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsStart).count();
        if (fpsElapsed >= 1000)
        {
            fps = static_cast<double>(fpsFrames) * 1000.0 / static_cast<double>(fpsElapsed);
            fpsFrames = 0;
            fpsStart = now;
        }

        const auto frameEnd = std::chrono::steady_clock::now();
        const auto frameBudget = std::chrono::microseconds(
            static_cast<int64_t>(1000000.0 / std::max(1.0, core.fps())));
        const auto used = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameBegin);
        if (used < frameBudget)
        {
            const auto sleepUs = std::chrono::duration_cast<std::chrono::microseconds>(frameBudget - used).count();
            if (sleepUs > 500)
                svcSleepThread(static_cast<int64_t>(sleepUs) * 1000);
        }
    }

    core.saveSram();
    flushDisplay();
    savePlayStats(options, playStats.playCount, playStats.playTime + std::max(0, sessionPlaySeconds));
    if (rewindContextInitialized)
        mCoreRewindContextDeinit(&rewindContext);
    gameRumble.stop();
    gameAudio.deinit();
    if (overlayTexture != 0)
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(overlayTexture);
    }
    core.release();
    Gfx::DeInit();
    romfsExit();

    if (pendingReturn)
        setReturnNro(options.returnNroPath);
    return pendingReturn ? 0 : 1;
}

} // namespace beiklive::mgba_stub
