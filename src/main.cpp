#ifdef __SWITCH__
#include <switch.h>
#endif


#include "core/common.h"
#include "core/AppUpdater.hpp"
#include "ui/audio/BKAudioPlayer.hpp"
#include "ui/page/StartPage.hpp"
#include "ui/page/UpdatePage.hpp"
#include "ui/utils/MyActivity.hpp"
#include "ui/utils/UpdateDialog.hpp"

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

	brls::Application::loadFontFromFile("chinese", BK_RES("font/switch_font.ttf"));

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

		// 从 config/version.json 读取本地版本号，不存在则用 APP_VERSION
		std::string localVersion = APP_VERSION;
		{
			std::ifstream f(beiklive::path::configPath() + "/version.json");
			if (f.is_open()) {
				nlohmann::json j;
				f >> j;
				std::string ver = j.value("version", "");
				brls::Logger::info("本地版本号: {}", ver);
				if (!ver.empty())
					localVersion = ver;
			}
		}

		brls::Logger::info("当前版本: {}", localVersion);

		auto& updater = beiklive::AppUpdater::instance();
		if (gExitFlag.load(std::memory_order_acquire)) return;

		updater.checkSync(localVersion);
		if (gExitFlag.load(std::memory_order_acquire)) return;

		auto start = std::chrono::steady_clock::now();
		while (!updater.hasUpdate() &&
			   std::chrono::duration_cast<std::chrono::seconds>(
				   std::chrono::steady_clock::now() - start).count() < 15) {
			if (gExitFlag.load(std::memory_order_acquire)) return;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		if (gExitFlag.load(std::memory_order_acquire)) return;

		brls::sync([&updater, localVersion]() {
			auto& info = updater.info();

			if (info.hasUpdate) {
				auto* dlg = new beiklive::UpdateDialog(
					"版本更新  " + info.version,
					info.changelog
				);
				dlg->addButton("更新", [&updater]() {
					auto* page = new beiklive::UpdatePage();
					auto* frame = new brls::AppletFrame(page);
					HIDE_BRLS_BAR(frame);
					brls::Application::pushActivity(
						new brls::Activity(frame), brls::TransitionAnimation::NONE);
					page->startDownload();
				});
				dlg->addButton("取消", []() {});
				dlg->addButton("不再提示", []() {
					SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_EMU_UPDATE, 0);
					brls::Application::notify("已关闭更新提示");
				});
				dlg->open();
			}
		});
	});

	// Run the app
	while (brls::Application::mainLoop())
		;

	// 通知线程退出并等待其完成
	gExitFlag.store(true, std::memory_order_release);
	if (updateThread.joinable())
		updateThread.join();

	// Cleanup
	// Exit
	return EXIT_SUCCESS;
}





#ifdef __WINRT__

#include <borealis/core/main.hpp>
#endif
