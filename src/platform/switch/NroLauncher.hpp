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

/// Records "the launcher process started, here is its argv" in
/// sdmc:/GBAStation/debug/external_core_launch.log.  Called as the very first
/// statement of main so that a chainload back from a core can be told apart
/// from a handoff that never reached the launcher at all.
void logLauncherEntry(int argc, char** argv);

} // namespace beiklive::switch_platform
