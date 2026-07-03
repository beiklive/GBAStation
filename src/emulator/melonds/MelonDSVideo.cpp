#include "emulator/melonds/MelonDSVideo.h"

#include "GPU3D.h"
#include "NDS.h"

#include <algorithm>
#include <cstring>

namespace beiklive::melonds {

namespace {

uint32_t toRgba8888(uint32_t pixel)
{
    return (pixel & 0xFF00FF00u) |
           ((pixel & 0x000000FFu) << 16) |
           ((pixel & 0x00FF0000u) >> 16);
}

void copyScreenRgba(std::vector<uint32_t>& dst,
                    size_t dstOffset,
                    const uint32_t* src,
                    size_t srcStride,
                    size_t width,
                    size_t height)
{
    uint32_t* out = dst.data() + dstOffset;
    for (size_t y = 0; y < height; ++y)
    {
        const uint32_t* line = src + y * srcStride;
        std::transform(line, line + width, out + y * width, toRgba8888);
    }
}

} // namespace

void MelonDSVideo::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& buffer : m_framebuffer)
        buffer.assign(static_cast<size_t>(kWidth) * kHeight, 0xFF000000u);
    m_width = { kWidth, kWidth };
    m_height = { kHeight, kHeight };
    m_front = 0;
    m_ready = false;
}

void MelonDSVideo::Capture(const melonDS::NDS& nds)
{
    const int front = nds.GPU.FrontBuffer;
    const uint32_t* top = nds.GPU.Framebuffer[front][0].get();
    const uint32_t* bottom = nds.GPU.Framebuffer[front][1].get();
    if (!top || !bottom)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    const unsigned back = m_front ^ 1u;
    auto& dst = m_framebuffer[back];
    if (dst.size() != static_cast<size_t>(kWidth) * kHeight)
        dst.resize(static_cast<size_t>(kWidth) * kHeight);

    const size_t screenHeight = 192;
    const size_t screenPixels = static_cast<size_t>(kWidth) * screenHeight;
    const size_t stride = nds.GPU.GetRenderer3D().Accelerated ? (256u * 3u + 1u) : kWidth;
    copyScreenRgba(dst, 0, top, stride, kWidth, screenHeight);
    copyScreenRgba(dst, screenPixels, bottom, stride, kWidth, screenHeight);
    m_width[back] = kWidth;
    m_height[back] = kHeight;
    m_front = back;
    m_ready = true;
}

void MelonDSVideo::CaptureAcceleratedRgba(const uint32_t* pixels,
                                          unsigned width,
                                          unsigned height,
                                          unsigned scale)
{
    if (!pixels || scale == 0 || width != kWidth * scale || height < (kHeight + 2u) * scale)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    const unsigned outWidth = kWidth * scale;
    const unsigned screenHeight = 192u * scale;
    const unsigned paddingHeight = 2u * scale;
    const unsigned outHeight = kHeight * scale;
    const unsigned back = m_front ^ 1u;
    auto& dst = m_framebuffer[back];
    const size_t outPixels = static_cast<size_t>(outWidth) * outHeight;
    if (dst.size() != outPixels)
        dst.resize(outPixels);

    for (unsigned y = 0; y < screenHeight; ++y)
    {
        const uint32_t* src = pixels + static_cast<size_t>(y) * width;
        uint32_t* out = dst.data() + static_cast<size_t>(y) * outWidth;
        std::transform(src, src + outWidth, out, toRgba8888);
    }

    const unsigned bottomSrcY = screenHeight + paddingHeight;
    for (unsigned y = 0; y < screenHeight; ++y)
    {
        const uint32_t* src = pixels + static_cast<size_t>(bottomSrcY + y) * width;
        uint32_t* out = dst.data() + static_cast<size_t>(screenHeight + y) * outWidth;
        std::transform(src, src + outWidth, out, toRgba8888);
    }

    m_front = back;
    m_width[back] = outWidth;
    m_height[back] = outHeight;
    m_ready = true;
}

LibretroLoader::VideoFrame MelonDSVideo::GetFrame() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    LibretroLoader::VideoFrame frame;
    if (!m_ready)
        return frame;
    frame.width = m_width[m_front];
    frame.height = m_height[m_front];
    frame.pixels = m_framebuffer[m_front];
    return frame;
}

const uint32_t* MelonDSVideo::GetFrameBuffer() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ready ? m_framebuffer[m_front].data() : nullptr;
}

} // namespace beiklive::melonds
