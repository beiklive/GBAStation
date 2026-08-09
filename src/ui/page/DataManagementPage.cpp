#include "DataManagementPage.hpp"
#include "core/Translation.hpp"

#include "ui/page/FileListPage.hpp"
#include "ui/utils/UiHelper.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/widget/DetailCell.hpp"
#include "core/ThreeDsTitlePaths.hpp"
#include "core/Tools.hpp"
#include "network/WebService.h"
#include "third_party/qrcodegen/qrcodegen.hpp"

#ifdef __SWITCH__
#include "platform/switch/NroLauncher.hpp"
#endif

#include <borealis/views/applet_frame.hpp>
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/rectangle.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{

struct ImportItem
{
    std::string romPath;
    std::string label;
};

struct ImportSharedConfig
{
    int platform = -1;
    std::string overlayPath;
    std::string shaderPath;
    bool overlayEnabled = false;
    bool shaderEnabled = false;
};

class QRCodeView : public brls::View
{
public:
    explicit QRCodeView(const std::string& text)
        : qr(qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM))
    {
        setDimensions(240.0f, 240.0f);
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;

        const int quiet = 4;
        const int qrSize = qr.getSize();
        const int modules = qrSize + quiet * 2;
        const float size = std::min(w, h);
        const float cell = std::floor(size / modules);
        if (cell <= 0.0f)
            return;

        const float drawn = cell * modules;
        const float ox = x + (w - drawn) * 0.5f;
        const float oy = y + (h - drawn) * 0.5f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, ox, oy, drawn, drawn, 8.0f);
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgFill(vg);

        nvgFillColor(vg, nvgRGB(18, 24, 32));
        for (int yy = 0; yy < qrSize; yy++)
        {
            for (int xx = 0; xx < qrSize; xx++)
            {
                if (!qr.getModule(xx, yy))
                    continue;
                nvgBeginPath(vg);
                nvgRect(vg, ox + (xx + quiet) * cell, oy + (yy + quiet) * cell, cell, cell);
                nvgFill(vg);
            }
        }
    }

private:
    qrcodegen::QrCode qr;
};

class LplImportConfirmView final : public brls::Box
{
public:
    LplImportConfirmView(std::string fileName, std::string platformName)
        : brls::Box(brls::Axis::COLUMN),
          m_fileName(std::move(fileName)),
          m_platformName(std::move(platformName))
    {
        setDimensions(650.f, 272.f);
        setBackground(brls::ViewBackground::NONE);
        setFocusable(false);
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(
                brls::FONT_MATERIAL_ICONS);

        const float iconX = x + 58.f;
        const float iconY = y + 58.f;
        const NVGpaint iconPaint = nvgLinearGradient(
            vg, iconX - 30.f, iconY - 30.f, iconX + 30.f, iconY + 30.f,
            nvgRGBA(255, 77, 109, 235), nvgRGBA(49, 177, 255, 220));
        nvgBeginPath(vg);
        nvgCircle(vg, iconX, iconY, 31.f);
        nvgFillPaint(vg, iconPaint);
        nvgFill(vg);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 32.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 245));
        nvgText(vg, iconX, iconY + 1.f, "\xEE\x8B\x86", nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 27.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 246));
        nvgText(vg, x + 108.f, y + 43.f,
                L("导入 RetroArch 播放列表").c_str(), nullptr);

        const std::string platformLabel = L("目标平台  ") + m_platformName;
        nvgFontSize(vg, 17.f);
        float badgeBounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, platformLabel.c_str(), nullptr,
                      badgeBounds);
        const float badgeW = badgeBounds[2] - badgeBounds[0] + 26.f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 108.f, y + 69.f, badgeW, 32.f, 7.f);
        nvgFillColor(vg, nvgRGBA(49, 177, 255, 42));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(90, 199, 255, 112));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgFillColor(vg, nvgRGBA(199, 232, 255, 238));
        nvgText(vg, x + 121.f, y + 85.f, platformLabel.c_str(), nullptr);

        const float fileX = x + 28.f;
        const float fileY = y + 124.f;
        const float fileW = w - 56.f;
        const float fileH = 75.f;
        const NVGpaint shadow = nvgBoxGradient(
            vg, fileX + 4.f, fileY + 5.f, fileW, fileH, 8.f, 5.f,
            nvgRGBA(0, 0, 0, 72), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, fileX - 3.f, fileY - 3.f, fileW + 14.f, fileH + 15.f);
        nvgRoundedRect(vg, fileX, fileY, fileW, fileH, 8.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, fileX, fileY, fileW, fileH, 8.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 10));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, fileX + 1.f, fileY + 1.f,
                       fileW - 2.f, fileH - 2.f, 7.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 48));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, fileX + 16.f, fileY + 20.f, 52.f, 35.f, 6.f);
        nvgFillColor(vg, nvgRGBA(255, 77, 109, 50));
        nvgFill(vg);
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 174, 188, 245));
        nvgText(vg, fileX + 42.f, fileY + 38.f, "LPL", nullptr);

        nvgSave(vg);
        nvgIntersectScissor(vg, fileX + 82.f, fileY + 8.f,
                            fileW - 100.f, fileH - 16.f);
        nvgFontSize(vg, 21.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(244, 246, 252, 242));
        nvgText(vg, fileX + 82.f, fileY + fileH * 0.5f,
                m_fileName.c_str(), nullptr);
        nvgRestore(vg);

        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(181, 188, 202, 225));
        nvgText(vg, x + 30.f, y + 229.f,
                L("将读取播放列表并导入有效 ROM，文件中的平台需与目标平台一致。").c_str(), nullptr);
    }

private:
    std::string m_fileName;
    std::string m_platformName;
    int m_defaultFont = -1;
    int m_materialFont = -1;
};

std::string expandTilde(const std::string& path)
{
    if (!path.empty() && path[0] == '~')
    {
        const char* home = nullptr;
#ifdef _WIN32
        home = std::getenv("USERPROFILE");
#else
        home = std::getenv("HOME");
#endif
        if (home)
            return std::string(home) + path.substr(1);
    }
    return path;
}

std::string fileNameFromPath(const std::string& path)
{
    return fs::path(path).filename().string();
}

std::string stemFromPath(const std::string& path)
{
    return fs::path(path).stem().string();
}

std::string parentPath(const std::string& path)
{
    return fs::path(path).parent_path().string();
}

std::string normalizeExtension(std::string ext)
{
    if (ext.size() > 1 && ext[0] == '.')
        ext = ext.substr(1);

    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    return ext;
}

int platformFromExtension(const std::string& ext)
{
    if (ext == "gba") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA);
    if (ext == "gbc") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC);
    if (ext == "gb") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGB);
    if (ext == "nes" || ext == "fds") return static_cast<int>(beiklive::enums::EmuPlatform::EmuNES);
    if (ext == "sfc" || ext == "smc") return static_cast<int>(beiklive::enums::EmuPlatform::EmuSNES);
    if (ext == "nds") return static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
    if (ext == "cia" || ext == "cci" || ext == "3ds")
        return static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS);
    if (ext == "md" || ext == "gen" || ext == "bin" || ext == "smd")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis);
    if (ext == "zip" || ext == "7z")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade);
    if (ext == "cdi" || ext == "gdi" || ext == "chd" || ext == "cue")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast);
    if (ext == "iso" || ext == "cso")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP);
    return -1;
}

int platformFromExistingPath(const fs::path& path)
{
    int detected = beiklive::tools::detectGamePlatform(path);
    if (detected >= 0)
        return detected;

    std::string ext = normalizeExtension(path.extension().string());
    if (ext == "zip" || ext == "7z")
        return -1;
    return platformFromExtension(ext);
}

std::string overlayKeyForPlatform(int platform)
{
    namespace sk = beiklive::SettingKey;
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
    case beiklive::enums::EmuPlatform::EmuGBA: return sk::KEY_DISPLAY_OVERLAY_GBA_PATH;
    case beiklive::enums::EmuPlatform::EmuGBC: return sk::KEY_DISPLAY_OVERLAY_GBC_PATH;
    case beiklive::enums::EmuPlatform::EmuGB: return sk::KEY_DISPLAY_OVERLAY_GB_PATH;
    case beiklive::enums::EmuPlatform::EmuNES: return sk::KEY_DISPLAY_OVERLAY_NES_PATH;
    case beiklive::enums::EmuPlatform::EmuSNES: return sk::KEY_DISPLAY_OVERLAY_SNES_PATH;
    case beiklive::enums::EmuPlatform::EmuNDS: return sk::KEY_DISPLAY_OVERLAY_NDS_PATH;
    case beiklive::enums::EmuPlatform::EmuGenesis: return sk::KEY_DISPLAY_OVERLAY_GENESIS_PATH;
    case beiklive::enums::EmuPlatform::EmuArcade: return sk::KEY_DISPLAY_OVERLAY_ARCADE_PATH;
    case beiklive::enums::EmuPlatform::EmuDreamcast: return sk::KEY_DISPLAY_OVERLAY_DC_PATH;
    case beiklive::enums::EmuPlatform::EmuPSP: return sk::KEY_DISPLAY_OVERLAY_PSP_PATH;
    default: return "";
    }
}

std::string shaderKeyForPlatform(int platform)
{
    namespace sk = beiklive::SettingKey;
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
    case beiklive::enums::EmuPlatform::EmuGBA: return sk::KEY_DISPLAY_SHADER_GBA_PATH;
    case beiklive::enums::EmuPlatform::EmuGBC: return sk::KEY_DISPLAY_SHADER_GBC_PATH;
    case beiklive::enums::EmuPlatform::EmuGB: return sk::KEY_DISPLAY_SHADER_GB_PATH;
    case beiklive::enums::EmuPlatform::EmuNES: return sk::KEY_DISPLAY_SHADER_NES_PATH;
    case beiklive::enums::EmuPlatform::EmuSNES: return sk::KEY_DISPLAY_SHADER_SNES_PATH;
    case beiklive::enums::EmuPlatform::EmuNDS: return sk::KEY_DISPLAY_SHADER_NDS_PATH;
    case beiklive::enums::EmuPlatform::EmuGenesis: return sk::KEY_DISPLAY_SHADER_GENESIS_PATH;
    case beiklive::enums::EmuPlatform::EmuArcade: return sk::KEY_DISPLAY_SHADER_ARCADE_PATH;
    case beiklive::enums::EmuPlatform::EmuDreamcast: return sk::KEY_DISPLAY_SHADER_DC_PATH;
    case beiklive::enums::EmuPlatform::EmuPSP: return sk::KEY_DISPLAY_SHADER_PSP_PATH;
    default: return "";
    }
}

ImportSharedConfig buildSharedConfig(int platform)
{
    namespace sk = beiklive::SettingKey;

    ImportSharedConfig config;
    config.platform = platform;
    if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) ||
        platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
        return config;
    config.overlayEnabled = beiklive::tools::shouldAutoEnableOverlayForPlatform(platform);
    config.shaderEnabled = beiklive::tools::shouldAutoEnableShaderForPlatform(platform);

    std::string overlayKey = overlayKeyForPlatform(platform);
    if (!overlayKey.empty())
        config.overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");

    std::string shaderKey = shaderKeyForPlatform(platform);
    if (!shaderKey.empty())
        config.shaderPath = GET_SETTING_KEY_STR(shaderKey.c_str(), "");
    if (config.shaderPath.empty())
        config.shaderPath = GET_SETTING_KEY_STR(sk::KEY_DISPLAY_SHADER_PATH, "");

    return config;
}

void preserveThreeDsMenuSettings(json& root, const std::filesystem::path& file)
{
    std::ifstream in(file);
    if (!in.is_open())
        return;

    json existing = json::parse(in, nullptr, false);
    if (!existing.is_object())
        return;

    constexpr const char* keys[] = {
        "fastforward.multiplier",
        "ndsScreenLayout",
        "ndsScreenOrientation",
        "ndsInternalResolution",
        "ndsIntegerScale",
        "ndsScreenGap",
        "ndsTopScale",
        "ndsTopOffsetX",
        "ndsTopOffsetY",
        "ndsBottomScale",
        "ndsBottomOffsetX",
        "ndsBottomOffsetY",
        "ndsBottomOpacity",
        "overlayEnabled",
        "overlayPath",
    };
    for (const char* key : keys)
    {
        if (!root.contains(key) && existing.contains(key))
            root[key] = existing[key];
    }
}

bool exportThreeDsCoreConfigForDataPage()
{
    if (!beiklive::SettingManager)
        return false;

    json root = json::object();
    const auto putInt = [&root](const char* outKey, const char* settingKey, int def) {
        root[outKey] = GET_SETTING_KEY_INT(settingKey, def);
    };
    const auto putFloat = [&root](const char* outKey, const char* settingKey, float def) {
        root[outKey] = GET_SETTING_KEY_FLOAT(settingKey, def);
    };
    const auto putStr = [&root](const char* outKey, const char* settingKey, const char* def) {
        root[outKey] = GET_SETTING_KEY_STR(settingKey, def);
    };

    putInt("upscale", "core.azahar.upscale", 1);
    putInt("use_cpu_jit", "core.azahar.use_cpu_jit", 1);
    putInt("new_3ds", "core.azahar.new_3ds", 1);
    putInt("cpu_clock", "core.azahar.cpu_clock", 100);
    putStr("region", "core.azahar.region", "auto");
    putStr("language", "core.azahar.language", "");
    putStr("username", "core.azahar.username", "");
    putStr("input_type", "core.azahar.input_type", "null");
    putInt("use_hw_shader", "core.azahar.use_hw_shader", 1);
    putInt("use_shader_jit", "core.azahar.use_shader_jit", 1);
    putInt("accurate_mul", "core.azahar.accurate_mul", 1);
    putInt("disk_shader_cache", "core.azahar.disk_shader_cache", 1);
    putInt("async_gpu", "core.azahar.async_gpu", 0);
    putInt("strict_gpu_sync", "core.azahar.strict_gpu_sync", 0);
    putInt("async_shaders", "core.azahar.async_shaders", 1);
    putInt("show_shader_compile_notice", "core.azahar.show_shader_compile_notice", 1);
    putInt("async_presentation", "core.azahar.async_presentation", 1);
    putInt("spirv_shader_gen", "core.azahar.spirv_shader_gen", 1);
    putInt("disable_spirv_optimizer", "core.azahar.disable_spirv_optimizer", 1);
    putInt("vsync", "core.azahar.vsync", 1);
    putFloat("frame_limit", "core.azahar.frame_limit", 100.0f);
    putInt("simulate_3ds_gpu_timings", "core.azahar.simulate_3ds_gpu_timings", 0);
    putInt("renderer_debug", "core.azahar.renderer_debug", 0);
    putInt("dump_command_buffers", "core.azahar.dump_command_buffers", 0);
    putInt("disable_right_eye", "core.azahar.disable_right_eye", 1);
    putStr("texture_filter", "core.azahar.texture_filter", "none");
    putStr("texture_sampling", "core.azahar.texture_sampling", "game");
    putInt("custom_textures", "core.azahar.custom_textures", 0);
    putInt("dump_textures", "core.azahar.dump_textures", 0);
    putInt("use_virtual_sd", "core.azahar.use_virtual_sd", 1);
    putStr("layout", "core.azahar.layout", "default");
    putStr("small_screen_position", "core.azahar.small_screen_position", "bottom_right");
    putStr("display_orientation", "core.azahar.display_orientation", "horizontal");
    putStr("display_rotation", "core.azahar.display_rotation", "0");
    putStr("display_size", "core.azahar.display_size", "default");
    putFloat("large_screen_proportion", "core.azahar.large_screen_proportion", 4.0f);
    putStr("audio_emulation", "core.azahar.audio_emulation", "hle");
    putInt("audio_stretching", "core.azahar.audio_stretching", 0);
    putInt("realtime_audio", "core.azahar.realtime_audio", 1);
    root["fastforward.multiplier"] = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);

#ifdef __SWITCH__
    const std::filesystem::path dir("sdmc:/GBAStation/3ds/config/cores");
    const std::filesystem::path file("sdmc:/GBAStation/3ds/config/cores/azahar.jsonc");
#else
    const std::filesystem::path dir = std::filesystem::path(beiklive::path::ROOT) /
        "GBAStation" / "3ds" / "config" / "cores";
    const std::filesystem::path file = dir / "azahar.jsonc";
#endif
    preserveThreeDsMenuSettings(root, file);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec)
        return false;

    std::ofstream out(file, std::ios::trunc);
    if (!out.is_open())
        return false;

    out << root.dump(2) << "\n";
    return true;
}

void applyDisplayDefaults(beiklive::GameEntry& entry)
{
    std::string mode = GET_SETTING_KEY_STR("display.mode", "original");
    if (mode == "fill")
        entry.displayMode = 1;
    else if (mode == "integer")
        entry.displayMode = 2;
    else if (mode == "custom")
        entry.displayMode = 3;
    else if (mode == "four_three" || mode == "4:3")
        entry.displayMode = 4;
    else
        entry.displayMode = 0;

    entry.integerAspectRatio =
        static_cast<float>(GET_SETTING_KEY_INT("display.integer_scale_mult", 0));
}

bool clearDirectoryContents(const fs::path& dir)
{
    std::error_code ec;
    if (!fs::exists(dir, ec))
        return true;

    std::vector<fs::path> targets;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
        targets.push_back(it->path());

    if (ec)
        return false;

    bool ok = true;
    for (const auto& target : targets)
    {
        ec.clear();
        fs::remove_all(target, ec);
        if (ec)
            ok = false;
    }

    return ok;
}

int findUnexpectedLplPlatform(const json& items, int expectedPlatform)
{
    for (const auto& item : items)
    {
        std::string romPath = item.value("path", "");
        if (romPath.empty())
            continue;

        fs::path path(romPath);
        int detectedPlatform = fs::exists(path) ? platformFromExistingPath(path) :
            platformFromExtension(normalizeExtension(path.extension().string()));
        if (detectedPlatform >= 0 && detectedPlatform != expectedPlatform)
            return detectedPlatform;
    }

    return -1;
}

} // namespace

namespace beiklive
{

static std::string encodeDataIcon(char32_t codepoint)
{
    std::string result;
    if (codepoint <= 0x7F)
    {
        result.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7FF)
    {
        result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint <= 0xFFFF)
    {
        result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else
    {
        result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return result;
}

static float dataClamp(float value)
{
    return std::max(0.f, std::min(1.f, value));
}

static float dataSmooth(float value)
{
    value = dataClamp(value);
    return value * value * (3.f - 2.f * value);
}

static float dataBack(float value)
{
    value = dataClamp(value);
    constexpr float c1 = 1.16f;
    constexpr float c3 = c1 + 1.f;
    const float shifted = value - 1.f;
    return 1.f + c3 * shifted * shifted * shifted + c1 * shifted * shifted;
}

static unsigned char dataAlpha(float value)
{
    return static_cast<unsigned char>(255.f * dataClamp(value));
}

class DataManagementCanvas final : public brls::View
{
public:
    struct Item
    {
        std::string title;
        std::string description;
        std::string badge;
        char32_t icon = material::STORAGE;
        std::function<void()> action;
        bool* toggle = nullptr;
        bool danger = false;
    };

    struct Tab
    {
        std::string title;
        std::string summary;
        std::string detail;
        char32_t icon = material::STORAGE;
        std::vector<Item> items;
    };

    DataManagementCanvas(std::vector<Tab> tabs, std::function<void()> onBack)
        : m_tabs(std::move(tabs))
        , m_onBack(std::move(onBack))
    {
        setFocusable(true);
        setGrow(1.f);
        setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);
        setCustomNavigationRoute(brls::FocusDirection::UP, this);
        setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        auto previousTab = [this](brls::View*) -> bool {
            if (_acceptNavigation(1))
                _switchTab(-1);
            return true;
        };
        auto nextTab = [this](brls::View*) -> bool {
            if (_acceptNavigation(2))
                _switchTab(1);
            return true;
        };
        auto moveUp = [this](brls::View*) -> bool {
            if (_acceptNavigation(3))
                _moveFocus(-1);
            return true;
        };
        auto moveDown = [this](brls::View*) -> bool {
            if (_acceptNavigation(4))
                _moveFocus(1);
            return true;
        };
        registerAction("", brls::BUTTON_LEFT, previousTab, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, nextTab, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, previousTab, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, nextTab, true, true, brls::SOUND_NONE);
        registerAction(L("上一页"), brls::BUTTON_LB, previousTab, true, false, brls::SOUND_NONE);
        registerAction(L("下一页"), brls::BUTTON_RB, nextTab, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_UP, moveUp, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, moveDown, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_UP, moveUp, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, moveDown, true, true, brls::SOUND_NONE);
        registerAction(L("选择"), brls::BUTTON_A, [this](brls::View*) -> bool {
            _activateFocused();
            return true;
        }, false, false, brls::SOUND_NONE);
        registerAction(L("返回"), brls::BUTTON_B, [this](brls::View*) -> bool {
            _beginClose();
            return true;
        }, false, false, brls::SOUND_NONE);

        m_focusIndices.assign(m_tabs.size(), 0);
        m_savedScroll.assign(m_tabs.size(), 0.f);
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    void frame(brls::FrameContext* ctx) override
    {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;
        m_time += dt;

        if (m_closing)
        {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 3.8f);
            if (m_pageEntrance <= 0.f && !m_closeQueued)
            {
                m_closeQueued = true;
                const auto callback = m_onBack;
                brls::sync([callback]() {
                    if (callback)
                        callback();
                });
            }
        }
        else
        {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 2.8f);
            m_tabEntrance = std::min(1.f, m_tabEntrance + dt * 4.8f);
        }

        if (m_clicking)
        {
            m_clickTime += dt;
            if (m_clickTime >= 0.2f)
            {
                m_clicking = false;
                m_clickTime = 0.f;
                _runFocusedAction();
            }
        }

        m_scroll += (m_targetScroll - m_scroll) * std::min(1.f, dt * 13.f);
        invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;
        _ensureFonts();
        const float pageProgress = dataBack(m_pageEntrance);
        const float pageAlpha = dataSmooth(m_pageEntrance);
        _drawHeader(vg, x, y - (1.f - pageProgress) * 56.f, w, pageAlpha);

        const float contentY = y + 112.f;
        const float contentH = std::max(1.f, h - 178.f);
        const float leftW = std::min(350.f, w * 0.29f);
        const Rect overview{x + 36.f, contentY, leftW, contentH};
        const Rect list{x + 36.f + leftW + 22.f, contentY,
                        w - leftW - 94.f, contentH};
        const float tabProgress = dataBack(m_tabEntrance);
        const float tabAlpha = dataSmooth(m_tabEntrance);
        const float centerX = x + w * 0.5f;
        const float centerY = contentY + contentH * 0.5f;
        const float scale = 0.968f + pageProgress * 0.032f;

        nvgSave(vg);
        nvgGlobalAlpha(vg, pageAlpha * tabAlpha);
        nvgTranslate(vg, centerX + static_cast<float>(m_tabDirection)
                         * (1.f - tabProgress) * 68.f,
                     centerY + (1.f - pageProgress) * 22.f);
        nvgScale(vg, scale, scale);
        nvgTranslate(vg, -centerX, -centerY);
        _drawOverview(vg, overview);
        _drawItems(vg, list);
        nvgRestore(vg);

        _drawFooter(vg, x, y, w, h, pageAlpha);
    }

private:
    struct Rect
    {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    std::vector<Tab> m_tabs;
    std::function<void()> m_onBack;
    std::vector<int> m_focusIndices;
    std::vector<float> m_savedScroll;
    int m_tab = 0;
    int m_tabDirection = 1;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_switchFont = -1;
    float m_time = 0.f;
    float m_pageEntrance = 0.f;
    float m_tabEntrance = 1.f;
    float m_scroll = 0.f;
    float m_targetScroll = 0.f;
    float m_contentHeight = 0.f;
    float m_viewportHeight = 0.f;
    float m_itemHeight = 72.f;
    float m_clickTime = 0.f;
    bool m_clicking = false;
    bool m_closing = false;
    bool m_closeQueued = false;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastNavigationTime;
    int m_lastNavigationAction = 0;

    void _ensureFonts()
    {
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
    }

    bool _acceptNavigation(int action)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastNavigationTime).count();
        if (action == m_lastNavigationAction && elapsed >= 0 && elapsed < 85)
            return false;
        m_lastNavigationAction = action;
        m_lastNavigationTime = now;
        return true;
    }

    const Tab& _currentTab() const
    {
        return m_tabs[static_cast<size_t>(m_tab)];
    }

    int _focusIndex() const
    {
        if (m_tab < 0 || m_tab >= static_cast<int>(m_focusIndices.size()))
            return 0;
        return m_focusIndices[static_cast<size_t>(m_tab)];
    }

    void _drawExternalShadow(NVGcontext* vg, const Rect& r, float radius,
                             float alpha = 1.f)
    {
        const NVGpaint shadow = nvgBoxGradient(
            vg, r.x + 5.f, r.y + 6.f, r.w, r.h, radius, 5.f,
            nvgRGBA(0, 0, 0, dataAlpha(0.31f * alpha)),
            nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, r.x - 3.f, r.y - 3.f, r.w + 16.f, r.h + 17.f);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
    }

    void _drawPanel(NVGcontext* vg, const Rect& r, float radius = 8.f)
    {
        _drawExternalShadow(vg, r, radius, 0.82f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 7));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x + 1.f, r.y + 1.f,
                       r.w - 2.f, r.h - 2.f, radius - 1.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
    }

    void _drawSwitchButton(NVGcontext* vg, brls::ControllerButton button,
                           float x, float y)
    {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 240));
        nvgText(vg, x, y, glyph.c_str(), nullptr);
    }

    void _drawHeader(NVGcontext* vg, float x, float y, float w, float alpha)
    {
        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 27.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, y + 43.f, L("数据管理").c_str(), nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 180));
        nvgText(vg, x + 36.f, y + 72.f, L("导入、维护与远程管理游戏库").c_str(), nullptr);

        const float centerX = x + w * 0.5f;
        const float centerY = y + 45.f;
        constexpr float spacing = 152.f;
        constexpr float selectorW = 132.f;
        constexpr float selectorH = 42.f;
        constexpr float selectorRadius = 21.f;
        const float eased = 1.f - std::pow(1.f - m_tabEntrance, 3.f);
        const float shift = static_cast<float>(m_tabDirection)
            * spacing * (1.f - eased);
        const int count = static_cast<int>(m_tabs.size());

        _drawSwitchButton(vg, brls::BUTTON_LB, centerX - 290.f, centerY);
        _drawSwitchButton(vg, brls::BUTTON_RB, centerX + 290.f, centerY);
        for (int relative = count == 1 ? 0 : -2;
             relative <= (count == 1 ? 0 : 2); ++relative)
        {
            int index = (m_tab + relative) % count;
            if (index < 0)
                index += count;
            const float labelX = centerX + relative * spacing + shift;
            const float distance = std::abs(labelX - centerX) / spacing;
            if (distance > 1.55f)
                continue;
            const float prominence = std::max(0.f, 1.f - distance);
            const float labelAlpha = 0.42f + prominence * 0.58f;
            if (prominence > 0.55f)
            {
                const Rect selector{labelX - selectorW * 0.5f,
                                    centerY - selectorH * 0.5f,
                                    selectorW, selectorH};
                _drawExternalShadow(vg, selector, selectorRadius,
                                    0.62f * prominence);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, selector.x, selector.y,
                               selector.w, selector.h, selectorRadius);
                nvgFillColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(22.f + 22.f * prominence)));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, selector.x + 1.f, selector.y + 1.f,
                               selector.w - 2.f, selector.h - 2.f,
                               selectorRadius - 1.f);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(70.f + 65.f * prominence)));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
            }
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 17.f + 5.f * prominence);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(255.f * labelAlpha)));
            nvgText(vg, labelX, centerY,
                    m_tabs[static_cast<size_t>(index)].title.c_str(), nullptr);
        }

        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 36.f, y + 94.f);
        nvgLineTo(vg, x + w - 36.f, y + 94.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 46));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgRestore(vg);
    }

    void _drawBadge(NVGcontext* vg, float x, float y,
                    const std::string& text, NVGcolor color)
    {
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 14.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, text.c_str(), nullptr, bounds);
        const float width = bounds[2] - bounds[0] + 22.f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, 28.f, 6.f);
        nvgFillColor(vg, nvgRGBAf(color.r, color.g, color.b, 0.11f));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 1.f, y + 1.f, width - 2.f, 26.f, 5.f);
        nvgStrokeColor(vg, nvgRGBAf(color.r, color.g, color.b, 0.5f));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(240, 244, 249, 230));
        nvgText(vg, x + width * 0.5f, y + 14.f, text.c_str(), nullptr);
    }

    void _drawOverview(NVGcontext* vg, const Rect& r)
    {
        _drawPanel(vg, r);
        const Tab& tab = _currentTab();
        const std::string icon = encodeDataIcon(tab.icon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 62.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 235));
        nvgText(vg, r.x + r.w * 0.5f, r.y + 82.f, icon.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 27.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, r.x + r.w * 0.5f, r.y + 145.f,
                tab.title.c_str(), nullptr);
        nvgFontSize(vg, 17.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgTextLineHeight(vg, 1.42f);
        nvgFillColor(vg, nvgRGBA(215, 221, 231, 200));
        nvgTextBox(vg, r.x + 30.f, r.y + 190.f, r.w - 60.f,
                   tab.summary.c_str(), nullptr);

        nvgBeginPath(vg);
        nvgMoveTo(vg, r.x + 28.f, r.y + 280.f);
        nvgLineTo(vg, r.x + r.w - 28.f, r.y + 280.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 35));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(200, 208, 220, 185));
        nvgTextBox(vg, r.x + 30.f, r.y + 310.f, r.w - 60.f,
                   tab.detail.c_str(), nullptr);

        float badgeX = r.x + 30.f;
        const float badgeY = r.y + r.h - 58.f;
        if (m_tab == 0)
        {
            _drawBadge(vg, badgeX, badgeY, "LPL", nvgRGB(79, 193, 255));
            badgeX += 62.f;
            _drawBadge(vg, badgeX, badgeY, L("6 平台"), nvgRGB(100, 220, 150));
        }
        else if (m_tab == 1)
        {
            int enabled = 0;
            for (const auto& item : tab.items)
                if (item.toggle && *item.toggle)
                    ++enabled;
            _drawBadge(vg, badgeX, badgeY,
                       std::to_string(enabled) + L(" 项已开启"),
                       nvgRGB(100, 220, 150));
        }
        else
        {
            _drawBadge(vg, badgeX, badgeY, L("谨慎操作"), nvgRGB(255, 190, 80));
        }
    }

    void _drawItems(NVGcontext* vg, const Rect& r)
    {
        const auto& items = _currentTab().items;
        const float gap = 10.f;
        m_itemHeight = items.size() <= 3 ? 116.f : (items.size() <= 6 ? 74.f : 62.f);
        m_viewportHeight = r.h;
        m_contentHeight = items.empty() ? 0.f
            : items.size() * m_itemHeight + (items.size() - 1) * gap;
        const float maximum = std::max(0.f, m_contentHeight - r.h);
        m_targetScroll = std::clamp(m_targetScroll, 0.f, maximum);
        m_scroll = std::clamp(m_scroll, 0.f, maximum);

        nvgSave(vg);
        nvgIntersectScissor(vg, r.x - 8.f, r.y - 8.f, r.w + 20.f, r.h + 16.f);
        for (size_t index = 0; index < items.size(); ++index)
        {
            const float itemY = r.y + index * (m_itemHeight + gap) - m_scroll;
            if (itemY + m_itemHeight < r.y || itemY > r.y + r.h)
                continue;
            const bool focused = static_cast<int>(index) == _focusIndex();
            float clickScale = 1.f;
            if (focused && m_clicking)
                clickScale = 1.f - 0.035f * std::sin(
                    3.14159265f * dataClamp(m_clickTime / 0.2f));
            Rect itemRect{r.x, itemY, r.w, m_itemHeight};
            itemRect.w *= clickScale;
            itemRect.h *= clickScale;
            itemRect.x += (r.w - itemRect.w) * 0.5f;
            itemRect.y += (m_itemHeight - itemRect.h) * 0.5f;
            _drawItem(vg, itemRect, items[index], focused);
        }
        nvgRestore(vg);

        if (maximum > 1.f)
        {
            const float trackH = r.h - 18.f;
            const float thumbH = std::max(40.f, trackH * r.h / m_contentHeight);
            const float thumbY = r.y + 9.f
                + (trackH - thumbH) * m_scroll / maximum;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, r.x + r.w + 8.f, thumbY, 3.f, thumbH, 1.5f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 95));
            nvgFill(vg);
        }
    }

    void _drawItem(NVGcontext* vg, const Rect& r, const Item& item, bool focused)
    {
        _drawExternalShadow(vg, r, 8.f, 0.78f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, 8.f);
        nvgFillColor(vg, item.danger
            ? (focused ? nvgRGBA(255, 96, 96, 34) : nvgRGBA(255, 96, 96, 9))
            : (focused ? nvgRGBA(79, 193, 255, 34) : nvgRGBA(255, 255, 255, 7)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x + 1.f, r.y + 1.f,
                       r.w - 2.f, r.h - 2.f, 7.f);
        nvgStrokeColor(vg, focused
            ? nvgRGBA(255, 255, 255, 145) : nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
        if (focused && isFocused())
        {
            beiklive::ui::drawGradientFocusBorder(
                vg, r.x, r.y, r.w, r.h, 8.f, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
        }

        const std::string icon = encodeDataIcon(item.icon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, m_itemHeight > 80.f ? 42.f : 34.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, item.danger
            ? nvgRGBA(255, 135, 135, 235)
            : (focused ? nvgRGBA(255, 255, 255, 255)
                       : nvgRGBA(220, 226, 235, 220)));
        nvgText(vg, r.x + 54.f, r.y + r.h * 0.5f, icon.c_str(), nullptr);

        const float textX = r.x + 96.f;
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, m_itemHeight > 80.f ? 22.f : 19.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused
            ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        const float titleY = m_itemHeight > 80.f ? r.y + r.h * 0.39f
                                                 : r.y + r.h * 0.43f;
        nvgText(vg, textX, titleY, item.title.c_str(), nullptr);
        if (m_itemHeight > 68.f)
        {
            nvgFontSize(vg, 14.f);
            nvgFillColor(vg, nvgRGBA(205, 212, 223, focused ? 215 : 165));
            nvgText(vg, textX, r.y + r.h * 0.68f,
                    item.description.c_str(), nullptr);
        }

        if (item.toggle)
        {
            const bool enabled = *item.toggle;
            const float switchW = 52.f;
            const float switchH = 28.f;
            const float switchX = r.x + r.w - switchW - 26.f;
            const float switchY = r.y + (r.h - switchH) * 0.5f;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, switchX, switchY, switchW, switchH, 14.f);
            nvgFillColor(vg, enabled
                ? nvgRGBA(79, 193, 255, 150) : nvgRGBA(255, 255, 255, 28));
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgCircle(vg, enabled ? switchX + switchW - 14.f : switchX + 14.f,
                      switchY + 14.f, 10.f);
            nvgFillColor(vg, enabled
                ? nvgRGBA(255, 255, 255, 250) : nvgRGBA(190, 197, 208, 220));
            nvgFill(vg);
        }
        else
        {
            if (!item.badge.empty())
            {
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 14.f);
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(205, 212, 223, 180));
                nvgText(vg, r.x + r.w - 48.f, r.y + r.h * 0.5f,
                        item.badge.c_str(), nullptr);
            }
            const std::string arrow = encodeDataIcon(material::PLAY_ARROW);
            nvgFontFaceId(vg, m_materialFont);
            nvgFontSize(vg, 24.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(225, 230, 238, 190));
            nvgText(vg, r.x + r.w - 24.f, r.y + r.h * 0.5f,
                    arrow.c_str(), nullptr);
        }
    }

    void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                   const char* label, float& cursor, float y, float alpha)
    {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
        cursor -= bounds[2] - bounds[0] + 43.f;
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, dataAlpha(alpha)));
        nvgText(vg, cursor + 13.f, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(230, 234, 241, dataAlpha(alpha)));
        nvgText(vg, cursor + 30.f, y, label, nullptr);
        cursor -= 16.f;
    }

    void _drawFooter(NVGcontext* vg, float x, float y, float w, float h,
                     float alpha)
    {
        const float hintY = y + h - 27.f
            + (1.f - dataBack(m_pageEntrance)) * 46.f;
        float cursor = x + w - 32.f;
        _drawHint(vg, brls::BUTTON_B, L("返回").c_str(), cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_A,
                  _currentTab().items[static_cast<size_t>(_focusIndex())].toggle
                      ? L("切换").c_str() : L("选择").c_str(),
                  cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_RB, L("下一页").c_str(), cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_LB, L("上一页").c_str(), cursor, hintY, alpha);
    }

    void _switchTab(int direction)
    {
        if (m_closing || m_clicking || m_pageEntrance < 0.72f
            || m_tabEntrance < 0.72f || m_tabs.size() <= 1)
            return;
        m_savedScroll[static_cast<size_t>(m_tab)] = m_targetScroll;
        const int count = static_cast<int>(m_tabs.size());
        m_tab = (m_tab + (direction < 0 ? -1 : 1) + count) % count;
        m_tabDirection = direction < 0 ? -1 : 1;
        m_tabEntrance = 0.f;
        m_scroll = m_savedScroll[static_cast<size_t>(m_tab)];
        m_targetScroll = m_scroll;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void _moveFocus(int direction)
    {
        if (m_closing || m_clicking || m_pageEntrance < 0.72f
            || m_tabEntrance < 0.72f)
            return;
        const int count = static_cast<int>(_currentTab().items.size());
        if (count <= 0)
            return;
        const int next = std::clamp(_focusIndex() + direction, 0, count - 1);
        if (next == _focusIndex())
            return;
        m_focusIndices[static_cast<size_t>(m_tab)] = next;
        _ensureFocusedVisible();
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    }

    void _ensureFocusedVisible()
    {
        const float gap = 10.f;
        const float top = _focusIndex() * (m_itemHeight + gap);
        constexpr float margin = 12.f;
        if (top < m_targetScroll + margin)
            m_targetScroll = top - margin;
        else if (top + m_itemHeight > m_targetScroll + m_viewportHeight - margin)
            m_targetScroll = top + m_itemHeight - m_viewportHeight + margin;
        const float maximum = std::max(0.f, m_contentHeight - m_viewportHeight);
        m_targetScroll = std::clamp(m_targetScroll, 0.f, maximum);
    }

    void _activateFocused()
    {
        if (m_closing || m_clicking || m_pageEntrance < 0.85f
            || m_tabEntrance < 0.85f || _currentTab().items.empty())
            return;
        m_clicking = true;
        m_clickTime = 0.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    void _runFocusedAction()
    {
        Item& item = m_tabs[static_cast<size_t>(m_tab)]
            .items[static_cast<size_t>(_focusIndex())];
        if (item.toggle)
        {
            *item.toggle = !*item.toggle;
            invalidate();
        }
        else if (item.action)
        {
            item.action();
        }
    }

    void _beginClose()
    {
        if (m_closing || m_clicking)
            return;
        m_closing = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
    }
};

DataManagementPage::DataManagementPage()
{
    this->showHeader(false);
    this->showFooter(false);
    this->setFocusable(false);
    init();
    setupProgressOverlay();
    brls::sync([this]() {
        if (m_mainCanvas)
            brls::Application::giveFocus(m_mainCanvas);
    });
}

DataManagementPage::~DataManagementPage()
{
    m_alive.store(false, std::memory_order_release);
    finishWorker();
}

void DataManagementPage::draw(
    NVGcontext* vg, float x, float y, float w, float h,
    brls::Style style, brls::FrameContext* ctx)
{
    beiklive::Box::draw(vg, x, y, w, h, style, ctx);

    if (!m_progressOverlay)
        return;

    if (m_importing.load(std::memory_order_acquire))
    {
        int cur = m_progress.load(std::memory_order_acquire);
        int tot = m_total.load(std::memory_order_acquire);
        float frac = (tot > 0) ? static_cast<float>(cur) / static_cast<float>(tot) : 0.f;

        {
            std::lock_guard<std::mutex> lock(m_statusMutex);
            m_progressNameLabel->setText(m_progressName.empty() ? " " : m_progressName);
        }

        m_progressBar->setWidth(400.f * frac);
        m_progressCountLabel->setText(std::to_string(cur) + " / " + std::to_string(tot));

        if (m_importError.load(std::memory_order_acquire))
        {
            m_importing.store(false, std::memory_order_release);
            m_progressTitleLabel->setText(
                m_progressTask == ProgressTask::Cleanup ? L("处理失败") : L("导入失败"));
            {
                std::lock_guard<std::mutex> lock(m_statusMutex);
                std::string err = m_errorMsg.empty() ? L("未知错误") : m_errorMsg;
                if (err.size() > 60)
                    err = err.substr(0, 60) + "...";
                m_progressCountLabel->setText(err);
            }
            hideProgressOverlay();

            if (!m_completionShown)
            {
                m_completionShown = true;
                std::string err;
                {
                    std::lock_guard<std::mutex> lock(m_statusMutex);
                    err = m_errorMsg.empty() ? L("未知错误") : m_errorMsg;
                }
                rememberFocusBeforeModal();
                auto* dialog = new brls::Dialog(
                    std::string(m_progressTask == ProgressTask::Cleanup ? L("处理失败\n\n")
                                                                       : L("导入失败\n\n")) +
                    err);
                dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
                dialog->open();
            }
        }
        else if (m_importDone.load(std::memory_order_acquire))
        {
            m_importing.store(false, std::memory_order_release);
            finishWorker();
            if (beiklive::GameDB && m_progressTask == ProgressTask::Import)
                beiklive::GameDB->flush();

            int total = m_total.load(std::memory_order_acquire);
            m_progressBar->setWidth(400.f);
            m_progressBar->setColor(nvgRGB(129, 199, 132));
            if (m_progressTask == ProgressTask::Cleanup)
            {
                int removed = m_cleanupRemoved.load(std::memory_order_acquire);
                m_progressTitleLabel->setText(L("处理完成"));
                m_progressCountLabel->setText(
                    L("已扫描 ") + std::to_string(total) + L(" 个游戏，移除 ") +
                    std::to_string(removed) + L(" 个无效记录"));
            }
            else
            {
                int skipped = m_importSkipped.load(std::memory_order_acquire);
                m_progressTitleLabel->setText(L("导入完成"));
                if (skipped > 0)
                    m_progressCountLabel->setText(
                        L("共处理 ") + std::to_string(total) + L(" 个游戏，跳过 ") +
                        std::to_string(skipped) + L(" 个已有游戏"));
                else
                    m_progressCountLabel->setText(L("共处理 ") + std::to_string(total) + L(" 个游戏"));
            }
            hideProgressOverlay();

            if (!m_completionShown)
            {
                m_completionShown = true;
                rememberFocusBeforeModal();
                std::string dialogText;
                if (m_progressTask == ProgressTask::Cleanup)
                {
                    int removed = m_cleanupRemoved.load(std::memory_order_acquire);
                    dialogText = removed > 0
                        ? L("处理完成\n\n已移除 ") + std::to_string(removed) + L(" 个无效游戏记录")
                        : L("处理完成\n\n没有发现无效游戏记录");
                }
                else
                {
                    int skipped = m_importSkipped.load(std::memory_order_acquire);
                    dialogText = skipped > 0
                        ? L("导入完成\n\n共处理 ") + std::to_string(total) +
                              L(" 个游戏，跳过 ") + std::to_string(skipped) + L(" 个已有游戏")
                        : L("导入完成\n\n共处理 ") + std::to_string(total) + L(" 个游戏");
                }
                auto* dialog = new brls::Dialog(dialogText);
                dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
                dialog->open();
            }
        }
        else
        {
            invalidate();
        }
    }
}

void DataManagementPage::init()
{
    using Canvas = DataManagementCanvas;
    std::vector<Canvas::Tab> tabs;

    Canvas::Tab bundle;
    bundle.title = L("整合包导入");
    bundle.summary = L("从 RetroArch 播放列表导入游戏，并沿用列表中的游戏名称与现有缩略图。");
    bundle.detail = L("请选择与播放列表内容一致的平台。导入前会检查 ROM 类型，选择错误时不会写入游戏库。");
    bundle.icon = material::DESCRIPTION;
    struct BundlePlatform
    {
        std::string title;
        std::string badge;
        int platform;
    };
    const BundlePlatform bundlePlatforms[] = {
        {L("导入 GBA lpl文件"), "GBA · .lpl", static_cast<int>(enums::EmuPlatform::EmuGBA)},
        {L("导入 GBC lpl文件"), "GBC · .lpl", static_cast<int>(enums::EmuPlatform::EmuGBC)},
        {L("导入 GB lpl文件"), "GB · .lpl", static_cast<int>(enums::EmuPlatform::EmuGB)},
        {L("导入 FC lpl文件"), "FC · .lpl", static_cast<int>(enums::EmuPlatform::EmuNES)},
        {L("导入 SFC lpl文件"), "SFC · .lpl", static_cast<int>(enums::EmuPlatform::EmuSNES)},
        {L("导入 NDS lpl文件"), "NDS · .lpl", static_cast<int>(enums::EmuPlatform::EmuNDS)},
        {L("导入 3DS lpl文件"), "3DS · .lpl", static_cast<int>(enums::EmuPlatform::Emu3DS)},
        {L("导入 MD lpl文件"), "MD · .lpl", static_cast<int>(enums::EmuPlatform::EmuGenesis)},
        {L("导入 街机 lpl文件"), "Arcade · .lpl", static_cast<int>(enums::EmuPlatform::EmuArcade)},
        {L("导入 DC lpl文件"), "DC · .lpl", static_cast<int>(enums::EmuPlatform::EmuDreamcast)},
        {L("导入 PSP lpl文件"), "PSP · .lpl", static_cast<int>(enums::EmuPlatform::EmuPSP)},
    };
    for (const auto& platform : bundlePlatforms)
    {
        bundle.items.push_back({
            platform.title,
            L("选择 RetroArch playlists 目录中的对应文件"),
            platform.badge,
            material::DESCRIPTION,
            [this, value = platform.platform]() {
                if (!m_importing.load(std::memory_order_acquire))
                    onSelectLpl(value);
            },
            nullptr,
            false,
        });
    }
    tabs.push_back(std::move(bundle));

    Canvas::Tab scan;
    scan.title = L("扫描导入");
    scan.summary = L("扫描指定目录中的 ROM 文件，根据扩展名识别平台并批量加入游戏库。");
    scan.detail = L("先设置扫描范围和平台开关，再选择目录。关闭全部平台时不会找到可导入的游戏。");
    scan.icon = material::SEARCH;
    scan.items.push_back({
        L("选择 ROM 目录并开始扫描"),
        L("打开目录浏览器，确认后在后台完成扫描和导入"),
        L("选择目录"),
        material::SEARCH,
        [this]() {
            if (!m_importing.load(std::memory_order_acquire))
                selectRomDir();
        },
        nullptr,
        false,
    });
    scan.items.push_back({L("扫描子目录"), L("同时扫描所选目录下的所有子目录，请做好游戏目录分类，部分游戏后缀相同，可能导致导入错误"), "",
                          material::STORAGE, {}, &m_autoSubDir, false});
    scan.items.push_back({L("读取映射名称"), L("存在名称映射时使用中文或规范化标题"), "",
                          material::EDIT, {}, &m_useNameMapping, false});
    scan.items.push_back({L("扫描 GBA 游戏"), L("识别 .gba 文件"), "",
                          material::MEMORY, {}, &m_scanGBA, false});
    scan.items.push_back({L("扫描 GBC 游戏"), L("识别 .gbc 文件"), "",
                          material::MEMORY, {}, &m_scanGBC, false});
    scan.items.push_back({L("扫描 GB 游戏"), L("识别 .gb 文件"), "",
                          material::MEMORY, {}, &m_scanGB, false});
    scan.items.push_back({L("扫描 FC 游戏"), L("识别 .nes 与 .fds 文件"), "",
                          material::MEMORY, {}, &m_scanNES, false});
    scan.items.push_back({L("扫描 SFC 游戏"), L("识别 .sfc 与 .smc 文件"), "",
                          material::MEMORY, {}, &m_scanSNES, false});
    scan.items.push_back({L("扫描 NDS 游戏"), L("识别 .nds 文件"), "",
                          material::MEMORY, {}, &m_scanNDS, false});
    scan.items.push_back({L("扫描 3DS 游戏"), L("识别 .cia、.cci 与 .3ds 文件"), "",
                          material::MEMORY, {}, &m_scan3DS, false});
    scan.items.push_back({L("扫描 MD 游戏"), L("识别 .md、.gen、.bin 与 .smd 文件"), "",
                          material::MEMORY, {}, &m_scanGenesis, false});
    scan.items.push_back({L("扫描 Arcade街机 游戏"), L("识别 .zip 与 .7z 文件"), "",
                          material::MEMORY, {}, &m_scanArcade, false});
    scan.items.push_back({L("扫描 DC 游戏"), L("识别 .cdi、.gdi 与 .chd 文件"), "",
                          material::MEMORY, {}, &m_scanDreamcast, false});
    tabs.push_back(std::move(scan));

    Canvas::Tab process;
    process.title = L("数据处理");
    process.summary = L("维护游戏库记录，或启动局域网 Web 服务进行远程管理。");
    process.detail = L("清理无效记录不会删除 ROM。清空游戏库会删除数据库内容，但不会删除游戏文件和存档。");
    process.icon = material::STORAGE;
    process.items.push_back({
        L("启动 Web 管理服务"),
        L("在同一局域网中上传 ROM、导入存档和修改封面"),
        L("局域网"),
        material::WIFI,
        [this]() { startWebService(); },
        nullptr,
        false,
    });
    process.items.push_back({
        L("安装 CIA 文件"),
        L("解密cia并将CIA游戏安装到数据库中"),
        "3DS",
        material::INSTALL_APP,
        [this]() { launchCiaInstaller(); },
        nullptr,
        false,
    });
    process.items.push_back({
        L("移除无效游戏记录"),
        L("检查 ROM 是否存在，只移除文件已经丢失的数据库记录"),
        L("不会删除 ROM"),
        material::DELETE_SWEEP_ICON,
        [this]() { removeInvalidGames(); },
        nullptr,
        false,
    });
    process.items.push_back({
        L("清空游戏库"),
        L("清除游戏库数据库，保留 ROM 文件和游戏存档"),
        L("危险操作"),
        material::DELETE_ICON,
        [this]() { clearGameLibrary(); },
        nullptr,
        true,
    });
    tabs.push_back(std::move(process));

    m_mainCanvas = new DataManagementCanvas(std::move(tabs), [this]() {
        beiklive::popActivity(this);
    });
    m_bundleDefaultFocus = m_mainCanvas;
    m_scanDefaultFocus = m_mainCanvas;
    m_processDefaultFocus = m_mainCanvas;
    this->getContentBox()->addView(m_mainCanvas);
}

void DataManagementPage::setupProgressOverlay()
{
    m_progressOverlay = new brls::Box(brls::Axis::COLUMN);
    m_progressOverlay->setVisibility(brls::Visibility::GONE);
    m_progressOverlay->setFocusable(true);
    m_progressOverlay->setHideHighlight(true);
    m_progressOverlay->setPositionType(brls::PositionType::ABSOLUTE);
    m_progressOverlay->setPositionTop(0);
    m_progressOverlay->setPositionLeft(0);
    m_progressOverlay->setWidthPercentage(100.f);
    m_progressOverlay->setHeightPercentage(100.f);
    m_progressOverlay->setBackgroundColor(nvgRGBA(0, 0, 0, 140));
    m_progressOverlay->setJustifyContent(brls::JustifyContent::CENTER);
    m_progressOverlay->setAlignItems(brls::AlignItems::CENTER);
    m_progressOverlay->registerAction(L("返回"), brls::BUTTON_B, [](brls::View*) { 
        return true; 
    
    });

    auto* card = new brls::Box(brls::Axis::COLUMN);
    card->setFocusable(false);
    card->setCornerRadius(18.f);
    card->setBackgroundColor(nvgRGBA(30, 30, 35, 235));
    card->setShadowType(brls::ShadowType::GENERIC);
    card->setShadowVisibility(true);
    card->setPadding(34.f, 44.f, 34.f, 44.f);
    card->setWidth(560.f);
    card->setAlignItems(brls::AlignItems::CENTER);

    m_progressTitleLabel = new brls::Label();
    m_progressTitleLabel->setText(L("准备就绪"));
    m_progressTitleLabel->setFontSize(24.f);
    m_progressTitleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_progressTitleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_progressTitleLabel->setMarginBottom(16.f);
    m_progressTitleLabel->setFocusable(false);
    card->addView(m_progressTitleLabel);

    m_progressNameLabel = new brls::Label();
    m_progressNameLabel->setText(" ");
    m_progressNameLabel->setFontSize(28.f);
    m_progressNameLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_progressNameLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_progressNameLabel->setMarginBottom(16.f);
    m_progressNameLabel->setFocusable(false);
    card->addView(m_progressNameLabel);

    m_progressCountLabel = new brls::Label();
    m_progressCountLabel->setText("0 / 0");
    m_progressCountLabel->setFontSize(18.f);
    m_progressCountLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_progressCountLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_progressCountLabel->setMarginBottom(18.f);
    m_progressCountLabel->setFocusable(false);
    card->addView(m_progressCountLabel);

    auto* progressTrack = new brls::Box(brls::Axis::ROW);
    progressTrack->setWidth(400.f);
    progressTrack->setHeight(8.f);
    progressTrack->setCornerRadius(4.f);
    progressTrack->setBackgroundColor(nvgRGBA(255, 255, 255, 30));
    progressTrack->setFocusable(false);

    m_progressBar = new brls::Rectangle(nvgRGB(79, 193, 255));
    m_progressBar->setWidth(0.f);
    m_progressBar->setHeight(8.f);
    m_progressBar->setCornerRadius(4.f);
    m_progressBar->setFocusable(false);
    progressTrack->addView(m_progressBar);

    card->addView(progressTrack);
    m_progressOverlay->addView(card);
    this->addView(m_progressOverlay);
}

void DataManagementPage::showProgressOverlay()
{
    if (!m_progressOverlay)
        return;
    rememberFocusBeforeModal();
    m_progressOverlay->setVisibility(brls::Visibility::VISIBLE);
    brls::Application::giveFocus(m_progressOverlay);
}

void DataManagementPage::hideProgressOverlay()
{
    if (!m_progressOverlay)
        return;
    m_progressOverlay->setVisibility(brls::Visibility::GONE);
    restoreFocusAfterModal();
}

void DataManagementPage::rememberFocusBeforeModal()
{
    brls::View* currentFocus = brls::Application::getCurrentFocus();
    if (!currentFocus || currentFocus == m_progressOverlay || currentFocus->isHidden())
        return;

    m_focusBeforeModal = currentFocus;
}

brls::View* DataManagementPage::getFallbackFocus()
{
    if (m_scanDefaultFocus && !m_scanDefaultFocus->isHidden())
        return m_scanDefaultFocus;

    if (m_bundleDefaultFocus && !m_bundleDefaultFocus->isHidden())
        return m_bundleDefaultFocus;

    if (m_processDefaultFocus && !m_processDefaultFocus->isHidden())
        return m_processDefaultFocus;

    if (m_scanDefaultFocus)
        return m_scanDefaultFocus;

    if (m_bundleDefaultFocus)
        return m_bundleDefaultFocus;

    return m_processDefaultFocus;
}

void DataManagementPage::restoreFocusAfterModal()
{
    brls::View* targetFocus = nullptr;
    if (m_focusBeforeModal && !m_focusBeforeModal->isHidden())
        targetFocus = m_focusBeforeModal;
    else
        targetFocus = getFallbackFocus();

    m_focusBeforeModal = nullptr;

    if (targetFocus)
        brls::Application::giveFocus(targetFocus);
}

brls::View* DataManagementPage::buildScanImportTab()
{
    using beiklive::ui::makeContentBox;
    using beiklive::ui::makeHeader;
    using beiklive::ui::makeHint;
    using beiklive::ui::makeScrollTab;

    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();

    box->addView(makeHeader(L("扫描目录并导入")));

    auto* scanBtn = new beiklive::DetailCell();
    scanBtn->setLeftText(L("选择ROM目录并导入"));
    scanBtn->setRightText("\uE14A");
    scanBtn->registerAction(L("选择"), brls::BUTTON_A, [this](brls::View*) -> bool {
        if (m_importing.load(std::memory_order_acquire))
            return true;
        selectRomDir();
        return true;
    });
    box->addView(scanBtn);
    m_scanDefaultFocus = scanBtn;

    box->addView(makeHint(L("扫描时会根据下面的开关确认扫描对象，默认全部类型都导入，可按自己需要开关")));

    auto* subDirSwitch = new brls::BooleanCell();
    subDirSwitch->init(L("自动扫描子目录"), m_autoSubDir, [this](bool on) { m_autoSubDir = on; });
    box->addView(subDirSwitch);

    auto* nameMapSwitch = new brls::BooleanCell();
    nameMapSwitch->init(L("自动读取映射名称(如果存在)"), m_useNameMapping, [this](bool on) { m_useNameMapping = on; });
    box->addView(nameMapSwitch);

    auto* gbaSwitch = new brls::BooleanCell();
    gbaSwitch->init(L("扫描GBA游戏"), m_scanGBA, [this](bool on) { m_scanGBA = on; });
    box->addView(gbaSwitch);

    auto* gbcSwitch = new brls::BooleanCell();
    gbcSwitch->init(L("扫描GBC游戏"), m_scanGBC, [this](bool on) { m_scanGBC = on; });
    box->addView(gbcSwitch);

    auto* gbSwitch = new brls::BooleanCell();
    gbSwitch->init(L("扫描GB游戏"), m_scanGB, [this](bool on) { m_scanGB = on; });
    box->addView(gbSwitch);

    auto* nesSwitch = new brls::BooleanCell();
    nesSwitch->init(L("扫描FC游戏"), m_scanNES, [this](bool on) { m_scanNES = on; });
    box->addView(nesSwitch);

    auto* snesSwitch = new brls::BooleanCell();
    snesSwitch->init(L("扫描SFC游戏"), m_scanSNES, [this](bool on) { m_scanSNES = on; });
    box->addView(snesSwitch);

    auto* ndsSwitch = new brls::BooleanCell();
    ndsSwitch->init(L("扫描NDS游戏"), m_scanNDS, [this](bool on) { m_scanNDS = on; });
    box->addView(ndsSwitch);

    auto* threeDsSwitch = new brls::BooleanCell();
    threeDsSwitch->init(L("扫描3DS游戏"), m_scan3DS, [this](bool on) { m_scan3DS = on; });
    box->addView(threeDsSwitch);

    auto* genesisSwitch = new brls::BooleanCell();
    genesisSwitch->init(L("扫描MD游戏"), m_scanGenesis, [this](bool on) { m_scanGenesis = on; });
    box->addView(genesisSwitch);

    auto* arcadeSwitch = new brls::BooleanCell();
    arcadeSwitch->init(L("扫描Arcade游戏"), m_scanArcade, [this](bool on) { m_scanArcade = on; });
    box->addView(arcadeSwitch);

    auto* dcSwitch = new brls::BooleanCell();
    dcSwitch->init(L("扫描DC游戏"), m_scanDreamcast, [this](bool on) { m_scanDreamcast = on; });
    box->addView(dcSwitch);


    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

brls::View* DataManagementPage::buildBundleImportTab()
{
    using beiklive::ui::makeContentBox;
    using beiklive::ui::makeHeader;
    using beiklive::ui::makeHint;
    using beiklive::ui::makeScrollTab;

    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();
    box->addView(makeHeader(L("导入 RetroArch 整合包")));

    struct LplButtonConfig
    {
        std::string text;
        std::string icon;
        int platform;
    };

    const LplButtonConfig configs[] = {
        {L("选择GBA游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA)},
        {L("选择GBC游戏的lpl文件"),  "img/ui/icon_gb.png",  static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC)},
        {L("选择GB游戏的lpl文件"),   "img/ui/icon_gb.png",  static_cast<int>(beiklive::enums::EmuPlatform::EmuGB)},
        {L("选择FC游戏的lpl文件"),   "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuNES)},
        {L("选择SFC游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuSNES)},
        {L("选择NDS游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)},
        {L("选择3DS游戏的lpl文件"),  "img/ui/3ds.png", static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS)},
        {L("选择MD游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis)},
        {L("选择Arcade游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade)},
        {L("选择DC游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast)},
        {L("选择PSP游戏的lpl文件"),  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP)},
    };
    
    box->addView(makeHint(L("lpl 文件通常位于 RetroArch 的 playlists 目录下，不懂lpl文件语法规则不要自行删改")));
    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); ++i)
    {
        const auto& config = configs[i];
        auto* btn = new beiklive::DetailCell();
        btn->setLeftText(config.text);
        btn->setRightText("\uE14A");
        btn->registerAction(L("选择"), brls::BUTTON_A, [this, config](brls::View*) -> bool {
            if (m_importing.load(std::memory_order_acquire))
                return true;
            onSelectLpl(config.platform);
            return true;
        });
        box->addView(btn);

        if (i == 0)
            m_bundleDefaultFocus = btn;
    }


    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

brls::View* DataManagementPage::buildDataProcessingTab()
{
    using beiklive::ui::makeContentBox;
    using beiklive::ui::makeHeader;
    using beiklive::ui::makeHint;
    using beiklive::ui::makeScrollTab;

    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();

    box->addView(makeHeader(L("库数据处理")));

    auto* webCell = new beiklive::DetailCell();
    webCell->setLeftText(L("启动 Web 管理服务"));
    webCell->setRightText("\uE14A");
    webCell->registerAction(L("启动"), brls::BUTTON_A, [this](brls::View*) -> bool {
        startWebService();
        return true;
    });
    box->addView(webCell);
    m_processDefaultFocus = webCell;

    box->addView(makeHint(L("启动后可在同一局域网浏览器中管理游戏库、上传 ROM、导入存档和修改封面。")));

    auto* cleanCell = new beiklive::DetailCell();
    cleanCell->setLeftText(L("从库中移除无效游戏"));
    cleanCell->setRightText("\uE14A");
    cleanCell->registerAction(L("打开"), brls::BUTTON_A, [this](brls::View*) -> bool {
        removeInvalidGames();
        return true;
    });
    box->addView(cleanCell);

    box->addView(makeHint(L("移除游戏库中仍有记录，但 ROM 文件已经不存在的游戏。")));

    auto* clearCell = new beiklive::DetailCell();
    clearCell->setLeftText(L("清空游戏库"));
    clearCell->setRightText("\uE14A");
    clearCell->registerAction(L("打开"), brls::BUTTON_A, [this](brls::View*) -> bool {
        clearGameLibrary();
        return true;
    });
    box->addView(clearCell);

    box->addView(makeHint(L("此功能不会删除游戏文件和存档，仅清空 GBAStation/data 下的游戏库数据。")));

    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

void DataManagementPage::resetProgressUi(const std::string& title)
{
    m_completionShown = false;
    m_cleanupRemoved.store(0, std::memory_order_release);
    m_importSkipped.store(0, std::memory_order_release);
    m_importDone.store(false, std::memory_order_release);
    m_importError.store(false, std::memory_order_release);
    m_progress.store(0, std::memory_order_release);
    m_total.store(0, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_errorMsg.clear();
        m_progressName.clear();
    }

    showProgressOverlay();
    m_progressTitleLabel->setText(title);
    m_progressNameLabel->setText(" ");
    m_progressCountLabel->setText("0 / 0");
    m_progressBar->setWidth(0.f);
    m_progressBar->setColor(nvgRGB(79, 193, 255));

    invalidate();
}

void DataManagementPage::updateProgressName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_progressName = name;
}

void DataManagementPage::setErrorMessage(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_errorMsg = msg;
}

void DataManagementPage::finishWorker()
{
    if (m_importThread.joinable())
        m_importThread.join();
}

void DataManagementPage::onSelectLpl(int platform)
{
    auto* flPage = new beiklive::FileListPage();
    flPage->setFliter(beiklive::enums::FilterMode::Whitelist, {"lpl"});
    flPage->onRequestClose = []() {
        brls::Application::popActivity(brls::TransitionAnimation::FADE);
    };
    flPage->onFileSelected = [this, platform, flPage](beiklive::DirListData item) {
        if (item.itemType == beiklive::enums::FileType::DRIVE ||
            item.itemType == beiklive::enums::FileType::DIRECTORY)
            return;

        std::string selectedPath = item.fullPath;
        flPage->onRequestClose = [this, selectedPath, platform]() {
            brls::Application::popActivity(
                brls::TransitionAnimation::FADE,
                [this, selectedPath, platform]() {
                    rememberFocusBeforeModal();
                    auto* content = new LplImportConfirmView(
                        fileNameFromPath(selectedPath),
                        beiklive::tools::platformName(platform));
                    auto* dialog = new brls::Dialog(content);
                    dialog->addButton(L("取消"), [this]() {
                        restoreFocusAfterModal();
                    });
                    dialog->addButton(
                        L("确定导入"), [this, selectedPath, platform]() {
                            startImport(selectedPath, platform);
                        });
                    dialog->open();
                });
        };
        flPage->requestClose();
    };

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->addView(flPage);
    container->registerAction(L("关闭"), brls::BUTTON_START,
                              [flPage](brls::View*) {
                                  flPage->requestClose();
                                  return true;
                              });

    auto* frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame));

    flPage->setPath("/retroarch/playlists");
}

void DataManagementPage::startImport(const std::string& lplPath, int platform)
{
    m_progressTask = ProgressTask::Import;
    resetProgressUi(L("正在解析LPL文件..."));
    finishWorker();

    std::string realPath = expandTilde(lplPath);
    std::ifstream ifs(realPath);
    if (!ifs.is_open())
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog(L("无法打开LPL文件"));
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();

    json lplJson;
    try
    {
        lplJson = json::parse(buffer.str());
    }
    catch (const std::exception& e)
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog(L("LPL文件解析失败"));
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    if (!lplJson.contains("items") || !lplJson["items"].is_array())
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog(L("LPL文件无数据"));
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    int unexpectedPlatform = findUnexpectedLplPlatform(lplJson["items"], platform);
    if (unexpectedPlatform >= 0)
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog(
            L("选择错误\n\n当前选择的是 ") +
            beiklive::tools::platformName(platform) +
            L(" 游戏的lpl导入按钮，但文件中的游戏类型是 ") +
            beiklive::tools::platformName(unexpectedPlatform) +
            L("。\n请返回后选择对应类型的按钮。"));
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    std::vector<ImportItem> importItems;
    for (const auto& item : lplJson["items"])
    {
        importItems.push_back({
            item.value("path", ""),
            item.value("label", ""),
        });
    }

    m_total.store(static_cast<int>(importItems.size()), std::memory_order_release);
    m_progressCountLabel->setText("0 / " + std::to_string(importItems.size()));
    m_progressTitleLabel->setText(L("正在导入游戏数据，请勿操作"));

    ImportSharedConfig config = buildSharedConfig(platform);
    m_importing.store(true, std::memory_order_release);

    m_importThread = std::thread([this, importItems = std::move(importItems), config, lplPath]() {
        std::string lplStem = stemFromPath(expandTilde(lplPath));
        for (int i = 0; i < static_cast<int>(importItems.size()); ++i)
        {
            const auto& item = importItems[i];
            std::string romPath = expandTilde(item.romPath);

            if (romPath.empty() || item.label.empty() || !fs::exists(romPath))
            {
                m_progress.store(i + 1, std::memory_order_release);
                continue;
            }

            // 游戏库中已存在该 ROM：跳过，绝不覆盖用户已有的独立配置
            // （遮罩/着色器/显示/金手指/收藏/游玩统计等）。
            if (beiklive::GameDB->findByPath(romPath))
            {
                m_importSkipped.fetch_add(1, std::memory_order_relaxed);
                m_progress.store(i + 1, std::memory_order_release);
                continue;
            }

            updateProgressName(item.label);
            std::string romStem = stemFromPath(romPath);

            std::string logoPath;
            std::string thumbPath = romPath;
            size_t romsPos = thumbPath.find("roms");
            if (romsPos != std::string::npos)
            {
                thumbPath.replace(romsPos, 4, "retroarch/thumbnails");
                std::string thumbDir = parentPath(thumbPath);
                std::string logoFile = thumbDir + "/Named_Snaps/" + romStem + ".png";
#ifdef _WIN32
                std::string altLogo = logoFile;
                for (auto& c : altLogo)
                    if (c == '/') c = '\\';
                if (fs::exists(altLogo))
                    logoPath = altLogo;
                else
#endif
                    logoPath = logoFile;
            }

            if (logoPath.empty() || !fs::exists(logoPath))
            {
                logoPath = beiklive::tools::getDefaultLogoPath(
                    static_cast<beiklive::enums::EmuPlatform>(config.platform),
                    romPath);
            }

            std::string savePath = (fs::path(beiklive::path::savePath()) /
                                    "retroarch" /
                                    lplStem /
                                    romStem).string();

            try
            {
                fs::create_directories(savePath);
            }
            catch (...)
            {
            }

            beiklive::GameEntry entry;
            entry.path = romPath;
            entry.title = item.label;
            entry.platform = config.platform;
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
                entry.threeDsTitleId = beiklive::three_ds::readNcsdTitleId(romPath);
            entry.logoPath = logoPath;
            entry.savePath = savePath;
            entry.overlayPath = config.overlayPath;
            entry.shaderPath = config.shaderPath;
            entry.overlayEnabled = config.overlayEnabled;
            entry.shaderEnabled = config.shaderEnabled;
            applyDisplayDefaults(entry);
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                entry.ndsScreenLayout = "priority_top";
                entry.ndsScreenOrientation = "0";
                entry.ndsIntegerScale = true;
                entry.ndsScreenGap = 0;
                entry.ndsBottomOpacity = 1.0f;
            }

            beiklive::GameDB->upsertByPath(entry);
            m_progress.store(i + 1, std::memory_order_release);
        }

        m_importDone.store(true, std::memory_order_release);
    });
}

void DataManagementPage::selectRomDir()
{
    auto* flPage = new beiklive::FileListPage();
    flPage->setDirSelectionMode(true);
    flPage->registerAction(L("选择目录"), brls::BUTTON_Y, [this, flPage](brls::View*) -> bool {
        std::string dirPath = flPage->getHeader()->getPath();
        if (dirPath.empty())
            return true;

        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        startDirImport(dirPath);
        return true;
    });

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->addView(flPage);
    container->registerAction(L("关闭"), brls::BUTTON_START,
                              [](brls::View*) { brls::Application::popActivity(); return true; });

    auto* frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame));

    flPage->showDriveList();
}

void DataManagementPage::startDirImport(const std::string& dirPath)
{
    m_progressTask = ProgressTask::Import;
    resetProgressUi(L("正在扫描ROM文件..."));
    finishWorker();

    std::unordered_set<std::string> exts;
    if (m_scanGBA) exts.insert("gba");
    if (m_scanGBC) exts.insert("gbc");
    if (m_scanGB) exts.insert("gb");
    if (m_scanNES) { exts.insert("nes"); exts.insert("fds"); }
    if (m_scanSNES) { exts.insert("sfc"); exts.insert("smc"); }
    if (m_scanNDS) exts.insert("nds");
    if (m_scan3DS) { exts.insert("cia"); exts.insert("cci"); exts.insert("3ds"); }
    if (m_scanGenesis) { exts.insert("md"); exts.insert("gen"); exts.insert("bin"); exts.insert("smd"); }
    if (m_scanArcade) { exts.insert("zip"); exts.insert("7z"); }
    if (m_scanDreamcast) { exts.insert("cdi"); exts.insert("gdi"); exts.insert("chd"); }

    m_importing.store(true, std::memory_order_release);

    m_importThread = std::thread([this, dirPath, exts = std::move(exts)]() {
        std::vector<fs::path> roms;
        try
        {
            if (m_autoSubDir)
            {
                for (auto& entry : fs::recursive_directory_iterator(dirPath))
                {
                    if (!entry.is_regular_file())
                        continue;

                    std::string ext = normalizeExtension(entry.path().extension().string());
                    if (exts.count(ext))
                        roms.push_back(entry.path());
                }
            }
            else
            {
                for (auto& entry : fs::directory_iterator(dirPath))
                {
                    if (!entry.is_regular_file())
                        continue;

                    std::string ext = normalizeExtension(entry.path().extension().string());
                    if (exts.count(ext))
                        roms.push_back(entry.path());
                }
            }
        }
        catch (const std::exception& e)
        {
            setErrorMessage(e.what());
            m_importError.store(true, std::memory_order_release);
            m_importDone.store(true, std::memory_order_release);
            return;
        }

        m_total.store(static_cast<int>(roms.size()), std::memory_order_release);

        for (int i = 0; i < static_cast<int>(roms.size()); ++i)
        {
            const auto& romPath = roms[i];
            std::string path = romPath.string();
            std::string romStem = romPath.stem().string();
            int platform = platformFromExistingPath(romPath);
            if (platform < 0)
            {
                m_progress.store(i + 1, std::memory_order_release);
                continue;
            }

            // 游戏库中已存在该 ROM：跳过，绝不覆盖用户已有的独立配置
            // （遮罩/着色器/显示/金手指/收藏/游玩统计等）。扫描导入只追加
            // 新游戏。
            if (beiklive::GameDB->findByPath(path))
            {
                m_importSkipped.fetch_add(1, std::memory_order_relaxed);
                m_progress.store(i + 1, std::memory_order_release);
                continue;
            }

            std::string displayName = romStem;
            if (m_useNameMapping)
            {
                auto nameVal = beiklive::NameMappingManager->Get(romStem);
                if (nameVal)
                {
                    auto nameStr = nameVal->AsString();
                    if (nameStr && !nameStr->empty())
                        displayName = *nameStr;
                }
            }

            updateProgressName(displayName);
            ImportSharedConfig config = buildSharedConfig(platform);

            beiklive::GameEntry entry;
            entry.path = path;
            entry.title = displayName;
            entry.platform = platform;
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
                entry.threeDsTitleId = beiklive::three_ds::readNcsdTitleId(path);
            entry.logoPath = beiklive::tools::getDefaultLogoPath(
                static_cast<beiklive::enums::EmuPlatform>(platform),
                path);
            std::string savePath = beiklive::tools::defaultGameSavePath(platform, path);
            try
            {
                fs::create_directories(savePath);
            }
            catch (...)
            {
            }
            entry.savePath = savePath;
            entry.overlayEnabled = config.overlayEnabled;
            entry.shaderEnabled = config.shaderEnabled;
            entry.overlayPath = config.overlayPath;
            entry.shaderPath = config.shaderPath;

            applyDisplayDefaults(entry);
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                entry.ndsScreenLayout = "priority_top";
                entry.ndsScreenOrientation = "0";
                entry.ndsIntegerScale = true;
                entry.ndsScreenGap = 0;
                entry.ndsBottomOpacity = 1.0f;
            }

            beiklive::GameDB->upsertByPath(entry);
            m_progress.store(i + 1, std::memory_order_release);
        }

        m_importDone.store(true, std::memory_order_release);
    });
}

void DataManagementPage::removeInvalidGames()
{
    rememberFocusBeforeModal();

    auto* dialog = new brls::Dialog(
        L("确定要从游戏库中移除无效游戏吗？\n\n此操作将删除数据库中 ROM 文件已不存在的游戏记录。"));
    dialog->addButton("取消", [this]() { restoreFocusAfterModal(); });
    dialog->addButton(L("确认移除"), [this]() {
        m_progressTask = ProgressTask::Cleanup;
        resetProgressUi(L("正在扫描无效游戏..."));
        finishWorker();
        m_progressTitleLabel->setText(L("正在扫描无效游戏，请勿操作"));
        m_importing.store(true, std::memory_order_release);

        m_importThread = std::thread([this]() {
            auto entries = beiklive::GameDB ? beiklive::GameDB->getAll()
                                            : std::vector<beiklive::GameEntry>{};
            int total = static_cast<int>(entries.size());
            int removed = 0;
            m_total.store(total, std::memory_order_release);

            try
            {
                if (beiklive::GameDB)
                {
                    for (int i = 0; i < total; ++i)
                    {
                        const auto& entry = entries[i];
                        updateProgressName(entry.title.empty() ? fileNameFromPath(entry.path)
                                                               : entry.title);

                        if (!fs::exists(entry.path) &&
                            beiklive::GameDB->removeByPath(entry.path))
                            removed++;

                        m_progress.store(i + 1, std::memory_order_release);
                    }

                    if (removed == total && total > 0)
                        beiklive::GameDB->clearAll();
                    else if (removed > 0)
                        beiklive::GameDB->flush();
                }
            }
            catch (const std::exception& e)
            {
                setErrorMessage(e.what());
                m_importError.store(true, std::memory_order_release);
                m_importDone.store(true, std::memory_order_release);
                return;
            }

            m_cleanupRemoved.store(removed, std::memory_order_release);
            m_importDone.store(true, std::memory_order_release);
        });
    });
    dialog->open();
}

void DataManagementPage::clearGameLibrary()
{
    rememberFocusBeforeModal();

    auto* firstDialog = new brls::Dialog(L("确定要清空游戏库吗？"));
    firstDialog->addButton("取消", [this]() { restoreFocusAfterModal(); });
    firstDialog->addButton(L("确定"), [this]() {
        auto* secondDialog = new brls::Dialog(L("真的要清空游戏库吗？\n数据都会丢失哦。"));
        secondDialog->addButton("取消", [this]() { restoreFocusAfterModal(); });
        secondDialog->addButton(L("确定"), [this]() {
            bool success = true;

            if (beiklive::GameDB)
                beiklive::GameDB->clearAll();

            success = clearDirectoryContents(beiklive::path::databasePath()) && success;

            restoreFocusAfterModal();
            brls::Application::notify(success ? L("游戏库数据已清空") : L("清空游戏库时发生错误"));
        });
        secondDialog->open();
    });
    firstDialog->open();
}

void DataManagementPage::launchCiaInstaller()
{
    if (m_importing.load(std::memory_order_acquire))
        return;

#ifndef __SWITCH__
    brls::Application::notify(L("CIA安装器仅支持Switch"));
#else
    exportThreeDsCoreConfigForDataPage();
    const std::string nroPath = GET_SETTING_KEY_STR(
        "3ds.externalNro.path", "/GBAStation/core/GBAStation3DSStub.nro");
    const std::string returnPath = GET_SETTING_KEY_STR(
        "3ds.externalNro.returnPath", "sdmc:/switch/GBAStation.nro");

    auto result = beiklive::switch_platform::launchNroOnExit({
        nroPath,
        "",
        returnPath,
        {"--install-cia"},
    });
    if (!result.success)
    {
        brls::Logger::error("3DS CIA installer launch failed: {}", result.message);
        brls::Application::notify(L("CIA安装器启动失败：") + result.message);
        return;
    }

    brls::Logger::info("3DS CIA installer configured: {}", result.message);
    brls::Application::notify(L("正在启动CIA安装器..."));
    brls::sync([]() { brls::Application::quit(); });
#endif
}

void DataManagementPage::startWebService()
{
    int port = GET_SETTING_KEY_INT("web.port", 8080);
    bool started = beiklive::network::WebService::Start(port);
    if (!started)
    {
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog(
            L("Web 管理服务启动失败\n\n") +
            beiklive::network::WebService::LastError() +
            L("\n\n请确认网络已连接，端口未被占用。"));
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    rememberFocusBeforeModal();
    std::string url = beiklive::network::WebService::Url();
    brls::Style style = brls::Application::getStyle();

    auto* content = new brls::Box(brls::Axis::COLUMN);
    content->setAlignItems(brls::AlignItems::CENTER);
    content->setJustifyContent(brls::JustifyContent::CENTER);
    content->setPadding(style["brls/dialog/paddingTopBottom"],
                        style["brls/dialog/paddingLeftRight"],
                        style["brls/dialog/paddingTopBottom"],
                        style["brls/dialog/paddingLeftRight"]);

    auto* title = new brls::Label();
    title->setText(L("Web 管理服务已启动"));
    title->setFontSize(style["brls/dialog/fontSize"]);
    title->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    title->setSingleLine(false);
    content->addView(title);

    auto* qr = new QRCodeView(url);
    qr->setMargins(24.0f, 0.0f, 20.0f, 0.0f);
    content->addView(qr);

    auto* address = new brls::Label();
    address->setText(L("访问地址:\n") + url);
    address->setFontSize(style["brls/dialog/fontSize"] * 0.82f);
    address->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    address->setSingleLine(false);
    content->addView(address);

    auto* hint = new brls::Label();
    hint->setText("\n" + beiklive::network::WebService::KeepAwakeMessage() +
                  L("\n关闭此窗口会停止 Web 服务"));
    hint->setFontSize(style["brls/dialog/fontSize"] * 0.72f);
    hint->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    hint->setSingleLine(false);
    content->addView(hint);

    auto* dialog = new brls::Dialog(content);
    dialog->setCancelable(false);
    dialog->addButton(L("关闭服务"), [this]() {
        beiklive::network::WebService::Stop();
        restoreFocusAfterModal();
    });
    dialog->open();
}

} // namespace beiklive
