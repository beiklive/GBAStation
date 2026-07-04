#include "nds_stub/NdsDekoRuntime.hpp"

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
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <switch.h>

#ifdef OGLRENDERER_ENABLED
#undef OGLRENDERER_ENABLED
#endif

#include "../../third_party/ArcDelta_melonDS/src/Config.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../third_party/ArcDelta_melonDS/src/NDS.h"
#include "../../third_party/ArcDelta_melonDS/src/NDSCart.h"
#include "../../third_party/ArcDelta_melonDS/src/Platform.h"
#include "../../third_party/ArcDelta_melonDS/src/Savestate.h"
#include "../../third_party/ArcDelta_melonDS/src/SPU.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/PlatformConfig.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"
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

std::string stateThumbCachePath(const std::string& stateDir, const std::string& romPath, int slot)
{
    return statePath(stateDir, romPath, slot) + ".thumb";
}

const char* layoutIdFromIndex(int layout)
{
    static constexpr const char* ids[] = {
        "vertical",
        "horizontal",
        "priority_top",
        "priority_bottom",
        "hybrid",
        "custom",
    };
    return ids[std::clamp(layout, 0, 5)];
}

int layoutIndexFromId(const std::string& layout)
{
    if (layout == "horizontal") return 1;
    if (layout == "priority_top" || layout == "top_priority") return 2;
    if (layout == "priority_bottom" || layout == "bottom_priority") return 3;
    if (layout == "hybrid") return 4;
    if (layout == "custom" || layout == "separate") return 5;
    return 0;
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
        info.thumbnailCachePath = stateThumbCachePath(stateDir, romPath, slot);
        info.exists = std::filesystem::exists(info.statePath);
        info.thumbnailCacheAvailable = std::filesystem::exists(info.thumbnailCachePath);
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

bool writeRgbaThumbCache(const std::string& path,
                         const std::vector<std::uint8_t>& rgba,
                         int width,
                         int height)
{
    if (rgba.empty() || width <= 0 || height <= 0)
        return false;

    const std::uint32_t w = static_cast<std::uint32_t>(width);
    const std::uint32_t h = static_cast<std::uint32_t>(height);
    const std::uint32_t magic = 0x4244544Eu; // "NTDB", little-endian
    const std::uint32_t version = 1;
    const std::uint32_t bytes = static_cast<std::uint32_t>(rgba.size());

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&w), sizeof(w));
    out.write(reinterpret_cast<const char*>(&h), sizeof(h));
    out.write(reinterpret_cast<const char*>(&bytes), sizeof(bytes));
    out.write(reinterpret_cast<const char*>(rgba.data()), static_cast<std::streamsize>(rgba.size()));
    return out.good();
}

bool readRgbaThumbCache(const std::string& path,
                        std::vector<std::uint8_t>& rgba,
                        int& width,
                        int& height)
{
    width = 0;
    height = 0;
    rgba.clear();

    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;

    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t w = 0;
    std::uint32_t h = 0;
    std::uint32_t bytes = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&w), sizeof(w));
    in.read(reinterpret_cast<char*>(&h), sizeof(h));
    in.read(reinterpret_cast<char*>(&bytes), sizeof(bytes));
    if (!in || magic != 0x4244544Eu || version != 1 || w == 0 || h == 0 ||
        w > 1024 || h > 1024 || bytes != w * h * 4)
        return false;

    rgba.resize(bytes);
    in.read(reinterpret_cast<char*>(rgba.data()), static_cast<std::streamsize>(rgba.size()));
    if (!in)
    {
        rgba.clear();
        return false;
    }

    width = static_cast<int>(w);
    height = static_cast<int>(h);
    return true;
}

bool captureAndWriteStateThumbnail(const beiklive::nds_stub::NdsGameLayer& gameLayer,
                                   const std::string& pngPath,
                                   const std::string& cachePath)
{
    std::vector<std::uint8_t> rgba;
    int width = 0;
    int height = 0;
    if (!gameLayer.captureCurrentFrameRgba(rgba, width, height))
        return false;

    const bool cacheOk = writeRgbaThumbCache(cachePath, rgba, width, height);
    const bool pngOk = writeRgbaPng(pngPath, rgba, width, height);
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: savestate thumbnail write cache=%d png=%d size=%dx%d",
                                      cacheOk ? 1 : 0,
                                      pngOk ? 1 : 0,
                                      width,
                                      height);
    return cacheOk;
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

bool hasPendingStateSlotTextures(const std::array<beiklive::nds_stub::NdsStateSlotInfo, 10>& slots)
{
    for (const auto& slot : slots)
    {
        if (slot.exists &&
            slot.thumbnailTexture == 0 &&
            !slot.thumbnailLoadAttempted &&
            !slot.thumbnailCachePath.empty() &&
            std::filesystem::exists(slot.thumbnailCachePath))
            return true;
    }
    return false;
}

bool loadStateSlotTexture(beiklive::nds_stub::NdsStateSlotInfo& slot, int slotIndex)
{
    if (!slot.exists || slot.thumbnailTexture != 0 || slot.thumbnailLoadAttempted ||
        slot.thumbnailCachePath.empty() || !std::filesystem::exists(slot.thumbnailCachePath))
        return false;

    slot.thumbnailLoadAttempted = true;
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: state thumbnail cache load begin slot=%d path=%s",
                                      slotIndex,
                                      slot.thumbnailCachePath.c_str());

    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!readRgbaThumbCache(slot.thumbnailCachePath, pixels, width, height))
    {
        beiklive::nds_stub::appendStubLog("GBAStationNDSStub: state thumbnail cache load failed slot=%d", slotIndex);
        return false;
    }

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
                       pixels.data(),
                       static_cast<u32>(width * 4));
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: state thumbnail upload queued slot=%d size=%dx%d",
                                      slotIndex,
                                      width,
                                      height);
    return true;
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
    initialDisplay.integerScale = inputConfig.intValue("nds.integerScale", 0) != 0;
    initialDisplay.layout = layoutIndexFromId(inputConfig.value("nds.screenLayout", "vertical"));
    initialDisplay.orientation = std::clamp(inputConfig.intValue("nds.screenOrientation", 0) / 90, 0, 3);
    menuLayer.setDisplaySettings(initialDisplay);
    gameLayer.setLinearFiltering(initialDisplay.linearFiltering);
    gameLayer.setIntegerScale(initialDisplay.integerScale);
    gameLayer.setScreenLayout(initialDisplay.layout);
    appendStubLog("GBAStationNDSStub: display init filter=%s integer=%d layout=%s",
                  initialDisplay.linearFiltering ? "linear" : "nearest",
                  initialDisplay.integerScale ? 1 : 0,
                  layoutIdFromIndex(initialDisplay.layout));
    auto stateSlots = loadStateSlots(stateDir, options.romPath);
    menuLayer.setStateSlots(stateSlots);
    bool stateSlotTexturesDirty = true;
    bool stateSlotTextureLoadStarted = false;
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

    auto doSaveState = [&](int slot) {
        const std::string path = statePath(stateDir, options.romPath, slot);
        appendStubLog("GBAStationNDSStub: savestate save begin slot=%d path=%s", slot, path.c_str());
        audio.pauseForCoreReset();
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        const bool ok = saveStateFile(path);
        if (ok)
        {
            const bool thumbOk = captureAndWriteStateThumbnail(gameLayer,
                                                               stateThumbPath(stateDir, options.romPath, slot),
                                                               stateThumbCachePath(stateDir, options.romPath, slot));
            appendStubLog("GBAStationNDSStub: savestate thumbnail %s slot=%d",
                          thumbOk ? "ok" : "failed",
                          slot);
        }
        NDSCart::FlushSRAMFile();
        audio.resumeAfterCoreReset();
        appendStubLog("GBAStationNDSStub: savestate save %s slot=%d", ok ? "ok" : "failed", slot);
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        stateSlots = reloadStateSlots(stateDir, options.romPath, stateSlots);
        stateSlotTexturesDirty = true;
        stateSlotTextureLoadStarted = false;
        menuLayer.setStateSlots(stateSlots);
        return ok;
    };

    auto doLoadState = [&](int slot) {
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
        return ok;
    };

    auto doDeleteState = [&](int slot) {
        const std::string path = statePath(stateDir, options.romPath, slot);
        const std::string thumb = stateThumbPath(stateDir, options.romPath, slot);
        const std::string thumbCache = stateThumbCachePath(stateDir, options.romPath, slot);
        std::error_code ec;
        const bool removedState = std::filesystem::remove(path, ec);
        ec.clear();
        const bool removedThumb = std::filesystem::remove(thumb, ec);
        ec.clear();
        const bool removedThumbCache = std::filesystem::remove(thumbCache, ec);
        appendStubLog("GBAStationNDSStub: savestate delete slot=%d state=%d thumb=%d cache=%d",
                      slot,
                      removedState ? 1 : 0,
                      removedThumb ? 1 : 0,
                      removedThumbCache ? 1 : 0);
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();
        stateSlots = reloadStateSlots(stateDir, options.romPath, stateSlots);
        stateSlotTexturesDirty = true;
        stateSlotTextureLoadStarted = false;
        menuLayer.setStateSlots(stateSlots);
    };

    if (autoLoadSlot > 0)
        doLoadState(autoLoadSlot - 1);

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
            menuLayer.toggle();
            blockGameInputUntilRelease = true;
            appendStubLog("GBAStationNDSStub: menu hotkey toggle visible=%d->%d",
                          wasVisible ? 1 : 0,
                          menuLayer.visible() ? 1 : 0);
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
                nextDisplay.layout = (nextDisplay.layout + 1) % 6;
                menuLayer.setDisplaySettings(nextDisplay);
                gameLayer.setScreenLayout(nextDisplay.layout);
                inputConfig.saveValue("nds.screenLayout", "s", layoutIdFromIndex(nextDisplay.layout));
                appendStubLog("GBAStationNDSStub: layout next=%s", layoutIdFromIndex(nextDisplay.layout));
            }
            if (anyComboDown(inputConfig.button("nds.hotkey.quicksave.pad"), input))
                doSaveState(0);
            if (anyComboDown(inputConfig.button("nds.hotkey.quickload.pad"), input))
                doLoadState(0);
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
            doSaveState(menuResult.slot);
        }
        else if (menuAction == NdsMenuAction::LoadState)
        {
            if (doLoadState(menuResult.slot))
                menuLayer.close();
        }
        else if (menuAction == NdsMenuAction::DeleteState)
        {
            doDeleteState(menuResult.slot);
        }
        else if (menuAction == NdsMenuAction::DisplaySettingsChanged)
        {
            gameLayer.setLinearFiltering(menuLayer.linearFiltering());
            gameLayer.setIntegerScale(menuLayer.integerScale());
            gameLayer.setScreenLayout(menuLayer.screenLayout());
            inputConfig.saveValue("display.filter", "s", menuLayer.linearFiltering() ? "linear" : "nearest");
            inputConfig.saveValue("fastforward.multiplier", "f", std::to_string(menuLayer.fastForwardMultiplier()));
            inputConfig.saveValue("nds.integerScale", "i", menuLayer.integerScale() ? "1" : "0");
            inputConfig.saveValue("nds.screenLayout", "s", layoutIdFromIndex(menuLayer.screenLayout()));
            appendStubLog("GBAStationNDSStub: Deko display settings filter=%s ff=%.2f integer=%d layout=%s",
                          menuLayer.linearFiltering() ? "linear" : "nearest",
                          menuLayer.fastForwardMultiplier(),
                          menuLayer.integerScale() ? 1 : 0,
                          layoutIdFromIndex(menuLayer.screenLayout()));
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

            fps = 0.0;
            fpsFrames = 0;
            lastRunMs = 0;
            fpsStart = std::chrono::steady_clock::now();
            continue;
        }
        else if (menuAction == NdsMenuAction::ExitGame)
        {
            pendingReturn = true;
            running = false;
        }

        if (stateSlotTexturesDirty && menuLayer.visible() && totalFrames > 2)
        {
            if (!stateSlotTextureLoadStarted)
            {
                stateSlotTextureLoadStarted = true;
                appendStubLog("GBAStationNDSStub: state thumbnail lazy load begin");
            }

            const int uploads = loadStateSlotTexturesStep(stateSlots, 1);
            if (uploads > 0)
                menuLayer.setStateSlots(stateSlots);

            if (!hasPendingStateSlotTextures(stateSlots))
            {
                stateSlotTexturesDirty = false;
                appendStubLog("GBAStationNDSStub: state thumbnail lazy load done");
            }
        }

        const bool menuActive = menuLayer.active();
        if (blockGameInputUntilRelease && input.held == 0 && input.virtualHeld == 0 && !touchHeld)
            blockGameInputUntilRelease = false;
        const bool suppressGameInput = menuActive || runtimePaused || blockGameInputUntilRelease;
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
        audio.setMuted(muted || (fastForwardActive && inputConfig.fastForwardMute()));

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

        if (!emulationPaused && autoSaveSlot > 0 && autoSaveInterval > 0)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - autoSaveStart).count();
            if (elapsed >= autoSaveInterval)
            {
                doSaveState(autoSaveSlot - 1);
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
            const RectF pointerRect = screensSwapped ? gameLayer.topRect() : gameLayer.bottomRect();
            const float px = pointerRect.x + (pointerX / 255.0f) * pointerRect.w;
            const float py = pointerRect.y + (pointerY / 191.0f) * pointerRect.h;
            const Gfx::Color cursorColor = pointerClickHeld
                ? Gfx::Color{1.0f, 0.92f, 0.35f, 0.95f}
                : Gfx::Color{0.35f, 0.78f, 1.0f, 0.92f};
            Gfx::DrawRectangle({px - 10.0f, py - 1.5f}, {20.0f, 3.0f}, cursorColor);
            Gfx::DrawRectangle({px - 1.5f, py - 10.0f}, {3.0f, 20.0f}, cursorColor);
        }

        if (!menuLayer.active())
            beiklive::nds_stub::ui::drawGameStatusBadges(fps,
                                                         inputConfig.intValue("display.showFps", 0) != 0,
                                                         fastForwardActive,
                                                         inputConfig.intValue("display.showFfOverlay", 1) != 0,
                                                         runtimePaused);

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

    if (autoSaveOnExit && autoSaveSlot > 0 && loaded)
        doSaveState(autoSaveSlot - 1);

    audio.setFastForwardActive(false);
    audio.setMuted(false);
    audio.stop();
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
