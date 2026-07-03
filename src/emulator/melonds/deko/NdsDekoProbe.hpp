#pragma once

#include <string>

namespace beiklive {

struct NdsDekoProbeResult {
    bool supported = false;
    bool success = false;
    int requestedLevel = 0;
    int reachedLevel = 0;
    int presentedFrames = 0;
    std::string message;
};

struct NdsDekoProbeOptions {
    int level = 1;
    int frameCount = 60;
};

NdsDekoProbeResult RunNdsDekoProbe(const NdsDekoProbeOptions& options);

} // namespace beiklive
