#pragma once

#include <string>
#include <cstdlib>
#include <filesystem>


namespace beiklive::path
{
namespace fs = std::filesystem;

inline std::string GetRootPath()
{
#if defined(_WIN32)
    return ".";
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    return std::string(home) +
           "/Library/Application Support/GBAStation";
#else
    return "";
#endif
}

// 路径分隔符和根路径常量定义
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    constexpr const char *SPLIT_CHAR = "\\";
#else
    constexpr const char *SPLIT_CHAR = "/";
#endif
    inline const std::string ROOT = GetRootPath();

    // 程序名和文件名常量定义
    constexpr const char *PROGRAM_NAME      = "GBAStation";

    constexpr const char *CONFIG_DIR        = "config";
    constexpr const char *LOG_DIR           = "log";
    constexpr const char *DATA_BASE_DIR     = "data";
    constexpr const char *CACHE_DIR         = "cache";
    constexpr const char *SCREENSHOT_DIR    = "screenshots";
    constexpr const char *ROM_DIR           = "roms";
    constexpr const char *SAVE_DIR          = "saves";
    constexpr const char *CHEATS_DIR        = "cheats";
    constexpr const char *SHADER_DIR        = "shaders";
    constexpr const char *CORE_DIR        = "cores";
    constexpr const char *BIOS_DIR        = "bios";
    constexpr const char *DBS_DIR         = "dbs";

    constexpr const char *CONFIG_FILE          = "config.cfg";
    constexpr const char *MAPPING_FILE         = "name_mapping.cfg";

    constexpr const char *LOG_FILE             = "GBAStation.log";
    constexpr const char *DATA_BASE_FILE       = "GameData.json";       ///< 合并主数据库文件（向后兼容）
    constexpr const char *DATA_BASE_FILE_GBA   = "GameData_GBA.json";   ///< GBA 平台数据库文件
    constexpr const char *DATA_BASE_FILE_GBC   = "GameData_GBC.json";   ///< GBC 平台数据库文件
    constexpr const char *DATA_BASE_FILE_GB    = "GameData_GB.json";    ///< GB 平台数据库文件
    constexpr const char *DATA_BASE_FILE_NES     = "GameData_NES.json";     ///< NES 平台数据库文件
    constexpr const char *DATA_BASE_FILE_SNES    = "GameData_SNES.json";    ///< SNES 平台数据库文件
    constexpr const char *DATA_BASE_FILE_NDS     = "GameData_NDS.json";     ///< NDS 平台数据库文件
    constexpr const char *DATA_BASE_FILE_3DS     = "GameData_3DS.json";     ///< Nintendo 3DS 平台数据库文件
    constexpr const char *DATA_BASE_FILE_GENESIS = "GameData_Genesis.json"; ///< Mega Drive / Genesis 平台数据库文件
    constexpr const char *DATA_BASE_FILE_ARCADE  = "GameData_Arcade.json";  ///< Arcade 平台数据库文件
    constexpr const char *DATA_BASE_FILE_DC      = "GameData_DC.json";      ///< Dreamcast 平台数据库文件
    constexpr const char *DATA_BASE_FILE_PSP     = "GameData_PSP.json";     ///< PSP 平台数据库文件
    constexpr const char *DATA_BASE_FILE_PS1     = "GameData_PS1.json";     ///< PlayStation 平台数据库文件
    constexpr const char *DATA_BASE_FILE_SATURN  = "GameData_Saturn.json";  ///< Sega Saturn 平台数据库文件
    constexpr const char *DATA_BASE_FILE_DOLPHIN = "GameData_Dolphin.json"; ///< GameCube / Wii 平台数据库文件

    namespace 
    {
        // 默认位置定义
        inline std::string configPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CONFIG_DIR;
        }
        inline std::string configFilePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CONFIG_DIR + SPLIT_CHAR + CONFIG_FILE;
        }
        inline std::string mappingFilePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CONFIG_DIR + SPLIT_CHAR + MAPPING_FILE;
        }
        inline std::string databasePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + DATA_BASE_DIR;
        }
        inline std::string databaseFilePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + DATA_BASE_DIR + SPLIT_CHAR + DATA_BASE_FILE;
        }
        /// 根据平台枚举返回对应的平台数据库文件名（不含路径）
        /// platform: 1=EmuGBA, 2=EmuGBC, 3=EmuGB（与 enums.h 中 EmuPlatform 枚举值对应）
        inline std::string platformDatabaseFileName(int platform)
        {
            switch (platform)
            {
            case 1: return DATA_BASE_FILE_GBA;  // EmuPlatform::EmuGBA
            case 2: return DATA_BASE_FILE_GBC;  // EmuPlatform::EmuGBC
            case 3: return DATA_BASE_FILE_GB;   // EmuPlatform::EmuGB
            case 4: return DATA_BASE_FILE_NES;     // EmuPlatform::EmuNES
            case 5: return DATA_BASE_FILE_SNES;    // EmuPlatform::EmuSNES
            case 6: return DATA_BASE_FILE_NDS;      // EmuPlatform::EmuNDS
            case 7: return DATA_BASE_FILE_3DS;      // EmuPlatform::Emu3DS
            case 8: return DATA_BASE_FILE_GENESIS;  // EmuPlatform::EmuGenesis
            case 9: return DATA_BASE_FILE_ARCADE;   // EmuPlatform::EmuArcade
            case 10: return DATA_BASE_FILE_DC;       // EmuPlatform::EmuDreamcast
            case 11: return DATA_BASE_FILE_PSP;      // EmuPlatform::EmuPSP
            case 12: return DATA_BASE_FILE_PS1;      // EmuPlatform::EmuPS1
            case 13: return DATA_BASE_FILE_SATURN;   // EmuPlatform::EmuSaturn
            case 14: return DATA_BASE_FILE_DOLPHIN;  // EmuPlatform::EmuDolphin
            default: return DATA_BASE_FILE;
            }
        }
        /// 根据平台枚举返回对应的平台数据库完整路径
        inline std::string platformDatabaseFilePath(int platform)
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + DATA_BASE_DIR + SPLIT_CHAR + platformDatabaseFileName(platform);
        }
        inline std::string logPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + LOG_DIR;
        }
        inline std::string logFilePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + LOG_DIR + SPLIT_CHAR + LOG_FILE;
        }
        inline std::string screenshotPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + SCREENSHOT_DIR;
        }
        inline std::string romPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + ROM_DIR;
        }
        inline std::string savePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + SAVE_DIR;
        }
        inline std::string cheatPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CHEATS_DIR;
        }
        inline std::string cachePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CACHE_DIR;
        }
        inline std::string shaderPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + SHADER_DIR;
        }
        inline std::string corePath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + CORE_DIR;
        }
        inline std::string biosPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + BIOS_DIR;
        }
        inline std::string dbsPath()
        {
            return ROOT + SPLIT_CHAR + PROGRAM_NAME + SPLIT_CHAR + DBS_DIR;
        }

    }

} // namespace beiklive::path


namespace beiklive::SettingKey
{
    // UI 设置
    constexpr const char *KEY_UI_START_PAGE         = "UI.startPage";              // 起始页面
    constexpr const char *KEY_UI_LANGUAGE           = "UI.language";               // 语言
    constexpr const char *KEY_UI_THEME              = "UI.theme";                  // 主题

    // UI 背景图片设置
    constexpr const char *KEY_UI_SHOW_BG_IMAGE      = "UI.showBgImage";            ///< 是否显示背景图片
    constexpr const char *KEY_UI_BG_IMAGE_PATH      = "UI.bgImagePath";            ///< 背景图片路径
    constexpr const char *KEY_UI_BG_GIF_SPEED       = "UI.bgGifSpeed";             ///< GIF 背景播放速度倍率
    constexpr const char *KEY_UI_BG_VIDEO_FRAME_RATE = "UI.bgVideoFrameRate";      ///< MP4 背景纹理更新帧率
    constexpr const char *KEY_UI_BG_BLUR_ENABLED    = "UI.bgBlurEnabled";          ///< 是否开启背景模糊
    constexpr const char *KEY_UI_BG_BLUR_RADIUS     = "UI.bgBlurRadius";           ///< 背景模糊半径

    // UI XMB 风格背景设置
    constexpr const char *KEY_UI_SHOW_XMB_BG        = "UI.showXmbBg";              ///< 是否显示 PSP XMB 风格背景
    constexpr const char *KEY_UI_SHOW_SHADER        = "UI.showShader";             ///< 是否显示动态渐变背景
    constexpr const char *KEY_UI_GRADIENT_THEME     = "UI.gradientTheme";          ///< 渐变背景主题
    constexpr const char *KEY_UI_PSPXMB_COLOR       = "UI.pspxmb.color";           ///< XMB 颜色预设 ID

    // UI 缩略图设置
    constexpr const char *KEY_UI_USE_SAVESTATE_THUMB = "UI.useSavestateThumbnail"; ///< 无封面时使用存档0截图作为缩略图
    constexpr const char *KEY_UI_PICO8_SHORTCUT_VISIBLE = "UI.pico8ShortcutVisible"; ///< 是否显示首页 PICO-8 快捷入口

    // 遮罩设置
    constexpr const char *KEY_DISPLAY_OVERLAY_ENABLED  ="display.overlay.enabled"; ///< 遮罩总开关
    constexpr const char *KEY_DISPLAY_OVERLAY_GBA_PATH ="display.overlay.gbaPath"; ///< 全局 GBA 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_GBC_PATH ="display.overlay.gbcPath"; ///< 全局 GBC 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_GB_PATH  ="display.overlay.gbPath";  ///< 全局 GB 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_NES_PATH  ="display.overlay.nesPath";  ///< 全局 NES 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_SNES_PATH ="display.overlay.snesPath"; ///< 全局 SNES 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_NDS_PATH  ="display.overlay.ndsPath";  ///< 全局 NDS 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_GENESIS_PATH ="display.overlay.genesisPath"; ///< 全局 MD 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_ARCADE_PATH ="display.overlay.arcadePath"; ///< 全局 Arcade 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_DC_PATH ="display.overlay.dcPath"; ///< 全局 DC 遮罩 PNG 路径
    constexpr const char *KEY_DISPLAY_OVERLAY_PSP_PATH ="display.overlay.pspPath"; ///< 全局 PSP 遮罩 PNG 路径

    // 扫描导入：各机型的 ROM 扫描目录（空 = 不扫描该机型）
    constexpr const char *KEY_SCAN_PATH_NES    ="scan.path.nes";
    constexpr const char *KEY_SCAN_PATH_SNES   ="scan.path.sfc";
    constexpr const char *KEY_SCAN_PATH_GB     ="scan.path.gb";
    constexpr const char *KEY_SCAN_PATH_GBC    ="scan.path.gbc";
    constexpr const char *KEY_SCAN_PATH_GBA    ="scan.path.gba";
    constexpr const char *KEY_SCAN_PATH_NDS    ="scan.path.nds";
    constexpr const char *KEY_SCAN_PATH_3DS    ="scan.path.3ds";
    constexpr const char *KEY_SCAN_PATH_ARCADE ="scan.path.arcade";
    constexpr const char *KEY_SCAN_PATH_DC     ="scan.path.dc";
    constexpr const char *KEY_SCAN_PATH_GENESIS="scan.path.md";
    constexpr const char *KEY_SCAN_PATH_PSP    ="scan.path.psp";
    constexpr const char *KEY_SCAN_PATH_PS1    ="scan.path.ps1";
    constexpr const char *KEY_SCAN_PATH_SATURN ="scan.path.saturn";
    constexpr const char *KEY_SCAN_PATH_DOLPHIN="scan.path.dolphin";

    // 着色器设置（全局默认）
    constexpr const char *KEY_DISPLAY_SHADER_ENABLED   ="display.shaderEnabled";   ///< 着色器总开关（true=启用）
    constexpr const char *KEY_DISPLAY_SHADER_PATH      ="display.shader";          ///< 着色器预设路径（.glslp）
    constexpr const char *KEY_DISPLAY_SHADER_GBA_PATH  ="display.shader.gba";      ///< GBA 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_GBC_PATH  ="display.shader.gbc";      ///< GBC 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_GB_PATH   ="display.shader.gb";       ///< GB 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_NES_PATH  ="display.shader.nes";      ///< NES 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_SNES_PATH ="display.shader.snes";     ///< SNES 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_NDS_PATH  ="display.shader.nds";      ///< NDS 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_GENESIS_PATH ="display.shader.genesis"; ///< MD 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_ARCADE_PATH ="display.shader.arcade"; ///< Arcade 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_DC_PATH ="display.shader.dc"; ///< DC 着色器预设路径
    constexpr const char *KEY_DISPLAY_SHADER_PSP_PATH ="display.shader.psp"; ///< PSP 着色器预设路径

    // 音频设置
    constexpr const char *KEY_AUDIO_BUTTON_SFX          = "audio.buttonSfx";          ///< 按钮音效开关
    constexpr const char *KEY_AUDIO_BUTTON_SFX_VOLUME   = "audio.buttonSfxVolume";    ///< 按钮音效音量（0-100）
    constexpr const char *KEY_AUDIO_MASTER_VOLUME       = "audio.masterVolume";       ///< 全局主音量（0-100）
    constexpr const char *KEY_AUDIO_TARGET_LATENCY_MS   = "audio.targetLatencyMs";    ///< 音频同步目标缓冲延迟（毫秒）
    constexpr const char *KEY_AUDIO_MAX_LATENCY_MS      = "audio.maxLatencyMs";       ///< 音频最大缓冲延迟（毫秒）
    constexpr const char *KEY_AUDIO_SYNC_STRENGTH       = "audio.syncStrength";       ///< 音画同步修正强度
    constexpr const char *KEY_AUDIO_TRANSITION_FADE_MS  = "audio.transitionFadeMs";   ///< 静音/暂停/读档切换淡入淡出时间（毫秒）

    // 倒带设置
    constexpr const char *KEY_REWIND_SAVE_INTERVAL      = "rewind.saveInterval";        ///< 倒带状态保存间隔（每 N 帧保存一次，默认 1）
    constexpr const char *KEY_REWIND_SHOW_UI            = "rewind.showUI";              ///< 是否启用可视化倒带界面（同时开启缩略图保存）
    constexpr const char *KEY_REWIND_UI_ITEM_COUNT      = "rewind.uiItemCount";         ///< 保留兼容性，已不使用（item 数量由保存间隔自动计算）
    constexpr const char *KEY_REWIND_THUMB_COMPRESSION  = "rewind.thumbCompression";    ///< 缩略图压缩策略（0=最近邻，1=双线性，默认 0）
    constexpr const char *KEY_REWIND_BUFFER_SIZE        = "rewind.bufferSize";          ///< 倒带缓冲区最大保存帧数（默认 600）

    // 调试设置
    constexpr const char *KEY_DEBUG_LOG_LEVEL       = "debug.logLevel";            ///< 日志级别
    constexpr const char *KEY_DEBUG_LOG_FILE        = "debug.logFile";             ///< 是否输出日志到文件
    constexpr const char *KEY_DEBUG_LOG_OVERLAY     = "debug.logOverlay";          ///< 是否显示调试覆盖层

    // 更新设置
    constexpr const char *KEY_EMU_UPDATE              = "emu.update";                 ///< 启用更新检查（1=启用 0=禁用）

    // 文件浏览器设置
    constexpr const char *KEY_FILE_LIST_SCROLL_ANIM   = "ui.fileListScrollAnim";     ///< 文件列表滚动动画（1=启用 0=禁用）

    // 游戏库设置
    constexpr const char *KEY_UI_LIBRARY_TITLE_SIZE   = "ui.libraryTitleSize";       ///< 游戏库标题字号 (0=16 1=19 2=22)
    constexpr const char *KEY_UI_LIBRARY_VIEW_MODE    = "ui.libraryViewMode";        ///< 游戏库视图 (0=网格 1=列表)


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

