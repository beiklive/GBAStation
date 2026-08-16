#include "common.h"
#include "Translation.hpp"
#include "ui/widget/Box.hpp"
#include "core/cheat/CheatSystem.hpp"
#include "game/control/InputMappingDefaults.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive
{
    namespace
    {
        uint32_t readLe32(const std::array<uint8_t, 4>& bytes)
        {
            return static_cast<uint32_t>(bytes[0]) |
                   (static_cast<uint32_t>(bytes[1]) << 8) |
                   (static_cast<uint32_t>(bytes[2]) << 16) |
                   (static_cast<uint32_t>(bytes[3]) << 24);
        }

        uint16_t readLe16(const uint8_t* bytes)
        {
            return static_cast<uint16_t>(bytes[0]) |
                   (static_cast<uint16_t>(bytes[1]) << 8);
        }

        uint32_t crc32ForPng(const uint8_t* data, size_t size)
        {
            uint32_t crc = 0xFFFFFFFFu;
            for (size_t i = 0; i < size; ++i)
            {
                crc ^= data[i];
                for (int bit = 0; bit < 8; ++bit)
                    crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
            }
            return ~crc;
        }

        uint32_t adler32(const std::vector<uint8_t>& data)
        {
            uint32_t a = 1;
            uint32_t b = 0;
            for (uint8_t byte : data)
            {
                a = (a + byte) % 65521u;
                b = (b + a) % 65521u;
            }
            return (b << 16) | a;
        }

        void appendBe32(std::vector<uint8_t>& out, uint32_t value)
        {
            out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(value & 0xFF));
        }

        void appendPngChunk(std::vector<uint8_t>& png, const char type[4], const std::vector<uint8_t>& data)
        {
            appendBe32(png, static_cast<uint32_t>(data.size()));
            const size_t typeOffset = png.size();
            png.insert(png.end(), type, type + 4);
            png.insert(png.end(), data.begin(), data.end());
            appendBe32(png, crc32ForPng(png.data() + typeOffset, 4 + data.size()));
        }

        bool writeRgbaPng(const fs::path& path, const std::array<uint8_t, 32 * 32 * 4>& rgba)
        {
            std::vector<uint8_t> scanlines;
            scanlines.reserve((32 * 4 + 1) * 32);
            for (int y = 0; y < 32; ++y)
            {
                scanlines.push_back(0);
                const size_t rowOffset = static_cast<size_t>(y) * 32 * 4;
                scanlines.insert(scanlines.end(), rgba.begin() + rowOffset, rgba.begin() + rowOffset + 32 * 4);
            }

            std::vector<uint8_t> zlib;
            zlib.reserve(scanlines.size() + 16);
            zlib.push_back(0x78);
            zlib.push_back(0x01);

            size_t offset = 0;
            while (offset < scanlines.size())
            {
                const uint16_t blockSize = static_cast<uint16_t>(
                    std::min<size_t>(65535, scanlines.size() - offset));
                const bool finalBlock = (offset + blockSize) == scanlines.size();
                zlib.push_back(finalBlock ? 0x01 : 0x00);
                zlib.push_back(static_cast<uint8_t>(blockSize & 0xFF));
                zlib.push_back(static_cast<uint8_t>((blockSize >> 8) & 0xFF));
                const uint16_t nlen = static_cast<uint16_t>(~blockSize);
                zlib.push_back(static_cast<uint8_t>(nlen & 0xFF));
                zlib.push_back(static_cast<uint8_t>((nlen >> 8) & 0xFF));
                zlib.insert(zlib.end(), scanlines.begin() + offset, scanlines.begin() + offset + blockSize);
                offset += blockSize;
            }
            appendBe32(zlib, adler32(scanlines));

            std::vector<uint8_t> png = {137, 80, 78, 71, 13, 10, 26, 10};

            std::vector<uint8_t> ihdr;
            ihdr.reserve(13);
            appendBe32(ihdr, 32);
            appendBe32(ihdr, 32);
            ihdr.push_back(8);
            ihdr.push_back(6);
            ihdr.push_back(0);
            ihdr.push_back(0);
            ihdr.push_back(0);
            appendPngChunk(png, "IHDR", ihdr);
            appendPngChunk(png, "IDAT", zlib);
            appendPngChunk(png, "IEND", {});

            std::ofstream out(path, std::ios::binary);
            if (!out)
                return false;
            out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
            return out.good();
        }

        bool decodeNdsIcon(const std::string& romPath, std::array<uint8_t, 32 * 32 * 4>& rgba)
        {
            std::ifstream rom(romPath, std::ios::binary);
            if (!rom)
                return false;

            rom.seekg(0x68, std::ios::beg);
            std::array<uint8_t, 4> offsetBytes {};
            rom.read(reinterpret_cast<char*>(offsetBytes.data()), static_cast<std::streamsize>(offsetBytes.size()));
            if (rom.gcount() != static_cast<std::streamsize>(offsetBytes.size()))
                return false;

            const uint32_t bannerOffset = readLe32(offsetBytes);
            if (bannerOffset == 0)
                return false;

            std::array<uint8_t, 512> icon {};
            std::array<uint8_t, 16 * 2> paletteBytes {};
            rom.seekg(static_cast<std::streamoff>(bannerOffset) + 0x20, std::ios::beg);
            rom.read(reinterpret_cast<char*>(icon.data()), static_cast<std::streamsize>(icon.size()));
            if (rom.gcount() != static_cast<std::streamsize>(icon.size()))
                return false;

            rom.seekg(static_cast<std::streamoff>(bannerOffset) + 0x220, std::ios::beg);
            rom.read(reinterpret_cast<char*>(paletteBytes.data()), static_cast<std::streamsize>(paletteBytes.size()));
            if (rom.gcount() != static_cast<std::streamsize>(paletteBytes.size()))
                return false;

            rgba.fill(0);
            for (int tileY = 0; tileY < 4; ++tileY)
            {
                for (int tileX = 0; tileX < 4; ++tileX)
                {
                    for (int py = 0; py < 8; ++py)
                    {
                        for (int px = 0; px < 8; ++px)
                        {
                            const int tileIndex = tileY * 4 + tileX;
                            const int iconOffset = tileIndex * 32 + py * 4 + px / 2;
                            const uint8_t packed = icon[static_cast<size_t>(iconOffset)];
                            const uint8_t colorIndex = (px & 1) ? (packed >> 4) : (packed & 0x0F);

                            const uint16_t color = readLe16(&paletteBytes[static_cast<size_t>(colorIndex) * 2]);
                            const int x = tileX * 8 + px;
                            const int y = tileY * 8 + py;
                            const size_t outOffset = (static_cast<size_t>(y) * 32 + x) * 4;
                            const uint8_t r5 = static_cast<uint8_t>(color & 0x1F);
                            const uint8_t g5 = static_cast<uint8_t>((color >> 5) & 0x1F);
                            const uint8_t b5 = static_cast<uint8_t>((color >> 10) & 0x1F);
                            rgba[outOffset + 0] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
                            rgba[outOffset + 1] = static_cast<uint8_t>((g5 << 3) | (g5 >> 2));
                            rgba[outOffset + 2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
                            rgba[outOffset + 3] = colorIndex == 0 ? 0 : 255;
                        }
                    }
                }
            }
            return true;
        }

        uint64_t fnv1a64(const std::string& text)
        {
            uint64_t hash = 14695981039346656037ull;
            for (unsigned char c : text)
            {
                hash ^= c;
                hash *= 1099511628211ull;
            }
            return hash;
        }

        std::string hex64(uint64_t value)
        {
            std::ostringstream oss;
            oss << std::hex << std::setw(16) << std::setfill('0') << value;
            return oss.str();
        }

        std::mutex g_ndsIconCacheMutex;
        std::unordered_map<std::string, std::string> g_ndsIconPathMemo;
    }

    ConfigManager *SettingManager = nullptr;     // 全局配置管理器实例
    ConfigManager *NameMappingManager = nullptr; // 全局名称映射管理器实例
    GameDatabase *GameDB = nullptr;              // 全局游戏数据库实例

    std::vector<brls::Box *> g_beiklive_boxes; // 全局盒子列表

    std::vector<FloatingIcon> g_backgroundIcons;
    float g_backgroundLastTime = 0.0f;
    std::unordered_set<std::string> g_forceRefreshPaths;

    GradientTheme g_gradientTheme = GradientTheme::Midnight;

    namespace
    {
        unsigned char uiAlpha(float alpha)
        {
            return static_cast<unsigned char>(std::max(0.0f, std::min(alpha, 1.0f)) * 255.0f);
        }

        bool useLightForeground()
        {
            return getUiThemeMode() == UiThemeMode::Dark;
        }
    }

    UiThemeMode getUiThemeMode()
    {
        return GET_SETTING_KEY_STR(SettingKey::KEY_UI_THEME, "dark") == "light"
            ? UiThemeMode::Light
            : UiThemeMode::Dark;
    }

    NVGcolor uiTextPrimary(float alpha)
    {
        if (useLightForeground())
            return nvgRGBA(248, 250, 252, uiAlpha(alpha));
        return nvgRGBA(15, 23, 42, uiAlpha(alpha));
    }

    NVGcolor uiTextSecondary(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(203, 213, 225, uiAlpha(alpha))
            : nvgRGBA(71, 85, 105, uiAlpha(alpha));
    }

    NVGcolor uiTextMuted(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(148, 163, 184, uiAlpha(alpha))
            : nvgRGBA(100, 116, 139, uiAlpha(alpha));
    }

    NVGcolor uiIconPrimary(float alpha) { return uiTextPrimary(alpha); }

    NVGcolor uiDivider(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(255, 255, 255, uiAlpha(alpha * 0.24f))
            : nvgRGBA(15, 23, 42, uiAlpha(alpha * 0.28f));
    }

    NVGcolor uiSurface(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(12, 18, 32, uiAlpha(alpha))
            : nvgRGBA(255, 255, 255, uiAlpha(alpha));
    }

    NVGcolor uiPanelSurface(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(255, 255, 255, uiAlpha(alpha))
            : nvgRGBA(248, 250, 252, uiAlpha(alpha));
    }

    NVGcolor uiPanelSubtle(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(255, 255, 255, uiAlpha(alpha))
            : nvgRGBA(15, 23, 42, uiAlpha(alpha));
    }

    NVGcolor uiDialogSurface(float alpha)
    {
        return useLightForeground()
            ? nvgRGBA(24, 31, 43, uiAlpha(alpha))
            : nvgRGBA(248, 250, 252, uiAlpha(alpha));
    }

    NVGcolor uiAccent(float alpha)
    {
        return getUiThemeMode() == UiThemeMode::Dark
            ? nvgRGBA(79, 193, 255, uiAlpha(alpha))
            : nvgRGBA(0, 102, 204, uiAlpha(alpha));
    }

    void ApplyUiTheme()
    {
        const bool light = getUiThemeMode() == UiThemeMode::Light;
        auto& theme = light ? brls::Theme::getLightTheme() : brls::Theme::getDarkTheme();
        theme.addColor("brls/text", uiTextPrimary());
        theme.addColor("brls/text_disabled", uiTextMuted());
        theme.addColor("brls/applet_frame/separator", uiDivider());
        theme.addColor("brls/sidebar/background", uiSurface(light ? 0.88f : 0.58f));
        theme.addColor("beiklive/CardText/color", uiTextPrimary());
        theme.addColor("beiklive/sidePanel", uiSurface(light ? 0.92f : 0.72f));
        theme.addColor("beiklive/subtitle", uiTextSecondary(0.80f));
        theme.addColor("beiklive/line", uiDivider());
        theme.addColor("brls/button/primary_enabled_background", light
            ? nvgRGBA(0, 102, 204, 235) : nvgRGBA(79, 193, 255, 235));
        theme.addColor("brls/button/primary_disabled_background", light
            ? nvgRGBA(148, 163, 184, 120) : nvgRGBA(100, 116, 139, 120));
        theme.addColor("brls/button/default_enabled_background", light
            ? nvgRGBA(226, 232, 240, 235) : nvgRGBA(255, 255, 255, 28));
        theme.addColor("brls/button/default_disabled_background", light
            ? nvgRGBA(226, 232, 240, 120) : nvgRGBA(255, 255, 255, 12));
        brls::Application::getPlatform()->setThemeVariant(
            light ? brls::ThemeVariant::LIGHT : brls::ThemeVariant::DARK);
    }

    GradientTheme gradientThemeFromId(const std::string& id)
    {
        if (id == "LemonYellow") return GradientTheme::LemonYellow;
        if (id == "AvocadoGreen") return GradientTheme::AvocadoGreen;
        if (id == "StrawberryRed") return GradientTheme::StrawberryRed;
        if (id == "OceanBlue") return GradientTheme::OceanBlue;
        if (id == "SakuraPink") return GradientTheme::SakuraPink;
        if (id == "AuroraTeal") return GradientTheme::AuroraTeal;
        if (id == "RoyalPurple") return GradientTheme::RoyalPurple;
        if (id == "SunsetOrange") return GradientTheme::SunsetOrange;
        if (id == "Graphite") return GradientTheme::Graphite;
        if (id == "CloudWhite") return GradientTheme::CloudWhite;
        if (id == "VscodeBlack") return GradientTheme::VscodeBlack;
        return GradientTheme::Midnight;
    }

    void GetGradientColors(NVGcolor &top, NVGcolor &bottom)
    {
        switch (g_gradientTheme)
        {
        case GradientTheme::LemonYellow:
            top = nvgRGBA(255, 235, 59, 128);
            bottom = nvgRGBA(251, 140, 0, 128);
            break;
        case GradientTheme::AvocadoGreen:
            top = nvgRGBA(136, 189, 111, 128);
            bottom = nvgRGBA(46, 88, 36, 128);
            break;
        case GradientTheme::StrawberryRed:
            top = nvgRGBA(255, 107, 107, 128);
            bottom = nvgRGBA(168, 28, 56, 128);
            break;
        case GradientTheme::OceanBlue:
            top = nvgRGBA(79, 172, 254, 128);
            bottom = nvgRGBA(0, 102, 204, 128);
            break;
        case GradientTheme::SakuraPink:
            top = nvgRGBA(255, 183, 178, 128);
            bottom = nvgRGBA(255, 105, 180, 128);
            break;
        case GradientTheme::VscodeBlack:
            top = nvgRGBA(118, 118, 118, 128);
            bottom = nvgRGBA(12, 12, 12, 128);
            break;
        case GradientTheme::AuroraTeal:
            top = nvgRGBA(45, 212, 191, 128);
            bottom = nvgRGBA(12, 74, 110, 128);
            break;
        case GradientTheme::RoyalPurple:
            top = nvgRGBA(124, 58, 237, 128);
            bottom = nvgRGBA(49, 20, 92, 128);
            break;
        case GradientTheme::SunsetOrange:
            top = nvgRGBA(251, 146, 60, 128);
            bottom = nvgRGBA(190, 24, 93, 128);
            break;
        case GradientTheme::Graphite:
            top = nvgRGBA(71, 85, 105, 128);
            bottom = nvgRGBA(15, 23, 42, 128);
            break;
        case GradientTheme::CloudWhite:
            top = nvgRGBA(241, 245, 249, 180);
            bottom = nvgRGBA(186, 230, 253, 180);
            break;
        case GradientTheme::Midnight:
        default:
            top = nvgRGBA(20, 28, 60, 128);
            bottom = nvgRGBA(8, 10, 22, 128);
            break;
        }
    }

    static float randRange(float min, float max)
    {
        float t = (float)std::rand() / (float)RAND_MAX;
        return min + (max - min) * t;
    }

    void InitBackgroundIcons()
    {
        std::srand((unsigned int)std::time(nullptr));
        g_backgroundIcons.clear();

        for (int i = 0; i < 24; i++)
        {
            FloatingIcon icon;
            icon.x = randRange(0.0f, 1280.0f);
            icon.y = randRange(0.0f, 720.0f);
            icon.speedX = randRange(-8.0f, 8.0f);
            icon.speedY = randRange(-22.0f, -8.0f);
            icon.size = randRange(24.0f, 60.0f);
            icon.rotation = randRange(0.0f, 6.28f);
            icon.rotateSpeed = randRange(-0.6f, 0.6f);
            icon.alpha = randRange(0.05f, 0.16f);
            icon.symbolIndex = std::rand() % 4;
            g_backgroundIcons.push_back(icon);
        }
    }

    void UpdateBackgroundIcons(float dt, float width, float height)
    {
        for (auto &icon : g_backgroundIcons)
        {
            icon.x += icon.speedX * dt;
            icon.y += icon.speedY * dt;
            icon.rotation += icon.rotateSpeed * dt;

            if (icon.y < -80.0f)
            {
                icon.y = height + randRange(20.0f, 80.0f);
                icon.x = randRange(0.0f, width);
            }

            if (icon.x < -80.0f)
                icon.x = width + 40.0f;

            if (icon.x > width + 80.0f)
                icon.x = -40.0f;
        }
    }

    void ConfigureInit()
    {
        // 确保必要的目录存在
        std::filesystem::create_directories(beiklive::path::configPath());
        std::filesystem::create_directories(beiklive::path::databasePath());
        std::filesystem::create_directories(beiklive::path::logPath());
        std::filesystem::create_directories(beiklive::path::screenshotPath());
        std::filesystem::create_directories(beiklive::path::romPath());
        std::filesystem::create_directories(beiklive::path::savePath());
        std::filesystem::create_directories(beiklive::path::corePath());
        std::filesystem::create_directories(beiklive::path::cheatPath());
        std::filesystem::create_directories(beiklive::path::shaderPath());
        std::filesystem::create_directories(beiklive::path::cachePath());
        std::filesystem::create_directories(beiklive::path::biosPath());
        std::filesystem::create_directories(beiklive::path::dbsPath());

        SettingManager = new beiklive::ConfigManager(beiklive::path::configFilePath());
        NameMappingManager = new beiklive::ConfigManager(beiklive::path::mappingFilePath());
        // ConfigManager 构造函数已调用 Load()，无需重复加载

        // 数据库初始化
        {
            std::string dbDir = beiklive::path::databasePath();
            GameDB = new beiklive::GameDatabase();
            GameDB->loadFromDir(dbDir);

            // 将目录路径写入配置（新版本以目录为准）
            SettingManager->Set("db_path", beiklive::ConfigValue(dbDir));
        }

        // ── 预设所有设置项的默认值（仅当配置文件中不存在时才设置）──────────────
        using namespace beiklive::SettingKey;

        // UI 背景图片设置
        SettingManager->SetDefault(KEY_UI_SHOW_BG_IMAGE, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_BG_IMAGE_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_UI_BG_GIF_SPEED, ConfigValue(1.0f));
        SettingManager->SetDefault(KEY_UI_BG_VIDEO_FRAME_RATE, ConfigValue(30));
        SettingManager->SetDefault(KEY_UI_BG_BLUR_ENABLED, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_BG_BLUR_RADIUS, ConfigValue(12.0f));
        SettingManager->SetDefault(KEY_UI_SHOW_XMB_BG, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_PSPXMB_COLOR, ConfigValue(std::string("blue")));
        SettingManager->SetDefault(KEY_UI_USE_SAVESTATE_THUMB, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_PICO8_SHORTCUT_VISIBLE, ConfigValue(1));
        SettingManager->SetDefault(KEY_UI_SHOW_SHADER, ConfigValue(1));
        SettingManager->SetDefault(KEY_UI_GRADIENT_THEME, ConfigValue(std::string("VscodeBlack")));
        SettingManager->SetDefault(KEY_UI_THEME, ConfigValue(std::string("dark")));
        SettingManager->SetDefault(KEY_UI_LANGUAGE, ConfigValue(std::string("zh-CN")));

        // 遮罩设置
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_ENABLED, ConfigValue(0));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GBA_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GBC_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GB_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_NES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_SNES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_NDS_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GENESIS_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_ARCADE_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_DC_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_PSP_PATH, ConfigValue(std::string("")));

        // 着色器设置
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_ENABLED, ConfigValue(0));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GBA_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GBC_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GB_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_NES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_SNES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_NDS_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GENESIS_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_ARCADE_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_DC_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_PSP_PATH, ConfigValue(std::string("")));

        // 调试设置
        SettingManager->SetDefault(KEY_DEBUG_LOG_LEVEL, ConfigValue(std::string("info")));
        SettingManager->SetDefault(KEY_DEBUG_LOG_FILE, ConfigValue(0));
        SettingManager->SetDefault(KEY_DEBUG_LOG_OVERLAY, ConfigValue(0));

        // 音频设置
        SettingManager->SetDefault(KEY_AUDIO_BUTTON_SFX, ConfigValue(1));
        SettingManager->SetDefault(KEY_AUDIO_BUTTON_SFX_VOLUME, ConfigValue(100));
        SettingManager->SetDefault(KEY_AUDIO_TARGET_LATENCY_MS, ConfigValue(90));
        SettingManager->SetDefault(KEY_AUDIO_MAX_LATENCY_MS, ConfigValue(180));
        SettingManager->SetDefault(KEY_AUDIO_SYNC_STRENGTH, ConfigValue(0.015f));
        SettingManager->SetDefault(KEY_AUDIO_TRANSITION_FADE_MS, ConfigValue(6));

        // 快进设置
        SettingManager->SetDefault("fastforward.enabled", ConfigValue(1));
        SettingManager->SetDefault("fastforward.mode", ConfigValue(std::string("hold")));
        SettingManager->SetDefault("fastforward.multiplier", ConfigValue(2.0f));
        SettingManager->SetDefault("fastforward.mute", ConfigValue(1));

        // 倒带设置
        SettingManager->SetDefault("rewind.enabled", ConfigValue(0));
        SettingManager->SetDefault("rewind.mode", ConfigValue(std::string("hold")));
        SettingManager->SetDefault(KEY_REWIND_BUFFER_SIZE, ConfigValue(600));
        SettingManager->SetDefault("rewind.step", ConfigValue(2));
        SettingManager->SetDefault("rewind.mute", ConfigValue(0));
        SettingManager->SetDefault(KEY_REWIND_SAVE_INTERVAL, ConfigValue(1));
        SettingManager->SetDefault(KEY_REWIND_SHOW_UI, ConfigValue(0));
        SettingManager->SetDefault(KEY_REWIND_UI_ITEM_COUNT, ConfigValue(10));
        SettingManager->SetDefault(KEY_REWIND_THUMB_COMPRESSION, ConfigValue(0));

        // NDS Deko 实验路径：页面分流和 Deko probe 分开控制，避免一开 NDS 就触发图形上下文切换。
        SettingManager->SetDefault("nds.dekoMode.enabled", ConfigValue(0));
        SettingManager->SetDefault("nds.dekoMode.probe.enabled", ConfigValue(0));
        SettingManager->SetDefault("nds.dekoMode.probe.level", ConfigValue(1));
        SettingManager->SetDefault("nds.externalNro.enabled", ConfigValue(1));
        SettingManager->SetDefault("nds.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationNDSStub.nro")));
        SettingManager->SetDefault("nds.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("3ds.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStation3DSStub.nro")));
        SettingManager->SetDefault("3ds.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("arcade.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationFBNeoStub.nro")));
        SettingManager->SetDefault("arcade.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("dc.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationFlycastStub.nro")));
        SettingManager->SetDefault("dc.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("psp.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationPPSSPPStub.nro")));
        SettingManager->SetDefault("psp.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("ps1.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationDuckStationStub.nro")));
        SettingManager->SetDefault("ps1.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("saturn.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationYabaSanshiroStub.nro")));
        SettingManager->SetDefault("saturn.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        SettingManager->SetDefault("dolphin.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationDolphinStub.nro")));
        SettingManager->SetDefault("dolphin.externalNro.returnPath", ConfigValue(std::string("sdmc:/switch/GBAStation.nro")));
        if (auto pathValue = SettingManager->Get("nds.externalNro.path"))
        {
            const auto path = pathValue->AsString().value_or("");
            if (path == "sdmc:/switch/GBAStationNDSStub.nro")
                SettingManager->Set("nds.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationNDSStub.nro")));
        }
        if (auto pathValue = SettingManager->Get("arcade.externalNro.path"))
        {
            const auto path = pathValue->AsString().value_or("");
            if (path == "/GBAStation/core/FBNeo.nro" || path == "sdmc:/switch/FBNeo.nro")
                SettingManager->Set("arcade.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationFBNeoStub.nro")));
        }
        if (auto pathValue = SettingManager->Get("dc.externalNro.path"))
        {
            const auto path = pathValue->AsString().value_or("");
            if (path == "/GBAStation/core/Flycast.nro" || path == "sdmc:/switch/Flycast.nro")
                SettingManager->Set("dc.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationFlycastStub.nro")));
        }
        if (auto pathValue = SettingManager->Get("psp.externalNro.path"))
        {
            const auto path = pathValue->AsString().value_or("");
            if (path == "/GBAStation/core/PPSSPP.nro" ||
                path == "/GBAStation/core/PPSSPPStub.nro" ||
                path == "sdmc:/switch/PPSSPP.nro")
                SettingManager->Set("psp.externalNro.path", ConfigValue(std::string("/GBAStation/core/GBAStationPPSSPPStub.nro")));
        }

        // 核心设置
        SettingManager->SetDefault("core.azahar.upscale", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.use_cpu_jit", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.new_3ds", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.cpu_clock", ConfigValue(100));
        SettingManager->SetDefault("core.azahar.region", ConfigValue(std::string("auto")));
        SettingManager->SetDefault("core.azahar.language", ConfigValue(std::string("")));
        SettingManager->SetDefault("core.azahar.username", ConfigValue(std::string("")));
        SettingManager->SetDefault("core.azahar.input_type", ConfigValue(std::string("null")));
        SettingManager->SetDefault("core.azahar.use_hw_shader", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.use_shader_jit", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.accurate_mul", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.disk_shader_cache", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.async_gpu", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.strict_gpu_sync", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.async_shaders", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.show_shader_compile_notice", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.async_presentation", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.spirv_shader_gen", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.disable_spirv_optimizer", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.vsync", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.frame_limit", ConfigValue(100.0f));
        SettingManager->SetDefault("core.azahar.simulate_3ds_gpu_timings", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.renderer_debug", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.dump_command_buffers", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.disable_right_eye", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.texture_filter", ConfigValue(std::string("none")));
        SettingManager->SetDefault("core.azahar.texture_sampling", ConfigValue(std::string("game")));
        SettingManager->SetDefault("core.azahar.custom_textures", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.dump_textures", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.use_virtual_sd", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.layout", ConfigValue(std::string("default")));
        SettingManager->SetDefault("core.azahar.small_screen_position", ConfigValue(std::string("bottom_right")));
        SettingManager->SetDefault("core.azahar.display_orientation", ConfigValue(std::string("horizontal")));
        SettingManager->SetDefault("core.azahar.display_rotation", ConfigValue(std::string("0")));
        SettingManager->SetDefault("core.azahar.display_size", ConfigValue(std::string("default")));
        SettingManager->SetDefault("core.azahar.large_screen_proportion", ConfigValue(4.0f));
        SettingManager->SetDefault("core.azahar.audio_emulation", ConfigValue(std::string("hle")));
        SettingManager->SetDefault("core.azahar.audio_stretching", ConfigValue(0));
        SettingManager->SetDefault("core.azahar.realtime_audio", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.pause_when_menu_open", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.movie_cpu_throttle", ConfigValue(1));
        SettingManager->SetDefault("core.azahar.movie_throttle_clock", ConfigValue(50));

        SettingManager->SetDefault("core.ppsspp.rendering_resolution", ConfigValue(std::string("1")));
        SettingManager->SetDefault("core.ppsspp.frameskip", ConfigValue(std::string("0")));
        SettingManager->SetDefault("core.ppsspp.auto_frameskip", ConfigValue(0));
        SettingManager->SetDefault("core.ppsspp.fast_memory", ConfigValue(1));
        SettingManager->SetDefault("core.ppsspp.io_thread", ConfigValue(1));
        SettingManager->SetDefault("core.ppsspp.display_mode", ConfigValue(std::string("Display")));
        SettingManager->SetDefault("core.ppsspp.display_size", ConfigValue(std::string("16:9")));

        SettingManager->SetDefault("core.fbneo.display_mode", ConfigValue(std::string("Display")));
        SettingManager->SetDefault("core.fbneo.display_size", ConfigValue(std::string("Auto")));
        SettingManager->SetDefault("core.fbneo.shader_type", ConfigValue(std::string("None")));

        SettingManager->SetDefault("core.flycast.display_mode", ConfigValue(std::string("Display")));
        SettingManager->SetDefault("core.flycast.display_size", ConfigValue(std::string("4:3")));

        // DuckStation Stub 直接读取 ps1.*，确保启动器设置即时生效。
        SettingManager->SetDefault("ps1.renderer", ConfigValue(std::string("deko3D")));
        SettingManager->SetDefault("ps1.resolutionScale", ConfigValue(1));
        SettingManager->SetDefault("ps1.aspectRatio", ConfigValue(std::string("Auto (Game Native)")));
        SettingManager->SetDefault("ps1.fastBoot", ConfigValue(1));

        SettingManager->SetDefault("core.saturn.emulated_bios", ConfigValue(0));
        SettingManager->SetDefault("core.saturn.frame_skip", ConfigValue(0));
        SettingManager->SetDefault("core.saturn.resolution_mode", ConfigValue(0));
        SettingManager->SetDefault("core.dolphin.dolphin_cpu_clock_rate", ConfigValue(std::string("1.0")));
        SettingManager->SetDefault("core.dolphin.dolphin_widescreen", ConfigValue(std::string("enabled")));
        SettingManager->SetDefault("core.dolphin.dolphin_enable_rumble", ConfigValue(std::string("enabled")));
        SettingManager->SetDefault("core.dolphin.dolphin_wiimote1_mode", ConfigValue(std::string("classic")));

        SettingManager->SetDefault("core.mgba_gb_model", ConfigValue(std::string("Autodetect")));
        SettingManager->SetDefault("core.mgba_use_bios", ConfigValue(std::string("ON")));
        SettingManager->SetDefault("core.mgba_skip_bios", ConfigValue(std::string("OFF")));
        SettingManager->SetDefault("core.mgba_gb_colors", ConfigValue(std::string("Grayscale")));
        SettingManager->SetDefault("core.mgba_sgb_borders", ConfigValue(std::string("OFF")));
        SettingManager->SetDefault("core.mgba_solar_sensor_level", ConfigValue(std::string("5")));
        SettingManager->SetDefault("core.mgba_rtc_mode", ConfigValue(std::string("persist")));
        SettingManager->SetDefault("core.mgba_idle_optimization", ConfigValue(std::string("Remove Known")));
        SettingManager->SetDefault("core.mgba_audio_low_pass_filter", ConfigValue(std::string("disabled")));
        SettingManager->SetDefault("core.mgba_audio_low_pass_range", ConfigValue(std::string("60")));
        SettingManager->SetDefault("core.mgba_allow_opposing_directions", ConfigValue(std::string("no")));
        SettingManager->SetDefault("core.mgba_force_gbp", ConfigValue(std::string("OFF")));
        SettingManager->SetDefault("core.mgba_frameskip", ConfigValue(std::string("0")));
        SettingManager->SetDefault("core.gambatte_gb_colorization", ConfigValue(std::string("disabled")));
        SettingManager->SetDefault("core.gambatte_gb_hwmode", ConfigValue(std::string("Auto")));
        SettingManager->SetDefault("core.gambatte_gbc_color_correction", ConfigValue(std::string("disabled")));
        SettingManager->SetDefault("core.gambatte_mix_frames", ConfigValue(std::string("disabled")));

        SettingManager->SetDefault("core.melonds_bios9_path", ConfigValue(std::string("sdmc:/GBAStation/bios/nds/bios9.bin")));
        SettingManager->SetDefault("core.melonds_bios7_path", ConfigValue(std::string("sdmc:/GBAStation/bios/nds/bios7.bin")));
        SettingManager->SetDefault("core.melonds_firmware_path", ConfigValue(std::string("sdmc:/GBAStation/bios/nds/firmware.bin")));
        SettingManager->SetDefault("core.melonds_direct_boot", ConfigValue(1));
        SettingManager->SetDefault("core.melonds_jit_enabled", ConfigValue(1));
        SettingManager->SetDefault("core.melonds_jit_block_size", ConfigValue(std::string("32")));
        SettingManager->SetDefault("core.melonds_jit_branch", ConfigValue(1));
        SettingManager->SetDefault("core.melonds_jit_literal", ConfigValue(1));
        SettingManager->SetDefault("core.melonds_jit_fast_memory", ConfigValue(1));
        SettingManager->SetDefault("core.melonds_threaded_renderer", ConfigValue(1));
        SettingManager->SetDefault("core.melonds_render_scale", ConfigValue(std::string("1")));
        SettingManager->SetDefault("core.melonds_better_polygons", ConfigValue(0));
        SettingManager->SetDefault("core.melonds_dldi_enabled", ConfigValue(0));
        SettingManager->SetDefault("core.melonds_dldi_path", ConfigValue(std::string("")));
        SettingManager->SetDefault("core.melonds_randomize_mac", ConfigValue(0));
        SettingManager->SetDefault("core.melonds_firmware_language", ConfigValue(-1));

        // DraStic Vulkan host 启动时读取共享 config.cfg，
        // 再由 host 导入 sdmc:/GBAStation/drastic/drastic.ini。
        SettingManager->SetDefault("core.drastic.layout", ConfigValue(std::string("horizontal")));
        SettingManager->SetDefault("core.drastic.rotation", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.screen_gap", ConfigValue(8));
        SettingManager->SetDefault("core.drastic.integer_scale", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.video_filter", ConfigValue(std::string("nearest")));
        SettingManager->SetDefault("core.drastic.volume", ConfigValue(100));
        SettingManager->SetDefault("core.drastic.microphone_source", ConfigValue(std::string("noise")));
        SettingManager->SetDefault("core.drastic.vibration", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.motion", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.stylus_mode", ConfigValue(std::string("stick")));
        SettingManager->SetDefault("core.drastic.stylus_speed", ConfigValue(8));
        SettingManager->SetDefault("core.drastic.frameskip", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.frameskip_type", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.frameskip_safe", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.fastforward_speed", ConfigValue(2));
        SettingManager->SetDefault("core.drastic.audio_latency", ConfigValue(2));
        SettingManager->SetDefault("core.drastic.cpu_threads", ConfigValue(3));
        SettingManager->SetDefault("core.drastic.threaded_3d", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.hires_3d", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.sound_enabled", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.cheats_enabled", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.mic_enabled", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.rtc_system_time", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.preload_roms", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.show_fps", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.autosave_interval", ConfigValue(300));
        SettingManager->SetDefault("core.drastic.autofire_speed", ConfigValue(2));
        SettingManager->SetDefault("core.drastic.mic_level", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.slot2_type", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.backup_in_savestates", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.ignore_gamecard_limit", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.use_16bit_color", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.auto_trim", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.fix_main_engine_screen", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.disable_edge_marking", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.lua_enabled", ConfigValue(1));
        SettingManager->SetDefault("core.drastic.blend", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.raw_save_format", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.firmware_nickname", ConfigValue(std::string("Switch")));
        SettingManager->SetDefault("core.drastic.firmware_language", ConfigValue(-1));
        SettingManager->SetDefault("core.drastic.firmware_color", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.firmware_birthday_month", ConfigValue(6));
        SettingManager->SetDefault("core.drastic.firmware_birthday_day", ConfigValue(6));
        SettingManager->SetDefault("core.drastic.lsfg_enabled", ConfigValue(0));
        SettingManager->SetDefault("core.drastic.lsfg_flow_scale", ConfigValue(25));
        SettingManager->SetDefault("core.drastic.lsfg_performance", ConfigValue(1));

        SettingManager->SetDefault("core.genesis.region", ConfigValue(std::string("auto")));
        SettingManager->SetDefault("core.genesis.pad_buttons", ConfigValue(6));
        SettingManager->SetDefault("core.genesis.no_sprite_limit", ConfigValue(std::string("disabled")));
        SettingManager->SetDefault("core.genesis.low_pass", ConfigValue(std::string("enabled")));
        SettingManager->SetDefault("core.genesis.low_pass_range", ConfigValue(60));
        SettingManager->SetDefault("core.genesis.hq_fm", ConfigValue(std::string("enabled")));
        SettingManager->SetDefault("core.genesis.hq_psg", ConfigValue(std::string("enabled")));
        SettingManager->SetDefault("core.genesis.mono", ConfigValue(std::string("disabled")));

        // BIOS 路径设置
        SettingManager->SetDefault("bios.path", ConfigValue(beiklive::path::biosPath()));

        // 画面设置
        SettingManager->SetDefault("display.mode", ConfigValue(std::string("original")));
        SettingManager->SetDefault("display.integer_scale_mult", ConfigValue(0));
        SettingManager->SetDefault("display.filter", ConfigValue(std::string("nearest")));
        SettingManager->SetDefault("display.showFps", ConfigValue(0));
        SettingManager->SetDefault("display.showFfOverlay", ConfigValue(1));
        SettingManager->SetDefault("display.showRewindOverlay", ConfigValue(1));
        SettingManager->SetDefault("display.showMuteOverlay", ConfigValue(1));

        // 存档设置
        SettingManager->SetDefault("save.autoSaveState", ConfigValue(0));
        SettingManager->SetDefault("save.autoSaveInterval", ConfigValue(0));
        SettingManager->SetDefault("save.autoLoadState0", ConfigValue(0));
        SettingManager->SetDefault("save.autoSaveOnExit", ConfigValue(0));
        SettingManager->SetDefault("save.sramDir", ConfigValue(std::string("")));
        SettingManager->SetDefault("save.stateDir", ConfigValue(std::string("")));

        // 连发设置
        SettingManager->SetDefault("handle.a_turbo", ConfigValue(std::string("none")));
        SettingManager->SetDefault("handle.b_turbo", ConfigValue(std::string("none")));
        SettingManager->SetDefault("turbo.rate", ConfigValue(10.0f));

        // 更新设置
        SettingManager->SetDefault(beiklive::SettingKey::KEY_EMU_UPDATE, ConfigValue(1));

        // 截图设置
        SettingManager->SetDefault("screenshot.dir", ConfigValue(0));

        // 金手指设置
        SettingManager->SetDefault("cheat.enabled", ConfigValue(0));
        SettingManager->SetDefault("cheat.dir", ConfigValue(std::string("")));

        // 按键绑定默认值。GBA 保持无前缀；GBC/GB 独立前缀首次默认继承旧的无前缀配置。
        const std::string mappingPrefixes[] = {"", "gbc.", "gb.", "nes.", "sfc.", "nds.", "3ds.", "md.", "arcade.", "dc.", "psp.", "ps1.", "saturn.", "dolphin."};
        for (const auto& prefix : mappingPrefixes)
        {
            const unsigned platformMask = beiklive::input_mapping::platformMaskForPrefix(prefix);
            for (const auto& entry : beiklive::input_mapping::kGameButtonDefaults)
            {
                if ((entry.platformMask & platformMask) == 0)
                    continue;
                std::string defaultValue =
                    beiklive::input_mapping::defaultHandleValueForPrefix(
                        prefix, entry.suffix, entry.defaultValue);
                if (beiklive::input_mapping::usesLegacyGbFamilyFallback(prefix) &&
                    !beiklive::input_mapping::isRightStickMapping(entry.suffix))
                {
                    auto legacy = SettingManager->Get(
                        beiklive::input_mapping::makeHandleKey("", entry.suffix));
                    if (legacy.has_value())
                    {
                        auto legacyStr = legacy->AsString();
                        if (legacyStr.has_value())
                            defaultValue = *legacyStr;
                    }
                }
                SettingManager->SetDefault(
                    beiklive::input_mapping::makeHandleKey(prefix, entry.suffix),
                    ConfigValue(defaultValue));
            }
            for (const auto& entry : beiklive::input_mapping::kHotkeyDefaults)
            {
                if ((prefix == "nds." && entry.hiddenOnNds) ||
                    (prefix == "3ds." && entry.hiddenOnThreeDs))
                    continue;
                std::string defaultValue = entry.defaultValue;
                if (beiklive::input_mapping::usesLegacyGbFamilyFallback(prefix))
                {
                    auto legacy = SettingManager->Get(
                        beiklive::input_mapping::makeKey("", entry.key));
                    if (legacy.has_value())
                    {
                        auto legacyStr = legacy->AsString();
                        if (legacyStr.has_value())
                            defaultValue = *legacyStr;
                    }
                }
                SettingManager->SetDefault(
                    beiklive::input_mapping::makeKey(prefix, entry.key),
                    ConfigValue(defaultValue));
            }
            std::string turboADefault = beiklive::input_mapping::kTurboADefault;
            std::string turboBDefault = beiklive::input_mapping::kTurboBDefault;
            if (beiklive::input_mapping::usesLegacyGbFamilyFallback(prefix))
            {
                auto legacyTurboA = SettingManager->Get(
                    beiklive::input_mapping::makeKey("", beiklive::input_mapping::kTurboAKey));
                if (legacyTurboA.has_value())
                {
                    auto legacyStr = legacyTurboA->AsString();
                    if (legacyStr.has_value())
                        turboADefault = *legacyStr;
                }

                auto legacyTurboB = SettingManager->Get(
                    beiklive::input_mapping::makeKey("", beiklive::input_mapping::kTurboBKey));
                if (legacyTurboB.has_value())
                {
                    auto legacyStr = legacyTurboB->AsString();
                    if (legacyStr.has_value())
                        turboBDefault = *legacyStr;
                }
            }
            SettingManager->SetDefault(
                beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboAKey),
                ConfigValue(turboADefault));
            SettingManager->SetDefault(
                beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboBKey),
                ConfigValue(turboBDefault));
        }

        // Saturn hotkeys mirror the external-core layout from the migration
        // design. They are separate from the generic defaults above so old
        // installations retain any mappings the user already chose.
        SettingManager->SetDefault("saturn.handle.fastforward", ConfigValue(std::string("PAD_RSB")));
        SettingManager->SetDefault("saturn.hotkey.menu.pad", ConfigValue(std::string("PAD_LSB")));
        SettingManager->SetDefault("saturn.hotkey.quicksave.pad", ConfigValue(std::string("PAD_LT+PAD_RT")));
        SettingManager->SetDefault("saturn.hotkey.quickload.pad", ConfigValue(std::string("PAD_LB+PAD_RB")));
        // 3DS 独立运行时不支持倒带，清理旧版本可能写入的无效绑定。
        SettingManager->Remove("3ds.handle.rewind");
        SettingManager->Remove("core.azahar.swap_screens");
        // DraStic screen order is now a runtime action driven exclusively by
        // nds.hotkey.swap_screens.pad, never a persisted core toggle.
        SettingManager->Remove("core.drastic.swap_screens");
        // The simulated microphone is supported by the DraStic NDS host only.
        // Keep clearing the legacy 3DS mapping, which has no matching action.
        SettingManager->Remove("3ds.hotkey.mic_input.pad");
        for (const char* prefix : {"nds.", "3ds."})
        {
            for (const auto& entry : beiklive::input_mapping::kPointerHotkeys)
            {
                if (std::strcmp(prefix, "3ds.") == 0 && entry.hiddenOnThreeDs)
                    continue;
                SettingManager->SetDefault(
                    beiklive::input_mapping::makeKey(prefix, entry.key),
                    ConfigValue(std::string(entry.defaultValue)));
            }
        }
        for (const auto& entry : beiklive::input_mapping::kNdsSpecialHotkeys)
        {
            SettingManager->SetDefault(
                beiklive::input_mapping::makeKey("nds.", entry.key),
                ConfigValue(std::string(entry.defaultValue)));
        }

        // 摇杆输入设置
        SettingManager->Set("input.joystick.enabled", ConfigValue(1), false);
        SettingManager->Set("input.joystick.diagonal", ConfigValue(1), false);

        SettingManager->Save();
        NameMappingManager->Save();

        InitBackgroundIcons();
    }

    //
    // 支持的格式：
    //
    // 1. RetroArch .cht 格式：
    //    cheats = N
    //    cheat0_desc = "Name"
    //    cheat0_enable = true
    //    cheat0_code = XXXXXXXX+YYYYYYYY
    //
    // 2. 简单逐行格式（默认启用）：
    //    # 注释
    //    +XXXXXXXX YYYYYYYY   （'+' 前缀 = 启用）
    //    -XXXXXXXX YYYYYYYY   （'-' 前缀 = 禁用）
    //    XXXXXXXX YYYYYYYY    （无前缀  = 启用）
    // ============================================================

    /// 解析 .cht 金手指文件，返回金手指条目列表。
    /// 若文件不存在或解析失败，返回空列表。
    std::vector<CheatEntry> parseChtFile(const std::string &path)
    {
        return beiklive::cheat::loadChtFile(path);
    }

    std::vector<CheatEntry> parseNdsUsrCheatDat(const std::string &datPath, const std::string &romPath)
    {
        return beiklive::cheat::loadNdsUsrCheatDat(datPath, romPath);
    }

    std::string GetNdsIconCachePath(const std::string& romPath)
    {
        if (romPath.empty())
            return "";

        std::error_code ec;
        fs::path romFsPath = fs::absolute(fs::path(romPath), ec);
        if (ec)
            romFsPath = fs::path(romPath);

        std::ostringstream key;
        key << romFsPath.lexically_normal().string();
        key << '|';
        const auto romSize = fs::file_size(romPath, ec);
        if (!ec)
            key << romSize;
        else {
            ec.clear();
            key << 0;
        }
        key << '|';
        const auto writeTime = fs::last_write_time(romPath, ec);
        if (!ec) {
            const auto writeTicks = std::chrono::duration_cast<std::chrono::nanoseconds>(
                writeTime.time_since_epoch()).count();
            key << writeTicks;
        } else
            key << 0;

        return (fs::path(beiklive::path::cachePath()) / "nds_icons" / (hex64(fnv1a64(key.str())) + ".png")).string();
    }

    std::string GetOrCreateNdsIconPath(const std::string& romPath)
    {
        if (romPath.empty())
            return "";

        std::lock_guard<std::mutex> lock(g_ndsIconCacheMutex);
        const std::string cachePath = GetNdsIconCachePath(romPath);
        if (cachePath.empty())
            return "";

        auto memoIt = g_ndsIconPathMemo.find(cachePath);
        if (memoIt != g_ndsIconPathMemo.end())
            return memoIt->second;

        std::error_code ec;
        if (fs::exists(cachePath, ec) && fs::is_regular_file(cachePath, ec))
        {
            g_ndsIconPathMemo[cachePath] = cachePath;
            return cachePath;
        }

        const fs::path cacheFsPath(cachePath);
        fs::create_directories(cacheFsPath.parent_path(), ec);
        if (ec)
        {
            brls::Logger::warning("GetOrCreateNdsIconPath: failed to create cache dir: {}", cacheFsPath.parent_path().string());
            g_ndsIconPathMemo[cachePath] = "";
            return "";
        }

        std::array<uint8_t, 32 * 32 * 4> rgba {};
        if (!decodeNdsIcon(romPath, rgba) || !writeRgbaPng(cacheFsPath, rgba))
        {
            brls::Logger::warning("GetOrCreateNdsIconPath: failed to extract NDS icon: {}", romPath);
            g_ndsIconPathMemo[cachePath] = "";
            return "";
        }

        g_ndsIconPathMemo[cachePath] = cachePath;
        return cachePath;
    }

    // 提取 NDS ROM header 的游戏名（偏移 0x60 起 12 字节 ASCII）。
    std::string ExtractNdsHeaderTitle(const std::string& romPath)
    {
        if (romPath.empty())
            return "";
        std::ifstream in(romPath, std::ios::binary);
        if (!in)
            return "";
        char title[12] = {};
        in.seekg(0x60, std::ios::beg);
        if (!in.read(title, sizeof(title)))
            return "";
        std::string out;
        out.reserve(sizeof(title));
        for (char c : title)
        {
            if (c == '\0' || static_cast<unsigned char>(c) < 0x20)
                break;
            out.push_back(c);
        }
        // 全空则视为无标题。
        bool any = false;
        for (char c : out)
            if (c != ' ')
                any = true;
        return any ? out : "";
    }

    /// 将金手指列表以 RetroArch .cht 格式写入文件。
    /// 返回 true 表示成功。
    bool saveChtFile(const std::string &path,
                     const std::vector<CheatEntry> &entries)
    {
        return beiklive::cheat::saveChtFile(path, entries);
    }

    int GetGamePixelHeight(int platform)
    {
        switch ((beiklive::enums::EmuPlatform)platform)
        {
        case beiklive::enums::EmuPlatform::EmuGBA:
            return 160;
        case beiklive::enums::EmuPlatform::EmuGBC:
            return 144;
        case beiklive::enums::EmuPlatform::EmuGB:
            return 144;
        case beiklive::enums::EmuPlatform::EmuNES:
            return 240;
        case beiklive::enums::EmuPlatform::EmuSNES:
            return 224;
        case beiklive::enums::EmuPlatform::EmuNDS:
            return 384;
        case beiklive::enums::EmuPlatform::Emu3DS:
            return 480;
        case beiklive::enums::EmuPlatform::EmuGenesis:
            return 224;
        case beiklive::enums::EmuPlatform::EmuArcade:
            return 240;
        case beiklive::enums::EmuPlatform::EmuDreamcast:
            return 480;
        case beiklive::enums::EmuPlatform::EmuPSP:
            return 272;
        default:
            break;
        }
        return 0;
    }

    int GetGamePixelWidth(int platform)
    {
        switch ((beiklive::enums::EmuPlatform)platform)
        {
        case beiklive::enums::EmuPlatform::EmuGBA:
            return 240;
        case beiklive::enums::EmuPlatform::EmuGBC:
            return 160;
        case beiklive::enums::EmuPlatform::EmuGB:
            return 160;
        case beiklive::enums::EmuPlatform::EmuNES:
            return 256;
        case beiklive::enums::EmuPlatform::EmuSNES:
            return 256;
        case beiklive::enums::EmuPlatform::EmuNDS:
            return 256;
        case beiklive::enums::EmuPlatform::Emu3DS:
            return 400;
        case beiklive::enums::EmuPlatform::EmuGenesis:
            return 320;
        case beiklive::enums::EmuPlatform::EmuArcade:
            return 320;
        case beiklive::enums::EmuPlatform::EmuDreamcast:
            return 640;
        case beiklive::enums::EmuPlatform::EmuPSP:
            return 480;
        default:
            break;
        }
        return 0;
    }

    std::string GetGameLogoLayerPath(int platform)
    {
        switch ((beiklive::enums::EmuPlatform)platform)
        {
        case beiklive::enums::EmuPlatform::EmuGBA:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuGBC:
            return BK_RES("img/LogoLayer/GBC_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuGB:
            return BK_RES("img/LogoLayer/GB_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuNES:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuSNES:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuNDS:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::Emu3DS:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuGenesis:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        case beiklive::enums::EmuPlatform::EmuArcade:
        case beiklive::enums::EmuPlatform::EmuDreamcast:
        case beiklive::enums::EmuPlatform::EmuPSP:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        default:
            return BK_RES("img/LogoLayer/GBA_LOGOLAY.png");
        }
    }

    static constexpr long POP_ACTIVITY_DEFER_DELETE_MS = 600;

    void pushActivity(brls::AppletFrame *frame, beiklive::Box *pre, beiklive::Box *next,
                      std::function<void()> onShow)
    {
        g_beiklive_boxes.push_back(pre);
        pre->animaHide(
            [frame, next, onShow = std::move(onShow)]() {
                brls::Application::pushActivity(new brls::Activity(frame));
                next->animaShow(std::move(onShow));
            }
        );
    }

    void popActivity(beiklive::Box *v, bool animate)
    {
        if (g_beiklive_boxes.empty()) {
            brls::Logger::error("Cannot restore previous page: activity page stack is empty");
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return;
        }
        auto* box = static_cast<beiklive::Box*>(g_beiklive_boxes.back());
        g_beiklive_boxes.pop_back();
        auto finishPop = [box]() {
                auto stack = brls::Application::getActivitiesStack();
                brls::Activity* activityToDelete = stack.empty() ? nullptr : stack.back();
                bool popped = brls::Application::popActivity(
                    brls::TransitionAnimation::NONE,
                    [box, activityToDelete]() {
                        box->animaShow([box]() {
                            box->onActivityResume();
                        });
                        if (!activityToDelete)
                            return;

                        brls::delay(POP_ACTIVITY_DEFER_DELETE_MS, [activityToDelete]() {
                            brls::Logger::debug("Deferred delete popped activity");
                            delete activityToDelete;
                        });
                    },
                    false);
                if (!popped) {
                    box->animaShow([box]() {
                        box->onActivityResume();
                    });
                }
            };
        if (animate)
            v->animaHide(std::move(finishPop));
        else
            finishPop();
    }

} // namespace beiklive
