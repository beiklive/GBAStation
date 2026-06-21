#include "emulator/melonds/MelonDSVideo.h"

#include "GPU3D.h"
#include "NDS.h"

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
        uint32_t* dstLine = out + y * width;
        size_t x = 0;
        for (; x + 3 < width; x += 4)
        {
            dstLine[x + 0] = toRgba8888(line[x + 0]);
            dstLine[x + 1] = toRgba8888(line[x + 1]);
            dstLine[x + 2] = toRgba8888(line[x + 2]);
            dstLine[x + 3] = toRgba8888(line[x + 3]);
        }
        for (; x < width; ++x)
            dstLine[x] = toRgba8888(line[x]);
    }
}

void copyContiguousRgba(uint32_t* dst, const uint32_t* src, size_t count)
{
    size_t i = 0;
    for (; i + 7 < count; i += 8)
    {
        dst[i + 0] = toRgba8888(src[i + 0]);
        dst[i + 1] = toRgba8888(src[i + 1]);
        dst[i + 2] = toRgba8888(src[i + 2]);
        dst[i + 3] = toRgba8888(src[i + 3]);
        dst[i + 4] = toRgba8888(src[i + 4]);
        dst[i + 5] = toRgba8888(src[i + 5]);
        dst[i + 6] = toRgba8888(src[i + 6]);
        dst[i + 7] = toRgba8888(src[i + 7]);
    }
    for (; i < count; ++i)
        dst[i] = toRgba8888(src[i]);
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
    (void)scale;
    if (!pixels || width != kWidth || height < kHeight + 2u)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    const unsigned outWidth = kWidth;
    const unsigned screenHeight = 192u;
    const unsigned paddingHeight = 2u;
    const unsigned outHeight = kHeight;
    const unsigned back = m_front ^ 1u;
    auto& dst = m_framebuffer[back];
    const size_t outPixels = static_cast<size_t>(outWidth) * outHeight;
    if (dst.size() != outPixels)
        dst.resize(outPixels);

    copyContiguousRgba(dst.data(), pixels, static_cast<size_t>(outWidth) * screenHeight);

    const unsigned bottomSrcY = screenHeight + paddingHeight;
    copyContiguousRgba(dst.data() + static_cast<size_t>(screenHeight) * outWidth,
                       pixels + static_cast<size_t>(bottomSrcY) * width,
                       static_cast<size_t>(outWidth) * screenHeight);

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
