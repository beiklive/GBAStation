#include "platform/switch/NroLauncher.hpp"

#include <cstdio>
#include <sstream>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive::switch_platform {

namespace {

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
        return {false, "NDS NRO path is empty"};
    if (request.romPath.empty())
        return {false, "NDS ROM path is empty"};
    if (!fileExistsWithSdmcAlias(request.nroPath))
        return {false, "NDS NRO does not exist: " + request.nroPath};
    if (!fileExistsWithSdmcAlias(request.romPath))
        return {false, "NDS ROM does not exist: " + request.romPath};
    if (!envHasNextLoad())
        return {false, "Current homebrew loader does not support envSetNextLoad"};

    std::ostringstream argv;
    argv << quoteArg(request.nroPath) << ' ' << quoteArg(request.romPath);
    if (!request.returnNroPath.empty())
        argv << " --return " << quoteArg(request.returnNroPath);

    const Result rc = envSetNextLoad(request.nroPath.c_str(), argv.str().c_str());
    if (R_FAILED(rc))
    {
        std::ostringstream msg;
        msg << "envSetNextLoad failed: 0x" << std::hex << rc;
        return {false, msg.str()};
    }

    return {true, "Next NRO configured: " + request.nroPath};
#endif
}

} // namespace beiklive::switch_platform
