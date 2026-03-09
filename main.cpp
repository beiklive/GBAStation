#include <cstdlib>
#include <iostream>
#include <string>

#include "common.hpp"
#include "UI/Img_text_cell.hpp"
#include "UI/List_view.hpp"
#include "UI/game_view.hpp"
#include "UI/tab_select_game.hpp"

#include "XMLUI/StartPageView.hpp"
#include "XMLUI/Utils/captioned_image.hpp"


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




void RunnerInit() {
	gameRunner = new beiklive::GameRunner();
	gameRunner->settingConfig = SettingManager;
	gameRunner->nameMappingConfig = NameMappingManager;
	gameRunner->uiParams = new beiklive::UIParams();
	gameRunner->core = nullptr;
	gameRunner->port = "switch";
	gameRunner->fps = 0;
	gameRunner->lastFpsCheck = 0;
	gameRunner->totalDelta = 0;
	CircleBufferInit(&gameRunner->fpsBuffer, FPS_BUFFER_SIZE * sizeof(uint32_t));
	mCoreConfigInit(&gameRunner->config, gameRunner->port);
	gameRunner->rewinding = false;
	gameRunner->rewindEnabled = false;
	gameRunner->rewindMuteEnabled = false;
	gameRunner->rewindBufferSize = REWIND_BUFFER_DEFAULT;
	gameRunner->rewindSaveInterval = REWIND_SAVE_INTERVAL_DEFAULT;
	gameRunner->rewindFrames = 0;
	gameRunner->rewindPaused = false;
	gameRunner->rewindShowStatus = 0;

	mCoreConfigSetDefaultIntValue(&gameRunner->config, "volume", 0x100);
	mCoreConfigSetDefaultValue(&gameRunner->config, "idleOptimization", "detect");
	mCoreConfigSetDefaultIntValue(&gameRunner->config, "autoload", true);
#ifdef DISABLE_THREADING
	mCoreConfigSetDefaultIntValue(&gameRunner->config, "autosave", false);
#else
	mCoreConfigSetDefaultIntValue(&gameRunner->config, "autosave", true);
#endif
	mCoreConfigSetDefaultIntValue(&gameRunner->config, "showOSD", true);
	BK_CoreConfigLoad(&gameRunner->config);
	BK_CoreConfigSave(&gameRunner->config);
	// 新的个性化设置 也添加在这里初始化默认值

    gameRunner->settingConfig->SetDefault(KEY_UI_START_PAGE, 0); 





	// 保存默认值到配置文件
	gameRunner->settingConfig->Save();
}

void ConfigManagerInit() {
	// 初始化，目录系统

	SettingManager = new beiklive::ConfigManager((std::string(BK_APP_CONFIG_DIR) + std::string("setting.cfg")));
	NameMappingManager =
	    new beiklive::ConfigManager((std::string(BK_APP_CONFIG_DIR) + std::string("name_mapping.cfg")));
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

	RunnerInit();
	// brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

	// brls::Application::setGlobalQuit(true);

	brls::Application::registerXMLView("HomeMenuListView", HomeMenuListView::create);
	brls::Application::registerXMLView("Img_text_cell", Img_text_cell::create);

	brls::Application::registerXMLView("CaptionedImage", CaptionedImage::create);
	brls::Application::registerXMLView("StartPageView", StartPageView::create);

	auto* t_StartPageView = new StartPageView();
	gameRunner->uiParams->StartPageframe = new brls::AppletFrame(t_StartPageView);
	t_StartPageView->Init();



	gameRunner->uiParams->StartPageframe->setTitle("beiklive/title"_i18n);
    gameRunner->uiParams->StartPageframe->setHeaderVisibility(brls::Visibility::GONE);
    gameRunner->uiParams->StartPageframe->setFooterVisibility(brls::Visibility::GONE);
	// gameRunner->uiParams->StartPageframe->getFooter()->setBackgroundColor(nvgRGBA(89, 164, 249, 100));
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
