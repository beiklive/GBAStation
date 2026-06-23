#include "common.h"
#include "ui/widget/Box.hpp"
#include "ARDatabaseDAT.h"
#include "ARCodeFile.h"
#include "CRC32.h"
#include "NDS_Header.h"
#include "game/control/InputMappingDefaults.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <variant>

namespace fs = std::filesystem;

namespace beiklive
{
    namespace
    {
        std::string ndsArCodeToText(const melonDS::ARCode& code)
        {
            std::ostringstream oss;
            oss << std::uppercase << std::hex << std::setfill('0');
            for (size_t i = 0; i + 1 < code.Code.size(); i += 2)
            {
                if (i > 0)
                    oss << '\n';
                oss << std::setw(8) << code.Code[i] << ' '
                    << std::setw(8) << code.Code[i + 1];
            }
            return oss.str();
        }

        void appendNdsDatCheatsFromCat(const melonDS::ARCodeCat& cat, std::vector<CheatEntry>& out, int depth = 0)
        {
            for (const auto& item : cat.Children)
            {
                if (std::holds_alternative<melonDS::ARCodeCat>(item))
                {
                    const auto& childCat = std::get<melonDS::ARCodeCat>(item);
                    if (!childCat.Name.empty())
                    {
                        CheatEntry catEntry;
                        catEntry.desc = std::string(static_cast<size_t>(std::max(0, depth)) * 2, ' ') + childCat.Name;
                        catEntry.code = "";
                        catEntry.enabled = false;
                        out.push_back(std::move(catEntry));
                    }
                    appendNdsDatCheatsFromCat(childCat, out, depth + 1);
                    continue;
                }

                const auto& code = std::get<melonDS::ARCode>(item);
                if (code.Code.empty())
                    continue;

                CheatEntry entry;
                entry.desc = code.Name.empty() ? code.Description : code.Name;
                if (entry.desc.empty())
                    entry.desc = "NDS AR Code";
                entry.code = ndsArCodeToText(code);
                entry.enabled = false;
                out.push_back(std::move(entry));
            }
        }
    }

    ConfigManager *SettingManager = nullptr;     // 全局配置管理器实例
    ConfigManager *NameMappingManager = nullptr; // 全局名称映射管理器实例
    GameDatabase *GameDB = nullptr;              // 全局游戏数据库实例

    std::vector<brls::Box *> g_beiklive_boxes; // 全局盒子列表

    std::vector<FloatingIcon> g_backgroundIcons;
    float g_backgroundLastTime = 0.0f;
    std::unordered_set<std::string> g_forceRefreshPaths;

    GradientTheme g_gradientTheme = GradientTheme::Midnight;

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
        SettingManager->SetDefault(KEY_UI_BG_BLUR_ENABLED, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_BG_BLUR_RADIUS, ConfigValue(12.0f));
        SettingManager->SetDefault(KEY_UI_SHOW_XMB_BG, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_PSPXMB_COLOR, ConfigValue(std::string("blue")));
        SettingManager->SetDefault(KEY_UI_USE_SAVESTATE_THUMB, ConfigValue(0));
        SettingManager->SetDefault(KEY_UI_SHOW_SHADER, ConfigValue(1));
        SettingManager->SetDefault(KEY_UI_GRADIENT_THEME, ConfigValue(std::string("VscodeBlack")));

        // 遮罩设置
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_ENABLED, ConfigValue(0));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GBA_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GBC_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GB_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_NES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_SNES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_NDS_PATH, ConfigValue(std::string("")));

        // 着色器设置
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_ENABLED, ConfigValue(0));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GBA_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GBC_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_GB_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_NES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_SNES_PATH, ConfigValue(std::string("")));
        SettingManager->SetDefault(KEY_DISPLAY_SHADER_NDS_PATH, ConfigValue(std::string("")));

        // 调试设置
        SettingManager->SetDefault(KEY_DEBUG_LOG_LEVEL, ConfigValue(std::string("info")));
        SettingManager->SetDefault(KEY_DEBUG_LOG_FILE, ConfigValue(0));
        SettingManager->SetDefault(KEY_DEBUG_LOG_OVERLAY, ConfigValue(0));

        // 音频设置
        SettingManager->SetDefault(KEY_AUDIO_BUTTON_SFX, ConfigValue(1));
        SettingManager->SetDefault(KEY_AUDIO_TARGET_LATENCY_MS, ConfigValue(90));
        SettingManager->SetDefault(KEY_AUDIO_MAX_LATENCY_MS, ConfigValue(180));
        SettingManager->SetDefault(KEY_AUDIO_SYNC_STRENGTH, ConfigValue(0.015f));
        SettingManager->SetDefault(KEY_AUDIO_TRANSITION_FADE_MS, ConfigValue(6));

        // 快进设置
        SettingManager->SetDefault("fastforward.enabled", ConfigValue(1));
        SettingManager->SetDefault("fastforward.mode", ConfigValue(std::string("hold")));
        SettingManager->SetDefault("fastforward.multiplier", ConfigValue(4.0f));
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

        // 核心设置
        SettingManager->SetDefault("core.mgba_gb_model", ConfigValue(std::string("Autodetect")));
        SettingManager->SetDefault("core.mgba_use_bios", ConfigValue(std::string("ON")));
        SettingManager->SetDefault("core.mgba_skip_bios", ConfigValue(std::string("OFF")));
        SettingManager->SetDefault("core.mgba_gb_colors", ConfigValue(std::string("Grayscale")));
        SettingManager->SetDefault("core.mgba_rtc_mode", ConfigValue(std::string("persist")));
        SettingManager->SetDefault("core.mgba_idle_optimization", ConfigValue(std::string("Remove Known")));
        SettingManager->SetDefault("core.mgba_audio_low_pass_filter", ConfigValue(std::string("disabled")));

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

        // 按键绑定默认值。GBA/GBC/GB 沿用无前缀键；其他机种使用平台前缀。
        const std::string mappingPrefixes[] = {"", "nes.", "sfc.", "nds."};
        for (const auto& prefix : mappingPrefixes)
        {
            const unsigned platformMask = beiklive::input_mapping::platformMaskForPrefix(prefix);
            for (const auto& entry : beiklive::input_mapping::kGameButtonDefaults)
            {
                if ((entry.platformMask & platformMask) == 0)
                    continue;
                SettingManager->SetDefault(
                    beiklive::input_mapping::makeHandleKey(prefix, entry.suffix),
                    ConfigValue(std::string(entry.defaultValue)));
            }
            for (const auto& entry : beiklive::input_mapping::kHotkeyDefaults)
            {
                SettingManager->SetDefault(
                    beiklive::input_mapping::makeKey(prefix, entry.key),
                    ConfigValue(std::string(entry.defaultValue)));
            }
            SettingManager->SetDefault(
                beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboAKey),
                ConfigValue(std::string(beiklive::input_mapping::kTurboADefault)));
            SettingManager->SetDefault(
                beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboBKey),
                ConfigValue(std::string(beiklive::input_mapping::kTurboBDefault)));
        }
        for (const auto& entry : beiklive::input_mapping::kNdsPointerHotkeys)
        {
            SettingManager->SetDefault(
                beiklive::input_mapping::makeKey("nds.", entry.key),
                ConfigValue(std::string(entry.defaultValue)));
        }
        SettingManager->SetDefault("hotkey.screenshot.pad", ConfigValue(std::string("none")));

        // 摇杆输入设置
        SettingManager->SetDefault("input.joystick.enabled", ConfigValue(1));
        SettingManager->SetDefault("input.joystick.diagonal", ConfigValue(1));

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
        std::vector<CheatEntry> result;

        if (!std::filesystem::exists(path))
        {
            brls::Logger::info("parseChtFile: no cheat file found at {}", path);
            return result;
        }

        std::ifstream file(path);
        if (!file)
        {
            brls::Logger::warning("parseChtFile: failed to open cheat file: {}",
                                  path);
            return result;
        }

        std::string content;
        {
            std::ostringstream oss;
            oss << file.rdbuf();
            content = oss.str();
        }

        if (content.find("cheats = ") != std::string::npos ||
            content.find("cheats=") != std::string::npos)
        {
            // ---- RetroArch .cht 格式 ----
            std::unordered_map<std::string, std::string> kv;
            std::istringstream iss(content);
            std::string line;
            while (std::getline(iss, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                auto hash = line.find('#');
                if (hash != std::string::npos)
                    line = line.substr(0, hash);
                auto eq = line.find('=');
                if (eq == std::string::npos)
                    continue;
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                auto trim = [](std::string s) -> std::string
                {
                    size_t b = s.find_first_not_of(" \t\"");
                    size_t e = s.find_last_not_of(" \t\"");
                    if (b == std::string::npos)
                        return "";
                    return s.substr(b, e - b + 1);
                };
                kv[trim(key)] = trim(value);
            }

            unsigned total = 0;
            {
                auto it = kv.find("cheats");
                if (it != kv.end())
                {
                    try
                    {
                        total = static_cast<unsigned>(std::stoi(it->second));
                    }
                    catch (...)
                    {
                    }
                }
            }

            for (unsigned i = 0; i < total; ++i)
            {
                std::string iStr = std::to_string(i);
                std::string descKey = "cheat" + iStr + "_desc";
                std::string enableKey = "cheat" + iStr + "_enable";
                std::string codeKey = "cheat" + iStr + "_code";

                auto cit = kv.find(codeKey);
                if (cit == kv.end())
                    continue;

                CheatEntry entry;
                entry.code = cit->second;
                entry.enabled = true;
                entry.desc = "cheat" + iStr;

                auto eit = kv.find(enableKey);
                if (eit != kv.end())
                {
                    std::string ev = eit->second;
                    for (char &c : ev)
                        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    entry.enabled = (ev == "true" || ev == "1" || ev == "yes");
                }

                auto dit = kv.find(descKey);
                if (dit != kv.end())
                    entry.desc = dit->second;

                result.push_back(std::move(entry));
            }
        }
        else
        {
            // ---- 简单逐行格式：支持 Raw / GameShark V3 / CodeBreaker ----
            std::istringstream iss(content);
            std::string line;
            while (std::getline(iss, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                // 去掉尾注 # 之后的部分
                auto hash = line.find('#');
                if (hash != std::string::npos)
                    line = line.substr(0, hash);

                // trim 首尾空白
                size_t b = line.find_first_not_of(" \t");
                if (b == std::string::npos) continue;
                size_t e = line.find_last_not_of(" \t");
                line = line.substr(b, e - b + 1);
                if (line.empty()) continue;

                // 提取前置注释中的描述名（可能被 # 切割后丢失，重新从原行提取）
                // 这里仅用代码串作为默认描述，后续可覆盖

                CheatEntry entry;
                entry.enabled = true;

                // +/- 前缀：启用/禁用
                if (line[0] == '+')
                {
                    entry.enabled = true;
                    line = line.substr(1);
                    b = line.find_first_not_of(" \t");
                    if (b == std::string::npos) continue;
                    line = line.substr(b);
                }
                else if (line[0] == '-')
                {
                    entry.enabled = false;
                    line = line.substr(1);
                    b = line.find_first_not_of(" \t");
                    if (b == std::string::npos) continue;
                    line = line.substr(b);
                }

                if (line.empty()) continue;

                // 去掉可能残留的前后空白
                e = line.find_last_not_of(" \t");
                line = line.substr(0, e + 1);

                // 跳过非代码行（描述头、空注释等）
                if (line[0] == '[' || line[0] == ';') continue;

                // 判断是否为合法金手指行：
                // Raw:     AAAAAAAA:VVVV  或 AAAAAAAA+VVVV
                // GS V3:   AAAAAAAA VVVVVVVV (8+8 hex)
                // CB:      AAAAAAAA VVVV       (8+4 hex)
                // RA code: AAAA:VV 或 AAAAAAAA+VVVV
                bool validCheat = false;
                // 含 : 或 + 即为 Raw/RA 格式
                if (line.find(':') != std::string::npos ||
                    line.find('+') != std::string::npos)
                {
                    validCheat = true;
                }
                else
                {
                    auto sp = line.find(' ');
                    if (sp != std::string::npos)
                    {
                        std::string addr = line.substr(0, sp);
                        std::string val  = line.substr(sp + 1);
                        // trim val
                        size_t vb = val.find_first_not_of(" \t");
                        if (vb != std::string::npos)
                            val = val.substr(vb);
                        // 8位hex地址 + 4或8位hex值
                        if (addr.size() == 8 &&
                            std::all_of(addr.begin(), addr.end(),
                                        [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }) &&
                            (val.size() == 4 || val.size() == 8) &&
                            std::all_of(val.begin(), val.end(),
                                        [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }))
                        {
                            validCheat = true;
                        }
                    }
                }
                if (!validCheat) continue;

                entry.code = line;
                entry.desc = line; // 默认描述，可从注释提取覆盖
                result.push_back(std::move(entry));
            }
        }

        return result;
    }

    std::vector<CheatEntry> parseNdsUsrCheatDat(const std::string &datPath, const std::string &romPath)
    {
        std::vector<CheatEntry> result;
        if (datPath.empty() || romPath.empty())
            return result;
        if (!std::filesystem::exists(datPath) || !std::filesystem::exists(romPath))
            return result;

        std::array<melonDS::u8, 0x200> headerBytes {};
        melonDS::NDSHeader header {};
        {
            std::ifstream rom(romPath, std::ios::binary);
            if (!rom)
                return result;
            rom.read(reinterpret_cast<char*>(headerBytes.data()), static_cast<std::streamsize>(headerBytes.size()));
            if (rom.gcount() != static_cast<std::streamsize>(headerBytes.size()))
                return result;
            std::memcpy(&header, headerBytes.data(), std::min(sizeof(header), headerBytes.size()));
        }

        melonDS::ARDatabaseDAT db(datPath);
        if (db.Error)
        {
            brls::Logger::warning("parseNdsUsrCheatDat: failed to load database: {}", datPath);
            return result;
        }

        const melonDS::u32 gameCode = header.GameCodeAsU32();
        const melonDS::u32 checksum = ~melonDS::CRC32(headerBytes.data(), static_cast<int>(headerBytes.size()), 0);
        auto entries = db.GetEntriesByGameCode(gameCode);
        if (entries.empty())
        {
            brls::Logger::info("parseNdsUsrCheatDat: no cheats for ROM {} in {}", romPath, datPath);
            return result;
        }

        bool hasChecksumMatch = false;
        for (const auto& entry : entries)
        {
            if (entry.Checksum == checksum)
            {
                hasChecksumMatch = true;
                break;
            }
        }

        for (const auto& entry : entries)
        {
            if (hasChecksumMatch && entry.Checksum != checksum)
                continue;
            appendNdsDatCheatsFromCat(entry.RootCat, result);
            if (!hasChecksumMatch)
                break;
        }

        brls::Logger::info("parseNdsUsrCheatDat: loaded {} cheats for {} from {}",
                           result.size(), romPath, datPath);
        return result;
    }

    /// 将金手指列表以 RetroArch .cht 格式写入文件。
    /// 返回 true 表示成功。
    bool saveChtFile(const std::string &path,
                     const std::vector<CheatEntry> &entries)
    {
        std::ofstream f(path);
        if (!f)
        {
            brls::Logger::warning("saveChtFile: 无法写入金手指文件: {}", path);
            return false;
        }

        f << "cheats = " << entries.size() << "\n\n";
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto &e = entries[i];
            f << "cheat" << i << "_desc = \"" << e.desc << "\"\n";
            f << "cheat" << i << "_enable = " << (e.enabled ? "true" : "false") << "\n";
            f << "cheat" << i << "_code = " << e.code << "\n\n";
        }

        f.close();
        if (!f.good())
        {
            brls::Logger::warning("saveChtFile: 写入金手指文件失败: {}", path);
            return false;
        }
        return true;
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

    void popActivity(beiklive::Box *v)
    {
        auto* box = static_cast<beiklive::Box*>(g_beiklive_boxes.back());
        g_beiklive_boxes.pop_back();
        v->animaHide(
            [box]() {
                auto stack = brls::Application::getActivitiesStack();
                brls::Activity* activityToDelete = stack.empty() ? nullptr : stack.back();
                bool popped = brls::Application::popActivity(
                    brls::TransitionAnimation::NONE,
                    [box, activityToDelete]() {
                        box->animaShow();
                        if (!activityToDelete)
                            return;

                        brls::delay(POP_ACTIVITY_DEFER_DELETE_MS, [activityToDelete]() {
                            brls::Logger::debug("Deferred delete popped activity");
                            delete activityToDelete;
                        });
                    },
                    false);
                if (!popped)
                    box->animaShow();
            }
        );
    }

} // namespace beiklive
