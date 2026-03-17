#pragma once
#include <algorithm>
#include <cstdint>
#include <borealis.hpp>

/// 像素颜色类型（XRGB8888，与 libretro RETRO_PIXEL_FORMAT_XRGB8888 一致）
using color_t = uint32_t;
using namespace brls::literals; // for _i18n

#include "Utils/ConfigManager.hpp"
#include "Utils/fileUtils.hpp"
#include "Utils/strUtils.hpp"
#include "UI/Utils/Utils.hpp"
#include "UI/Utils/ProImage.hpp"
#include "UI/Utils/ImageFileCache.hpp"

// 函数宏定义

#define BK_RES(path) beiklive::file::TransStrToRes(path)
#define ADD_STYLE(name, value) \
    brls::Application::getStyle().addMetric(name, value)
#define ADD_THEME_COLOR(name, color) \
    brls::Application::getTheme().addColor(name, color)
#define GET_STYLE(name) \
    brls::Application::getStyle().getMetric(name)
#define GET_THEME_COLOR(name) \
    brls::Application::getTheme().getColor(name)

	
#define BK_APP_NAME "BKStation"
#ifdef __SWITCH__
#define BK_APP_ROOT_DIR "/BKStation/"
#define BK_APP_LOG_DIR "/BKStation/log/"
#define BK_APP_LOG_FILE "/BKStation/log/BKStation.log"
#define BK_APP_CONFIG_DIR "/BKStation/config/"
#define BK_APP_CONFIG_FILE "/BKStation/config/config.ini"
#define BK_GAME_CORE_DIR "/BKStation/cores/"
#else
#define BK_APP_ROOT_DIR "./BKStation/"
#define BK_APP_LOG_DIR "./BKStation/log/"
#define BK_APP_LOG_FILE "./BKStation/log/BKStation.log"
#define BK_APP_CONFIG_DIR "./BKStation/config/"
#define BK_APP_CONFIG_FILE "./BKStation/config/config.ini"
#define BK_GAME_CORE_DIR "./BKStation/cores/"
#endif

// 应用默认图标
#define BK_APP_DEFAULT_LOGO BK_RES("icon/default.png")
#define BK_APP_DEFAULT_BG BK_RES("img/bg2.png")



#define KEY_UI_START_PAGE "UI.startPage" // 开始页面
#define KEY_UI_LANGUAGE "UI.language" // 语言
#define KEY_UI_THEME "UI.theme" // 主题

// UI / 背景设置
#define KEY_UI_SHOW_BG_IMAGE  "UI.showBgImage"
#define KEY_UI_BG_IMAGE_PATH  "UI.bgImagePath"
#define KEY_UI_BG_BLUR_ENABLED "UI.bgBlurEnabled"
#define KEY_UI_BG_BLUR_RADIUS  "UI.bgBlurRadius"
#define KEY_UI_SHOW_XMB_BG    "UI.showXmbBg"
#define KEY_UI_PSPXMB_COLOR   "UI.pspxmb.color"
// #define KEY_UI_TEXT_COLOR     "UI.textColor"

// 音频设置
#define KEY_AUDIO_BUTTON_SFX  "audio.buttonSfx"

// 调试设置
#define KEY_DEBUG_LOG_LEVEL   "debug.logLevel"
#define KEY_DEBUG_LOG_FILE    "debug.logFile"
#define KEY_DEBUG_LOG_OVERLAY "debug.logOverlay"

using bklog = brls::Logger;

namespace beiklive {

void RegisterStyles();
void RegisterThemes();
void CheckGLSupport();
void InsertBackground(brls::Box* view);
/// 应用 XMB 颜色预设（UI.pspxmb.color）到指定 img。
void ApplyXmbColor(beiklive::UI::ProImage* img);
/// 根据当前 SettingManager 配置将所有背景设置应用到 img。
void ApplyXmbBg(beiklive::UI::ProImage* img);
/// 将所有背景设置应用到所有已注册的背景 ProImage 实例。
void ApplyXmbColorToAll();
/// 注册/注销背景 ProImage 实例，供 ApplyXmbColorToAll() 访问。
void RegisterXmbBackground(beiklive::UI::ProImage* img);
void UnregisterXmbBackground(beiklive::UI::ProImage* img);
/// 清空文件字节图片缓存并标记所有 brls TextureCache 为脏。
/// 在打开 GameView 前调用以释放内存。
void clearUIImageCache();
// 吞噬一个按钮事件，使其不再被后续处理
void swallow(brls::View* v, brls::ControllerButton btn);

// ─── Config 读写辅助函数 ──────────────────────────────────────────────────────
// 从 SettingManager 读取/写入配置项，写入后自动保存
bool        cfgGetBool(const std::string& key, bool def);
std::string cfgGetStr(const std::string& key, const std::string& def);
float       cfgGetFloat(const std::string& key, float def);
int         cfgGetInt(const std::string& key, int def);
void        cfgSetStr(const std::string& key, const std::string& val);
void        cfgSetBool(const std::string& key, bool val);




struct GameEntry {
    std::string path;    ///< ROM 文件完整路径
    std::string title;   ///< 显示标题
    std::string cover;   ///< 封面图路径（可空，空则显示占位符）
};

/// ROM 平台类型（独立于任何模拟器后端）
enum class EmuPlatform { None, GBA, GB };

struct Gamefile {
	// 游戏文件的路径
	std::string path;
	// 游戏文件名称
	std::string name;
	// 游戏文件显示名称
	std::string displayName;
	// 游戏文件类型（例如：GBA、GB）
	EmuPlatform type = EmuPlatform::None;
	// 游戏输出分辨率
	int width;
	int height;
};

struct UIParams {
	// 起始页面索引，0为APP页面，1为文件列表页面
	brls::AppletFrame *StartPageframe;



};


// 游戏运行时环境
struct GameRunner {
	beiklive::ConfigManager* settingConfig;
	beiklive::ConfigManager* nameMappingConfig;
	Gamefile gameFile;
	std::string currentPath;


	const char* port;

	struct UIParams *uiParams; // UI 相关参数


};

};

// GameMenu 依赖 beiklive::EmuPlatform，须在此命名空间定义之后包含
#include "UI/Utils/GameMenu.hpp"

// 全局配置管理器实例
extern beiklive::ConfigManager* SettingManager;
extern beiklive::ConfigManager* NameMappingManager;
/// 游戏数据管理器：key = 文件名(不含后缀).字段名，例如 pokemon.logopath
extern beiklive::ConfigManager* gamedataManager;
/// 游戏列表管理器：统计所有打开过的游戏文件，listcount 为总数，game0/game1/... 为文件名
extern beiklive::ConfigManager* PlaylistManager;
extern beiklive::GameRunner* gameRunner;

// Display overlay settings
#define KEY_DISPLAY_OVERLAY_ENABLED  "display.overlay.enabled"   ///< 遮罩总开关
#define KEY_DISPLAY_OVERLAY_GBA_PATH "display.overlay.gbaPath"   ///< 全局 GBA 遮罩 PNG 路径
#define KEY_DISPLAY_OVERLAY_GBC_PATH "display.overlay.gbcPath"   ///< 全局 GBC 遮罩 PNG 路径

// 着色器设置
#define KEY_DISPLAY_SHADER_ENABLED   "display.shaderEnabled"     ///< 着色器总开关（true=启用）
#define KEY_DISPLAY_SHADER_PATH      "display.shader"            ///< 着色器预设路径（.glslp）

// ─── gamedataManager 字段名常量 ──────────────────────────────────────────────
#define GAMEDATA_FIELD_LOGOPATH   "logopath"   ///< logo 图片路径（空=未设置）
#define GAMEDATA_FIELD_GAMEPATH   "gamepath"   ///< 游戏文件路径
#define GAMEDATA_FIELD_TOTALTIME  "totaltime"  ///< 游玩总时长（秒，默认 0）
#define GAMEDATA_FIELD_LASTOPEN   "lastopen"   ///< 上次游玩时间（默认"从未游玩"）
#define GAMEDATA_FIELD_PLATFORM   "platform"   ///< 游戏平台（EmuPlatform 名称字符串）
#define GAMEDATA_FIELD_OVERLAY    "overlay"    ///< 游戏专属遮罩 PNG 路径（空=使用全局）
#define GAMEDATA_FIELD_CHEATPATH  "cheatpath"  ///< 金手指 .cht 文件路径（空=使用默认路径）
#define GAMEDATA_FIELD_PLAYCOUNT  "playcount"  ///< 游戏启动次数（每次启动加一，默认 0）
#define GAMEDATA_FIELD_X_OFFSET   "xoffset"    ///< 游戏专属 X 坐标偏移（像素，浮点）
#define GAMEDATA_FIELD_Y_OFFSET   "yoffset"    ///< 游戏专属 Y 坐标偏移（像素，浮点）
#define GAMEDATA_FIELD_CUSTOM_SCALE "customscale" ///< 游戏专属自定义缩放倍率（浮点）

/// 返回 gamedata key 的前缀（文件名不含后缀）
inline std::string gamedataKeyPrefix(const std::string& fileName)
{
    auto dot = fileName.rfind('.');
    return (dot != std::string::npos) ? fileName.substr(0, dot) : fileName;
}

/// 初始化 gamedataManager 中某个游戏条目的默认值（若已存在则不覆盖）。
/// @param fileName  文件名（含后缀），用来推导 key 前缀。
/// @param platform  游戏平台枚举。
inline void initGameData(const std::string& fileName, beiklive::EmuPlatform platform)
{
    if (!gamedataManager) return;
    std::string prefix = gamedataKeyPrefix(fileName);

    auto setIfAbsent = [&](const std::string& field, const beiklive::ConfigValue& defVal) {
        std::string k = prefix + "." + field;
        if (!gamedataManager->Contains(k))
            gamedataManager->Set(k, defVal);
    };

    setIfAbsent(GAMEDATA_FIELD_LOGOPATH,  beiklive::ConfigValue(std::string("")));
    setIfAbsent(GAMEDATA_FIELD_GAMEPATH,  beiklive::ConfigValue(std::string("")));
    setIfAbsent(GAMEDATA_FIELD_TOTALTIME, beiklive::ConfigValue(0));
    setIfAbsent(GAMEDATA_FIELD_LASTOPEN,  beiklive::ConfigValue(std::string("从未游玩")));

    // Platform: 将枚举转为字符串
    std::string platformStr;
    switch (platform) {
        case beiklive::EmuPlatform::GBA: platformStr = "GBA"; break;
        case beiklive::EmuPlatform::GB:  platformStr = "GB";  break;
        default:                          platformStr = "None"; break;
    }
    setIfAbsent(GAMEDATA_FIELD_PLATFORM, beiklive::ConfigValue(platformStr));

    // Overlay：默认使用该平台的全局遮罩路径
    {
        std::string globalOverlay;
        if (SettingManager) {
            std::string key;
            switch (platform) {
                case beiklive::EmuPlatform::GBA: key = KEY_DISPLAY_OVERLAY_GBA_PATH; break;
                case beiklive::EmuPlatform::GB:  key = KEY_DISPLAY_OVERLAY_GBC_PATH; break;
                default: break;
            }
            if (!key.empty()) {
                auto v = SettingManager->Get(key);
                if (v) { if (auto s = v->AsString()) globalOverlay = *s; }
            }
        }
        setIfAbsent(GAMEDATA_FIELD_OVERLAY, beiklive::ConfigValue(globalOverlay));
    }
    // 金手指路径：默认为空（启动游戏时由 GameView 写入实际路径）
    setIfAbsent(GAMEDATA_FIELD_CHEATPATH, beiklive::ConfigValue(std::string("")));
    // 启动次数：默认 0
    setIfAbsent(GAMEDATA_FIELD_PLAYCOUNT, beiklive::ConfigValue(0));
}

/// 读取 gamedataManager 中某个游戏字段的字符串值。
inline std::string getGameDataStr(const std::string& fileName, const std::string& field,
                                   const std::string& def = "")
{
    if (!gamedataManager) return def;
    std::string k = gamedataKeyPrefix(fileName) + "." + field;
    auto v = gamedataManager->Get(k);
    if (!v) return def;
    if (auto s = v->AsString()) return *s;
    return def;
}

/// 设置 gamedataManager 中某个游戏字段并保存。
inline void setGameDataStr(const std::string& fileName, const std::string& field,
                            const std::string& val)
{
    if (!gamedataManager) return;
    std::string k = gamedataKeyPrefix(fileName) + "." + field;
    gamedataManager->Set(k, beiklive::ConfigValue(val));
    gamedataManager->Save();
}

/// 读取 gamedataManager 中某个游戏字段的整数值。
inline int getGameDataInt(const std::string& fileName, const std::string& field,
                           int def = 0)
{
    if (!gamedataManager) return def;
    std::string k = gamedataKeyPrefix(fileName) + "." + field;
    auto v = gamedataManager->Get(k);
    if (!v) return def;
    if (auto i = v->AsInt()) return *i;
    return def;
}

/// 设置 gamedataManager 中某个游戏整数字段并保存。
inline void setGameDataInt(const std::string& fileName, const std::string& field,
                            int val)
{
    if (!gamedataManager) return;
    std::string k = gamedataKeyPrefix(fileName) + "." + field;
    gamedataManager->Set(k, beiklive::ConfigValue(val));
    gamedataManager->Save();
}

/// 读取 gamedataManager 中某个游戏字段的浮点值。
inline float getGameDataFloat(const std::string& fileName, const std::string& field,
                               float def = 0.0f)
{
    if (!gamedataManager) return def;
    std::string k = gamedataKeyPrefix(fileName) + "." + field;
    auto v = gamedataManager->Get(k);
    if (!v) return def;
    if (auto f = v->AsFloat()) return *f;
    if (auto i = v->AsInt())   return static_cast<float>(*i);
    return def;
}

/// 设置 gamedataManager 中某个游戏浮点字段并保存。
inline void setGameDataFloat(const std::string& fileName, const std::string& field,
                              float val)
{
    if (!gamedataManager) return;
    std::string k = gamedataKeyPrefix(fileName) + "." + field;
    gamedataManager->Set(k, beiklive::ConfigValue(val));
    gamedataManager->Save();
}

// ─── 近期游戏队列辅助 ─────────────────────────────────────────────────────────
#define RECENT_GAME_COUNT 10
#define RECENT_GAME_KEY_PREFIX "recent.game."

/// 近期游戏列表变更时置 true，由 StartPageView 每帧检查。
extern bool g_recentGamesDirty;

/// 从近期游戏队列中移除 gameFileName 并保存到 SettingManager。
/// @param gameFileName  文件名（含后缀），与 pushRecentGame 传入的值保持一致。
inline void removeRecentGame(const std::string& gameFileName)
{
    if (!SettingManager || gameFileName.empty()) return;
    std::vector<std::string> queue;
    for (int i = 0; i < RECENT_GAME_COUNT; ++i) {
        std::string key = std::string(RECENT_GAME_KEY_PREFIX) + std::to_string(i);
        auto v = SettingManager->Get(key);
        if (v && v->AsString() && !v->AsString()->empty())
            queue.push_back(*v->AsString());
    }
    queue.erase(std::remove(queue.begin(), queue.end(), gameFileName), queue.end());
    for (int i = 0; i < RECENT_GAME_COUNT; ++i) {
        std::string key = std::string(RECENT_GAME_KEY_PREFIX) + std::to_string(i);
        std::string val = (i < static_cast<int>(queue.size())) ? queue[i] : "";
        SettingManager->Set(key, beiklive::ConfigValue(val));
    }
    SettingManager->Save();
}

/// 将 gameName 推入近期游戏队列（队首为最新）并保存到 SettingManager。
inline void pushRecentGame(const std::string& gameName)
{
    if (!SettingManager || gameName.empty()) return;
    // 读取现有队列
    std::vector<std::string> queue;
    for (int i = 0; i < RECENT_GAME_COUNT; ++i) {
        std::string key = std::string(RECENT_GAME_KEY_PREFIX) + std::to_string(i);
        auto v = SettingManager->Get(key);
        if (v && v->AsString()) queue.push_back(*v->AsString());
        else queue.push_back("");
    }
    // 去重
    queue.erase(std::remove(queue.begin(), queue.end(), gameName), queue.end());
    // 插入队首
    queue.insert(queue.begin(), gameName);
    // 截断到 RECENT_GAME_COUNT
    if (static_cast<int>(queue.size()) > RECENT_GAME_COUNT)
        queue.resize(RECENT_GAME_COUNT);
    // 回写
    for (int i = 0; i < RECENT_GAME_COUNT; ++i) {
        std::string key = std::string(RECENT_GAME_KEY_PREFIX) + std::to_string(i);
        std::string val = (i < static_cast<int>(queue.size())) ? queue[i] : "";
        SettingManager->Set(key, beiklive::ConfigValue(val));
    }
    SettingManager->Save();
    g_recentGamesDirty = true;
}

// ─── 游戏列表（PlaylistManager）辅助函数 ──────────────────────────────────────

/// 若 gameFileName 尚未在游戏列表中，则将其追加并将 listcount 加一。
/// 列表存储在 PlaylistManager 中：key "listcount" 记录总数，
/// "game0"/"game1"/... 记录对应游戏文件名。
inline void pushPlaylistGame(const std::string& gameFileName)
{
    if (!PlaylistManager || gameFileName.empty()) return;

    // 读取当前列表数量
    int count = 0;
    auto v = PlaylistManager->Get("listcount");
    if (v && v->AsInt()) count = *v->AsInt();

    // 检查游戏是否已在列表中
    for (int i = 0; i < count; ++i) {
        std::string key = "game" + std::to_string(i);
        auto gv = PlaylistManager->Get(key);
        if (gv && gv->AsString() && *gv->AsString() == gameFileName)
            return; // 已存在，不重复添加
    }

    // 追加新游戏并更新计数
    PlaylistManager->Set("game" + std::to_string(count), beiklive::ConfigValue(gameFileName));
    PlaylistManager->Set("listcount", beiklive::ConfigValue(count + 1));
    PlaylistManager->Save();
}