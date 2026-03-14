#include <cstdlib>
#include <iostream>
#include <string>

#include "common.hpp"
#include "Audio/BKAudioPlayer.hpp"
#include "Game/game_view.hpp"

#include "UI/StartPageView.hpp"


#if defined(BOREALIS_USE_OPENGL)
// Needed for the OpenGL driver to work
extern "C" unsigned int sceLibcHeapSize = 2 * 1024 * 1024;
#endif

#define AUTOSAVE_GRANULARITY 600
#define FPS_GRANULARITY 30
#define FPS_BUFFER_SIZE 4
#define REWIND_BUFFER_DEFAULT 300 // 默认倒带缓冲区
#define REWIND_SAVE_INTERVAL_DEFAULT 1 // 默认每帧保存

beiklive::GameRunner* gameRunner = nullptr;
beiklive::ConfigManager* SettingManager = nullptr;
beiklive::ConfigManager* NameMappingManager = nullptr;
beiklive::ConfigManager* gamedataManager = nullptr;




void RunnerInit() {
	gameRunner = new beiklive::GameRunner();
	gameRunner->settingConfig = SettingManager;
	gameRunner->nameMappingConfig = NameMappingManager;
	gameRunner->uiParams = new beiklive::UIParams();
	gameRunner->port = "switch";
	gameRunner->fps = 0;
	gameRunner->lastFpsCheck = 0;
	gameRunner->totalDelta = 0;
	gameRunner->rewinding = false;
	gameRunner->rewindEnabled = false;
	gameRunner->rewindMuteEnabled = false;
	gameRunner->rewindBufferSize = REWIND_BUFFER_DEFAULT;
	gameRunner->rewindSaveInterval = REWIND_SAVE_INTERVAL_DEFAULT;
	gameRunner->rewindFrames = 0;
	gameRunner->rewindPaused = false;
	gameRunner->rewindShowStatus = 0;


    gameRunner->settingConfig->SetDefault(KEY_UI_START_PAGE, 0); 

    // UI / background settings
    gameRunner->settingConfig->SetDefault(KEY_UI_SHOW_BG_IMAGE,  beiklive::ConfigValue(std::string("false")));
    gameRunner->settingConfig->SetDefault(KEY_UI_BG_IMAGE_PATH,  beiklive::ConfigValue(std::string("")));
    gameRunner->settingConfig->SetDefault(KEY_UI_SHOW_XMB_BG,    beiklive::ConfigValue(std::string("false")));
    gameRunner->settingConfig->SetDefault(KEY_UI_PSPXMB_COLOR,   beiklive::ConfigValue(std::string("blue")));
    // gameRunner->settingConfig->SetDefault(KEY_UI_TEXT_COLOR,      beiklive::ConfigValue(std::string("white")));

    // Audio settings
    gameRunner->settingConfig->SetDefault(KEY_AUDIO_BUTTON_SFX,  beiklive::ConfigValue(std::string("false")));

    // Debug settings
    gameRunner->settingConfig->SetDefault(KEY_DEBUG_LOG_LEVEL,   beiklive::ConfigValue(std::string("info")));
    gameRunner->settingConfig->SetDefault(KEY_DEBUG_LOG_FILE,    beiklive::ConfigValue(std::string("false")));
    gameRunner->settingConfig->SetDefault(KEY_DEBUG_LOG_OVERLAY, beiklive::ConfigValue(std::string("false")));

    // Recent games queue (0 = most recent)
    for (int i = 0; i < RECENT_GAME_COUNT; ++i) {
        std::string key = std::string(RECENT_GAME_KEY_PREFIX) + std::to_string(i);
        gameRunner->settingConfig->SetDefault(key, beiklive::ConfigValue(std::string("")));
    }


	// 保存默认值到配置文件
	gameRunner->settingConfig->Save();
}

void ConfigManagerInit() {
	// 初始化，目录系统

	SettingManager = new beiklive::ConfigManager((std::string(BK_APP_CONFIG_DIR) + std::string("setting.cfg")));
	NameMappingManager =
	    new beiklive::ConfigManager((std::string(BK_APP_CONFIG_DIR) + std::string("name_mapping.cfg")));
	gamedataManager =
	    new beiklive::ConfigManager((std::string(BK_APP_CONFIG_DIR) + std::string("gamedata.cfg")));
	if (!SettingManager->Load()) {
		brls::Logger::warning("Failed to load setting.cfg, using default configuration.");
	} else {
		brls::Logger::info("Configuration loaded successfully from setting.cfg.");
		if (!SettingManager->Contains("platform")) {
#ifdef SWITCH
			SettingManager->Set("platform", "switch");
#elif defined(WIN32)
			SettingManager->Set("platform", "windows");
#elif defined(__linux__)
			SettingManager->Set("platform", "linux");
#elif defined(__APPLE__)
			SettingManager->Set("platform", "macos");
#else
			SettingManager->Set("platform", "unknown");
#endif
			brls::Logger::info("Platform information saved to setting.cfg.");
		}
	}

	if (!NameMappingManager->Load()) {
		brls::Logger::warning("Failed to load name_mapping.cfg, using default name mapping.");
	} else {
		brls::Logger::info("Name mapping loaded successfully from name_mapping.cfg.");
	}
	if (!gamedataManager->Load()) {
		brls::Logger::warning("Failed to load gamedata.cfg, using empty game data.");
	} else {
		brls::Logger::info("Game data loaded successfully from gamedata.cfg.");
	}
	// 打印当前平台信息
	std::string platform = "unknown";
	if (auto v = SettingManager->Get("platform"); v && v->AsString()) {
		platform = *v->AsString();
	}
	SettingManager->Save();
	brls::Logger::info("Current platform: {}", platform);
}

int main(int argc, char* argv[]) {
    std::filesystem::create_directories(BK_APP_ROOT_DIR);
    std::filesystem::create_directories(BK_APP_LOG_DIR);
    std::filesystem::create_directories(BK_APP_CONFIG_DIR);
    std::filesystem::create_directories(BK_GAME_CORE_DIR);
    std::filesystem::create_directories(BK_APP_ROOT_DIR + std::string("saves"));
    std::filesystem::create_directories(BK_APP_ROOT_DIR + std::string("cheats"));
	
	ConfigManagerInit();
	// We recommend to use INFO for real apps
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "-d") == 0) { // Set log level
			brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
		} else if (std::strcmp(argv[i], "-o") == 0) {
			const char* path = (i + 1 < argc) ? argv[++i] : BK_APP_LOG_FILE;
			brls::Logger::setLogOutput(std::fopen(path, "w+"));
		} else if (std::strcmp(argv[i], "-v") == 0) {
			brls::Application::enableDebuggingView(true);
		}
	}

	brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_AUTO;
	// Init the app and i18n
	if (!brls::Application::init()) {
		brls::Logger::error("Unable to init Borealis application");
		return EXIT_FAILURE;
	}
	brls::Application::createWindow("beiklive/title"_i18n);

	// On desktop platforms, inject BeikLive's own audio player so that
	// borealis UI sound effects (button clicks, navigation, etc.) are played
	// via native OS audio rather than the built-in NullAudioPlayer stub.
#ifndef __SWITCH__
	static beiklive::BKAudioPlayer g_bkAudioPlayer;
	brls::Application::setAudioPlayer(&g_bkAudioPlayer);
#endif

	RunnerInit();

	brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
	beiklive::RegisterStyles();
	beiklive::RegisterThemes();



	// brls::Application::setGlobalQuit(true);


	auto* t_StartPageView = new StartPageView();
	gameRunner->uiParams->StartPageframe = new brls::AppletFrame(t_StartPageView);
	t_StartPageView->Init();



	gameRunner->uiParams->StartPageframe->setTitle("beiklive/title"_i18n);
    gameRunner->uiParams->StartPageframe->setHeaderVisibility(brls::Visibility::GONE);
    gameRunner->uiParams->StartPageframe->setFooterVisibility(brls::Visibility::GONE);
	brls::Application::pushActivity(new brls::Activity(gameRunner->uiParams->StartPageframe));

	// Run the app
	while (brls::Application::mainLoop())
		;

	// Cleanup

	SettingManager->Save();
	brls::Logger::info("SettingManager->Save");
	// Exit
	return EXIT_SUCCESS;
}

#ifdef __WINRT__

#include <borealis/core/main.hpp>
#endif
