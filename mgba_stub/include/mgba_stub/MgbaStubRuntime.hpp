#pragma once

#include <string>

namespace beiklive::mgba_stub {

struct RunOptions {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string returnNroPath;
    int platform = 1;
    bool hasDisplaySettings = false;
    int displayMode = 0;
    float integerAspectRatio = 0.0f;
    float customScale = 1.0f;
    float customOffsetX = 0.0f;
    float customOffsetY = 0.0f;
};

int RunRuntime(const RunOptions& options);

} // namespace beiklive::mgba_stub
