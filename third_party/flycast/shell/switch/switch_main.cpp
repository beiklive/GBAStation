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

namespace {

std::string g_returnNroPath = "sdmc:/switch/GBAStation.nro";

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

void parseGbastationArgs(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (!argv[i])
			continue;
		if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc && argv[i + 1])
		{
			g_returnNroPath = argv[i + 1];
			++i;
		}
	}
}

void returnToGbastation()
{
	if (g_returnNroPath.empty())
		return;
	if (!envHasNextLoad())
		return;

	const std::string args = quoteArg(g_returnNroPath);
	const Result rc = envSetNextLoad(g_returnNroPath.c_str(), args.c_str());
	if (R_FAILED(rc))
		std::printf("Flycast: envSetNextLoad return to GBAStation failed: %x\n", rc);
}

} // namespace

int main(int argc, char *argv[])
{
	parseGbastationArgs(argc, argv);

	socketInitializeDefault();
	nxlinkStdio();
	//appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);

	LogManager::Init();

	// Set directories
	flycast::mkdir("/flycast", 0755);
	flycast::mkdir("/flycast/data", 0755);
	set_user_config_dir("/flycast/");
	set_user_data_dir("/flycast/data/");

	add_system_config_dir("/flycast");
	add_system_config_dir("./");
	add_system_data_dir("/flycast/data/");
	add_system_data_dir("./");
	add_system_data_dir("data/");

	if (flycast_init(argc, argv))
		die("Flycast initialization failed");

	mainui_loop();

	flycast_term();

	socketExit();
	returnToGbastation();

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
