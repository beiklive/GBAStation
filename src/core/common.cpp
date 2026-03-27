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




} // namespace beiklive
