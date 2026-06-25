#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

#include "game/retro/LibretroLoader.hpp"

namespace melonDS {
class NDS;
}

namespace beiklive::melonds {

class MelonDSVideo {
public:
    static constexpr unsigned kWidth = 256;
    static constexpr unsigned kHeight = 384;

    void Reset();
    void Capture(const melonDS::NDS& nds);
    void CaptureAcceleratedRgba(const uint32_t* pixels,
                                unsigned width,
                                unsigned height,
                                unsigned scale);
    LibretroLoader::VideoFrame GetFrame() const;
    const uint32_t* GetFrameBuffer() const;

private:
    mutable std::mutex m_mutex;
    std::array<std::vector<uint32_t>, 2> m_framebuffer;
    std::array<unsigned, 2> m_width {};
    std::array<unsigned, 2> m_height {};
    unsigned m_front = 0;
    bool m_ready = false;
};

} // namespace beiklive::melonds
