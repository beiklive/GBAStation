#pragma once

#include <string>

namespace beiklive::nds_stub {

struct MelonPlatformData {
    std::string savePath;
    std::string firmwarePath;
};

void appendStubLog(const char* format, ...);

} // namespace beiklive::nds_stub
