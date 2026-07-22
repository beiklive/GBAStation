#pragma once

#include <string>
#include <vector>

namespace beiklive::switch_platform {

struct NroLaunchRequest {
    std::string nroPath;
    std::string romPath;
    std::string returnNroPath;
    std::vector<std::string> extraArgs;
};

struct NroLaunchResult {
    bool success = false;
    std::string message;
};

NroLaunchResult launchNroOnExit(const NroLaunchRequest& request);
NroLaunchResult commitPendingNroLaunch();

} // namespace beiklive::switch_platform
