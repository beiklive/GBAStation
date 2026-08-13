#ifdef __SWITCH__
#include <switch.h>
#include "platform/switch/NroLauncher.hpp"
#endif


#include "core/common.h"
#include "core/Translation.hpp"
#include "core/AppUpdater.hpp"
#include "core/ThreadPool.hpp"
#include "core/ThreeDsTitlePaths.hpp"
#include "core/Tools.hpp"
#include "core/rom/PspMeta.hpp"
#include "core/ExternalCoreSession.hpp"
#include "ui/utils/BKAudioPlayer.hpp"
#include "ui/page/StartPage.hpp"
#include "ui/widget/VideoBackgroundView.hpp"
#include "ui/utils/MyActivity.hpp"
#include "network/WebService.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <optional>
#include <utility>

namespace {

bool endsWithNoCase(const std::string& value, const char* suffix)
{
	const size_t suffixLen = std::strlen(suffix);
	if (value.size() < suffixLen)
		return false;

	const size_t offset = value.size() - suffixLen;
	for (size_t i = 0; i < suffixLen; ++i)
	{
		const char a = value[offset + i];
		const char b = suffix[i];
		if (std::tolower(static_cast<unsigned char>(a)) !=
			std::tolower(static_cast<unsigned char>(b)))
			return false;
	}
	return true;
}

bool isDirectLaunchRomType(beiklive::enums::FileType type)
{
	using beiklive::enums::FileType;
	return type == FileType::GBA_ROM ||
		   type == FileType::GBC_ROM ||
		   type == FileType::GB_ROM ||
		   type == FileType::NES_ROM ||
		   type == FileType::SNES_ROM ||
		   type == FileType::GENESIS_ROM;
}

bool isLibraryRomType(beiklive::enums::FileType type)
{
	return isDirectLaunchRomType(type) ||
		   type == beiklive::enums::FileType::THREEDS_ROM ||
		   type == beiklive::enums::FileType::ARCADE_ROM ||
		   type == beiklive::enums::FileType::DREAMCAST_ROM ||
		   type == beiklive::enums::FileType::PSP_ROM ||
		   type == beiklive::enums::FileType::PS1_ROM ||
		   type == beiklive::enums::FileType::SATURN_ROM ||
		   type == beiklive::enums::FileType::DOLPHIN_ROM;
}

std::optional<std::string> parseDirectLaunchRom(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (!argv[i] || !argv[i][0])
			continue;

		const std::string arg = argv[i];
		if (arg == "-d" || arg == "-v")
			continue;
		if (arg == "-o" || arg == "--return")
		{
			if (i + 1 < argc)
				++i;
			continue;
		}
		if (arg == "--external-return")
		{
			if (i + 1 < argc)
				++i;
			continue;
		}
		if (arg == "--rom" || arg == "--game")
		{
			if (i + 1 < argc && argv[i + 1] && argv[i + 1][0])
				return std::string(argv[i + 1]);
			continue;
		}
		if (!arg.empty() && arg[0] == '-')
			continue;
		if (endsWithNoCase(arg, ".nro"))
			continue;

		return arg;
	}

	return std::nullopt;
}

void ensureDirectGameDbEntry(const std::string& romPath, beiklive::enums::FileType fileType)
{
	if (!beiklive::GameDB || romPath.empty() || !isLibraryRomType(fileType))
		return;

	auto entryOpt = beiklive::GameDB->findByPath(romPath);
	beiklive::GameEntry entry = entryOpt.value_or(beiklive::GameEntry{});
	bool changed = !entryOpt.has_value();

	const int platform = static_cast<int>(fileType);
	const std::filesystem::path path(romPath);
	const std::string stem = path.stem().string().empty() ? "game" : path.stem().string();

	if (entry.path.empty())
	{
		entry.path = romPath;
		changed = true;
	}
	if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::NONE))
	{
		entry.platform = platform;
		changed = true;
	}
	if (entry.core.empty())
	{
		entry.core = beiklive::GetDefaultCoreId(platform);
		changed = true;
	}
	entry.core = beiklive::NormalizeCoreId(entry.platform, entry.core);
	if (entry.title.empty())
	{
		entry.title = GET_MAPPING_KEY_STR(stem, stem);
		changed = true;
	}
	if (entry.savePath.empty())
	{
		entry.savePath = beiklive::tools::defaultGameSavePath(entry.platform, entry.path);
		changed = true;
	}
	if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP))
	{
		// PSP ROM：入库时提取真实游戏标题与 ICON0 封面（保存到该 ROM 的存档目录）。
		// TITLE 仅在仍是默认文件名时覆盖（映射名优先保留）；封面仅当仍是默认资源图时替换。
		const std::string realTitle = beiklive::psp_meta::ExtractTitle(entry.path);
		if (!realTitle.empty() && entry.title == stem)
		{
			entry.title = realTitle;
			changed = true;
		}
		if (entry.logoPath.empty() ||
			entry.logoPath == beiklive::tools::getDefaultLogoPath(
				static_cast<beiklive::enums::EmuPlatform>(entry.platform), entry.path))
		{
			const std::string icon = beiklive::psp_meta::ExtractIcon0(
				entry.path, entry.savePath);
			if (!icon.empty())
			{
				entry.logoPath = icon;
				changed = true;
			}
		}
	}

	if (entry.logoPath.empty())
	{
		entry.logoPath = beiklive::tools::getDefaultLogoPath(
			static_cast<beiklive::enums::EmuPlatform>(entry.platform),
			entry.path);
		changed = true;
	}
	if (beiklive::tools::tryUseNdsInternalIconCover(entry))
		changed = true;
	if (entry.screenShotPath.empty())
	{
		entry.screenShotPath = beiklive::path::screenshotPath();
		changed = true;
	}
	if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
	{
		const std::string titleId = beiklive::three_ds::resolveTitleId(
			entry.threeDsTitleId, entry.path);
		if (!titleId.empty() && titleId != entry.threeDsTitleId)
		{
			entry.threeDsTitleId = titleId;
			changed = true;
		}
	}
	std::error_code ec;
	std::filesystem::create_directories(entry.savePath, ec);
	beiklive::GameDB->upsertByPath(entry);
	if (changed)
		beiklive::GameDB->flush();
}

bool launchDirectGameActivity(const std::string& romPath)
{
	const auto fileType = beiklive::tools::getFileType(romPath);
	if (!isLibraryRomType(fileType))
	{
		brls::Logger::error("Direct launch path is not a supported ROM: {}", romPath);
		return false;
	}

	ensureDirectGameDbEntry(romPath, fileType);

#ifdef __SWITCH__
	const auto launchExternalCore = [&](const char* label, int platform,
	                                    const char* pathKey, const char* defaultPath,
	                                    const char* returnKey) {
		if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast))
		{
			std::string extension = std::filesystem::path(romPath).extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
			if (extension == ".zip" || extension == ".7z")
			{
				brls::Application::notify(L("DC外部核心不能直接运行压缩包，请先解压为CHD/GDI/CDI/CUE"));
				return false;
			}
		}

		const std::string nroPath = GET_SETTING_KEY_STR(pathKey, defaultPath);
		const std::string returnPath = GET_SETTING_KEY_STR(returnKey, "sdmc:/switch/GBAStation.nro");
		const std::string sessionToken = beiklive::makeExternalCoreSessionToken(romPath);

		beiklive::switch_platform::NroLaunchRequest request;
		request.nroPath = nroPath;
		request.romPath = romPath;
		request.returnNroPath = returnPath;
		request.extraArgs = {"--gbastation-session", sessionToken};
		auto result = beiklive::switch_platform::launchNroOnExit(request);
		if (!result.success)
		{
			brls::Logger::error("Direct {} NRO launch failed: {}", label, result.message);
			brls::Application::notify(std::string(label) + L("独立NRO启动失败：") + result.message);
			return false;
		}

		if (!beiklive::beginExternalCoreSession(romPath, platform, sessionToken))
			brls::Logger::error("Direct {} external session tracking could not start for {}", label, romPath);

		brls::Logger::info("Direct {} NRO launch configured: {}", label, result.message);
		brls::Application::quit();
		return true;
	};

	if (fileType == beiklive::enums::FileType::THREEDS_ROM)
	{
		const std::string nroPath = GET_SETTING_KEY_STR(
			"3ds.externalNro.path", "/GBAStation/core/GBAStation3DSStub.nro");
		const std::string returnPath = GET_SETTING_KEY_STR(
			"3ds.externalNro.returnPath", "sdmc:/switch/GBAStation.nro");
		auto result = beiklive::switch_platform::launchNroOnExit(
			{nroPath, romPath, returnPath});
		if (!result.success)
		{
			brls::Logger::error("Direct 3DS NRO launch failed: {}", result.message);
			brls::Application::notify(L("3DS独立NRO启动失败：") + result.message);
			return false;
		}
		brls::Logger::info("Direct 3DS NRO launch configured: {}", result.message);
		brls::Application::quit();
		return true;
	}
	if (fileType == beiklive::enums::FileType::ARCADE_ROM)
	{
		return launchExternalCore("Arcade",
			static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade),
			"arcade.externalNro.path", "/GBAStation/core/GBAStationFBNeoStub.nro",
			"arcade.externalNro.returnPath");
	}
	if (fileType == beiklive::enums::FileType::DREAMCAST_ROM)
	{
		return launchExternalCore("DC",
			static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast),
			"dc.externalNro.path", "/GBAStation/core/GBAStationFlycastStub.nro",
			"dc.externalNro.returnPath");
	}
	if (fileType == beiklive::enums::FileType::PSP_ROM)
	{
		return launchExternalCore("PSP",
			static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP),
			"psp.externalNro.path", "/GBAStation/core/GBAStationPPSSPPStub.nro",
			"psp.externalNro.returnPath");
	}
	if (fileType == beiklive::enums::FileType::PS1_ROM)
	{
		return launchExternalCore("PS1",
			static_cast<int>(beiklive::enums::EmuPlatform::EmuPS1),
			"ps1.externalNro.path", "/GBAStation/core/GBAStationDuckStationStub.nro",
			"ps1.externalNro.returnPath");
	}
	if (fileType == beiklive::enums::FileType::SATURN_ROM)
	{
		return launchExternalCore("Saturn",
			static_cast<int>(beiklive::enums::EmuPlatform::EmuSaturn),
			"saturn.externalNro.path", "/GBAStation/core/GBAStationYabaSanshiroStub.nro",
			"saturn.externalNro.returnPath");
	}
	if (fileType == beiklive::enums::FileType::DOLPHIN_ROM)
	{
		return launchExternalCore("GC / Wii",
			static_cast<int>(beiklive::enums::EmuPlatform::EmuDolphin),
			"dolphin.externalNro.path", "/GBAStation/core/GBAStationDolphinStub.nro",
			"dolphin.externalNro.returnPath");
	}
#endif

	if (!isDirectLaunchRomType(fileType))
		return false;

	beiklive::GamePage* gamePage = nullptr;
	if (beiklive::GameDB)
	{
		auto entry = beiklive::GameDB->findByPath(romPath);
		if (entry.has_value())
			gamePage = new beiklive::GamePage(*entry, true);
	}

	if (!gamePage)
	{
		std::filesystem::path path(romPath);
		beiklive::DirListData dirItem {
			path.filename().string(),
			romPath,
			beiklive::tools::getIconPath(fileType),
			fileType,
			beiklive::tools::getFileSizeString(path),
			0,
		};
		gamePage = new beiklive::GamePage(std::move(dirItem), true);
	}

	auto* frame = new brls::AppletFrame(gamePage);
	HIDE_BRLS_BAR(frame);
	brls::Application::pushActivity(new brls::Activity(frame), brls::TransitionAnimation::NONE);
	gamePage->startGame();
	brls::Logger::info("Direct game launch started: {}", romPath);
	return true;
}

} // namespace

int main(int argc, char* argv[]) {
#ifdef __SWITCH__
    appletInitializeGamePlayRecording();
#endif


	beiklive::ConfigureInit();

	const std::string externalReturnToken = beiklive::externalCoreReturnToken(argc, argv);
	if (!externalReturnToken.empty())
	{
		const bool updated = beiklive::finishExternalCoreSession(externalReturnToken);
		brls::Logger::info("External core session stats {} for token {}",
			updated ? "updated" : "not updated", externalReturnToken);
	}

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

	const auto directLaunchRom = parseDirectLaunchRom(argc, argv);
	if (directLaunchRom.has_value())
		brls::Logger::info("Direct launch argument detected: {}", *directLaunchRom);

	brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_AUTO;
	if (!brls::Application::init()) {
		brls::Logger::error("Unable to init Borealis application");
		return EXIT_FAILURE;
	}

	// ── 初始化翻译管理器（语言由 UI.language 配置决定）──────────────
	static beiklive::TranslationManager s_translation;
	beiklive::Translation = &s_translation;
	s_translation.Load();

	brls::Application::getPlatform()->forceEnableGamePlayRecording();


	brls::Application::createWindow("beiklive/title"_i18n);

	// brls::Application::loadFontFromFile("chinese", BK_RES("font/switch_font.ttf"));

	// ── 应用初始化后读取调试覆盖层设置 ──────────────────────────
	if (beiklive::getKeyInt(beiklive::SettingManager, "debug.logOverlay", 0))
		brls::Application::enableDebuggingView(true);

	auto* audioPlayer = new beiklive::BKAudioPlayer();
	brls::Application::setAudioPlayer(audioPlayer);

	beiklive::ApplyUiTheme();
	beiklive::RegisterStyles();
	beiklive::RegisterThemes();

	bool directLaunchStarted = false;
	if (directLaunchRom.has_value())
		directLaunchStarted = launchDirectGameActivity(*directLaunchRom);

	if (!directLaunchStarted)
	{
		auto* mStartPage = new beiklive::StartPage();
		auto* frame = new brls::AppletFrame(mStartPage);
		HIDE_BRLS_BAR(frame);
		beiklive::MyActivity* activity = new beiklive::MyActivity(frame);
		activity->setPageView(mStartPage);
		brls::Application::pushActivity(activity);
	}

	// ── 更新检查线程 ──────────────────────────────────────────
	// 标志位：程序退出时设置为 true，通知线程提前终止
	std::atomic<bool> gExitFlag{false};

	// Borealis fires exitEvent while the UI thread still owns a live NanoVG
	// context. Stop the shared video there: mainLoop() returns only after
	// Application::exit() has cleared views and destroyed the platform, so
	// deleting an MP4 texture afterwards can dereference a dead context.
	brls::Application::getExitEvent()->subscribe(
		[&gExitFlag]() {
			gExitFlag.store(true, std::memory_order_release);
			brls::Logger::info("MP4: shutting down shared background before UI teardown");
			beiklive::VideoBackgroundView::shutdownSharedVideo();
		});

	std::thread updateThread;
	if (!directLaunchStarted)
	{
		updateThread = std::thread([&gExitFlag]() {
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
					brls::Application::notify(L("新版本 ") + info.version + L(" 可用，请到关于界面更新"));
				}
			});
		});
	}

	// Run the app
	while (brls::Application::mainLoop())
		beiklive::network::WebService::Update();

	// 通知线程退出并等待其完成
	gExitFlag.store(true, std::memory_order_release);
	if (updateThread.joinable())
		updateThread.join();

	beiklive::network::WebService::Stop();
	beiklive::ThreadPool::instance().shutdown();

	brls::Application::setAudioPlayer(nullptr);
	delete audioPlayer;
	audioPlayer = nullptr;

#ifdef __SWITCH__
	auto launchResult = beiklive::switch_platform::commitPendingNroLaunch();
	if (!launchResult.success)
		brls::Logger::error("Pending NRO launch commit failed: {}", launchResult.message);
	else if (launchResult.message != "No pending NRO launch")
		brls::Logger::info("{}", launchResult.message);
#endif

	// Cleanup
	// Exit
	return EXIT_SUCCESS;
}





#ifdef __WINRT__

#include <borealis/core/main.hpp>
#endif
