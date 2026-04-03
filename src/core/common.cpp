#include "common.h"
#include <filesystem>



namespace beiklive
{

ConfigManager* SettingManager = nullptr; // 全局配置管理器实例
ConfigManager* NameMappingManager = nullptr; // 全局名称映射管理器实例
GameDatabase* GameDB = nullptr; // 全局游戏数据库实例

void ConfigureInit(){
    // 确保必要的目录存在
    std::filesystem::create_directories(beiklive::path::configPath());
    std::filesystem::create_directories(beiklive::path::databasePath());
    std::filesystem::create_directories(beiklive::path::logPath());
    std::filesystem::create_directories(beiklive::path::screenshotPath());
    std::filesystem::create_directories(beiklive::path::romPath());
    std::filesystem::create_directories(beiklive::path::savePath());
    std::filesystem::create_directories(beiklive::path::cheatPath());
    std::filesystem::create_directories(beiklive::path::shaderPath());


    SettingManager = new beiklive::ConfigManager(beiklive::path::configFilePath());
    NameMappingManager = new beiklive::ConfigManager(beiklive::path::mappingFilePath());
    SettingManager->Load();
    NameMappingManager->Load();

    // 数据库初始化
    if(SettingManager->Contains("db_path")){
        GameDB = new beiklive::GameDatabase(GET_SETTING_KEY_STR("db_path", beiklive::path::databaseFilePath()));
    }else{
        GameDB = new beiklive::GameDatabase(beiklive::path::databaseFilePath());
        SettingManager->Set("db_path", beiklive::ConfigValue(beiklive::path::databaseFilePath()));
    }

    // ── 预设所有设置项的默认值（仅当配置文件中不存在时才设置）──────────────
    using namespace beiklive::SettingKey;

    // UI 背景图片设置
    SettingManager->SetDefault(KEY_UI_SHOW_BG_IMAGE,       ConfigValue(0));
    SettingManager->SetDefault(KEY_UI_BG_IMAGE_PATH,        ConfigValue(std::string("")));
    SettingManager->SetDefault(KEY_UI_BG_BLUR_ENABLED,      ConfigValue(0));
    SettingManager->SetDefault(KEY_UI_BG_BLUR_RADIUS,       ConfigValue(12.0f));
    SettingManager->SetDefault(KEY_UI_SHOW_XMB_BG,          ConfigValue(0));
    SettingManager->SetDefault(KEY_UI_PSPXMB_COLOR,         ConfigValue(std::string("blue")));
    SettingManager->SetDefault(KEY_UI_USE_SAVESTATE_THUMB,  ConfigValue(0));

    // 遮罩设置
    SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_ENABLED,  ConfigValue(0));
    SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GBA_PATH, ConfigValue(std::string("")));
    SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GBC_PATH, ConfigValue(std::string("")));
    SettingManager->SetDefault(KEY_DISPLAY_OVERLAY_GB_PATH,  ConfigValue(std::string("")));

    // 着色器设置
    SettingManager->SetDefault(KEY_DISPLAY_SHADER_ENABLED,  ConfigValue(0));
    SettingManager->SetDefault(KEY_DISPLAY_SHADER_PATH,     ConfigValue(std::string("")));
    SettingManager->SetDefault(KEY_DISPLAY_SHADER_GBA_PATH, ConfigValue(std::string("")));
    SettingManager->SetDefault(KEY_DISPLAY_SHADER_GBC_PATH, ConfigValue(std::string("")));
    SettingManager->SetDefault(KEY_DISPLAY_SHADER_GB_PATH,  ConfigValue(std::string("")));

    // 调试设置
    SettingManager->SetDefault(KEY_DEBUG_LOG_LEVEL,   ConfigValue(std::string("info")));
    SettingManager->SetDefault(KEY_DEBUG_LOG_FILE,    ConfigValue(0));
    SettingManager->SetDefault(KEY_DEBUG_LOG_OVERLAY, ConfigValue(0));

    // 快进设置
    SettingManager->SetDefault("fastforward.enabled",    ConfigValue(1));
    SettingManager->SetDefault("fastforward.mode",       ConfigValue(std::string("hold")));
    SettingManager->SetDefault("fastforward.multiplier", ConfigValue(4.0f));
    SettingManager->SetDefault("fastforward.mute",       ConfigValue(1));

    // 倒带设置
    SettingManager->SetDefault("rewind.enabled",    ConfigValue(0));
    SettingManager->SetDefault("rewind.mode",       ConfigValue(std::string("hold")));
    SettingManager->SetDefault("rewind.bufferSize", ConfigValue(3600));
    SettingManager->SetDefault("rewind.step",       ConfigValue(2));
    SettingManager->SetDefault("rewind.mute",       ConfigValue(0));

    // 核心设置
    SettingManager->SetDefault("core.mgba_gb_model",              ConfigValue(std::string("Autodetect")));
    SettingManager->SetDefault("core.mgba_use_bios",              ConfigValue(std::string("ON")));
    SettingManager->SetDefault("core.mgba_skip_bios",             ConfigValue(std::string("OFF")));
    SettingManager->SetDefault("core.mgba_gb_colors",             ConfigValue(std::string("Grayscale")));
    SettingManager->SetDefault("core.mgba_idle_optimization",     ConfigValue(std::string("Remove Known")));
    SettingManager->SetDefault("core.mgba_audio_low_pass_filter", ConfigValue(std::string("disabled")));

    // 画面设置
    SettingManager->SetDefault("display.mode",                ConfigValue(std::string("original")));
    SettingManager->SetDefault("display.integer_scale_mult",  ConfigValue(0));
    SettingManager->SetDefault("display.filter",              ConfigValue(std::string("nearest")));
    SettingManager->SetDefault("display.showFps",             ConfigValue(0));
    SettingManager->SetDefault("display.showFfOverlay",       ConfigValue(1));
    SettingManager->SetDefault("display.showRewindOverlay",   ConfigValue(1));
    SettingManager->SetDefault("display.showMuteOverlay",     ConfigValue(1));

    // 存档设置
    SettingManager->SetDefault("save.autoSaveState",    ConfigValue(0));
    SettingManager->SetDefault("save.autoSaveInterval", ConfigValue(0));
    SettingManager->SetDefault("save.autoLoadState0",   ConfigValue(0));
    SettingManager->SetDefault("save.sramDir",          ConfigValue(std::string("")));
    SettingManager->SetDefault("save.stateDir",         ConfigValue(std::string("")));

    // 截图设置
    SettingManager->SetDefault("screenshot.dir", ConfigValue(0));

    // 金手指设置
    SettingManager->SetDefault("cheat.enabled", ConfigValue(0));
    SettingManager->SetDefault("cheat.dir",     ConfigValue(std::string("")));

    SettingManager->Save();
    NameMappingManager->Save();
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
std::vector<CheatEntry> parseChtFile(const std::string& path)
{
    std::vector<CheatEntry> result;

    if (!std::filesystem::exists(path)) {
        brls::Logger::info("GameView: no cheat file found at {}", path);
        return result;
    }

    std::ifstream f(path);
    if (!f) {
        brls::Logger::warning("GameView: failed to open cheat file: {}", path);
        return result;
    }

    std::string content;
    {
        std::ostringstream oss;
        oss << f.rdbuf();
        content = oss.str();
    }

    if (content.find("cheats = ") != std::string::npos ||
        content.find("cheats=")   != std::string::npos)
    {
        // ---- RetroArch .cht 格式 ----
        std::unordered_map<std::string, std::string> kv;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key   = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            auto trim = [](std::string s) -> std::string {
                size_t b = s.find_first_not_of(" \t\"");
                size_t e = s.find_last_not_of(" \t\"");
                if (b == std::string::npos) return "";
                return s.substr(b, e - b + 1);
            };
            kv[trim(key)] = trim(value);
        }

        unsigned total = 0;
        {
            auto it = kv.find("cheats");
            if (it != kv.end()) {
                try { total = static_cast<unsigned>(std::stoi(it->second)); } catch (...) {}
            }
        }

        for (unsigned i = 0; i < total; ++i) {
            std::string iStr      = std::to_string(i);
            std::string descKey   = "cheat" + iStr + "_desc";
            std::string enableKey = "cheat" + iStr + "_enable";
            std::string codeKey   = "cheat" + iStr + "_code";

            auto cit = kv.find(codeKey);
            if (cit == kv.end()) continue;

            CheatEntry entry;
            entry.code    = cit->second;
            entry.enabled = true;
            entry.desc    = "cheat" + iStr;

            auto eit = kv.find(enableKey);
            if (eit != kv.end()) {
                std::string ev = eit->second;
                for (char& c : ev) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                entry.enabled = (ev == "true" || ev == "1" || ev == "yes");
            }

            auto dit = kv.find(descKey);
            if (dit != kv.end()) entry.desc = dit->second;

            result.push_back(std::move(entry));
        }
    }
    else
    {
        // ---- 简单逐行格式 ----
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            size_t b = line.find_first_not_of(" \t");
            if (b == std::string::npos) continue;
            line = line.substr(b);
            if (line.empty()) continue;

            CheatEntry entry;
            entry.enabled = true;
            if (line[0] == '+') {
                entry.enabled = true;
                line = line.substr(1);
            } else if (line[0] == '-') {
                entry.enabled = false;
                line = line.substr(1);
            }
            b = line.find_first_not_of(" \t");
            if (b == std::string::npos) continue;
            line = line.substr(b);
            if (line.empty()) continue;

            entry.code = line;
            entry.desc = line; // 简单格式无名称，使用代码作为描述
            result.push_back(std::move(entry));
        }
    }

    return result;
}

/// 将金手指列表以 RetroArch .cht 格式写入文件。
/// 返回 true 表示成功。
bool saveChtFile(const std::string& path,
                        const std::vector<CheatEntry>& entries)
{
    std::ofstream f(path);
    if (!f) {
        brls::Logger::warning("GameView: 无法写入金手指文件: {}", path);
        return false;
    }

    f << "cheats = " << entries.size() << "\n\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        f << "cheat" << i << "_desc = \"" << e.desc << "\"\n";
        f << "cheat" << i << "_enable = " << (e.enabled ? "true" : "false") << "\n";
        f << "cheat" << i << "_code = " << e.code << "\n\n";
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
    default:
        break;
    }
    return 0;
}

} // namespace beiklive
