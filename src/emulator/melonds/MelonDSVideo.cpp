#include "emulator/melonds/MelonDSVideo.h"

#include "NDS.h"

#include <algorithm>

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
                    size_t count)
{
    std::transform(src, src + count, dst.data() + dstOffset, toRgba8888);
}

} // namespace

void MelonDSVideo::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& buffer : m_framebuffer)
        buffer.assign(static_cast<size_t>(kWidth) * kHeight, 0xFF000000u);
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

    const size_t screenPixels = static_cast<size_t>(kWidth) * 192;
    copyScreenRgba(dst, 0, top, screenPixels);
    copyScreenRgba(dst, screenPixels, bottom, screenPixels);
    m_front = back;
    m_ready = true;
}

LibretroLoader::VideoFrame MelonDSVideo::GetFrame() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    LibretroLoader::VideoFrame frame;
    if (!m_ready)
        return frame;
    frame.width = kWidth;
    frame.height = kHeight;
    frame.pixels = m_framebuffer[m_front];
    return frame;
}

const uint32_t* MelonDSVideo::GetFrameBuffer() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ready ? m_framebuffer[m_front].data() : nullptr;
}

} // namespace beiklive::melonds
