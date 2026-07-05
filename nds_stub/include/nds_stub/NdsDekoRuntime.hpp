#pragma once

#include <string>

#include "nds_stub/NdsStubTypes.hpp"

namespace beiklive::nds_stub {

struct DekoRunOptions {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string returnNroPath;
    std::string screenLayout = "hybrid";
    std::string screenOrientation = "0";
    bool integerScale = true;
    int screenGap = 0;
    NdsCustomLayoutSettings customLayout;
};

bool ShouldUseDekoRuntime();
int RunDekoRuntime(const DekoRunOptions& options);

} // namespace beiklive::nds_stub
