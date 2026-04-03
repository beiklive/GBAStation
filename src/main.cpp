#ifdef __SWITCH__
#include <switch.h>
#endif


#include "core/common.h"
#include "ui/audio/BKAudioPlayer.hpp"
#include "ui/page/StartPage.hpp"
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
	brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
	brls::Application::enableDebuggingView(true);

	brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_AUTO;
	// Init the app and i18n
	if (!brls::Application::init()) {
		brls::Logger::error("Unable to init Borealis application");
		return EXIT_FAILURE;
	}
	brls::Application::createWindow("beiklive/title"_i18n);

// #ifndef __SWITCH__
	// Switch平台由borealis内置的SwitchAudioPlayer处理，其他平台使用BKAudioPlayer
	brls::Application::setAudioPlayer(new beiklive::BKAudioPlayer());
// #endif

	brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::LIGHT);
	beiklive::RegisterStyles();
	beiklive::RegisterThemes();
	
	auto* mStartPage = new beiklive::StartPage();
	auto* frame = new brls::AppletFrame(mStartPage);
	HIDE_BRLS_BAR(frame);
	beiklive::MyActivity* activity = new beiklive::MyActivity(frame);
	activity->setPageView(mStartPage);
	brls::Application::pushActivity(activity);

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
