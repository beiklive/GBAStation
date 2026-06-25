#pragma once

#include <cstdint>
#include <mutex>

namespace beiklive {

struct EmulatorVideoTexture {
    uint32_t texture = 0;
    unsigned width = 0;
    unsigned height = 0;
    unsigned scale = 1;
};

class IEmulatorVideoTexture {
public:
    virtual ~IEmulatorVideoTexture() = default;
    virtual bool GetVideoTexture(EmulatorVideoTexture& out) = 0;
};

std::recursive_mutex& EmulatorGLMutex();

} // namespace beiklive
