#include "platform/switch/NroLauncher.hpp"

#include <cstdio>
#include <mutex>
#include <sstream>
#include <cstdarg>

#ifdef __SWITCH__
#include <switch.h>
#include <sys/stat.h>
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

void appendLaunchLog(const char* fmt, ...)
{
#ifdef __SWITCH__
    mkdir("sdmc:/GBAStation", 0777);
    mkdir("sdmc:/GBAStation/debug", 0777);
    FILE* fp = std::fopen("sdmc:/GBAStation/debug/external_core_launch.log", "a");
    if (!fp)
        return;
    va_list args;
    va_start(args, fmt);
    std::vfprintf(fp, fmt, args);
    va_end(args);
    std::fputc('\n', fp);
    std::fflush(fp);
    std::fclose(fp);
#else
    (void)fmt;
#endif
}

} // namespace

NroLaunchResult launchNroOnExit(const NroLaunchRequest& request)
{
#ifndef __SWITCH__
    (void)request;
    return {false, "NRO chainload is only available on Switch"};
#else
    appendLaunchLog("launch request nro=%s rom=%s return=%s",
        request.nroPath.c_str(), request.romPath.c_str(), request.returnNroPath.c_str());
    if (request.nroPath.empty())
    {
        appendLaunchLog("launch reject: empty nro path");
        return {false, "NRO path is empty"};
    }
    if (!fileExistsWithSdmcAlias(request.nroPath))
    {
        appendLaunchLog("launch reject: missing nro %s", request.nroPath.c_str());
        return {false, "NRO does not exist: " + request.nroPath};
    }
    if (!request.romPath.empty() && !fileExistsWithSdmcAlias(request.romPath))
    {
        appendLaunchLog("launch reject: missing rom %s", request.romPath.c_str());
        return {false, "ROM does not exist: " + request.romPath};
    }
    if (!envHasNextLoad())
    {
        appendLaunchLog("launch reject: envSetNextLoad unavailable");
        return {false, "Current homebrew loader does not support envSetNextLoad"};
    }

    std::ostringstream argv;
    argv << quoteArg(request.nroPath);
    if (!request.romPath.empty())
        argv << ' ' << quoteArg(request.romPath);
    for (const auto& arg : request.extraArgs)
        argv << ' ' << quoteArg(arg);
    if (!request.returnNroPath.empty())
        argv << " --return " << quoteArg(request.returnNroPath);

    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        g_pendingLaunch.nroPath = request.nroPath;
        g_pendingLaunch.argv = argv.str();
    }

    appendLaunchLog("launch pending nro=%s argv=%s", request.nroPath.c_str(), argv.str().c_str());
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
    {
        appendLaunchLog("commit: no pending nro");
        return {true, "No pending NRO launch"};
    }

    if (!envHasNextLoad())
    {
        appendLaunchLog("commit reject: envSetNextLoad unavailable");
        return {false, "Current homebrew loader does not support envSetNextLoad"};
    }

    const Result rc = envSetNextLoad(pending.nroPath.c_str(), pending.argv.c_str());
    if (R_FAILED(rc))
    {
        std::ostringstream msg;
        msg << "envSetNextLoad failed: 0x" << std::hex << rc;
        appendLaunchLog("commit failed nro=%s rc=0x%x argv=%s",
            pending.nroPath.c_str(), static_cast<unsigned>(rc), pending.argv.c_str());
        return {false, msg.str()};
    }

    appendLaunchLog("commit ok nro=%s argv=%s", pending.nroPath.c_str(), pending.argv.c_str());
    return {true, "Next NRO configured: " + pending.nroPath};
#endif
}

} // namespace beiklive::switch_platform
