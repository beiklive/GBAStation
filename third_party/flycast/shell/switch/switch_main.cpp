/*
    This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef LIBRETRO
#include "nswitch.h"
#include "stdclass.h"
#include "log/LogManager.h"
#include "emulator.h"
#include "ui/mainui.h"
#include "oslib/directory.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <exception>

namespace {

std::string g_returnNroPath = "sdmc:/switch/GBAStation.nro";
std::string g_gbastationSessionToken;
constexpr const char *FLYCAST_ROOT = "sdmc:/GBAStation/Flycast";
constexpr const char *FLYCAST_DATA = "sdmc:/GBAStation/Flycast/data";
constexpr const char *STARTUP_LOG = "sdmc:/GBAStation/Flycast/flycast_startup.log";
constexpr const char *LAST_ERROR = "sdmc:/GBAStation/Flycast/last_error.txt";

void logStage(FILE *log, const char *stage)
{
	if (!log)
		return;
	std::fprintf(log, "%s\n", stage);
	std::fflush(log);
}

void writeLastError(const char *message)
{
	FILE *error = std::fopen(LAST_ERROR, "w");
	if (!error)
		return;
	std::fprintf(error, "%s\n", message ? message : "Unknown Flycast error");
	std::fclose(error);
}

std::string quoteArg(const std::string& value)
{
	std::string out;
	out.reserve(value.size() + 2);
	out.push_back('"');
	for (char c : value)
	{
		if (c == '"' || c == '\\')
			out.push_back('\\');
		out.push_back(c);
	}
	out.push_back('"');
	return out;
}

void parseGbastationArgs(int& argc, char *argv[])
{
	int output = 1;
	for (int i = 1; i < argc; ++i)
	{
		if (!argv[i])
			continue;
		if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc && argv[i + 1])
		{
			g_returnNroPath = argv[i + 1];
			++i;
			continue;
		}
		else if (std::strcmp(argv[i], "--gbastation-session") == 0 && i + 1 < argc && argv[i + 1])
		{
			g_gbastationSessionToken = argv[i + 1];
			++i;
			continue;
		}
		argv[output++] = argv[i];
	}
	argc = output;
	argv[output] = nullptr;
}

void returnToGbastation()
{
	if (g_returnNroPath.empty())
		return;
	if (!envHasNextLoad())
		return;

	std::string args = quoteArg(g_returnNroPath);
	if (!g_gbastationSessionToken.empty())
		args += " --external-return " + quoteArg(g_gbastationSessionToken);
	const Result rc = envSetNextLoad(g_returnNroPath.c_str(), args.c_str());
	if (R_FAILED(rc))
		std::printf("Flycast: envSetNextLoad return to GBAStation failed: %x\n", rc);
}

} // namespace

int main(int argc, char *argv[])
{
	parseGbastationArgs(argc, argv);

	// Keep every Flycast-owned file below GBAStation instead of creating files
	// beside the NRO or at the SD root.
	flycast::mkdir("sdmc:/GBAStation", 0755);
	flycast::mkdir(FLYCAST_ROOT, 0755);
	flycast::mkdir(FLYCAST_DATA, 0755);
	std::remove(LAST_ERROR);
	FILE *startupLog = std::fopen(STARTUP_LOG, "w");
	logStage(startupLog, "Flycast Switch startup");
	if (startupLog)
	{
		std::fprintf(startupLog, "argc=%d\n", argc);
		for (int i = 0; i < argc; ++i)
			std::fprintf(startupLog, "argv[%d]=%s\n", i, argv[i] ? argv[i] : "<null>");
		std::fflush(startupLog);
	}

	set_user_config_dir(FLYCAST_ROOT);
	set_user_data_dir(FLYCAST_DATA);
	add_system_config_dir(FLYCAST_ROOT);
	add_system_config_dir("./");
	add_system_data_dir(FLYCAST_DATA);
	add_system_data_dir("./");
	add_system_data_dir("data/");

	const Result socketResult = socketInitializeDefault();
	const bool socketReady = R_SUCCEEDED(socketResult);
	if (startupLog)
	{
		std::fprintf(startupLog, "socketInitializeDefault=0x%x\n", socketResult);
		std::fflush(startupLog);
	}
	nxlinkStdio();
	//appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);

	logStage(startupLog, "LogManager::Init begin");
	LogManager::Init();
	logStage(startupLog, "LogManager::Init complete");

	bool initialized = false;
	try
	{
		logStage(startupLog, "flycast_init begin");
		const int initResult = flycast_init(argc, argv);
		if (initResult != 0)
		{
			if (startupLog)
			{
				std::fprintf(startupLog, "flycast_init failed: %d\n", initResult);
				std::fflush(startupLog);
			}
			writeLastError("Flycast 初始化失败，请查看 flycast_startup.log 和 data/flycast.log");
		}
		else
		{
			initialized = true;
			logStage(startupLog, "flycast_init complete; mainui_loop begin");
			mainui_loop();
			logStage(startupLog, "mainui_loop complete; flycast_term begin");
			flycast_term();
			initialized = false;
			logStage(startupLog, "flycast_term complete");
		}
	}
	catch (const std::exception& e)
	{
		if (startupLog)
		{
			std::fprintf(startupLog, "Unhandled exception: %s\n", e.what());
			std::fflush(startupLog);
		}
		writeLastError(e.what());
	}
	catch (...)
	{
		logStage(startupLog, "Unhandled non-standard exception");
		writeLastError("Flycast 发生未知异常，请查看 flycast_startup.log 和 data/flycast.log");
	}

	if (initialized)
	{
		try
		{
			flycast_term();
		}
		catch (...)
		{
			logStage(startupLog, "flycast_term failed during cleanup");
		}
	}

	if (socketReady)
		socketExit();
	logStage(startupLog, "returning to GBAStation");
	returnToGbastation();
	if (startupLog)
		std::fclose(startupLog);

	return 0;
}

void os_DoEvents()
{
}

namespace hostfs
{

void saveScreenshot(const std::string& name, const std::vector<u8>& data)
{
	throw FlycastException("Not supported on Switch");
}

}
#endif	//!LIBRETRO
