#include "video/MelonDSVideo.hpp"
#include "MelonDSInstance.hpp"

#include <cstring>

namespace beiklive::melonds
{

MelonDSVideo::MelonDSVideo(MelonDSInstance& instance)
    : m_instance(instance)
{
}

const uint32_t* MelonDSVideo::topBuffer() const
{
    return m_instance.GetFramebuffer(0);
}

const uint32_t* MelonDSVideo::bottomBuffer() const
{
    return m_instance.GetFramebuffer(1);
}

void MelonDSVideo::captureFrame(uint32_t* out, unsigned outWidth, unsigned outHeight) const
{
    const uint32_t* top = topBuffer();
    const uint32_t* bot = bottomBuffer();

    if (!top || !bot || !out)
        return;

    for (unsigned row = 0; row < kScreenHeight; ++row)
        std::memcpy(&out[row * outWidth], &top[row * kScreenWidth], kScreenWidth * sizeof(uint32_t));

    unsigned botStartRow = kScreenHeight;
    if (static_cast<unsigned>(outHeight) >= botStartRow + kScreenHeight)
    {
        for (unsigned row = 0; row < kScreenHeight; ++row)
            std::memcpy(&out[(botStartRow + row) * outWidth], &bot[row * kScreenWidth], kScreenWidth * sizeof(uint32_t));
    }
}

} // namespace beiklive::melonds
