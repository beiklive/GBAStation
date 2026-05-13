#ifdef __SWITCH__
#include <switch.h>
#endif


#include "core/common.h"
#include "core/AppUpdater.hpp"
#include "ui/audio/BKAudioPlayer.hpp"
#include "ui/page/StartPage.hpp"
#include "ui/page/UpdatePage.hpp"
#include "ui/utils/MyActivity.hpp"
#if defined(BOREALIS_USE_OPENGL)
// Needed for the OpenGL driver to work
extern "C" unsigned int sceLibcHeapSize = 2 * 1024 * 1024;
#endif

int main(int argc, char* argv[]) {
#ifdef __SWITCH__
    appletInitializeGamePlayRecording();
#endif


	beiklive::ConfigureInit();

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

#ifndef __SWITCH__
    {
        std::string logPath = beiklive::path::logFilePath();
        FILE* fp = std::fopen(logPath.c_str(), "w+");

        if (fp)
            brls::Logger::setLogOutput(fp);
        // brls::Application::enableDebuggingView(true);
    }
#endif

	brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_AUTO;
	// Init the app and i18n
	if (!brls::Application::init()) {
		brls::Logger::error("Unable to init Borealis application");
		return EXIT_FAILURE;
	}
	brls::Application::createWindow("beiklive/title"_i18n);

	// 所有平台统一使用BKAudioPlayer播放WAV音效文件
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

#ifdef __SWITCH__
	// ── 启动时异步检查更新 ──
	brls::async([]() {
		// 延迟 2 秒，避免阻塞启动画面
		std::this_thread::sleep_for(std::chrono::seconds(2));

		int updateEnabled = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_EMU_UPDATE, 1);
		if (!updateEnabled) return;

		auto& updater = beiklive::AppUpdater::instance();
		updater.check(APP_VERSION);

		// 等待检查完成（最多 10 秒）
		auto start = std::chrono::steady_clock::now();
		while (!updater.hasUpdate()) {
			if (std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::steady_clock::now() - start).count() > 10)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		// 在主线程显示更新对话框
		brls::sync([&updater]() {
			auto& info = updater.info();
			std::string msg = "发现新版本\n\n" + info.version + "\n\n" + info.changelog;

			auto* dlg = new brls::Dialog(msg);
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
		});
	});
#endif

	// Run the app
	while (brls::Application::mainLoop())
		;

	// Cleanup
	// Exit
	return EXIT_SUCCESS;
}





#ifdef __WINRT__

#include <borealis/core/main.hpp>
#endif
