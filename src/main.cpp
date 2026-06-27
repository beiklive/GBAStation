#ifdef __SWITCH__
#include <switch.h>
#endif


#include "core/common.h"
#include "core/AppUpdater.hpp"
#include "ui/utils/BKAudioPlayer.hpp"
#include "ui/page/StartPage.hpp"
#include "ui/utils/MyActivity.hpp"
#include "network/WebService.h"

int main(int argc, char* argv[]) {
#ifdef __SWITCH__
    appletInitializeGamePlayRecording();
#endif


	beiklive::ConfigureInit();

	// ── 从配置文件读取调试设置 ──────────────────────────────────
	{
		std::string logLevel = beiklive::getKeyStr(beiklive::SettingManager, "debug.logLevel", "info");
		if (logLevel == "debug")
			brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
		else if (logLevel == "warning")
			brls::Logger::setLogLevel(brls::LogLevel::LOG_WARNING);
		else if (logLevel == "error")
			brls::Logger::setLogLevel(brls::LogLevel::LOG_ERROR);
		else
			brls::Logger::setLogLevel(brls::LogLevel::LOG_INFO);

		if (beiklive::getKeyInt(beiklive::SettingManager, "debug.logFile", 0)) {
			std::string logPath = beiklive::path::logFilePath();
			FILE* fp = std::fopen(logPath.c_str(), "w+");
			if (fp)
				brls::Logger::setLogOutput(fp);
		}
	}

	// CLI 参数可覆盖配置文件中的调试设置
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "-d") == 0) {
			brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
		} else if (std::strcmp(argv[i], "-o") == 0) {
			const char* path = (i + 1 < argc) ? argv[++i] : beiklive::path::logFilePath().c_str();
			brls::Logger::setLogOutput(std::fopen(path, "w+"));
		} else if (std::strcmp(argv[i], "-v") == 0) {
			brls::Application::enableDebuggingView(true);
		}
	}

	brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_AUTO;
	if (!brls::Application::init()) {
		brls::Logger::error("Unable to init Borealis application");
		return EXIT_FAILURE;
	}
	brls::Application::createWindow("beiklive/title"_i18n);

	// brls::Application::loadFontFromFile("chinese", BK_RES("font/switch_font.ttf"));

	// ── 应用初始化后读取调试覆盖层设置 ──────────────────────────
	if (beiklive::getKeyInt(beiklive::SettingManager, "debug.logOverlay", 0))
		brls::Application::enableDebuggingView(true);

	brls::Application::setAudioPlayer(new beiklive::BKAudioPlayer());

	brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
	beiklive::RegisterStyles();
	beiklive::RegisterThemes();
	
	auto* mStartPage = new beiklive::StartPage();
	auto* frame = new brls::AppletFrame(mStartPage);
	HIDE_BRLS_BAR(frame);
	beiklive::MyActivity* activity = new beiklive::MyActivity(frame);
	activity->setPageView(mStartPage);
	brls::Application::pushActivity(activity);

	// ── 更新检查线程 ──────────────────────────────────────────
	// 标志位：程序退出时设置为 true，通知线程提前终止
	std::atomic<bool> gExitFlag{false};

	// 订阅退出事件，在 mainLoop 返回前尽早设置退出标志
	brls::Application::getExitEvent()->subscribe(
		[&gExitFlag]() { gExitFlag.store(true, std::memory_order_release); });

	std::thread updateThread([&gExitFlag]() {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		if (gExitFlag.load(std::memory_order_acquire)) return;

		brls::Logger::debug("开始更新检查线程");

		int updateEnabled = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_EMU_UPDATE, 1);
		if (!updateEnabled) return;

		brls::Logger::info("当前版本: {}", APP_VERSION);

		auto& updater = beiklive::AppUpdater::instance();
		if (gExitFlag.load(std::memory_order_acquire)) return;

		updater.checkSync();

		if (gExitFlag.load(std::memory_order_acquire)) return;

		brls::sync([&updater]() {
			auto& info = updater.info();
			if (info.hasUpdate) {
				brls::Application::notify("新版本 " + info.version + " 可用，请到关于界面更新");
			}
		});
	});

	// Run the app
	while (brls::Application::mainLoop())
		beiklive::network::WebService::Update();

	// 通知线程退出并等待其完成
	gExitFlag.store(true, std::memory_order_release);
	if (updateThread.joinable())
		updateThread.join();

	beiklive::network::WebService::Stop();

	// Cleanup
	// Exit
	return EXIT_SUCCESS;
}





#ifdef __WINRT__

#include <borealis/core/main.hpp>
#endif
