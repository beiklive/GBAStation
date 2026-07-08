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
    int integerScaleMultiplier = 0;
    float customScale = 1.0f;
    float customOffsetX = 0.0f;
    float customOffsetY = 0.0f;
    bool overlayEnabled = false;
    std::string overlayPath;
    bool shaderEnabled = false;
    std::string shaderType;
    std::string shaderPath;
};

int RunRuntime(const RunOptions& options);

} // namespace beiklive::mgba_stub
