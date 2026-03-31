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


} // namespace beiklive
