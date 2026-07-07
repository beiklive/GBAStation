#include "platform/switch/NroLauncher.hpp"

#include <cstdio>
#include <mutex>
#include <sstream>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive::switch_platform {

namespace {

struct PendingNroLaunch {
    std::string nroPath;
    std::string argv;
};

std::mutex g_pendingMutex;
PendingNroLaunch g_pendingLaunch;

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

bool fileExists(const std::string& path)
{
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

bool fileExistsWithSdmcAlias(const std::string& path)
{
    if (fileExists(path))
        return true;

    if (!path.empty() && path.front() == '/')
        return fileExists("sdmc:" + path);

    if (path.rfind("sdmc:/", 0) == 0)
        return fileExists(path.substr(5));

    return false;
}

} // namespace

NroLaunchResult launchNroOnExit(const NroLaunchRequest& request)
{
#ifndef __SWITCH__
    (void)request;
    return {false, "NRO chainload is only available on Switch"};
#else
    if (request.nroPath.empty())
        return {false, "External NRO path is empty"};
    if (request.romPath.empty())
        return {false, "ROM path is empty"};
    if (!fileExistsWithSdmcAlias(request.nroPath))
        return {false, "External NRO does not exist: " + request.nroPath};
    if (!fileExistsWithSdmcAlias(request.romPath))
        return {false, "ROM does not exist: " + request.romPath};
    if (!envHasNextLoad())
        return {false, "Current homebrew loader does not support envSetNextLoad"};

    std::ostringstream argv;
    argv << quoteArg(request.nroPath) << ' ' << quoteArg(request.romPath);
    if (!request.returnNroPath.empty())
        argv << " --return " << quoteArg(request.returnNroPath);

    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        g_pendingLaunch.nroPath = request.nroPath;
        g_pendingLaunch.argv = argv.str();
    }

    return {true, "Next NRO pending: " + request.nroPath};
#endif
}

NroLaunchResult commitPendingNroLaunch()
{
#ifndef __SWITCH__
    return {false, "NRO chainload is only available on Switch"};
#else
    PendingNroLaunch pending;
    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        pending = g_pendingLaunch;
        g_pendingLaunch = {};
    }

    if (pending.nroPath.empty())
        return {true, "No pending NRO launch"};

    if (!envHasNextLoad())
        return {false, "Current homebrew loader does not support envSetNextLoad"};

    const Result rc = envSetNextLoad(pending.nroPath.c_str(), pending.argv.c_str());
    if (R_FAILED(rc))
    {
        std::ostringstream msg;
        msg << "envSetNextLoad failed: 0x" << std::hex << rc;
        return {false, msg.str()};
    }

    return {true, "Next NRO configured: " + pending.nroPath};
#endif
}

} // namespace beiklive::switch_platform
