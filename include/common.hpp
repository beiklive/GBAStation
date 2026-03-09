#pragma once
extern "C" {
#include <mgba-util/circle-buffer.h>
#include <mgba-util/gui.h>
#include <mgba-util/gui/font.h>
#include <mgba-util/gui/menu.h>
#include <mgba-util/vfs.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/rewind.h>
#include <mgba/gba/interface.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gb/video.h>
#include <mgba/internal/gba/audio.h>
#include <mgba/internal/gba/input.h>
}
#include <borealis.hpp>
using namespace brls::literals; // for _i18n

#include "Utils/ConfigManager.hpp"
#include "Utils/fileUtils.hpp"
#include "Utils/strUtils.hpp"


// FUNCTIONS DEFINITIONS

#define BK_CoreConfigLoad(config) mCoreConfigLoadPath(config, BK_APP_CONFIG_FILE)
#define BK_CoreConfigSave(config) mCoreConfigSavePath(config, BK_APP_CONFIG_FILE)
#define BK_RES(path) beiklive::file::TransStrToRes(path)

#define BK_APP_NAME "mGBA-NX"
#ifdef __SWITCH__
#define BK_APP_ROOT_DIR "/mGBA-NX/"
#define BK_APP_LOG_DIR "/mGBA-NX/log/"
#define BK_APP_LOG_FILE "/mGBA-NX/log/mGBA-NX.log"
#define BK_APP_CONFIG_DIR "/mGBA-NX/config/"
#define BK_APP_CONFIG_FILE "/mGBA-NX/config/config.ini"
#else
#define BK_APP_ROOT_DIR "./mGBA-NX/"
#define BK_APP_LOG_DIR "./mGBA-NX/log/"
#define BK_APP_LOG_FILE "./mGBA-NX/log/mGBA-NX.log"
#define BK_APP_CONFIG_DIR "./mGBA-NX/config/"
#define BK_APP_CONFIG_FILE "./mGBA-NX/config/config.ini"
#endif

// 应用默认图标
#define BK_APP_DEFAULT_LOGO BK_RES("img/ui/gba-icon-256.png")
#define BK_APP_DEFAULT_BG BK_RES("img/bg2.png")



#define KEY_UI_START_PAGE "UI.startPage" // 开始页面
#define KEY_UI_LANGUAGE "UI.language" // 语言
#define KEY_UI_THEME "UI.theme" // 主题

using bklog = brls::Logger;

namespace beiklive {


// ---- Swallow helper: register a no-op action that consumes a button ----
// This prevents Borealis from acting on navigation / face buttons while the
// game is running (e.g. BUTTON_B would otherwise trigger a back-navigation).
static void swallow(brls::View* v, brls::ControllerButton btn)
{
    v->registerAction("", btn, [](brls::View*) { return true; },
                      /*hidden=*/true);
}

struct GameEntry {
    std::string path;    ///< ROM 文件完整路径
    std::string title;   ///< 显示标题
    std::string cover;   ///< 封面图路径（可空，空则显示占位符）
};


struct Gamefile {
	// 游戏文件的路径
	std::string path;
	// 游戏文件名称
	std::string name;
	// 游戏文件显示名称
	std::string displayName;
	// 游戏文件类型（例如：GBA、GB）
	mPlatform type = mPLATFORM_NONE;
	// 游戏输出分辨率
	int width;
	int height;
};
struct mGUIRunnerLux {
	struct GBALuminanceSource d;
	int luxLevel;
};

struct UIParams {
	// 起始页面索引，0为APP页面，1为文件列表页面
	brls::AppletFrame *StartPageframe;



};


// 游戏运行时环境
struct GameRunner {
	struct mCore* core;
	struct mCoreConfig config;
	beiklive::ConfigManager* settingConfig;
	beiklive::ConfigManager* nameMappingConfig;
	Gamefile gameFile;
	std::string currentPath;

	struct mGUIRunnerLux luminanceSource; // 亮度信息来源

	struct mCoreRewindContext rewind; // 倒带上下文
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
	struct CircleBuffer fpsBuffer;

	struct UIParams *uiParams; // UI 相关参数


};

};

// 全局配置管理器实例
extern beiklive::ConfigManager* SettingManager;
extern beiklive::ConfigManager* NameMappingManager;
extern beiklive::GameRunner* gameRunner;