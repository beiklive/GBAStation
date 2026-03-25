#pragma once

#include <string>

namespace beiklive::path
{

// 路径分隔符和根路径常量定义
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    constexpr const char *SPLIT_CHAR = "\\";
    constexpr const char *ROOT = ".";  // Windows系统使用程序目录作为根路径
#else
    constexpr const char *SPLIT_CHAR = "/";
    constexpr const char *ROOT = "";   // 类Unix系统使用根目录作为根路径
#endif

    // 程序名和文件名常量定义
    constexpr const char *PROGRAM_NAME      = "GBAStation";

    constexpr const char *CONFIG_DIR        = "config";
    constexpr const char *LOG_DIR           = "log";
    constexpr const char *DATA_BASE_DIR     = "data";
    constexpr const char *SCREENSHOT_DIR    = "screenshots";
    constexpr const char *ROM_DIR           = "roms";
    constexpr const char *SAVE_DIR          = "saves";
    constexpr const char *CHEATS_DIR        = "cheats";
    constexpr const char *SHADER_DIR        = "shaders";

    constexpr const char *CONFIG_FILE       = "config.cfg";
    constexpr const char *MAPPING_FILE       = "name_mapping.cfg";

    constexpr const char *LOG_FILE          = "GBAStation.log";
    constexpr const char *DATA_BASE_FILE    = "database.db";

    namespace 
    {
        // 默认位置定义
        inline std::string configPath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CONFIG_DIR;
        }
        inline std::string configFilePath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CONFIG_DIR + SPLIT_CHAR + CONFIG_FILE;
        }
        inline std::string mappingFilePath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CONFIG_DIR + SPLIT_CHAR + MAPPING_FILE;
        }
        inline std::string databasePath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + DATA_BASE_DIR;
        }
        inline std::string databaseFilePath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + DATA_BASE_DIR + SPLIT_CHAR + DATA_BASE_FILE;
        }
        inline std::string logPath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + LOG_DIR;
        }
        inline std::string logFilePath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + LOG_DIR + SPLIT_CHAR + LOG_FILE;
        }
        inline std::string screenshotPath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + SCREENSHOT_DIR;
        }
        inline std::string romPath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + ROM_DIR;
        }
        inline std::string savePath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + SAVE_DIR;
        }
        inline std::string cheatPath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CHEATS_DIR;
        }
        inline std::string shaderPath()
        {
            return std::string(ROOT) + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + SHADER_DIR;
        }

    }

} // namespace beiklive::path


namespace beiklive::SettingKey
{
    // UI 设置
    constexpr const char *KEY_UI_START_PAGE     = "UI.startPage";   // 起始页面
    constexpr const char *KEY_UI_LANGUAGE       = "UI.language";    // 语言
    constexpr const char *KEY_UI_THEME          = "UI.theme";       // 主题

    //遮罩设置
    constexpr const char *KEY_DISPLAY_OVERLAY_ENABLED  ="display.overlay.enabled";  ///< 遮罩总开关
    constexpr const char *KEY_DISPLAY_OVERLAY_GBA_PATH ="display.overlay.gbaPath";  ///< 全局 GBA 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_GBC_PATH ="display.overlay.gbcPath";  ///< 全局 GBC 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_GB_PATH  ="display.overlay.gbPath";   ///< 全局 GB 遮罩 PNG 路径

    // 着色器设置（全局默认）
    constexpr const char *KEY_DISPLAY_SHADER_ENABLED   ="display.shaderEnabled";    ///< 着色器总开关（true=启用）
    constexpr const char *KEY_DISPLAY_SHADER_PATH      ="display.shader";           ///< 着色器预设路径（.glslp）
    constexpr const char *KEY_DISPLAY_SHADER_GBA_PATH  ="display.shader.gba";        ///< GBA 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_GBC_PATH  ="display.shader.gbc";        ///< GBC 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_GB_PATH   ="display.shader.gb";         ///< GB 着色器预设路径





} // namespace beiklive::SettingKey

namespace beiklive::GameDataKey
{

    constexpr const char *GAMEDATA_FIELD_LOGOPATH               = "logopath";                       ///< logo 图片路径（空=未设置）
    constexpr const char *GAMEDATA_FIELD_GAMEPATH               = "gamepath";                       ///< 游戏文件路径
    constexpr const char *GAMEDATA_FIELD_TOTALTIME              = "totaltime";                      ///< 游玩总时长（秒，默认 0）
    constexpr const char *GAMEDATA_FIELD_LASTOPEN               = "lastopen";                       ///< 上次游玩时间（默认"从未游玩"）
    constexpr const char *GAMEDATA_FIELD_PLATFORM               = "platform";                       ///< 游戏平台（EmuPlatform 名称字符串）
    constexpr const char *GAMEDATA_FIELD_OVERLAY                = "overlay";                        ///< 游戏专属遮罩 PNG 路径（空=使用全局）
    constexpr const char *GAMEDATA_FIELD_CHEATPATH              = "cheatpath";                      ///< 金手指 .cht 文件路径（空=使用默认路径）
    constexpr const char *GAMEDATA_FIELD_PLAYCOUNT              = "playcount";                      ///< 游戏启动次数（每次启动加一，默认 0）
    constexpr const char *GAMEDATA_FIELD_X_OFFSET               = "xoffset";                        ///< 游戏专属 X 坐标偏移（像素，浮点）
    constexpr const char *GAMEDATA_FIELD_Y_OFFSET               = "yoffset";                        ///< 游戏专属 Y 坐标偏移（像素，浮点）
    constexpr const char *GAMEDATA_FIELD_CUSTOM_SCALE           = "customscale";                    ///< 游戏专属自定义缩放倍率（浮点）
    constexpr const char *GAMEDATA_FIELD_DISPLAY_FILTER         = "display.filter";                 ///< 纹理过滤模式（"nearest"/"linear"）
    constexpr const char *GAMEDATA_FIELD_DISPLAY_INT_SCALE      = "display.integer_scale_mult";     ///< 整数倍缩放倍率
    constexpr const char *GAMEDATA_FIELD_DISPLAY_MODE           = "display.mode";                   ///< 显示模式（"fit"/"fill"/...）
    constexpr const char *GAMEDATA_FIELD_DISPLAY_X_OFFSET       = "display.x_offset";               ///< X 坐标偏移（与 xoffset 别名兼容）
    constexpr const char *GAMEDATA_FIELD_DISPLAY_Y_OFFSET       = "display.y_offset";               ///< Y 坐标偏移
    constexpr const char *GAMEDATA_FIELD_DISPLAY_CUSTOM_SCALE   = "display.custom_scale";           ///< 自定义缩放倍率
    constexpr const char *GAMEDATA_FIELD_SHADER_ENABLED         = "shader.enabled";                 ///< 游戏专属着色器开关（"true"/"false"）
    constexpr const char *GAMEDATA_FIELD_SHADER_PATH            = "shader.path";                    ///< 游戏专属着色器路径（.glslp）
    constexpr const char *GAMEDATA_FIELD_SHADER_PARAM_NAMES     = "shader.params.name";             ///< 参数名列表（StringArray）
    constexpr const char *GAMEDATA_FIELD_SHADER_PARAM_VALUES    = "shader.params.value";            ///< 参数值列表（FloatArray）


} // namespace beiklive::GameDataKey


namespace beiklive::DefaultFile
{
    constexpr const char *DEFAULT_LOGO = ""; // 默认 logo 路径（空=未设置）

}