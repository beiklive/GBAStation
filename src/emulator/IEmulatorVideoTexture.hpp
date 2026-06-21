#pragma once

#include <cstdint>
#include <functional>
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
    virtual bool WithVideoTextureLocked(const std::function<bool(const EmulatorVideoTexture&)>& consumer)
    {
        EmulatorVideoTexture texture;
        if (!GetVideoTexture(texture))
            return false;
        return consumer(texture);
    }
    virtual void SetVideoTextureConsumerActive(bool active) { (void)active; }
};

std::recursive_mutex& EmulatorGLMutex();

} // namespace beiklive
