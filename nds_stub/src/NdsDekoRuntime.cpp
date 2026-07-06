#include "nds_stub/NdsDekoRuntime.hpp"

#include "nds_stub/NdsCheatDatabase.hpp"
#include "nds_stub/NdsGameLayer.hpp"
#include "nds_stub/NdsMenuLayer.hpp"
#include "nds_stub/ui/UiComponents.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <functional>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>
#include <switch.h>

#ifdef OGLRENDERER_ENABLED
#undef OGLRENDERER_ENABLED
#endif

#include "../../third_party/ArcDelta_melonDS/src/Config.h"
#include "../../third_party/ArcDelta_melonDS/src/ARCodeFile.h"
#include "../../third_party/ArcDelta_melonDS/src/AREngine.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../third_party/ArcDelta_melonDS/src/NDS.h"
#include "../../third_party/ArcDelta_melonDS/src/NDSCart.h"
#include "../../third_party/ArcDelta_melonDS/src/Platform.h"
#include "../../third_party/ArcDelta_melonDS/src/Savestate.h"
#include "../../third_party/ArcDelta_melonDS/src/SPU.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/mic_blow.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/PlatformConfig.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "nds_stub/StubLog.hpp"

namespace {

constexpr uint32_t kNdsKeyA      = 1u << 0;
constexpr uint32_t kNdsKeyB      = 1u << 1;
constexpr uint32_t kNdsKeySelect = 1u << 2;
constexpr uint32_t kNdsKeyStart  = 1u << 3;
constexpr uint32_t kNdsKeyRight  = 1u << 4;
constexpr uint32_t kNdsKeyLeft   = 1u << 5;
constexpr uint32_t kNdsKeyUp     = 1u << 6;
constexpr uint32_t kNdsKeyDown   = 1u << 7;
constexpr uint32_t kNdsKeyR      = 1u << 8;
constexpr uint32_t kNdsKeyL      = 1u << 9;
constexpr uint32_t kNdsKeyX      = 1u << 10;
constexpr uint32_t kNdsKeyY      = 1u << 11;

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

std::string defaultSaveDir(const std::string& romPath)
{
    return joinPath(joinPath("sdmc:/GBAStation/save/NDS", pathStem(romPath)), "");
}

std::string resolveSavePath(const beiklive::nds_stub::DekoRunOptions& options)
{
    const std::string saveDir = options.savePath.empty() ? defaultSaveDir(options.romPath) : options.savePath;
    std::error_code ec;
    std::filesystem::create_directories(saveDir, ec);
    return joinPath(saveDir, pathStem(options.romPath) + ".sav");
}

std::string statePath(const std::string& stateDir, const std::string& romPath, int slot)
{
    slot = std::max(0, slot);
    return joinPath(stateDir, pathStem(romPath) + ".ss" + std::to_string(slot));
}

std::string stateThumbPath(const std::string& stateDir, const std::string& romPath, int slot)
{
    return statePath(stateDir, romPath, slot) + ".png";
}

const char* layoutIdFromIndex(int layout)
{
    static constexpr const char* ids[] = {
        "vertical",
        "horizontal",
        "priority_top",
        "priority_bottom",
        "hybrid",
        "top",
        "bottom",
        "custom",
    };
    return ids[std::clamp(layout, 0, 7)];
}

int layoutIndexFromId(const std::string& layout)
{
    if (layout == "horizontal") return 1;
    if (layout == "priority_top" || layout == "top_priority") return 2;
    if (layout == "priority_bottom" || layout == "bottom_priority") return 3;
    if (layout == "hybrid") return 4;
    if (layout == "top" || layout == "single_top") return 5;
    if (layout == "bottom" || layout == "single_bottom") return 6;
    if (layout == "custom" || layout == "separate") return 7;
    return 0;
}

int orientationIndexFromId(std::string value)
{
    value.erase(std::remove_if(value.begin(), value.end(),
                               [](unsigned char c) { return std::isspace(c) != 0; }),
                value.end());
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (value == "90" || value == "90deg" || value == "90°" || value == "horizontal")
        return 1;
    if (value == "180" || value == "180deg" || value == "180°" || value == "vertical_reverse")
        return 2;
    if (value == "270" || value == "270deg" || value == "270°" || value == "horizontal_reverse")
        return 3;
    return 0;
}

const char* orientationIdFromIndex(int orientation)
{
    static constexpr const char* ids[] = {"0", "90", "180", "270"};
    return ids[std::clamp(orientation, 0, 3)];
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

std::string jsonString(const nlohmann::json& item, const char* key)
{
    if (!item.contains(key) || !item.at(key).is_string())
        return {};
    return item.at(key).get<std::string>();
}

std::string normalizedNdsShaderType(const std::string& type)
{
    if (type == "dot-clear" ||
        type == "xbrz-freescale" ||
        type == "lcd-grid-v2-nds-color" ||
        type == "scanline" ||
        type == "crt")
    {
        return type;
    }
    return "dot";
}

std::vector<beiklive::nds_stub::NdsShaderParam> defaultNdsShaderParams(const std::string& type)
{
    using beiklive::nds_stub::NdsShaderParam;
    const std::string shader = normalizedNdsShaderType(type);
    if (shader == "dot")
    {
        return {
            {"gamma", "Gamma", 2.4f, 2.4f, 0.5f, 6.0f, 0.1f, 1},
            {"shine", "Shine", 0.05f, 0.05f, 0.0f, 0.5f, 0.01f, 2},
            {"blend", "Blend", 0.65f, 0.65f, 0.0f, 1.0f, 0.05f, 2},
        };
    }
    if (shader == "dot-clear")
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
    if (shader == "lcd-grid-v2-nds-color")
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

std::string ndsShaderConfigPath(const std::string& type)
{
    return joinPath("sdmc:/GBAStation/config/ndsshaderconfig", normalizedNdsShaderType(type) + ".ini");
}

std::vector<beiklive::nds_stub::NdsShaderParam> loadNdsShaderParams(const std::string& type)
{
    auto params = defaultNdsShaderParams(type);
    const std::string path = ndsShaderConfigPath(type);
    std::ifstream in(path);
    if (!in)
        return params;

    std::map<std::string, float> values;
    std::string line;
    while (std::getline(in, line))
    {
        const auto comment = line.find_first_of("#;");
        if (comment != std::string::npos)
            line.erase(comment);
        const auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) == 0; }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c) == 0; }).base(), s.end());
        };
        trim(key);
        trim(value);
        try
        {
            values[key] = std::stof(value);
        }
        catch (...)
        {
        }
    }

    for (auto& param : params)
    {
        const auto it = values.find(param.name);
        if (it != values.end())
            param.value = std::clamp(it->second, param.minValue, param.maxValue);
    }
    return params;
}

void saveNdsShaderParams(const std::string& type,
                         const std::vector<beiklive::nds_stub::NdsShaderParam>& params)
{
    const std::string path = ndsShaderConfigPath(type);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    if (!out)
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: shader config save failed open path=%s", path.c_str());
        return;
    }
    out << "# GBAStation NDS shader config\n";
    out << "shader=" << normalizedNdsShaderType(type) << "\n";
    for (const auto& param : params)
        out << param.name << '=' << param.value << "\n";
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: shader config saved path=%s params=%d",
                                      path.c_str(),
                                      static_cast<int>(params.size()));
}

std::array<float, 8> ndsShaderParamUniforms(const std::string& type,
                                            const std::vector<beiklive::nds_stub::NdsShaderParam>& params)
{
    std::array<float, 8> values {};
    const std::string shader = normalizedNdsShaderType(type);
    auto valueOf = [&](const char* name, float fallback) {
        for (const auto& param : params)
        {
            if (param.name == name)
                return param.value;
        }
        return fallback;
    };

    if (shader == "dot")
    {
        values[0] = valueOf("gamma", 2.4f);
        values[1] = valueOf("shine", 0.05f);
        values[2] = valueOf("blend", 0.65f);
    }
    else if (shader == "dot-clear")
    {
        values[0] = valueOf("screen_gamma", 2.2f);
        values[1] = valueOf("dot_gamma", 2.2f);
        values[2] = valueOf("dot_scale_x", 1.1f);
        values[3] = valueOf("dot_scale_y", 1.1f);
        values[4] = valueOf("dot_opacity", 0.7f);
        values[5] = valueOf("halftone_strength", 0.7f);
    }
    else if (shader == "lcd-grid-v2-nds-color")
    {
        values[0] = valueOf("gain", 1.5f);
        values[1] = valueOf("gamma", 2.2f);
        values[2] = valueOf("blacklevel", 0.0f);
        values[3] = valueOf("ambient", 0.0f);
        values[4] = valueOf("bgr", 1.0f);
        values[5] = valueOf("nds_color", 1.0f);
    }
    return values;
}

std::string currentLastPlayedTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&tt);
    if (!tm)
        return {};
    char buffer[64] = {};
    std::strftime(buffer, sizeof(buffer), "%y-%m-%d %H-%M-%S", tm);
    return buffer;
}

struct NdsPlayStats {
    int playCount = 0;
    int playTime = 0;
    bool found = false;
};

NdsPlayStats loadAndIncrementNdsPlayCount(const std::string& romPath)
{
    NdsPlayStats stats {};
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_NDS.json",
        "/GBAStation/data/GameData_NDS.json",
    };

    for (const char* dbPath : paths)
    {
        std::ifstream in(dbPath);
        if (!in)
            continue;

        try
        {
            nlohmann::json data;
            in >> data;
            in.close();
            if (!data.is_array())
                continue;

            bool updated = false;
            for (auto& item : data)
            {
                const std::string itemPath = normalizePathForCompare(jsonString(item, "path"));
                if (itemPath != normalizedRom)
                    continue;

                stats.playCount = item.value("playCount", 0);
                stats.playTime = item.value("playTime", 0);
                ++stats.playCount;
                item["playCount"] = stats.playCount;
                item["playTime"] = stats.playTime;
                stats.found = true;
                updated = true;
                break;
            }

            if (!updated)
                continue;

            std::ofstream out(dbPath, std::ios::trunc);
            if (!out)
                return stats;
            out << data.dump(4) << '\n';
            const bool ok = out.good();
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: play stats start save %s path=%s playCount=%d playTime=%d",
                                              ok ? "ok" : "failed",
                                              dbPath,
                                              stats.playCount,
                                              stats.playTime);
            return stats;
        }
        catch (const std::exception& e)
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: play stats start exception path=%s error=%s",
                                              dbPath,
                                              e.what());
        }
    }

    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: play stats start skipped no match rom=%s",
                                      romPath.c_str());
    return stats;
}

bool saveNdsPlayStatsToGameDb(const std::string& romPath,
                              int playCount,
                              int playTime,
                              const std::string& lastPlayed)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_NDS.json",
        "/GBAStation/data/GameData_NDS.json",
    };

    for (const char* dbPath : paths)
    {
        std::ifstream in(dbPath);
        if (!in)
            continue;

        try
        {
            nlohmann::json data;
            in >> data;
            in.close();
            if (!data.is_array())
                continue;

            bool updated = false;
            for (auto& item : data)
            {
                const std::string itemPath = normalizePathForCompare(jsonString(item, "path"));
                if (itemPath != normalizedRom)
                    continue;

                item["playCount"] = std::max(0, playCount);
                item["playTime"] = std::max(0, playTime);
                if (!lastPlayed.empty())
                    item["lastPlayed"] = lastPlayed;
                updated = true;
                break;
            }

            if (!updated)
                continue;

            std::ofstream out(dbPath, std::ios::trunc);
            if (!out)
                return false;
            out << data.dump(4) << '\n';
            const bool ok = out.good();
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: play stats exit save %s path=%s playCount=%d playTime=%d lastPlayed=%s",
                                              ok ? "ok" : "failed",
                                              dbPath,
                                              std::max(0, playCount),
                                              std::max(0, playTime),
                                              lastPlayed.c_str());
            return ok;
        }
        catch (const std::exception& e)
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: play stats exit exception path=%s error=%s",
                                              dbPath,
                                              e.what());
        }
    }

    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: play stats exit skipped no match rom=%s",
                                      romPath.c_str());
    return false;
}

bool saveNdsSettingsToGameDb(const std::string& romPath,
                             const beiklive::nds_stub::NdsDisplaySettings& settings,
                             bool includeCustomLayout)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_NDS.json",
        "/GBAStation/data/GameData_NDS.json",
    };

    for (const char* dbPath : paths)
    {
        std::ifstream in(dbPath);
        if (!in)
            continue;

        try
        {
            nlohmann::json data;
            in >> data;
            in.close();
            if (!data.is_array())
                continue;

            bool updated = false;
            for (auto& item : data)
            {
                const std::string itemPath = normalizePathForCompare(jsonString(item, "path"));
                if (itemPath != normalizedRom)
                    continue;

                item["ndsScreenLayout"] = layoutIdFromIndex(settings.layout);
                item["ndsScreenOrientation"] = orientationIdFromIndex(settings.orientation);
                item["ndsIntegerScale"] = settings.integerScale;
                item["ndsScreenGap"] = settings.screenGap;
                item["overlayEnabled"] = settings.overlayEnabled;
                item["overlayPath"] = settings.overlayPath;
                item["shaderEnabled"] = settings.shaderEnabled;
                const std::string shaderType = normalizedNdsShaderType(settings.ndsShaderType);
                item["NdsShaderType"] = shaderType;
                item["shaderParaPath"] = shaderType;
                item["shaderParaNames"] = nlohmann::json::array();
                item["shaderParaValues"] = nlohmann::json::array();
                if (includeCustomLayout)
                {
                    item["ndsTopScale"] = settings.customLayout.topScale;
                    item["ndsTopOffsetX"] = settings.customLayout.topOffsetX;
                    item["ndsTopOffsetY"] = settings.customLayout.topOffsetY;
                    item["ndsBottomScale"] = settings.customLayout.bottomScale;
                    item["ndsBottomOffsetX"] = settings.customLayout.bottomOffsetX;
                    item["ndsBottomOffsetY"] = settings.customLayout.bottomOffsetY;
                }
                updated = true;
                break;
            }

            if (!updated)
                continue;

            std::ofstream out(dbPath, std::ios::trunc);
            if (!out)
                return false;
            out << data.dump(4) << '\n';
            const bool ok = out.good();
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS settings GameDB save %s path=%s layout=%s orientation=%s integer=%d gap=%d custom=%d top=%.2f/%.1f/%.1f bottom=%.2f/%.1f/%.1f",
                                              ok ? "ok" : "failed",
                                              dbPath,
                                              layoutIdFromIndex(settings.layout),
                                              orientationIdFromIndex(settings.orientation),
                                              settings.integerScale ? 1 : 0,
                                              settings.screenGap,
                                              includeCustomLayout ? 1 : 0,
                                              settings.customLayout.topScale,
                                              settings.customLayout.topOffsetX,
                                              settings.customLayout.topOffsetY,
                                              settings.customLayout.bottomScale,
                                              settings.customLayout.bottomOffsetX,
                                              settings.customLayout.bottomOffsetY);
            return ok;
        }
        catch (const std::exception& e)
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS settings GameDB save exception path=%s error=%s",
                                              dbPath,
                                              e.what());
        }
    }

    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS settings GameDB save skipped no match rom=%s",
                                      romPath.c_str());
    return false;
}

int syncNdsDisplaySettingsToGameDb(const std::string& romPath,
                                   const beiklive::nds_stub::NdsDisplaySettings& settings)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_NDS.json",
        "/GBAStation/data/GameData_NDS.json",
    };

    for (const char* dbPath : paths)
    {
        std::ifstream in(dbPath);
        if (!in)
            continue;

        try
        {
            nlohmann::json data;
            in >> data;
            in.close();
            if (!data.is_array())
                continue;

            int count = 0;
            for (auto& item : data)
            {
                const std::string itemPath = normalizePathForCompare(jsonString(item, "path"));
                if (itemPath.empty() || itemPath == normalizedRom)
                    continue;

                item["ndsScreenLayout"] = layoutIdFromIndex(settings.layout);
                item["ndsScreenOrientation"] = orientationIdFromIndex(settings.orientation);
                item["ndsIntegerScale"] = settings.integerScale;
                item["ndsScreenGap"] = settings.screenGap;
                item["ndsTopScale"] = settings.customLayout.topScale;
                item["ndsTopOffsetX"] = settings.customLayout.topOffsetX;
                item["ndsTopOffsetY"] = settings.customLayout.topOffsetY;
                item["ndsBottomScale"] = settings.customLayout.bottomScale;
                item["ndsBottomOffsetX"] = settings.customLayout.bottomOffsetX;
                item["ndsBottomOffsetY"] = settings.customLayout.bottomOffsetY;
                ++count;
            }

            std::ofstream out(dbPath, std::ios::trunc);
            if (!out)
                return -1;
            out << data.dump(4) << '\n';
            if (!out.good())
                return -1;

            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS display settings synced count=%d path=%s layout=%s orientation=%s integer=%d gap=%d",
                                              count,
                                              dbPath,
                                              layoutIdFromIndex(settings.layout),
                                              orientationIdFromIndex(settings.orientation),
                                              settings.integerScale ? 1 : 0,
                                              settings.screenGap);
            return count;
        }
        catch (const std::exception& e)
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS display sync exception path=%s error=%s",
                                              dbPath,
                                              e.what());
        }
    }

    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS display sync skipped no db rom=%s",
                                      romPath.c_str());
    return -1;
}

int syncNdsOverlaySettingsToGameDb(const std::string& romPath,
                                   const beiklive::nds_stub::NdsDisplaySettings& settings)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    constexpr const char* paths[] = {
        "sdmc:/GBAStation/data/GameData_NDS.json",
        "/GBAStation/data/GameData_NDS.json",
    };

    for (const char* dbPath : paths)
    {
        std::ifstream in(dbPath);
        if (!in)
            continue;

        try
        {
            nlohmann::json data;
            in >> data;
            in.close();
            if (!data.is_array())
                continue;

            int count = 0;
            for (auto& item : data)
            {
                const std::string itemPath = normalizePathForCompare(jsonString(item, "path"));
                if (itemPath.empty() || itemPath == normalizedRom)
                    continue;

                item["overlayEnabled"] = settings.overlayEnabled;
                item["overlayPath"] = settings.overlayPath;
                ++count;
            }

            std::ofstream out(dbPath, std::ios::trunc);
            if (!out)
                return -1;
            out << data.dump(4) << '\n';
            if (!out.good())
                return -1;

            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS overlay settings synced count=%d path=%s enabled=%d overlayPath=%s",
                                              count,
                                              dbPath,
                                              settings.overlayEnabled ? 1 : 0,
                                              settings.overlayPath.c_str());
            return count;
        }
        catch (const std::exception& e)
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS overlay sync exception path=%s error=%s",
                                              dbPath,
                                              e.what());
        }
    }

    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: NDS overlay sync skipped no db rom=%s",
                                      romPath.c_str());
    return -1;
}

std::unique_ptr<ARCodeFile> buildRuntimeCheatFile(const std::vector<beiklive::nds_stub::NdsCheatItem>& items,
                                                  int& enabledCount,
                                                  int& truncatedCount)
{
    enabledCount = 0;
    truncatedCount = 0;

    auto file = std::make_unique<ARCodeFile>("");
    file->Categories.clear();

    ARCodeCat cat {};
    std::strncpy(cat.Name, "GBAStation", sizeof(cat.Name) - 1);
    cat.Name[sizeof(cat.Name) - 1] = '\0';

    for (const auto& item : items)
    {
        if (item.type != beiklive::nds_stub::NdsCheatItem::Type::Code ||
            !item.enabled ||
            item.words.size() < 2)
            continue;

        ARCode code {};
        std::strncpy(code.Name,
                     item.name.empty() ? "NDS AR Code" : item.name.c_str(),
                     sizeof(code.Name) - 1);
        code.Name[sizeof(code.Name) - 1] = '\0';
        code.Enabled = true;

        std::size_t wordCount = item.words.size() & ~std::size_t(1);
        if (wordCount > 128)
        {
            wordCount = 128;
            ++truncatedCount;
        }
        code.CodeLen = static_cast<u32>(wordCount);
        for (std::size_t i = 0; i < wordCount; ++i)
            code.Code[i] = item.words[i];

        cat.Codes.push_back(code);
        ++enabledCount;
    }

    if (enabledCount > 0)
        file->Categories.push_back(cat);
    return file;
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

std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>
loadStateSlots(const std::string& stateDir, const std::string& romPath)
{
    std::array<beiklive::nds_stub::NdsStateSlotInfo, 10> slots {};
    for (int slot = 0; slot < static_cast<int>(slots.size()); ++slot)
    {
        auto& info = slots[slot];
        info.statePath = statePath(stateDir, romPath, slot);
        info.thumbnailPath = stateThumbPath(stateDir, romPath, slot);
        info.exists = std::filesystem::exists(info.statePath);
        info.thumbnailAvailable = std::filesystem::exists(info.thumbnailPath);
        if (info.exists)
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

bool writeRgbaPng(const std::string& path,
                  const std::vector<std::uint8_t>& rgba,
                  int width,
                  int height)
{
    if (rgba.empty() || width <= 0 || height <= 0)
        return false;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    return stbi_write_png(path.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
}

bool captureAndWritePng(const beiklive::nds_stub::NdsGameLayer& gameLayer, const std::string& path)
{
    std::vector<std::uint8_t> rgba;
    int width = 0;
    int height = 0;
    if (!gameLayer.captureCurrentFrameRgba(rgba, width, height))
        return false;
    return writeRgbaPng(path, rgba, width, height);
}

bool captureAndWriteStateThumbnail(const beiklive::nds_stub::NdsGameLayer& gameLayer,
                                   const std::string& pngPath)
{
    std::vector<std::uint8_t> rgba;
    int width = 0;
    int height = 0;
    if (!gameLayer.captureCurrentFrameRgba(rgba, width, height))
        return false;

    const bool pngOk = writeRgbaPng(pngPath, rgba, width, height);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: savestate thumbnail write png=%d size=%dx%d",
                                      pngOk ? 1 : 0,
                                      width,
                                      height);
    return pngOk;
}

bool writeStateThumbnailFromRgba(const std::vector<std::uint8_t>& rgba,
                                 int width,
                                 int height,
                                 const std::string& pngPath)
{
    const bool pngOk = writeRgbaPng(pngPath, rgba, width, height);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: savestate thumbnail write memory png=%d size=%dx%d",
                                      pngOk ? 1 : 0,
                                      width,
                                      height);
    return pngOk;
}

bool readRgbaPngFile(const std::string& path,
                     std::vector<std::uint8_t>& rgba,
                     int& width,
                     int& height)
{
    width = 0;
    height = 0;
    rgba.clear();

    int comp = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &comp, 4);
    if (!pixels || width <= 0 || height <= 0 || width > 1024 || height > 1024)
    {
        if (pixels)
            stbi_image_free(pixels);
        width = 0;
        height = 0;
        return false;
    }

    rgba.assign(pixels, pixels + static_cast<std::size_t>(width) * height * 4);
    stbi_image_free(pixels);
    return true;
}

void releaseStateSlotTextures(std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>& slots)
{
    for (auto& slot : slots)
    {
        if (slot.thumbnailTexture != 0)
        {
            Gfx::TextureDelete(slot.thumbnailTexture);
            slot.thumbnailTexture = 0;
        }
        slot.thumbnailWidth = 0;
        slot.thumbnailHeight = 0;
        slot.thumbnailLoadAttempted = false;
    }
}

void releaseStateSlotTexture(beiklive::nds_stub::NdsStateSlotInfo& slot)
{
    if (slot.thumbnailTexture != 0)
    {
        Gfx::TextureDelete(slot.thumbnailTexture);
        slot.thumbnailTexture = 0;
    }
    slot.thumbnailWidth = 0;
    slot.thumbnailHeight = 0;
    slot.thumbnailLoadAttempted = false;
}

bool hasPendingStateSlotTextures(const std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>& slots)
{
    for (const auto& slot : slots)
    {
        if (slot.exists &&
            slot.thumbnailTexture == 0 &&
            !slot.thumbnailLoadAttempted &&
            !slot.thumbnailPath.empty() &&
            std::filesystem::exists(slot.thumbnailPath))
            return true;
    }
    return false;
}

bool uploadStateSlotTexture(beiklive::nds_stub::NdsStateSlotInfo& slot,
                            const std::vector<std::uint8_t>& pixels,
                            int width,
                            int height,
                            int slotIndex)
{
    if (pixels.empty() || width <= 0 || height <= 0)
        return false;

    releaseStateSlotTexture(slot);
    slot.thumbnailLoadAttempted = true;
    slot.thumbnailTexture = Gfx::TextureCreate(static_cast<u32>(width),
                                               static_cast<u32>(height),
                                               DkImageFormat_RGBA8_Unorm);
    slot.thumbnailWidth = width;
    slot.thumbnailHeight = height;
    Gfx::TextureUpload(slot.thumbnailTexture,
                       0,
                       0,
                       static_cast<u32>(width),
                       static_cast<u32>(height),
                       const_cast<std::uint8_t*>(pixels.data()),
                       static_cast<u32>(width * 4));
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: state thumbnail upload queued slot=%d size=%dx%d tex=%u",
                                      slotIndex,
                                      width,
                                      height,
                                      slot.thumbnailTexture);
    return slot.thumbnailTexture != 0;
}

bool loadStateSlotTexture(beiklive::nds_stub::NdsStateSlotInfo& slot, int slotIndex)
{
    if (!slot.exists || slot.thumbnailTexture != 0 || slot.thumbnailLoadAttempted ||
        slot.thumbnailPath.empty() || !std::filesystem::exists(slot.thumbnailPath))
        return false;

    slot.thumbnailLoadAttempted = true;
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: state thumbnail png load begin slot=%d path=%s",
                                      slotIndex,
                                      slot.thumbnailPath.c_str());

    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!readRgbaPngFile(slot.thumbnailPath, pixels, width, height))
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: state thumbnail png load failed slot=%d path=%s",
                                          slotIndex,
                                          slot.thumbnailPath.c_str());
        return false;
    }

    return uploadStateSlotTexture(slot, pixels, width, height, slotIndex);
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

bool loadPngTextureFromFile(const std::string& path, std::uint32_t& texture, int& width, int& height)
{
    texture = 0;
    width = 0;
    height = 0;
    if (path.empty() || !endsWithNoCase(path, ".png"))
        return false;

    auto makePathCandidates = [](const std::string& source) {
        std::vector<std::string> paths;
        paths.push_back(source);
        if (source.rfind("sdmc:", 0) == 0)
            paths.push_back(source.substr(5));
        else if (!source.empty() && source[0] == '/')
            paths.push_back("sdmc:" + source);
        return paths;
    };

    FILE* fp = nullptr;
    std::string openedPath;
    for (const auto& candidate : makePathCandidates(path))
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png open try path=%s", candidate.c_str());
        fp = std::fopen(candidate.c_str(), "rb");
        if (fp)
        {
            openedPath = candidate;
            break;
        }
    }
    if (!fp)
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png open failed path=%s", path.c_str());
        return false;
    }

    if (std::fseek(fp, 0, SEEK_END) != 0)
    {
        std::fclose(fp);
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png seek end failed path=%s", openedPath.c_str());
        return false;
    }
    const long fileSize = std::ftell(fp);
    if (fileSize <= 0 || fileSize > 64 * 1024 * 1024)
    {
        std::fclose(fp);
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png size invalid path=%s bytes=%ld", openedPath.c_str(), fileSize);
        return false;
    }
    std::rewind(fp);
    std::vector<unsigned char> bytes(static_cast<std::size_t>(fileSize));
    const std::size_t readBytes = std::fread(bytes.data(), 1, bytes.size(), fp);
    std::fclose(fp);
    if (readBytes != bytes.size())
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png read failed path=%s read=%u expected=%u",
                      openedPath.c_str(),
                      static_cast<unsigned>(readBytes),
                      static_cast<unsigned>(bytes.size()));
        return false;
    }
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png read ok path=%s bytes=%u",
                  openedPath.c_str(),
                  static_cast<unsigned>(bytes.size()));

    auto readBe32 = [](const unsigned char* p) {
        return (static_cast<std::uint32_t>(p[0]) << 24) |
               (static_cast<std::uint32_t>(p[1]) << 16) |
               (static_cast<std::uint32_t>(p[2]) << 8) |
               static_cast<std::uint32_t>(p[3]);
    };
    if (bytes.size() >= 33 &&
        bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G' &&
        bytes[12] == 'I' && bytes[13] == 'H' && bytes[14] == 'D' && bytes[15] == 'R')
    {
        const std::uint32_t headerW = readBe32(bytes.data() + 16);
        const std::uint32_t headerH = readBe32(bytes.data() + 20);
        const unsigned bitDepth = bytes[24];
        const unsigned colorType = bytes[25];
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png header size=%ux%u bit=%u color=%u path=%s",
                                          headerW,
                                          headerH,
                                          bitDepth,
                                          colorType,
                                          openedPath.c_str());
    }
    else
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png header invalid path=%s", openedPath.c_str());
        return false;
    }

    int comp = 0;
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png decode file begin path=%s", openedPath.c_str());
    unsigned char* pixels = stbi_load(openedPath.c_str(),
                                      &width,
                                      &height,
                                      &comp,
                                      4);
    if (!pixels || width <= 0 || height <= 0 || width > 4096 || height > 4096)
    {
        if (pixels)
            stbi_image_free(pixels);
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png decode failed path=%s width=%d height=%d reason=%s",
                      openedPath.c_str(),
                      width,
                      height,
                      stbi_failure_reason() ? stbi_failure_reason() : "(null)");
        width = 0;
        height = 0;
        return false;
    }
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png decode ok path=%s size=%dx%d comp=%d",
                  openedPath.c_str(),
                  width,
                  height,
                  comp);

    std::vector<unsigned char> scaled;
    const int decodedW = width;
    const int decodedH = height;
    constexpr int kMaxUploadPixels = beiklive::nds_stub::kScreenWidth * beiklive::nds_stub::kScreenHeight;
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
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png downscale %dx%d -> %dx%d uploadBytes=%u",
                      decodedW,
                      decodedH,
                      width,
                      height,
                      static_cast<unsigned>(scaled.size()));
    }

    const unsigned char* uploadPixels = scaled.empty() ? pixels : scaled.data();
    const std::size_t uploadBytes = static_cast<std::size_t>(width) * height * 4;
    if (uploadBytes > 7 * 1024 * 1024)
    {
        stbi_image_free(pixels);
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png upload rejected path=%s uploadBytes=%u",
                      openedPath.c_str(),
                      static_cast<unsigned>(uploadBytes));
        width = 0;
        height = 0;
        return false;
    }

    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png texture create begin size=%dx%d uploadBytes=%u",
                  width,
                  height,
                  static_cast<unsigned>(uploadBytes));
    texture = Gfx::TextureCreate(static_cast<u32>(width),
                                 static_cast<u32>(height),
                                 DkImageFormat_RGBA8_Unorm);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png texture create ok tex=%u", texture);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png texture upload begin tex=%u stride=%u",
                  texture,
                  static_cast<unsigned>(width * 4));
    Gfx::TextureUpload(texture,
                       0,
                       0,
                       static_cast<u32>(width),
                       static_cast<u32>(height),
                       const_cast<unsigned char*>(uploadPixels),
                       static_cast<u32>(width * 4));
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: png texture upload queued tex=%u", texture);
    stbi_image_free(pixels);
    return texture != 0;
}

int loadStateSlotTexturesStep(std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>& slots, int maxUploads)
{
    int uploads = 0;
    for (int i = 0; i < static_cast<int>(slots.size()) && uploads < maxUploads; ++i)
    {
        if (loadStateSlotTexture(slots[i], i))
            ++uploads;
    }
    return uploads;
}

std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>
reloadStateSlots(const std::string& stateDir,
                 const std::string& romPath,
                 std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>& oldSlots)
{
    releaseStateSlotTextures(oldSlots);
    return loadStateSlots(stateDir, romPath);
}

bool writeScreenshot(const beiklive::nds_stub::NdsGameLayer& gameLayer, const std::string& saveDir)
{
    std::error_code ec;
    std::filesystem::create_directories(saveDir, ec);
    if (ec)
        return false;

    std::filesystem::path out = std::filesystem::path(saveDir) / ("screenshot_" + timestampString() + ".png");
    for (int suffix = 1; std::filesystem::exists(out, ec) && suffix < 1000; ++suffix)
    {
        ec.clear();
        out = std::filesystem::path(saveDir) /
            ("screenshot_" + timestampString() + "_" + std::to_string(suffix) + ".png");
    }

    return captureAndWritePng(gameLayer, out.string());
}

bool saveStateFile(const std::string& path)
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    Savestate state(path.c_str(), true);
    if (state.Error)
        return false;
    return NDS::DoSavestate(&state) && !state.Error;
}

bool loadStateFile(const std::string& path)
{
    if (!std::filesystem::exists(path))
        return false;
    Savestate state(path.c_str(), false);
    if (state.Error)
        return false;
    return NDS::DoSavestate(&state) && !state.Error;
}

bool fileExists(const char* path)
{
    FILE* fp = std::fopen(path, "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

long long elapsedMs(std::chrono::steady_clock::time_point begin)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - begin).count();
}

bool touchScreenPressed()
{
    HidTouchScreenState state {};
    return hidGetTouchScreenStates(&state, 1) && state.count > 0;
}

void feedMicSilence()
{
    NDS::MicInputFrame(nullptr, 0);
}

void feedMicBlow()
{
    static int samplePos = 0;
    constexpr int kMicFrameSamples = 735;
    s16 samples[kMicFrameSamples] {};
    const int sampleCount = static_cast<int>(sizeof(mic_blow) / sizeof(mic_blow[0]));
    for (int i = 0; i < kMicFrameSamples; ++i)
    {
        samples[i] = static_cast<s16>(mic_blow[samplePos]);
        samplePos = (samplePos + 1) % sampleCount;
    }
    NDS::MicInputFrame(samples, kMicFrameSamples);
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

std::string upper(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
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

struct InputCombo {
    u64 hid = 0;
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
    u64 held = 0;
    u64 down = 0;
    std::uint32_t virtualHeld = 0;
    std::uint32_t virtualDown = 0;
    HidAnalogStickState leftStick {};
    HidAnalogStickState rightStick {};
};

u64 hidFromToken(const std::string& token)
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
    if (!comboHeld(combo, input))
        return false;
    return ((input.down & combo.hid) != 0) || ((input.virtualDown & combo.virtualBits) != 0);
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
        if (stick.y > kStickThreshold)
            input.virtualHeld |= up;
        if (stick.y < -kStickThreshold)
            input.virtualHeld |= down;
        if (stick.x < -kStickThreshold)
            input.virtualHeld |= left;
        if (stick.x > kStickThreshold)
            input.virtualHeld |= right;
    };

    addStick(input.leftStick, VirtualLeftStickUp, VirtualLeftStickDown,
             VirtualLeftStickLeft, VirtualLeftStickRight);
    addStick(input.rightStick, VirtualRightStickUp, VirtualRightStickDown,
             VirtualRightStickLeft, VirtualRightStickRight);
    input.virtualDown = input.virtualHeld & ~previousVirtualHeld;
    previousVirtualHeld = input.virtualHeld;
    return input;
}

class NdsInputConfig {
public:
    struct ButtonBinding {
        const char* key;
        uint32_t ndsBit;
        const char* fallback;
    };

    void load()
    {
        constexpr const char* paths[] = {
            "sdmc:/GBAStation/config/config.cfg",
            "/GBAStation/config/config.cfg",
        };

        for (const char* path : paths)
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
                std::string key = trim(line.substr(0, eq));
                if (key.rfind("nds.", 0) != 0 &&
                    key.rfind("fastforward.", 0) != 0 &&
                    key.rfind("save.", 0) != 0 &&
                    key.rfind("display.", 0) != 0 &&
                    key != "turbo.rate")
                    continue;

                m_values[key] = configValuePayload(line.substr(eq + 1));
            }

            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: input config loaded path=%s keys=%u",
                                             path,
                                             static_cast<unsigned>(m_values.size()));
            m_loadedPath = path;
            break;
        }

        buildMappings();
    }

    const std::vector<InputCombo>& button(const char* key) const
    {
        static const std::vector<InputCombo> empty;
        auto it = m_comboValues.find(key);
        return it == m_comboValues.end() ? empty : it->second;
    }

    std::string value(const std::string& key, const std::string& fallback) const
    {
        auto it = m_values.find(key);
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

    void setValue(const std::string& key, const std::string& value)
    {
        m_values[key] = value;
        buildMappings();
    }

    bool saveValue(const std::string& key, const std::string& type, const std::string& value)
    {
        setValue(key, value);
        std::string path = m_loadedPath.empty() ? "sdmc:/GBAStation/config/config.cfg" : m_loadedPath;
        std::vector<std::string> lines;
        {
            std::ifstream in(path);
            std::string line;
            while (std::getline(in, line))
                lines.push_back(line);
        }

        bool replaced = false;
        const std::string prefix = key + "=";
        const std::string encoded = key + "=" + type + "|" + value;
        for (std::string& line : lines)
        {
            if (line.rfind(prefix, 0) == 0)
            {
                line = encoded;
                replaced = true;
                break;
            }
        }
        if (!replaced)
            lines.push_back(encoded);

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        std::ofstream out(path, std::ios::trunc);
        if (!out)
            return false;
        for (const std::string& line : lines)
            out << line << '\n';
        return out.good();
    }

    bool fastForwardEnabled() const { return intValue("fastforward.enabled", 1) != 0; }
    bool fastForwardToggleMode() const { return value("fastforward.mode", "hold") == "toggle"; }
    bool fastForwardMute() const { return intValue("fastforward.mute", 1) != 0; }
    float fastForwardMultiplier() const
    {
        return std::clamp(floatValue("fastforward.multiplier", 4.0f), 0.1f, 5.0f);
    }

    int turboIntervalFrames() const
    {
        float turboHz = std::clamp(floatValue("turbo.rate", 10.0f), 1.0f, 30.0f);
        return std::max(1, static_cast<int>(60.0f / (turboHz * 2.0f)));
    }

    uint32_t dsKeyMask(const InputSnapshot& input, bool turboAOn, bool turboBOn) const
    {
        uint32_t mask = 0x0FFFu;
        for (const auto& binding : kButtonBindings)
        {
            if (anyComboHeld(button(binding.key), input))
                mask &= ~binding.ndsBit;
        }
        if (turboAOn)
            mask &= ~kNdsKeyA;
        if (turboBOn)
            mask &= ~kNdsKeyB;
        return mask;
    }

private:
    void buildMappings()
    {
        for (const auto& binding : kButtonBindings)
            m_comboValues[binding.key] = parseCombos(value(binding.key, binding.fallback));

        const std::pair<const char*, const char*> hotkeys[] = {
            {"nds.handle.fastforward", "PAD_LSB"},
            {"nds.handle.a_turbo", "none"},
            {"nds.handle.b_turbo", "none"},
            {"nds.hotkey.menu.pad", "PAD_LT+PAD_RT"},
            {"nds.hotkey.quicksave.pad", "none"},
            {"nds.hotkey.quickload.pad", "none"},
            {"nds.hotkey.screenshot.pad", "none"},
            {"nds.hotkey.mute.pad", "none"},
            {"nds.hotkey.pause.pad", "none"},
            {"nds.hotkey.pointer_mode.pad", "none"},
            {"nds.hotkey.pointer_click.pad", "PAD_RT"},
            {"nds.hotkey.swap_screens.pad", "none"},
            {"nds.hotkey.mic_blow.pad", "none"},
            {"nds.layout.next", "none"},
            {"nds.pointer.touch", "none"},
        };
        for (const auto& hotkey : hotkeys)
            m_comboValues[hotkey.first] = parseCombos(value(hotkey.first, hotkey.second));
    }

    static constexpr ButtonBinding kButtonBindings[] = {
        {"nds.handle.a", kNdsKeyA, "PAD_A"},
        {"nds.handle.b", kNdsKeyB, "PAD_B"},
        {"nds.handle.select", kNdsKeySelect, "PAD_BACK"},
        {"nds.handle.start", kNdsKeyStart, "PAD_START"},
        {"nds.handle.right", kNdsKeyRight, "PAD_RIGHT"},
        {"nds.handle.left", kNdsKeyLeft, "PAD_LEFT"},
        {"nds.handle.up", kNdsKeyUp, "PAD_UP"},
        {"nds.handle.down", kNdsKeyDown, "PAD_DOWN"},
        {"nds.handle.r", kNdsKeyR, "PAD_RB"},
        {"nds.handle.l", kNdsKeyL, "PAD_LB"},
        {"nds.handle.x", kNdsKeyX, "PAD_X"},
        {"nds.handle.y", kNdsKeyY, "PAD_Y"},
    };

    std::map<std::string, std::string> m_values;
    std::map<std::string, std::vector<InputCombo>> m_comboValues;
    std::string m_loadedPath;
};

constexpr NdsInputConfig::ButtonBinding NdsInputConfig::kButtonBindings[];

std::string resolveStateDir(const beiklive::nds_stub::DekoRunOptions& options, const NdsInputConfig& inputConfig)
{
    std::string dir = options.savePath;
    if (dir.empty())
        dir = inputConfig.value("save.stateDir", "");
    if (dir.empty())
        dir = joinPath("sdmc:/GBAStation/states/NDS", pathStem(options.romPath));
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

uint32_t dsKeyMaskFromPad(const PadState& pad)
{
    uint32_t mask = 0x0FFFu;
    const u64 buttons = padGetButtons(&pad);

    auto press = [&](u64 hid, uint32_t ndsBit) {
        if (buttons & hid)
            mask &= ~ndsBit;
    };

    press(HidNpadButton_A, kNdsKeyA);
    press(HidNpadButton_B, kNdsKeyB);
    press(HidNpadButton_X, kNdsKeyX);
    press(HidNpadButton_Y, kNdsKeyY);
    press(HidNpadButton_L, kNdsKeyL);
    press(HidNpadButton_R, kNdsKeyR);
    press(HidNpadButton_ZL, kNdsKeyL);
    press(HidNpadButton_Plus, kNdsKeyStart);
    press(HidNpadButton_Minus, kNdsKeySelect);
    press(HidNpadButton_StickL, kNdsKeySelect);
    press(HidNpadButton_AnyUp, kNdsKeyUp);
    press(HidNpadButton_AnyDown, kNdsKeyDown);
    press(HidNpadButton_AnyLeft, kNdsKeyLeft);
    press(HidNpadButton_AnyRight, kNdsKeyRight);
    return mask;
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

    const Result rc = envSetNextLoad(returnNro.c_str(), quoted.c_str());
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko envSetNextLoad rc=0x%x path=%s",
                                      rc,
                                      returnNro.c_str());
    return R_SUCCEEDED(rc);
}

void configureArcDelta()
{
    std::strncpy(Config::BIOS9Path, "sdmc:/GBAStation/bios/nds/bios9.bin", sizeof(Config::BIOS9Path) - 1);
    std::strncpy(Config::BIOS7Path, "sdmc:/GBAStation/bios/nds/bios7.bin", sizeof(Config::BIOS7Path) - 1);
    std::strncpy(Config::FirmwarePath, "sdmc:/GBAStation/bios/nds/firmware.bin", sizeof(Config::FirmwarePath) - 1);
    Config::DLDIEnable = 0;
    Config::RandomizeMAC = 0;

#ifdef JIT_ENABLED
    Config::JIT_Enable = 1;
    Config::JIT_MaxBlockSize = 32;
    Config::JIT_BranchOptimisations = 1;
    Config::JIT_LiteralOptimisations = 1;
    Config::JIT_FastMemory = 1;
#endif

    Config::ConsoleType = 0;
    Config::DirectBoot = 1;
}

class DekoAudioOutput {
public:
    bool start()
    {
        if (m_running.load(std::memory_order_acquire))
            return true;

        const AudioRendererConfig config = {
            .output_rate = AudioRendererOutputRate_48kHz,
            .num_voices = 4,
            .num_effects = 0,
            .num_sinks = 1,
            .num_mix_objs = 1,
            .num_mix_buffers = 2,
        };

        Result rc = audrenInitialize(&config);
        if (R_FAILED(rc))
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrenInitialize failed rc=0x%x", rc);
            return false;
        }

        rc = audrvCreate(&m_driver, &config, 2);
        if (R_FAILED(rc))
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrvCreate failed rc=0x%x", rc);
            audrenExit();
            return false;
        }

        m_memPool = std::aligned_alloc(AUDREN_MEMPOOL_ALIGNMENT, kPoolBytes);
        if (!m_memPool)
        {
            audrvClose(&m_driver);
            audrenExit();
            return false;
        }
        std::memset(m_memPool, 0, kPoolBytes);

        m_memPoolId = audrvMemPoolAdd(&m_driver, m_memPool, kPoolBytes);
        audrvMemPoolAttach(&m_driver, m_memPoolId);

        static const u8 sinkChannels[] = {0, 1};
        audrvDeviceSinkAdd(&m_driver, AUDREN_DEFAULT_DEVICE_NAME, 2, sinkChannels);
        audrvUpdate(&m_driver);

        rc = audrenStartAudioRenderer();
        if (R_FAILED(rc))
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrenStartAudioRenderer rc=0x%x", rc);

        if (!audrvVoiceInit(&m_driver, 0, 2, PcmFormat_Int16, kInputSampleRate))
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrvVoiceInit failed");

        audrvVoiceSetDestinationMix(&m_driver, 0, AUDREN_FINAL_MIX_ID);
        audrvVoiceSetMixFactor(&m_driver, 0, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&m_driver, 0, 1.0f, 1, 1);
        audrvVoiceStart(&m_driver, 0);

        m_running.store(true, std::memory_order_release);
        m_thread = std::thread(&DekoAudioOutput::threadMain, this);
        return true;
    }

    void stop()
    {
        if (m_running.exchange(false, std::memory_order_acq_rel))
        {
            if (m_thread.joinable())
                m_thread.join();
        }

        audrvClose(&m_driver);
        audrenExit();

        if (m_memPool)
        {
            std::free(m_memPool);
            m_memPool = nullptr;
        }
    }

    void pauseForCoreReset()
    {
        m_paused.store(true, std::memory_order_release);

        // Wait until any in-flight SPU::ReadOutput() call has left the melonDS
        // audio buffer before NDS::LoadROM()/NDS::Reset() reinitializes SPU.
        std::lock_guard<std::mutex> lock(m_spuReadMutex);
    }

    void resumeAfterCoreReset()
    {
        m_paused.store(false, std::memory_order_release);
    }

    void setFastForwardActive(bool enabled)
    {
        if (m_fastForwardAudio.exchange(enabled, std::memory_order_acq_rel) == enabled)
            return;

        std::lock_guard<std::mutex> lock(m_spuReadMutex);
        if (enabled)
        {
            SPU::TrimOutput();
        }
        else
        {
            SPU::DrainOutput();
        }
    }

    void setMuted(bool enabled)
    {
        m_muted.store(enabled, std::memory_order_release);
    }

    void push(const int16_t* samples, size_t stereoFrames)
    {
        (void)samples;
        (void)stereoFrames;
    }

private:
    static constexpr int kInputSampleRate = 32823;
    static constexpr size_t kBufferFrames = 768;
    static constexpr size_t kBufferCount = 2;
    static constexpr size_t kBufferBytes = kBufferFrames * 2 * sizeof(int16_t);
    static constexpr size_t kPoolBytes = (kBufferBytes * kBufferCount + (AUDREN_MEMPOOL_ALIGNMENT - 1)) &
                                         ~(AUDREN_MEMPOOL_ALIGNMENT - 1);

    void threadMain()
    {
        std::array<AudioDriverWaveBuf, kBufferCount> buffers {};
        for (size_t i = 0; i < kBufferCount; ++i)
        {
            buffers[i].data_pcm16 = static_cast<int16_t*>(m_memPool);
            buffers[i].size = kBufferBytes;
            buffers[i].start_sample_offset = static_cast<u32>(i * kBufferFrames);
            buffers[i].end_sample_offset = static_cast<u32>((i + 1) * kBufferFrames);
        }

        while (m_running.load(std::memory_order_acquire))
        {
            if (m_paused.load(std::memory_order_acquire))
            {
                svcSleepThread(1000000);
                continue;
            }

            AudioDriverWaveBuf* refill = nullptr;
            for (auto& buffer : buffers)
            {
                if (buffer.state == AudioDriverWaveBufState_Free ||
                    buffer.state == AudioDriverWaveBufState_Done)
                {
                    refill = &buffer;
                    break;
                }
            }

            if (refill)
            {
                auto* data = static_cast<int16_t*>(m_memPool) + refill->start_sample_offset * 2;

                int frames = 0;
                while (m_running.load(std::memory_order_acquire) &&
                       !m_paused.load(std::memory_order_acquire))
                {
                    {
                        std::lock_guard<std::mutex> lock(m_spuReadMutex);
                        if (m_fastForwardAudio.load(std::memory_order_acquire))
                            SPU::Sync(false);
                        frames = SPU::ReadOutput(data, static_cast<int>(kBufferFrames));
                    }
                    if (frames > 0)
                        break;
                    svcSleepThread(10000);
                }

                if (frames > 0)
                {
                    if (m_muted.load(std::memory_order_acquire))
                        std::memset(data, 0, frames * 2 * sizeof(int16_t));

                    const u32 lastStereo = reinterpret_cast<u32*>(data)[frames - 1];
                    while (frames < static_cast<int>(kBufferFrames))
                        reinterpret_cast<u32*>(data)[frames++] = lastStereo;

                    armDCacheFlush(data, frames * 2 * sizeof(int16_t));
                    refill->end_sample_offset = refill->start_sample_offset + frames;
                    audrvVoiceAddWaveBuf(&m_driver, 0, refill);
                    audrvVoiceStart(&m_driver, 0);
                }
            }

            audrvUpdate(&m_driver);
            audrenWaitFrame();
        }
    }

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_fastForwardAudio{false};
    std::atomic<bool> m_muted{false};
    std::mutex m_spuReadMutex;
    std::thread m_thread;
    AudioDriver m_driver {};
    void* m_memPool = nullptr;
    int m_memPoolId = -1;
};

} // namespace

namespace Platform {

void Init(int, char**) {}
void DeInit() {}
void StopEmu() {}

FILE* OpenFile(const char* path, const char* mode, bool mustexist)
{
    if (mustexist)
    {
        FILE* fp = std::fopen(path, "rb");
        if (!fp)
            return nullptr;
        return std::freopen(path, mode, fp);
    }
    return std::fopen(path, mode);
}

FILE* OpenLocalFile(const char* path, const char* mode)
{
    if (!path || !path[0])
        return nullptr;

    if (std::strncmp(path, "sdmc:/", 6) == 0 || path[0] == '/')
        return std::fopen(path, mode);

    char finalPath[1024];
    std::snprintf(finalPath, sizeof(finalPath), "sdmc:/GBAStation/bios/nds/%s", path);
    FILE* fp = std::fopen(finalPath, mode);
    if (fp)
        return fp;

    std::snprintf(finalPath, sizeof(finalPath), "/GBAStation/bios/nds/%s", path);
    fp = std::fopen(finalPath, mode);
    if (fp)
        return fp;

    return std::fopen(path, mode);
}

FILE* OpenDataFile(const char* path)
{
    return OpenLocalFile(path, "rb");
}

void Sleep(u64 usecs)
{
    svcSleepThread(usecs * 1000);
}

struct ThreadEntryData {
    std::function<void()> entryPoint;
};

void ThreadEntry(void* param)
{
    ThreadEntryData* data = static_cast<ThreadEntryData*>(param);
    data->entryPoint();
    delete data;
}

Thread* Thread_Create(std::function<void()> func)
{
    ::Thread* thread = new ::Thread();
    threadCreate(thread, ThreadEntry, new ThreadEntryData{std::move(func)}, nullptr, 1024 * 1024 * 2, 0x30, -2);
    threadStart(thread);
    return reinterpret_cast<Thread*>(thread);
}

void Thread_Free(Thread* thread)
{
    threadClose(reinterpret_cast<::Thread*>(thread));
    delete reinterpret_cast<::Thread*>(thread);
}

void Thread_Wait(Thread* thread)
{
    threadWaitForExit(reinterpret_cast<::Thread*>(thread));
}

struct MySemaphore {
    ::CondVar condvar;
    ::Mutex mutex;
    u64 count;
};

Semaphore* Semaphore_Create()
{
    MySemaphore* sema = new MySemaphore();
    sema->count = 0;
    mutexInit(&sema->mutex);
    condvarInit(&sema->condvar);
    return reinterpret_cast<Semaphore*>(sema);
}

void Semaphore_Free(Semaphore* sema)
{
    delete reinterpret_cast<MySemaphore*>(sema);
}

void Semaphore_Reset(Semaphore* sema)
{
    MySemaphore* s = reinterpret_cast<MySemaphore*>(sema);
    mutexLock(&s->mutex);
    s->count = 0;
    mutexUnlock(&s->mutex);
}

void Semaphore_Wait(Semaphore* sema)
{
    MySemaphore* s = reinterpret_cast<MySemaphore*>(sema);
    mutexLock(&s->mutex);
    while (s->count == 0)
        condvarWait(&s->condvar, &s->mutex);
    --s->count;
    mutexUnlock(&s->mutex);
}

void Semaphore_Post(Semaphore* sema, int count)
{
    if (count <= 0)
        return;
    MySemaphore* s = reinterpret_cast<MySemaphore*>(sema);
    mutexLock(&s->mutex);
    s->count += static_cast<u64>(count);
    mutexUnlock(&s->mutex);
    condvarWake(&s->condvar, count);
}

Mutex* Mutex_Create()
{
    ::Mutex* mutex = new ::Mutex();
    mutexInit(mutex);
    return reinterpret_cast<Mutex*>(mutex);
}

void Mutex_Free(Mutex* mutex)
{
    delete reinterpret_cast<::Mutex*>(mutex);
}

void Mutex_Lock(Mutex* mutex)
{
    mutexLock(reinterpret_cast<::Mutex*>(mutex));
}

void Mutex_Unlock(Mutex* mutex)
{
    mutexUnlock(reinterpret_cast<::Mutex*>(mutex));
}

bool Mutex_TryLock(Mutex* mutex)
{
    return mutexTryLock(reinterpret_cast<::Mutex*>(mutex));
}

bool MP_Init() { return false; }
void MP_DeInit() {}
int MP_SendPacket(u8*, int) { return 0; }
int MP_RecvPacket(u8*, bool) { return 0; }
bool LAN_Init() { return false; }
void LAN_DeInit() {}
int LAN_SendPacket(u8*, int) { return 0; }
int LAN_RecvPacket(u8*) { return 0; }

} // namespace Platform

namespace beiklive::nds_stub {

bool ShouldUseDekoRuntime()
{
    if (fileExists("sdmc:/GBAStation/config/nds_stub_software.flag") ||
        fileExists("/GBAStation/config/nds_stub_software.flag"))
    {
        appendStubLog("GBAStationNDSStub: Deko runtime disabled by nds_stub_software.flag");
        return false;
    }
    return true;
}

int RunDekoRuntime(const DekoRunOptions& options)
{
    appendStubLog("GBAStationNDSStub: Deko runtime start rom=%s", options.romPath.c_str());
    if (options.romPath.empty())
        return 1;

    NdsPlayStats playStats {};
    int sessionPlaySeconds = 0;
    double playTimeFraction = 0.0;
    auto playTimeLast = std::chrono::steady_clock::now();
    constexpr double kPlayTimeSuspendGapSec = 5.0;

    if (!fileExists("sdmc:/GBAStation/bios/nds/bios9.bin") ||
        !fileExists("sdmc:/GBAStation/bios/nds/bios7.bin") ||
        !fileExists("sdmc:/GBAStation/bios/nds/firmware.bin"))
    {
        appendStubLog("GBAStationNDSStub: Deko runtime missing DS BIOS/firmware");
        return 1;
    }

    NdsInputConfig inputConfig;
    inputConfig.load();
    const std::string savePath = resolveSavePath(options);
    const std::string stateDir = resolveStateDir(options, inputConfig);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    hidInitializeTouchScreen();
    PadState pad;
    padInitializeDefault(&pad);
    std::uint32_t previousVirtualHeld = 0;

    if (R_FAILED(romfsInit()))
    {
        appendStubLog("GBAStationNDSStub: Deko romfsInit failed");
        return 1;
    }

    auto checkpointBegin = std::chrono::steady_clock::now();
    configureArcDelta();
    appendStubLog("GBAStationNDSStub: Deko checkpoint config done ms=%lld", elapsedMs(checkpointBegin));

    appendStubLog("GBAStationNDSStub: Deko checkpoint Gfx::Init begin");
    checkpointBegin = std::chrono::steady_clock::now();
    Gfx::Init();
    appendStubLog("GBAStationNDSStub: Deko checkpoint Gfx::Init ok ms=%lld", elapsedMs(checkpointBegin));

    appendStubLog("GBAStationNDSStub: Deko checkpoint NDS::Init begin");
    checkpointBegin = std::chrono::steady_clock::now();
    NDS::Init();
    appendStubLog("GBAStationNDSStub: Deko checkpoint NDS::Init ok ms=%lld", elapsedMs(checkpointBegin));
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::InitRenderer begin");
    checkpointBegin = std::chrono::steady_clock::now();
    GPU::InitRenderer(0);
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::InitRenderer ok ms=%lld", elapsedMs(checkpointBegin));
    GPU::RenderSettings settings {true, 1, false};
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::SetRenderSettings begin");
    checkpointBegin = std::chrono::steady_clock::now();
    GPU::SetRenderSettings(0, settings);
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::SetRenderSettings ok ms=%lld", elapsedMs(checkpointBegin));

    auto* deko2d = static_cast<GPU2D::DekoRenderer*>(GPU::GPU2D_Renderer.get());
    NdsGameLayer gameLayer;
    appendStubLog("GBAStationNDSStub: Deko checkpoint gameLayer.init begin renderer=%p", deko2d);
    checkpointBegin = std::chrono::steady_clock::now();
    gameLayer.init(deko2d);
    gameLayer.setWaitForFramebufferReady(false);
    appendStubLog("GBAStationNDSStub: Deko display fence mode=signal-presented-only");
    appendStubLog("GBAStationNDSStub: Deko checkpoint gameLayer.init ok ms=%lld", elapsedMs(checkpointBegin));

    appendStubLog("GBAStationNDSStub: Deko checkpoint LoadROM begin");
    checkpointBegin = std::chrono::steady_clock::now();
    bool loaded = NDS::LoadROM(options.romPath.c_str(), savePath.c_str(), true);
    appendStubLog("GBAStationNDSStub: Deko LoadROM loaded=%d ms=%lld save=%s",
                  loaded ? 1 : 0,
                  elapsedMs(checkpointBegin),
                  savePath.c_str());
    if (loaded)
        playStats = loadAndIncrementNdsPlayCount(options.romPath);

    DekoAudioOutput audio;
    appendStubLog("GBAStationNDSStub: Deko checkpoint audio.start begin");
    checkpointBegin = std::chrono::steady_clock::now();
    const bool audioStarted = audio.start();
    appendStubLog("GBAStationNDSStub: Deko checkpoint audio.start result=%d ms=%lld",
                  audioStarted ? 1 : 0,
                  elapsedMs(checkpointBegin));

    bool running = loaded;
    bool pendingReturn = false;
    NdsMenuLayer menuLayer;
    NdsDisplaySettings initialDisplay {};
    initialDisplay.fastForwardMultiplier = inputConfig.fastForwardMultiplier();
    initialDisplay.linearFiltering = inputConfig.value("display.filter", "nearest") == "linear";
    initialDisplay.integerScale = options.integerScale;
    initialDisplay.layout = layoutIndexFromId(options.screenLayout.empty() ? "hybrid" : options.screenLayout);
    initialDisplay.orientation = orientationIndexFromId(options.screenOrientation.empty() ? "0" : options.screenOrientation);
    initialDisplay.screenGap = std::clamp(options.screenGap, -256, 256);
    initialDisplay.overlayEnabled = options.overlayEnabled;
    initialDisplay.overlayPath = options.overlayPath;
    initialDisplay.shaderEnabled = options.shaderEnabled;
    initialDisplay.ndsShaderType = normalizedNdsShaderType(options.ndsShaderType);
    initialDisplay.shaderParams = loadNdsShaderParams(initialDisplay.ndsShaderType);
    initialDisplay.customLayout = options.customLayout;
    menuLayer.setDisplaySettings(initialDisplay);
    gameLayer.setLinearFiltering(initialDisplay.linearFiltering);
    gameLayer.setIntegerScale(initialDisplay.integerScale);
    gameLayer.setScreenLayout(initialDisplay.layout);
    gameLayer.setOrientation(initialDisplay.orientation);
    gameLayer.setScreenGap(static_cast<float>(initialDisplay.screenGap));
    gameLayer.setCustomLayoutSettings(initialDisplay.customLayout);
    gameLayer.setShaderEnabled(initialDisplay.shaderEnabled);
    gameLayer.setShaderType(initialDisplay.ndsShaderType);
    gameLayer.setShaderParams(ndsShaderParamUniforms(initialDisplay.ndsShaderType, initialDisplay.shaderParams));
    bool appliedOverlayValid = false;
    bool appliedOverlayEnabled = false;
    std::string appliedOverlayPath;
    auto reloadOverlayTexture = [&](const NdsDisplaySettings& display) {
        const bool wantOverlay = display.overlayEnabled && !display.overlayPath.empty();
        const std::string wantPath = wantOverlay ? display.overlayPath : std::string();
        if (appliedOverlayValid &&
            appliedOverlayEnabled == wantOverlay &&
            appliedOverlayPath == wantPath)
        {
            appendStubLog("GBAStationNDSStub: overlay unchanged enabled=%d path=%s",
                          wantOverlay ? 1 : 0,
                          wantPath.c_str());
            return false;
        }

        if (!wantOverlay)
        {
            gameLayer.setOverlayEnabled(false);
            gameLayer.clearOverlayTexture();
            appliedOverlayValid = true;
            appliedOverlayEnabled = false;
            appliedOverlayPath.clear();
            appendStubLog("GBAStationNDSStub: overlay disabled enabled=%d path=%s",
                          display.overlayEnabled ? 1 : 0,
                          display.overlayPath.c_str());
            return true;
        }

        appendStubLog("GBAStationNDSStub: overlay load begin path=%s", wantPath.c_str());
        std::uint32_t texture = 0;
        int width = 0;
        int height = 0;
        if (!loadPngTextureFromFile(wantPath, texture, width, height))
        {
            gameLayer.setOverlayEnabled(false);
            gameLayer.clearOverlayTexture();
            appliedOverlayValid = true;
            appliedOverlayEnabled = false;
            appliedOverlayPath.clear();
            appendStubLog("GBAStationNDSStub: overlay load failed path=%s", wantPath.c_str());
            return true;
        }

        gameLayer.setOverlayTexture(texture, width, height);
        gameLayer.setOverlayEnabled(true);
        appliedOverlayValid = true;
        appliedOverlayEnabled = true;
        appliedOverlayPath = wantPath;
        appendStubLog("GBAStationNDSStub: overlay load ok size=%dx%d", width, height);
        return true;
    };
    auto applyShaderSettings = [&](bool saveConfig) {
        NdsDisplaySettings display = menuLayer.displaySettings();
        const std::string shaderType = normalizedNdsShaderType(display.ndsShaderType);
        if (display.ndsShaderType != shaderType)
            display.ndsShaderType = shaderType;

        const auto defaults = defaultNdsShaderParams(shaderType);
        bool sameShape = display.shaderParams.size() == defaults.size();
        if (sameShape)
        {
            for (std::size_t i = 0; i < defaults.size(); ++i)
            {
                if (display.shaderParams[i].name != defaults[i].name)
                {
                    sameShape = false;
                    break;
                }
            }
        }
        if (!sameShape)
            display.shaderParams = loadNdsShaderParams(shaderType);

        menuLayer.setDisplaySettings(display);
        const NdsDisplaySettings& applied = menuLayer.displaySettings();
        gameLayer.setShaderEnabled(applied.shaderEnabled);
        gameLayer.setShaderType(applied.ndsShaderType);
        gameLayer.setShaderParams(ndsShaderParamUniforms(applied.ndsShaderType, applied.shaderParams));
        if (saveConfig)
            saveNdsShaderParams(applied.ndsShaderType, applied.shaderParams);
    };
    auto refreshMenuFreeze = [&](const char* reason) {
        const bool ok = gameLayer.refreshMenuFreezeTexture();
        appendStubLog("GBAStationNDSStub: menu freeze refresh %s reason=%s",
                      ok ? "ok" : "failed",
                      reason ? reason : "");
        return ok;
    };
    reloadOverlayTexture(initialDisplay);
    applyShaderSettings(false);
    appendStubLog("GBAStationNDSStub: display init filter=%s integer=%d layout=%s orientation=%d gap=%d overlay=%d overlayPath=%s shader=%d shaderType=%s customTop=%.2f/%.1f/%.1f customBottom=%.2f/%.1f/%.1f",
                  initialDisplay.linearFiltering ? "linear" : "nearest",
                  initialDisplay.integerScale ? 1 : 0,
                  layoutIdFromIndex(initialDisplay.layout),
                  initialDisplay.orientation * 90,
                  initialDisplay.screenGap,
                  initialDisplay.overlayEnabled ? 1 : 0,
                  initialDisplay.overlayPath.c_str(),
                  initialDisplay.shaderEnabled ? 1 : 0,
                  initialDisplay.ndsShaderType.c_str(),
                  initialDisplay.customLayout.topScale,
                  initialDisplay.customLayout.topOffsetX,
                  initialDisplay.customLayout.topOffsetY,
                  initialDisplay.customLayout.bottomScale,
                  initialDisplay.customLayout.bottomOffsetX,
                  initialDisplay.customLayout.bottomOffsetY);
    auto stateSlots = loadStateSlots(stateDir, options.romPath);
    menuLayer.setStateSlots(stateSlots);
    NdsCheatLoadResult cheatLoad = LoadUsrCheatDatForRom(options.romPath);
    menuLayer.setCheatItems(cheatLoad.items);
    std::unique_ptr<ARCodeFile> runtimeCheatFile;
    bool cheatApplyPending = false;
    auto applyMenuCheats = [&]() {
        int enabledCount = 0;
        int truncatedCount = 0;
        auto nextCheatFile = buildRuntimeCheatFile(menuLayer.cheatItems(), enabledCount, truncatedCount);
        runtimeCheatFile = std::move(nextCheatFile);
        AREngine::SetCodeFile(enabledCount > 0 ? runtimeCheatFile.get() : nullptr);
        appendStubLog("GBAStationNDSStub: cheats apply enabled=%d truncated=%d source=%s",
                      enabledCount,
                      truncatedCount,
                      cheatLoad.sourcePath.c_str());
    };
    applyMenuCheats();
    double fps = 0.0;
    int fpsFrames = 0;
    uint64_t totalFrames = 0;
    long long lastRunMs = 0;
    auto fpsStart = std::chrono::steady_clock::now();
    bool blockGameInputUntilRelease = false;
    bool lastFastForwardActive = false;
    bool fastForwardToggle = false;
    bool runtimePaused = false;
    bool muted = false;
    bool screensSwapped = false;
    bool micBlowActive = false;
    bool pointerMode = false;
    bool pointerClickHeld = false;
    float pointerX = 128.0f;
    float pointerY = 96.0f;
    auto pointerLastUpdate = std::chrono::steady_clock::now();
    bool turboAHeld = false;
    bool turboBHeld = false;
    bool turboAOn = false;
    bool turboBOn = false;
    int turboFrameCount = 0;
    double fastForwardFrameCredit = 0.0;
    const int turboIntervalFrames = inputConfig.turboIntervalFrames();
    const int autoLoadSlot = inputConfig.intValue("save.autoLoadState0", 0);
    const int autoSaveSlot = inputConfig.intValue("save.autoSaveState", 0);
    const int autoSaveInterval = inputConfig.intValue("save.autoSaveInterval", 0);
    const bool autoSaveOnExit = inputConfig.intValue("save.autoSaveOnExit", 0) != 0;
    auto autoSaveStart = std::chrono::steady_clock::now();
    int pendingMenuSaveSlot = -1;
    int pendingMenuSaveFrames = 0;
    bool exitAutoSavePending = false;
    bool exitAutoSaveDrawn = false;

    auto doSaveState = [&](int slot, bool showToast) {
        const std::string path = statePath(stateDir, options.romPath, slot);
        const std::string thumbPath = stateThumbPath(stateDir, options.romPath, slot);
        std::vector<std::uint8_t> thumbnailRgba;
        int thumbnailWidth = 0;
        int thumbnailHeight = 0;
        appendStubLog("GBAStationNDSStub: savestate save begin slot=%d path=%s", slot, path.c_str());
        const bool thumbnailCaptured = gameLayer.captureCurrentFrameRgba(thumbnailRgba,
                                                                         thumbnailWidth,
                                                                         thumbnailHeight);
        appendStubLog("GBAStationNDSStub: savestate thumbnail capture %s slot=%d size=%dx%d",
                      thumbnailCaptured ? "ok" : "failed",
                      slot,
                      thumbnailWidth,
                      thumbnailHeight);
        if (slot >= 0 && slot < static_cast<int>(stateSlots.size()))
            releaseStateSlotTexture(stateSlots[slot]);
        std::error_code removeEc;
        const bool removedOldState = std::filesystem::remove(path, removeEc);
        const bool removeStateFailed = static_cast<bool>(removeEc);
        removeEc.clear();
        const bool removedOldThumb = std::filesystem::remove(thumbPath, removeEc);
        const bool removeThumbFailed = static_cast<bool>(removeEc);
        appendStubLog("GBAStationNDSStub: savestate old files remove slot=%d state=%d stateErr=%d thumb=%d thumbErr=%d",
                      slot,
                      removedOldState ? 1 : 0,
                      removeStateFailed ? 1 : 0,
                      removedOldThumb ? 1 : 0,
                      removeThumbFailed ? 1 : 0);
        audio.pauseForCoreReset();
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        const bool ok = saveStateFile(path);
        bool thumbOk = false;
        if (ok)
        {
            if (thumbnailCaptured)
            {
                thumbOk = writeStateThumbnailFromRgba(thumbnailRgba,
                                                      thumbnailWidth,
                                                      thumbnailHeight,
                                                      thumbPath);
            }
            else
            {
                thumbOk = captureAndWriteStateThumbnail(gameLayer, thumbPath);
            }
            appendStubLog("GBAStationNDSStub: savestate thumbnail %s slot=%d",
                          thumbOk ? "ok" : "failed",
                          slot);
        }
        NDSCart::FlushSRAMFile();
        audio.resumeAfterCoreReset();
        appendStubLog("GBAStationNDSStub: savestate save %s slot=%d", ok ? "ok" : "failed", slot);
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        if (slot >= 0 && slot < static_cast<int>(stateSlots.size()))
        {
            releaseStateSlotTexture(stateSlots[slot]);
            auto refreshed = loadStateSlots(stateDir, options.romPath);
            stateSlots[slot] = refreshed[slot];
        }
        menuLayer.setStateSlots(stateSlots);
        if (showToast)
            menuLayer.showToast(ok ? "保存状态完成" : "保存状态失败");
        return ok;
    };

    auto doLoadState = [&](int slot, bool showToast) {
        const std::string path = statePath(stateDir, options.romPath, slot);
        appendStubLog("GBAStationNDSStub: savestate load begin slot=%d path=%s", slot, path.c_str());
        NDS::SetKeyMask(0x0FFFu);
        NDS::ReleaseScreen();
        audio.pauseForCoreReset();
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        const bool ok = loadStateFile(path);
        audio.resumeAfterCoreReset();
        appendStubLog("GBAStationNDSStub: savestate load %s slot=%d", ok ? "ok" : "failed", slot);
        if (showToast)
            menuLayer.showToast(ok ? "读取状态完成" : "读取状态失败");
        return ok;
    };

    auto doDeleteState = [&](int slot) {
        const std::string path = statePath(stateDir, options.romPath, slot);
        const std::string thumb = stateThumbPath(stateDir, options.romPath, slot);
        std::error_code ec;
        const bool removedState = std::filesystem::remove(path, ec);
        ec.clear();
        const bool removedThumb = std::filesystem::remove(thumb, ec);
        appendStubLog("GBAStationNDSStub: savestate delete slot=%d state=%d thumb=%d",
                      slot,
                      removedState ? 1 : 0,
                      removedThumb ? 1 : 0);
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        if (slot >= 0 && slot < static_cast<int>(stateSlots.size()))
        {
            releaseStateSlotTexture(stateSlots[slot]);
            auto refreshed = loadStateSlots(stateDir, options.romPath);
            stateSlots[slot] = refreshed[slot];
        }
        menuLayer.setStateSlots(stateSlots);
    };

    if (autoLoadSlot > 0)
        doLoadState(autoLoadSlot - 1, false);

    playTimeLast = std::chrono::steady_clock::now();

    while (appletMainLoop() && running)
    {
        const bool traceFrame = totalFrames < 5;
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu loop begin",
                          static_cast<unsigned long long>(totalFrames));
        const auto frameBegin = std::chrono::steady_clock::now();
        padUpdate(&pad);
        const InputSnapshot input = makeInputSnapshot(pad, previousVirtualHeld);
        const bool touchHeld = touchScreenPressed();

        if (anyComboDown(inputConfig.button("nds.hotkey.menu.pad"), input))
        {
            const bool wasVisible = menuLayer.visible();
            if (!wasVisible)
            {
                refreshMenuFreeze("open");
                menuLayer.open();
                blockGameInputUntilRelease = true;
                appendStubLog("GBAStationNDSStub: menu hotkey toggle visible=%d->%d",
                              wasVisible ? 1 : 0,
                              menuLayer.visible() ? 1 : 0);
            }
            else
            {
                menuLayer.close();
                blockGameInputUntilRelease = true;
                appendStubLog("GBAStationNDSStub: menu hotkey toggle visible=%d->%d",
                              wasVisible ? 1 : 0,
                              menuLayer.visible() ? 1 : 0);
            }
        }

        if (!menuLayer.active())
        {
            if (inputConfig.fastForwardEnabled() &&
                anyComboDown(inputConfig.button("nds.handle.fastforward"), input) &&
                inputConfig.fastForwardToggleMode())
            {
                fastForwardToggle = !fastForwardToggle;
                appendStubLog("GBAStationNDSStub: fastforward toggle=%d", fastForwardToggle ? 1 : 0);
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.pause.pad"), input))
            {
                runtimePaused = !runtimePaused;
                appendStubLog("GBAStationNDSStub: runtime pause=%d", runtimePaused ? 1 : 0);
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.mute.pad"), input))
            {
                muted = !muted;
                appendStubLog("GBAStationNDSStub: mute=%d", muted ? 1 : 0);
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.mic_blow.pad"), input))
            {
                micBlowActive = !micBlowActive;
                appendStubLog("GBAStationNDSStub: mic blow=%d", micBlowActive ? 1 : 0);
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.pointer_mode.pad"), input))
            {
                pointerMode = !pointerMode;
                pointerClickHeld = false;
                pointerLastUpdate = std::chrono::steady_clock::now();
                appendStubLog("GBAStationNDSStub: pointer mode=%d", pointerMode ? 1 : 0);
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.swap_screens.pad"), input))
            {
                screensSwapped = !screensSwapped;
                gameLayer.setScreensSwapped(screensSwapped);
                appendStubLog("GBAStationNDSStub: swap screens=%d", screensSwapped ? 1 : 0);
            }
            if (anyComboDown(inputConfig.button("nds.layout.next"), input))
            {
                NdsDisplaySettings nextDisplay = menuLayer.displaySettings();
                nextDisplay.layout = (nextDisplay.layout + 1) % 8;
                menuLayer.setDisplaySettings(nextDisplay);
                gameLayer.setScreenLayout(nextDisplay.layout);
                saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), false);
                appendStubLog("GBAStationNDSStub: layout next=%s", layoutIdFromIndex(nextDisplay.layout));
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.quicksave.pad"), input))
                doSaveState(0, true);
            if (anyComboDown(inputConfig.button("nds.hotkey.quickload.pad"), input))
                doLoadState(0, true);
            if (anyComboDown(inputConfig.button("nds.hotkey.screenshot.pad"), input))
            {
                const std::string screenshotDir = options.savePath.empty() ? stateDir : options.savePath;
                Gfx::PresentQueue.waitIdle();
                Gfx::EmuQueue.waitIdle();
                const bool ok = writeScreenshot(gameLayer, screenshotDir);
                appendStubLog("GBAStationNDSStub: screenshot %s dir=%s",
                              ok ? "ok" : "failed",
                              screenshotDir.c_str());
            }
        }

        const bool wasMenuVisible = menuLayer.visible();
        const NdsMenuResult menuResult = menuLayer.update(input.down, input.held);
        const NdsMenuAction menuAction = menuResult.action;
        if (wasMenuVisible != menuLayer.visible())
            blockGameInputUntilRelease = true;

        if (menuAction == NdsMenuAction::SaveState)
        {
            pendingMenuSaveSlot = menuResult.slot;
            pendingMenuSaveFrames = 1;
            gameLayer.requestDeferredCapture();
            menuLayer.close();
            blockGameInputUntilRelease = true;
            appendStubLog("GBAStationNDSStub: savestate menu save deferred slot=%d", menuResult.slot);
        }
        else if (menuAction == NdsMenuAction::LoadState)
        {
            if (doLoadState(menuResult.slot, true))
                menuLayer.close();
        }
        else if (menuAction == NdsMenuAction::DeleteState)
        {
            doDeleteState(menuResult.slot);
        }
        else if (menuAction == NdsMenuAction::CheatSettingsChanged)
        {
            cheatApplyPending = true;
            appendStubLog("GBAStationNDSStub: cheats changed pending apply");
        }
        else if (menuAction == NdsMenuAction::OverlaySettingsChanged)
        {
            if (reloadOverlayTexture(menuLayer.displaySettings()))
                refreshMenuFreeze("overlay_changed");
            appendStubLog("GBAStationNDSStub: overlay settings changed enabled=%d path=%s",
                          menuLayer.displaySettings().overlayEnabled ? 1 : 0,
                          menuLayer.displaySettings().overlayPath.c_str());
        }
        else if (menuAction == NdsMenuAction::OverlayPathSelected)
        {
            if (reloadOverlayTexture(menuLayer.displaySettings()))
                refreshMenuFreeze("overlay_path");
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), false);
            appendStubLog("GBAStationNDSStub: overlay path selected path=%s", menuResult.path.c_str());
        }
        else if (menuAction == NdsMenuAction::OverlaySettingsCommitted)
        {
            if (reloadOverlayTexture(menuLayer.displaySettings()))
                refreshMenuFreeze("overlay_commit");
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), false);
            appendStubLog("GBAStationNDSStub: overlay settings commit enabled=%d path=%s",
                          menuLayer.displaySettings().overlayEnabled ? 1 : 0,
                          menuLayer.displaySettings().overlayPath.c_str());
        }
        else if (menuAction == NdsMenuAction::ShaderSettingsChanged)
        {
            applyShaderSettings(true);
            refreshMenuFreeze("shader_changed");
            appendStubLog("GBAStationNDSStub: shader settings changed enabled=%d type=%s params=%d",
                          menuLayer.displaySettings().shaderEnabled ? 1 : 0,
                          menuLayer.displaySettings().ndsShaderType.c_str(),
                          static_cast<int>(menuLayer.displaySettings().shaderParams.size()));
        }
        else if (menuAction == NdsMenuAction::ShaderSettingsCommitted)
        {
            applyShaderSettings(true);
            refreshMenuFreeze("shader_commit");
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), false);
            appendStubLog("GBAStationNDSStub: shader settings commit enabled=%d type=%s params=%d",
                          menuLayer.displaySettings().shaderEnabled ? 1 : 0,
                          menuLayer.displaySettings().ndsShaderType.c_str(),
                          static_cast<int>(menuLayer.displaySettings().shaderParams.size()));
        }
        else if (menuAction == NdsMenuAction::DisplaySettingsChanged)
        {
            gameLayer.setLinearFiltering(menuLayer.linearFiltering());
            gameLayer.setIntegerScale(menuLayer.integerScale());
            gameLayer.setScreenLayout(menuLayer.screenLayout());
            gameLayer.setOrientation(menuLayer.displaySettings().orientation);
            gameLayer.setScreenGap(static_cast<float>(menuLayer.displaySettings().screenGap));
            gameLayer.setCustomLayoutSettings(menuLayer.customLayoutSettings());
            inputConfig.saveValue("display.filter", "s", menuLayer.linearFiltering() ? "linear" : "nearest");
            inputConfig.saveValue("fastforward.multiplier", "f", std::to_string(menuLayer.fastForwardMultiplier()));
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), false);
            appendStubLog("GBAStationNDSStub: Deko display settings filter=%s ff=%.2f integer=%d layout=%s orientation=%d gap=%d",
                          menuLayer.linearFiltering() ? "linear" : "nearest",
                          menuLayer.fastForwardMultiplier(),
                          menuLayer.integerScale() ? 1 : 0,
                          layoutIdFromIndex(menuLayer.screenLayout()),
                          menuLayer.displaySettings().orientation * 90,
                          menuLayer.displaySettings().screenGap);
        }
        else if (menuAction == NdsMenuAction::SyncDisplaySettings)
        {
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), true);
            const int count = syncNdsDisplaySettingsToGameDb(options.romPath, menuLayer.displaySettings());
            menuLayer.showSyncResult(NdsMenuAction::SyncDisplaySettings, count);
            appendStubLog("GBAStationNDSStub: sync display settings result count=%d", count);
        }
        else if (menuAction == NdsMenuAction::SyncOverlaySettings)
        {
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), false);
            const int count = syncNdsOverlaySettingsToGameDb(options.romPath, menuLayer.displaySettings());
            inputConfig.saveValue("display.overlay.ndsPath", "s", menuLayer.displaySettings().overlayPath);
            inputConfig.saveValue("display.overlay.enabled", "i", menuLayer.displaySettings().overlayEnabled ? "1" : "0");
            menuLayer.showSyncResult(NdsMenuAction::SyncOverlaySettings, count);
            appendStubLog("GBAStationNDSStub: sync overlay settings result count=%d", count);
        }
        else if (menuAction == NdsMenuAction::CustomLayoutChanged)
        {
            NdsDisplaySettings customDisplay = menuLayer.displaySettings();
            customDisplay.layout = 7;
            menuLayer.setDisplaySettings(customDisplay);
            gameLayer.setScreenLayout(7);
            gameLayer.setCustomLayoutSettings(menuLayer.customLayoutSettings());
            appendStubLog("GBAStationNDSStub: custom layout preview top=%.2f/%.1f/%.1f bottom=%.2f/%.1f/%.1f",
                          menuLayer.customLayoutSettings().topScale,
                          menuLayer.customLayoutSettings().topOffsetX,
                          menuLayer.customLayoutSettings().topOffsetY,
                          menuLayer.customLayoutSettings().bottomScale,
                          menuLayer.customLayoutSettings().bottomOffsetX,
                          menuLayer.customLayoutSettings().bottomOffsetY);
        }
        else if (menuAction == NdsMenuAction::CustomLayoutCommitted)
        {
            NdsDisplaySettings customDisplay = menuLayer.displaySettings();
            customDisplay.layout = 7;
            menuLayer.setDisplaySettings(customDisplay);
            gameLayer.setScreenLayout(7);
            gameLayer.setCustomLayoutSettings(menuLayer.customLayoutSettings());
            saveNdsSettingsToGameDb(options.romPath, menuLayer.displaySettings(), true);
            appendStubLog("GBAStationNDSStub: custom layout commit top=%.2f/%.1f/%.1f bottom=%.2f/%.1f/%.1f",
                          menuLayer.customLayoutSettings().topScale,
                          menuLayer.customLayoutSettings().topOffsetX,
                          menuLayer.customLayoutSettings().topOffsetY,
                          menuLayer.customLayoutSettings().bottomScale,
                          menuLayer.customLayoutSettings().bottomOffsetX,
                          menuLayer.customLayoutSettings().bottomOffsetY);
        }
        if (menuAction == NdsMenuAction::ResetGame)
        {
            appendStubLog("GBAStationNDSStub: Deko reset begin");
            menuLayer.close();
            NDS::SetKeyMask(0x0FFFu);
            NDS::ReleaseScreen();

            audio.pauseForCoreReset();
            Gfx::PresentQueue.waitIdle();
            Gfx::EmuQueue.waitIdle();
            NDSCart::FlushSRAMFile();

            loaded = NDS::LoadROM(options.romPath.c_str(), savePath.c_str(), true);
            appendStubLog("GBAStationNDSStub: Deko reset LoadROM loaded=%d", loaded ? 1 : 0);
            audio.resumeAfterCoreReset();

            if (!loaded)
            {
                running = false;
                continue;
            }

            applyMenuCheats();
            fps = 0.0;
            fpsFrames = 0;
            lastRunMs = 0;
            fpsStart = std::chrono::steady_clock::now();
            continue;
        }
        else if (menuAction == NdsMenuAction::ExitGame)
        {
            pendingReturn = true;
            if (autoSaveOnExit && autoSaveSlot > 0 && loaded)
            {
                exitAutoSavePending = true;
                exitAutoSaveDrawn = false;
                menuLayer.close();
                menuLayer.clearToast();
                audio.setFastForwardActive(false);
                lastFastForwardActive = false;
                blockGameInputUntilRelease = true;
                appendStubLog("GBAStationNDSStub: exit autosave pending slot=%d", autoSaveSlot - 1);
            }
            else
            {
                running = false;
            }
        }

        if (cheatApplyPending && !menuLayer.active())
        {
            applyMenuCheats();
            cheatApplyPending = false;
        }

        const bool menuActive = menuLayer.active();
        if (!menuActive && gameLayer.menuFreezeEnabled())
            gameLayer.setMenuFreezeEnabled(false);
        if (blockGameInputUntilRelease && input.held == 0 && input.virtualHeld == 0 && !touchHeld)
            blockGameInputUntilRelease = false;
        const bool suppressGameInput = menuActive || runtimePaused || blockGameInputUntilRelease || exitAutoSavePending;
        const bool fastForwardHeld = inputConfig.fastForwardToggleMode()
            ? fastForwardToggle
            : anyComboHeld(inputConfig.button("nds.handle.fastforward"), input);
        const bool fastForwardActive =
            !suppressGameInput &&
            inputConfig.fastForwardEnabled() &&
            fastForwardHeld &&
            std::fabs(menuLayer.fastForwardMultiplier() - 1.0f) > 0.01f;
        if (fastForwardActive != lastFastForwardActive)
        {
            appendStubLog("GBAStationNDSStub: Deko fastforward %s x%d",
                          fastForwardActive ? "on" : "off",
                          static_cast<int>(std::round(menuLayer.fastForwardMultiplier())));
            audio.setFastForwardActive(fastForwardActive);
            lastFastForwardActive = fastForwardActive;
        }
        audio.setMuted(muted || exitAutoSavePending || (fastForwardActive && inputConfig.fastForwardMute()));

        if (!suppressGameInput)
        {
            turboAHeld = anyComboHeld(inputConfig.button("nds.handle.a_turbo"), input);
            turboBHeld = anyComboHeld(inputConfig.button("nds.handle.b_turbo"), input);
            ++turboFrameCount;
            if (turboFrameCount >= turboIntervalFrames)
            {
                turboFrameCount = 0;
                if (turboAHeld)
                    turboAOn = !turboAOn;
                if (turboBHeld)
                    turboBOn = !turboBOn;
            }
            if (!turboAHeld)
                turboAOn = false;
            if (!turboBHeld)
                turboBOn = false;
        }
        else
        {
            turboAHeld = false;
            turboBHeld = false;
            turboAOn = false;
            turboBOn = false;
        }

        const uint32_t keyMask = suppressGameInput ? 0x0FFFu : inputConfig.dsKeyMask(input, turboAOn, turboBOn);
        NDS::SetKeyMask(keyMask);

        u16 touchX = 0;
        u16 touchY = 0;
        pointerClickHeld = anyComboHeld(inputConfig.button("nds.hotkey.pointer_click.pad"), input) ||
                           anyComboHeld(inputConfig.button("nds.pointer.touch"), input);
        if (pointerMode && !suppressGameInput)
        {
            const auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - pointerLastUpdate).count();
            pointerLastUpdate = now;
            dt = std::clamp(dt, 0.0f, 0.05f);
            constexpr float kStickMax = 32767.0f;
            constexpr float kDeadzone = 0.18f;
            constexpr float kPointerSpeed = 320.0f;
            float sx = static_cast<float>(input.rightStick.x) / kStickMax;
            float sy = static_cast<float>(input.rightStick.y) / kStickMax;
            if (std::fabs(sx) < kDeadzone) sx = 0.0f;
            if (std::fabs(sy) < kDeadzone) sy = 0.0f;
            pointerX = std::clamp(pointerX + sx * kPointerSpeed * dt, 0.0f, 255.0f);
            pointerY = std::clamp(pointerY - sy * kPointerSpeed * dt, 0.0f, 191.0f);
        }

        if (!suppressGameInput && gameLayer.readTouch(touchX, touchY))
            NDS::TouchScreen(touchX, touchY);
        else if (!suppressGameInput && pointerMode && pointerClickHeld)
            NDS::TouchScreen(static_cast<u16>(pointerX + 0.5f), static_cast<u16>(pointerY + 0.5f));
        else
            NDS::ReleaseScreen();

        const auto runBegin = std::chrono::steady_clock::now();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu RunFrame begin",
                          static_cast<unsigned long long>(totalFrames));
        int framesToRun = 1;
        if (fastForwardActive)
        {
            fastForwardFrameCredit += menuLayer.fastForwardMultiplier();
            framesToRun = static_cast<int>(fastForwardFrameCredit);
            fastForwardFrameCredit -= framesToRun;
            if (framesToRun < 0)
                framesToRun = 0;
        }
        else
        {
            fastForwardFrameCredit = 0.0;
        }
        const bool emulationPaused = menuActive || runtimePaused;
        int framesRan = 0;
        for (int i = 0; i < framesToRun && !emulationPaused; ++i)
        {
            if (micBlowActive)
                feedMicBlow();
            else
                feedMicSilence();
            NDS::RunFrame();
            ++framesRan;
            if (fastForwardActive)
                SPU::Sync(false);
        }
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu RunFrame ok",
                          static_cast<unsigned long long>(totalFrames));
        const auto runEnd = std::chrono::steady_clock::now();
        lastRunMs = emulationPaused ? 0 :
            std::chrono::duration_cast<std::chrono::milliseconds>(runEnd - runBegin).count();

        if (!emulationPaused && framesRan > 0)
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
            else if (elapsed > kPlayTimeSuspendGapSec)
            {
                appendStubLog("GBAStationNDSStub: play stats ignored gap %.2fs", elapsed);
            }
        }
        else
        {
            playTimeLast = std::chrono::steady_clock::now();
        }

        if (!emulationPaused && autoSaveSlot > 0 && autoSaveInterval > 0)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - autoSaveStart).count();
            if (elapsed >= autoSaveInterval)
            {
                doSaveState(autoSaveSlot - 1, false);
                autoSaveStart = std::chrono::steady_clock::now();
            }
        }

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::StartFrame begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::StartFrame();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::StartFrame ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PushScissor begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::PushScissor(0, 0, kScreenWidth, kScreenHeight);
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PushScissor ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu gameLayer.drawScreens begin",
                          static_cast<unsigned long long>(totalFrames));
        gameLayer.drawScreens();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu gameLayer.drawScreens ok",
                          static_cast<unsigned long long>(totalFrames));

        if (pointerMode && !menuLayer.active())
        {
            float px = 0.0f;
            float py = 0.0f;
            if (gameLayer.ndsPointToScreen(false, pointerX, pointerY, px, py))
            {
                const Gfx::Color cursorColor = pointerClickHeld
                    ? Gfx::Color{1.0f, 0.92f, 0.35f, 0.95f}
                    : Gfx::Color{0.35f, 0.78f, 1.0f, 0.92f};
                Gfx::DrawRectangle({px - 10.0f, py - 1.5f}, {20.0f, 3.0f}, cursorColor);
                Gfx::DrawRectangle({px - 1.5f, py - 10.0f}, {3.0f, 20.0f}, cursorColor);
            }
        }

        if (!menuLayer.active())
            beiklive::nds_stub::ui::drawGameStatusBadges(fps,
                                                         inputConfig.intValue("display.showFps", 0) != 0,
                                                         fastForwardActive,
                                                         inputConfig.intValue("display.showFfOverlay", 1) != 0,
                                                         runtimePaused);

        if (exitAutoSavePending)
            beiklive::nds_stub::ui::drawBusyDialog("正在自动保存", "保存完毕后将自动退出游戏。", 1.0f);

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu menuLayer.draw begin",
                          static_cast<unsigned long long>(totalFrames));
        menuLayer.draw();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu menuLayer.draw ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PopScissor begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::PopScissor();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PopScissor ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::EndFrame begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::EndFrame({0.015f, 0.020f, 0.026f, 1.0f}, 0);
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::EndFrame ok",
                          static_cast<unsigned long long>(totalFrames));

        if (pendingMenuSaveSlot >= 0 && !menuLayer.active() && !runtimePaused && framesRan > 0)
        {
            const bool cached = gameLayer.refreshCaptureCache();
            appendStubLog("GBAStationNDSStub: savestate deferred capture cache %s slot=%d wait=%d",
                          cached ? "ok" : "failed",
                          pendingMenuSaveSlot,
                          pendingMenuSaveFrames);
            if (pendingMenuSaveFrames > 0)
            {
                --pendingMenuSaveFrames;
                gameLayer.requestDeferredCapture();
            }
            else
            {
                const int slot = pendingMenuSaveSlot;
                pendingMenuSaveSlot = -1;
                pendingMenuSaveFrames = 0;
                doSaveState(slot, true);
            }
        }

        if (exitAutoSavePending && !menuLayer.active() && !runtimePaused && framesRan > 0)
        {
            if (!exitAutoSaveDrawn)
            {
                exitAutoSaveDrawn = true;
                appendStubLog("GBAStationNDSStub: exit autosave notice drawn slot=%d", autoSaveSlot - 1);
            }
            else
            {
                appendStubLog("GBAStationNDSStub: exit autosave begin slot=%d", autoSaveSlot - 1);
                doSaveState(autoSaveSlot - 1, false);
                exitAutoSavePending = false;
                running = false;
                appendStubLog("GBAStationNDSStub: exit autosave done");
            }
        }

        fpsFrames += framesRan;
        ++totalFrames;
        if (totalFrames % 60 == 0)
        {
            appendStubLog("GBAStationNDSStub: Deko heartbeat frame=%llu fps=%.1f run=%lldms ff=%d filter=%s",
                          static_cast<unsigned long long>(totalFrames),
                          fps,
                          lastRunMs,
                          fastForwardActive ? static_cast<int>(std::round(menuLayer.fastForwardMultiplier())) : 1,
                          menuLayer.linearFiltering() ? "linear" : "nearest");
        }
        const auto now = std::chrono::steady_clock::now();
        const auto fpsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsStart).count();
        if (fpsElapsed >= 1000)
        {
            fps = static_cast<double>(fpsFrames) * 1000.0 / static_cast<double>(fpsElapsed);
            fpsFrames = 0;
            fpsStart = now;
        }

        const auto frameEnd = std::chrono::steady_clock::now();
        constexpr auto frameBudget = std::chrono::microseconds(16667);
        const auto used = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameBegin);
        if ((!fastForwardActive || menuLayer.fastForwardMultiplier() < 1.0f) && used < frameBudget)
        {
            const auto sleepUs = std::chrono::duration_cast<std::chrono::microseconds>(frameBudget - used).count();
            if (sleepUs > 500)
                svcSleepThread(static_cast<int64_t>(sleepUs) * 1000);
        }
    }

    if (playStats.found)
    {
        if (playTimeFraction >= 0.5)
            ++sessionPlaySeconds;
        const int totalPlayTime = playStats.playTime + std::max(0, sessionPlaySeconds);
        const std::string lastPlayed = currentLastPlayedTimestamp();
        saveNdsPlayStatsToGameDb(options.romPath, playStats.playCount, totalPlayTime, lastPlayed);
        appendStubLog("GBAStationNDSStub: play stats session seconds=%d total=%d count=%d lastPlayed=%s",
                      sessionPlaySeconds,
                      totalPlayTime,
                      playStats.playCount,
                      lastPlayed.c_str());
    }

    audio.setFastForwardActive(false);
    audio.setMuted(false);
    audio.stop();
    AREngine::SetCodeFile(nullptr);
    runtimeCheatFile.reset();
    NDSCart::FlushSRAMFile();
    releaseStateSlotTextures(stateSlots);
    gameLayer.deinit();
    GPU::DeInitRenderer();
    NDS::DeInit();
    Gfx::DeInit();
    romfsExit();

    if (pendingReturn)
        setReturnNro(options.returnNroPath);

    appendStubLog("GBAStationNDSStub: Deko runtime exit pendingReturn=%d", pendingReturn ? 1 : 0);
    return pendingReturn ? 0 : 1;
}

} // namespace beiklive::nds_stub
