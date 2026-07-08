#include "mgba_stub/MgbaStubRuntime.hpp"

#include "mgba_stub/MgbaMenuLayer.hpp"
#include "mgba_stub/MgbaUiAudio.hpp"
#include "mgba_stub/ui/UiComponents.hpp"

#include <mgba-util/vfs.h>
#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/interface.h>
#include <mgba/core/serialize.h>

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
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <fcntl.h>
#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace {

constexpr unsigned kMaxVideoWidth = 256;
constexpr unsigned kMaxVideoHeight = 224;
constexpr unsigned kScreenWidth = 1280;
constexpr unsigned kScreenHeight = 720;

void appendMgbaStubLog(const char*, ...)
{
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

std::string platformName(int platform)
{
    switch (platform)
    {
    case 1: return "GBA";
    case 2: return "GBC";
    case 3: return "GB";
    default: return "GBA";
    }
}

std::string defaultSaveDir(const beiklive::mgba_stub::RunOptions& options)
{
    return joinPath(joinPath("sdmc:/GBAStation/save", platformName(options.platform)),
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
    slot = std::clamp(slot, 0, 9);
    return joinPath(dir, pathStem(romPath) + ".ss" + std::to_string(slot));
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

void releaseStateSlotTextures(std::array<beiklive::mgba_stub::MgbaStateSlotInfo, 10>& slots)
{
    for (auto& slot : slots)
    {
        if (slot.thumbnailTexture != 0)
        {
            Gfx::TextureDelete(slot.thumbnailTexture);
            slot.thumbnailTexture = 0;
        }
    }
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
    appendMgbaStubLog("GBAStationMgbaStub: envSetNextLoad rc=0x%x path=%s",
                      rc,
                      returnNro.c_str());
    return R_SUCCEEDED(rc);
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
    return makeRGBA8888(
        static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
        static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
        static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
#elif defined(COLOR_16_BIT)
    const uint8_t r5 = static_cast<uint8_t>(px & 0x1F);
    const uint8_t g5 = static_cast<uint8_t>((px >> 5) & 0x1F);
    const uint8_t b5 = static_cast<uint8_t>((px >> 10) & 0x1F);
    return makeRGBA8888(
        static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
        static_cast<uint8_t>((g5 << 3) | (g5 >> 2)),
        static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
#else
    return static_cast<uint32_t>(px) | 0xFF000000u;
#endif
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

bool fileExists(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return static_cast<bool>(file);
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

constexpr const char* kConfigPaths[] = {
    "sdmc:/GBAStation/config/config.cfg",
    "/GBAStation/config/config.cfg",
};

constexpr const char* kGameDataPaths[] = {
    "sdmc:/GBAStation/data/GameData_GBA.json",
    "sdmc:/GBAStation/data/GameData_GBC.json",
    "sdmc:/GBAStation/data/GameData_GB.json",
    "/GBAStation/data/GameData_GBA.json",
    "/GBAStation/data/GameData_GBC.json",
    "/GBAStation/data/GameData_GB.json",
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

float configFloat(const std::map<std::string, std::string>& values,
                  const std::string& key,
                  float fallback)
{
    const auto text = configString(values, key, {});
    if (text.empty())
        return fallback;
    try
    {
        return std::stof(text);
    }
    catch (...)
    {
        return fallback;
    }
}

int configInt(const std::map<std::string, std::string>& values,
              const std::string& key,
              int fallback)
{
    const auto text = configString(values, key, {});
    if (text.empty())
        return fallback;
    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return fallback;
    }
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
        if (eq == std::string::npos)
            continue;
        if (trim(line.substr(0, eq)) == key)
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

int uiLayoutFromConfigMode(const std::string& mode)
{
    if (mode == "fill")
        return 1;
    if (mode == "original")
        return 2;
    if (mode == "four_three" || mode == "4:3")
        return 3;
    if (mode == "integer")
        return 4;
    if (mode == "custom")
        return 5;
    return 0;
}

int uiLayoutFromScreenMode(int mode)
{
    switch (mode)
    {
    case 1: return 1;
    case 2: return 4;
    case 3: return 5;
    case 4: return 3;
    case 0:
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
    case 0:
    case 2:
    default:
        return 0;
    }
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
        catch (...)
        {
        }
    }
    return std::nullopt;
}

bool saveDisplaySettingsToGameData(const std::string& romPath,
                                   const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    const auto dbPath = findGameDataPathForRom(romPath);
    if (!dbPath)
        return false;

    try
    {
        std::ifstream in(*dbPath);
        nlohmann::json data;
        in >> data;
        if (!data.is_array())
            return false;

        const std::string normalizedRom = normalizePathForCompare(romPath);
        bool changed = false;
        for (auto& item : data)
        {
            if (!item.is_object() ||
                normalizePathForCompare(item.value("path", std::string{})) != normalizedRom)
                continue;

            item["displayMode"] = screenModeFromUiLayout(display.layout);
            item["integerAspectRatio"] = static_cast<float>(display.integerScaleMultiplier);
            item["customScale"] = display.customLayout.topScale;
            item["customOffsetX"] = display.customLayout.topOffsetX;
            item["customOffsetY"] = display.customLayout.topOffsetY;
            changed = true;
            break;
        }
        if (!changed)
            return false;

        std::ofstream out(*dbPath, std::ios::trunc);
        if (!out)
            return false;
        out << data.dump(4);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

int syncDisplaySettingsToGameData(const std::string& romPath,
                                  const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    const std::string normalizedRom = normalizePathForCompare(romPath);
    int count = 0;

    for (const char* dbPath : kGameDataPaths)
    {
        if (!fileExists(dbPath))
            continue;

        try
        {
            std::ifstream in(dbPath);
            nlohmann::json data;
            in >> data;
            if (!data.is_array())
                continue;

            bool changed = false;
            for (auto& item : data)
            {
                if (!item.is_object())
                    continue;
                const std::string itemPath = normalizePathForCompare(item.value("path", std::string{}));
                if (itemPath.empty() || itemPath == normalizedRom)
                    continue;

                item["displayMode"] = screenModeFromUiLayout(display.layout);
                item["integerAspectRatio"] = static_cast<float>(display.integerScaleMultiplier);
                item["customScale"] = display.customLayout.topScale;
                item["customOffsetX"] = display.customLayout.topOffsetX;
                item["customOffsetY"] = display.customLayout.topOffsetY;
                changed = true;
                ++count;
            }

            if (!changed)
                continue;

            std::ofstream out(dbPath, std::ios::trunc);
            if (out)
                out << data.dump(4);
        }
        catch (...)
        {
        }
    }

    return count;
}

void saveRuntimeDisplaySettings(const std::string& romPath,
                                const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    writeConfigValue("fastforward.multiplier",
                     "f|" + std::to_string(display.fastForwardMultiplier));
    writeConfigValue("display.filter",
                     std::string("s|") + (display.linearFiltering ? "linear" : "nearest"));
    saveDisplaySettingsToGameData(romPath, display);
}

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

std::string inputPrefixForPlatform(int platform)
{
    switch (platform)
    {
    case 2: return "gbc.";
    case 3: return "gb.";
    case 1:
    default:
        return "";
    }
}

class MgbaInputConfig {
public:
    explicit MgbaInputConfig(int platform)
        : m_prefix(inputPrefixForPlatform(platform))
    {
    }

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
                const std::string key = trim(line.substr(0, eq));
                if (!key.empty())
                    m_values[key] = configValuePayload(line.substr(eq + 1));
            }

            appendMgbaStubLog("GBAStationMgbaStub: input config loaded path=%s keys=%u",
                              path,
                              static_cast<unsigned>(m_values.size()));
            break;
        }

        buildMappings();
    }

    bool menuDown(const InputSnapshot& input) const
    {
        return anyComboDown(button(hotkeyKey("hotkey.menu.pad")), input);
    }

    bool fastForwardHeld(const InputSnapshot& input) const
    {
        return anyComboHeld(button(handleKey("fastforward")), input);
    }

    uint32_t keyMask(const InputSnapshot& input) const
    {
        uint32_t keys = 0;
        for (const auto& binding : kButtonBindings)
        {
            if (anyComboHeld(button(handleKey(binding.suffix)), input))
                keys |= binding.mgbaBit;
        }
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

    std::string handleKey(const char* suffix) const
    {
        return m_prefix + "handle." + suffix;
    }

    std::string hotkeyKey(const char* key) const
    {
        return m_prefix + key;
    }

    void buildMappings()
    {
        for (const auto& binding : kButtonBindings)
        {
            const std::string key = handleKey(binding.suffix);
            m_comboValues[key] = parseCombos(value(key, binding.fallback));
        }

        const std::string menuKey = hotkeyKey("hotkey.menu.pad");
        m_comboValues[menuKey] = parseCombos(value(menuKey, "PAD_LT+PAD_RT"));

        const std::string fastForwardKey = handleKey("fastforward");
        m_comboValues[fastForwardKey] = parseCombos(value(fastForwardKey, "PAD_LSB"));
    }

    static constexpr ButtonBinding kButtonBindings[] = {
        {"a", 1u << 0, "PAD_A"},
        {"b", 1u << 1, "PAD_B"},
        {"select", 1u << 2, "PAD_BACK"},
        {"start", 1u << 3, "PAD_START"},
        {"right", 1u << 4, "PAD_RIGHT"},
        {"left", 1u << 5, "PAD_LEFT"},
        {"up", 1u << 6, "PAD_UP"},
        {"down", 1u << 7, "PAD_DOWN"},
        {"r", 1u << 8, "PAD_RB"},
        {"l", 1u << 9, "PAD_LB"},
    };

    std::string m_prefix;
    std::map<std::string, std::string> m_values;
    std::map<std::string, std::vector<InputCombo>> m_comboValues;
};

class MgbaRuntimeCore {
public:
    ~MgbaRuntimeCore() { release(); }

    bool load(const beiklive::mgba_stub::RunOptions& options, const std::string& savePath)
    {
        release();
        const mPlatform platform = options.platform == 1 ? mPLATFORM_GBA : mPLATFORM_GB;
        m_core = mCoreCreate(platform);
        if (!m_core)
            return false;

        if (!m_core->init(m_core))
        {
            release();
            return false;
        }
        m_coreInitialized = true;

        mCoreInitConfig(m_core, "GBAStationMgbaStub");
        m_configInitialized = true;
        mCoreConfigSetDefaultIntValue(&m_core->config, "useBios", 0);
        mCoreConfigSetDefaultIntValue(&m_core->config, "skipBios", 1);
        mCoreConfigSetDefaultIntValue(&m_core->config, "mute", 1);
        mCoreConfigSetDefaultIntValue(&m_core->config, "volume", 0);
        mCoreConfigSetDefaultIntValue(&m_core->config, "sampleRate", 48000);
        mCoreConfigSetDefaultUIntValue(&m_core->config, "audioBuffers", 1600);
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

    void reset()
    {
        if (m_core)
            m_core->reset(m_core);
    }

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

    bool ready() const { return m_ready; }
    unsigned width() const { return m_width; }
    unsigned height() const { return m_height; }
    double fps() const { return m_fps; }
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

struct DrawRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

DrawRect computeGameDrawRect(unsigned width,
                             unsigned height,
                             const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    DrawRect rect;
    if (width == 0 || height == 0)
        return rect;

    constexpr float screenW = static_cast<float>(kScreenWidth);
    constexpr float screenH = static_cast<float>(kScreenHeight);
    const float gameW = static_cast<float>(width);
    const float gameH = static_cast<float>(height);

    switch (display.layout)
    {
    case 1:
        return {0.0f, 0.0f, screenW, screenH};
    case 2:
        rect.w = gameW;
        rect.h = gameH;
        break;
    case 3:
        rect.h = screenH;
        rect.w = screenH * (4.0f / 3.0f);
        break;
    case 4:
    {
        const float maxScale = std::max(1.0f, std::floor(std::min(screenW / gameW, screenH / gameH)));
        const float scale = display.integerScaleMultiplier > 0
                                ? static_cast<float>(display.integerScaleMultiplier)
                                : maxScale;
        rect.w = gameW * std::max(1.0f, scale);
        rect.h = gameH * std::max(1.0f, scale);
        break;
    }
    case 5:
    {
        const float scale = std::clamp(display.customLayout.topScale, 1.0f, 10.0f);
        rect.w = gameW * scale;
        rect.h = gameH * scale;
        rect.x = (screenW - rect.w) * 0.5f + display.customLayout.topOffsetX;
        rect.y = (screenH - rect.h) * 0.5f + display.customLayout.topOffsetY;
        return rect;
    }
    case 0:
    default:
    {
        const float scale = std::min(screenW / gameW, screenH / gameH);
        rect.w = gameW * scale;
        rect.h = gameH * scale;
        break;
    }
    }

    rect.x = (screenW - rect.w) * 0.5f;
    rect.y = (screenH - rect.h) * 0.5f;
    return rect;
}

void drawGameTexture(uint32_t texture,
                     unsigned width,
                     unsigned height,
                     const beiklive::mgba_stub::MgbaDisplaySettings& display)
{
    if (texture == 0 || width == 0 || height == 0)
        return;

    const DrawRect rect = computeGameDrawRect(width, height, display);

    Gfx::SetSampler((display.linearFiltering ? Gfx::sampler_Linear : Gfx::sampler_Nearest) |
                    Gfx::sampler_ClampToEdge);
    Gfx::DrawRectangle(texture,
                       {rect.x, rect.y},
                       {rect.w, rect.h},
                       {0.0f, 0.0f},
                       {static_cast<float>(width), static_cast<float>(height)},
                       {1.0f, 1.0f, 1.0f, 1.0f});
}

} // namespace

namespace beiklive::mgba_stub {

int RunRuntime(const RunOptions& options)
{
    appendMgbaStubLog("GBAStationMgbaStub: runtime start rom=%s",
                      options.romPath.c_str());
    if (options.romPath.empty())
        return 1;

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    hidInitializeTouchScreen();
    PadState pad;
    padInitializeDefault(&pad);
    std::uint32_t previousVirtualHeld = 0;
    MgbaInputConfig inputConfig(options.platform);
    inputConfig.load();

    if (R_FAILED(romfsInit()))
    {
        appendMgbaStubLog("GBAStationMgbaStub: romfsInit failed");
        return 1;
    }

    Gfx::Init();

    const std::string saveDir = resolveSaveDir(options);
    const std::string savePath = saveFilePath(saveDir, options.romPath);
    const std::string states = stateDir(saveDir);

    MgbaRuntimeCore core;
    bool loaded = core.load(options, savePath);
    bool running = loaded;
    bool pendingReturn = false;

    uint32_t gameTexture = 0;
    if (loaded)
    {
        gameTexture = Gfx::TextureCreate(core.width(), core.height(), DkImageFormat_RGBA8_Unorm);
        appendMgbaStubLog("GBAStationMgbaStub: ROM loaded size=%ux%u save=%s",
                          core.width(),
                          core.height(),
                          savePath.c_str());
    }
    else
    {
        appendMgbaStubLog("GBAStationMgbaStub: ROM load failed");
    }

    MgbaMenuLayer menuLayer;
    MgbaUiAudioPlayer uiAudio;
    menuLayer.setCheatPagePlaceholder(true);
    menuLayer.setDisplayPagePlaceholder(false);
    const auto configValues = loadConfigValues();
    MgbaDisplaySettings displaySettings;
    displaySettings.fastForwardMultiplier = std::clamp(
        configFloat(configValues, "fastforward.multiplier", 4.0f),
        0.1f,
        10.0f);
    displaySettings.linearFiltering = configString(configValues, "display.filter", "nearest") == "linear";
    displaySettings.layout = options.hasDisplaySettings
                                 ? uiLayoutFromScreenMode(options.displayMode)
                                 : uiLayoutFromConfigMode(configString(configValues, "display.mode", "original"));
    displaySettings.integerScaleMultiplier = std::clamp(
        options.hasDisplaySettings
            ? static_cast<int>(std::lround(options.integerAspectRatio))
            : configInt(configValues, "display.integer_scale_mult", 0),
        0,
        8);
    displaySettings.customLayout.topScale = std::clamp(options.customScale, 1.0f, 10.0f);
    displaySettings.customLayout.topOffsetX = std::clamp(options.customOffsetX, -1024.0f, 1024.0f);
    displaySettings.customLayout.topOffsetY = std::clamp(options.customOffsetY, -1024.0f, 1024.0f);
    menuLayer.setDisplaySettings(displaySettings);
    auto stateSlots = loadStateSlots(states, options.romPath);
    menuLayer.setStateSlots(stateSlots);

    double fps = 0.0;
    int fpsFrames = 0;
    uint64_t totalFrames = 0;
    double fastForwardAccumulator = 0.0;
    bool fastForwardActive = false;
    auto fpsStart = std::chrono::steady_clock::now();

    auto refreshSlots = [&]() {
        releaseStateSlotTextures(stateSlots);
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
                uiAudio.play(MgbaMenuSound::Back);
            }
            else
            {
                menuLayer.open();
                uiAudio.play(MgbaMenuSound::Click);
            }
        }

        const MgbaMenuResult result = menuLayer.update(buttonsDown, buttonsHeld);
        for (MgbaMenuSound sound : menuLayer.consumeSounds())
            uiAudio.play(sound);

        switch (result.action)
        {
        case MgbaMenuAction::DisplaySettingsChanged:
        case MgbaMenuAction::CustomLayoutChanged:
        case MgbaMenuAction::CustomLayoutCommitted:
            saveRuntimeDisplaySettings(options.romPath, menuLayer.displaySettings());
            break;
        case MgbaMenuAction::SyncDisplaySettings:
        {
            saveRuntimeDisplaySettings(options.romPath, menuLayer.displaySettings());
            const int count = syncDisplaySettingsToGameData(options.romPath, menuLayer.displaySettings());
            menuLayer.showSyncResult(MgbaMenuAction::SyncDisplaySettings, count);
            break;
        }
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
            pendingReturn = true;
            running = false;
            break;
        default:
            break;
        }

        int framesRan = 0;
        fastForwardActive = false;
        if (!menuLayer.active())
        {
            const uint32_t keyMask = inputConfig.keyMask(input);
            const bool fastForwardHeld = inputConfig.fastForwardHeld(input);
            const float multiplier = fastForwardHeld
                                         ? std::clamp(menuLayer.fastForwardMultiplier(), 0.1f, 10.0f)
                                         : 1.0f;
            fastForwardActive = fastForwardHeld && std::fabs(multiplier - 1.0f) > 0.001f;
            if (!fastForwardHeld)
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
        }
        else
        {
            fastForwardAccumulator = 0.0;
        }

        if (core.captureFrame() && gameTexture != 0)
        {
            Gfx::TextureUpload(gameTexture,
                               0,
                               0,
                               core.width(),
                               core.height(),
                               core.rgbaBuffer().data(),
                               core.width() * sizeof(uint32_t));
        }

        Gfx::StartFrame();
        Gfx::PushScissor(0, 0, kScreenWidth, kScreenHeight);
        drawGameTexture(gameTexture, core.width(), core.height(), menuLayer.displaySettings());
        if (!menuLayer.active())
            beiklive::mgba_stub::ui::drawGameStatusBadges(fps, true, false, true, fastForwardActive);
        menuLayer.draw();
        Gfx::PopScissor();
        Gfx::EndFrame({0.015f, 0.020f, 0.026f, 1.0f}, 0);

        fpsFrames += framesRan;
        ++totalFrames;
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
    uiAudio.stop();
    releaseStateSlotTextures(stateSlots);
    if (gameTexture != 0)
        Gfx::TextureDelete(gameTexture);
    core.release();
    Gfx::DeInit();
    romfsExit();

    if (pendingReturn)
        setReturnNro(options.returnNroPath);

    appendMgbaStubLog("GBAStationMgbaStub: runtime exit pendingReturn=%d",
                      pendingReturn ? 1 : 0);
    return pendingReturn ? 0 : 1;
}

} // namespace beiklive::mgba_stub
