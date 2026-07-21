#pragma once

#include <string>

namespace beiklive::switch_platform {

struct NroLaunchRequest {
    std::string nroPath;
    std::string romPath;
    std::string returnNroPath;
    std::string extraArguments;
};

struct NroLaunchResult {
    bool success = false;
    std::string message;
};

NroLaunchResult launchNroOnExit(const NroLaunchRequest& request);
NroLaunchResult commitPendingNroLaunch();

} // namespace beiklive::switch_platform
