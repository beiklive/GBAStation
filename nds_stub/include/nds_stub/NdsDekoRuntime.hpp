#pragma once

#include <string>

#include "nds_stub/NdsStubTypes.hpp"

namespace beiklive::nds_stub {

struct DekoRunOptions {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string returnNroPath;
    bool returnToNroOnExit = true;
    std::string screenLayout = "priority_top";
    std::string screenOrientation = "0";
    bool integerScale = true;
    int screenGap = 0;
    bool overlayEnabled = false;
    std::string overlayPath;
    bool shaderEnabled = false;
    std::string ndsShaderType = "RetroArch_dot";
    NdsCustomLayoutSettings customLayout;
};

bool ShouldUseDekoRuntime();
int RunDekoRuntime(const DekoRunOptions& options);

} // namespace beiklive::nds_stub
