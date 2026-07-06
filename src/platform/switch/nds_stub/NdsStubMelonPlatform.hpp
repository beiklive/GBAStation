#pragma once

#include <string>

namespace beiklive::nds_stub {

struct MelonPlatformData {
    std::string savePath;
    std::string firmwarePath;
};

inline void appendStubLog(const char*, ...)
{
}

} // namespace beiklive::nds_stub
