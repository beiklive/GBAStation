#pragma once
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

// FUNCTIONS DEFINITIONS

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
#define BK_APP_DEFAULT_LOGO BK_RES("img/ui/gba-icon-256.png")
#define BK_APP_DEFAULT_BG BK_RES("img/bg2.png")



#define KEY_UI_START_PAGE "UI.startPage" // 开始页面
#define KEY_UI_LANGUAGE "UI.language" // 语言
#define KEY_UI_THEME "UI.theme" // 主题

// UI / background settings
#define KEY_UI_SHOW_BG_IMAGE  "UI.showBgImage"
#define KEY_UI_BG_IMAGE_PATH  "UI.bgImagePath"
#define KEY_UI_SHOW_XMB_BG    "UI.showXmbBg"
#define KEY_UI_PSPXMB_COLOR   "UI.pspxmb.color"
// #define KEY_UI_TEXT_COLOR     "UI.textColor"

// Audio settings
#define KEY_AUDIO_BUTTON_SFX  "audio.buttonSfx"

// Debug settings
#define KEY_DEBUG_LOG_LEVEL   "debug.logLevel"
#define KEY_DEBUG_LOG_FILE    "debug.logFile"
#define KEY_DEBUG_LOG_OVERLAY "debug.logOverlay"

using bklog = brls::Logger;

namespace beiklive {

void RegisterStyles();
void RegisterThemes();
void CheckGLSupport();
void InsertBackground(brls::Box* view);
/// Apply the XMB colour preset (UI.pspxmb.color) from SettingManager to @a img.
void ApplyXmbColor(beiklive::UI::ProImage* img);
/// Apply ALL background settings (visibility, image, XMB shader, color) to @a img
/// based on the current SettingManager values (UI.showXmbBg, UI.showBgImage, etc.).
void ApplyXmbBg(beiklive::UI::ProImage* img);
/// Apply ALL background settings to ALL registered background ProImage instances.
void ApplyXmbColorToAll();
/// Register/unregister a background ProImage instance so ApplyXmbColorToAll() can reach it.
void RegisterXmbBackground(beiklive::UI::ProImage* img);
void UnregisterXmbBackground(beiklive::UI::ProImage* img);
// 吞噬一个按钮事件，使其不再被后续处理
void swallow(brls::View* v, brls::ControllerButton btn);




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

	bool rewinding; // 当前是否正在倒带
	bool rewindEnabled; // 倒带功能是否启用
	bool rewindMuteEnabled; // 倒带静音功能是否启用
	int rewindBufferSize; // 倒带缓冲区大小（帧数）
	int rewindSaveInterval; // 保存间隔（每N帧保存一次）
	unsigned rewindFrames; // 已保存的帧数

	// 用于倒带状态显示
	bool rewindPaused; // 倒带时是否暂停
	int rewindShowStatus; // 倒带状态显示

	const char* port;
	float fps;
	int64_t lastFpsCheck;
	int32_t totalDelta;

	struct UIParams *uiParams; // UI 相关参数


};

};

// 全局配置管理器实例
extern beiklive::ConfigManager* SettingManager;
extern beiklive::ConfigManager* NameMappingManager;
/// Logo 路径映射：key = 文件名(不含后缀), value = logo 图片路径
extern beiklive::ConfigManager* logoManager;
extern beiklive::GameRunner* gameRunner;