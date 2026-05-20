#include "video/FrameComposer.hpp"

#include <cstring>
#include <algorithm>

namespace beiklive::melonds
{

FrameComposer::FrameComposer()
{
    updateOutputSize();
}

void FrameComposer::setLayout(unsigned mode)
{
    m_layoutMode = mode;
    updateOutputSize();
}

void FrameComposer::setScales(float top, float bottom)
{
    m_topScale = std::max(0.5f, std::min(top, 2.0f));
    m_bottomScale = std::max(0.5f, std::min(bottom, 2.0f));
    updateOutputSize();
}

void FrameComposer::setGap(float gap)
{
    m_gap = std::max(0.0f, gap);
    updateOutputSize();
}

void FrameComposer::updateOutputSize()
{
    switch (m_layoutMode)
    {
    case 0: // Vertical (default)
        m_outWidth = static_cast<unsigned>(kScreenWidth * m_topScale);
        m_outHeight = static_cast<unsigned>(kScreenHeight * (m_topScale + m_bottomScale) + m_gap);
        break;
    case 1: // Horizontal
        m_outWidth = static_cast<unsigned>(kScreenWidth * (m_topScale + m_bottomScale) + m_gap);
        m_outHeight = static_cast<unsigned>(kScreenHeight * std::max(m_topScale, m_bottomScale));
        break;
    case 2: // Single (top only)
        m_outWidth = static_cast<unsigned>(kScreenWidth * m_topScale);
        m_outHeight = static_cast<unsigned>(kScreenHeight * m_topScale);
        break;
    case 3: // Hybrid (top big, bottom small at corner)
        m_outWidth = static_cast<unsigned>(kScreenWidth * m_topScale);
        m_outHeight = static_cast<unsigned>(kScreenHeight * m_topScale);
        break;
    default:
        m_outWidth = static_cast<unsigned>(kScreenWidth * m_topScale);
        m_outHeight = static_cast<unsigned>(kScreenHeight * (m_topScale + m_bottomScale) + m_gap);
        break;
    }

    m_outputBuffer.resize(m_outWidth * m_outHeight);
}

unsigned FrameComposer::composedWidth() const
{
    return m_outWidth;
}

unsigned FrameComposer::composedHeight() const
{
    return m_outHeight;
}

static void copyScaled(const uint32_t* src, unsigned srcW, unsigned srcH,
                       uint32_t* dst, unsigned dstW, unsigned dstH,
                       unsigned dstOffsetX, unsigned dstOffsetY,
                       float scale)
{
    unsigned scaledW = static_cast<unsigned>(srcW * scale);
    unsigned scaledH = static_cast<unsigned>(srcH * scale);

    if (scale == 1.0f)
    {
        for (unsigned row = 0; row < srcH; ++row)
            std::memcpy(&dst[(dstOffsetY + row) * dstW + dstOffsetX],
                        &src[row * srcW], srcW * sizeof(uint32_t));
    }
    else
    {
        for (unsigned dy = 0; dy < scaledH; ++dy)
        {
            unsigned sy = static_cast<unsigned>(dy / scale);
            if (sy >= srcH) sy = srcH - 1;
            for (unsigned dx = 0; dx < scaledW; ++dx)
            {
                unsigned sx = static_cast<unsigned>(dx / scale);
                if (sx >= srcW) sx = srcW - 1;
                dst[(dstOffsetY + dy) * dstW + dstOffsetX + dx] = src[sy * srcW + sx];
            }
        }
    }
}

void FrameComposer::compose(const uint32_t* topBuffer,
                             const uint32_t* bottomBuffer,
                             unsigned screenW, unsigned screenH,
                             uint32_t* outBuffer,
                             unsigned outWidth, unsigned outHeight)
{
    (void)screenW; (void)screenH;

    if (!topBuffer || !bottomBuffer || !outBuffer)
        return;

    std::fill(outBuffer, outBuffer + (outWidth * outHeight), 0x000000FFu);

    switch (m_layoutMode)
    {
    case 0: // Vertical
    {
        unsigned topH = static_cast<unsigned>(kScreenHeight * m_topScale);
        copyScaled(topBuffer, kScreenWidth, kScreenHeight,
                   outBuffer, outWidth, outHeight, 0, 0, m_topScale);
        if (m_bottomScale > 0.0f)
        {
            unsigned botOffsetY = topH + static_cast<unsigned>(m_gap);
            copyScaled(bottomBuffer, kScreenWidth, kScreenHeight,
                       outBuffer, outWidth, outHeight, 0, botOffsetY, m_bottomScale);
        }
        break;
    }
    case 1: // Horizontal
    {
        unsigned topW = static_cast<unsigned>(kScreenWidth * m_topScale);
        copyScaled(topBuffer, kScreenWidth, kScreenHeight,
                   outBuffer, outWidth, outHeight, 0, 0, m_topScale);
        if (m_bottomScale > 0.0f)
        {
            unsigned botOffsetX = topW + static_cast<unsigned>(m_gap);
            copyScaled(bottomBuffer, kScreenWidth, kScreenHeight,
                       outBuffer, outWidth, outHeight, botOffsetX, 0, m_bottomScale);
        }
        break;
    }
    case 2: // Single
    {
        copyScaled(topBuffer, kScreenWidth, kScreenHeight,
                   outBuffer, outWidth, outHeight, 0, 0, m_topScale);
        break;
    }
    case 3: // Hybrid
    {
        copyScaled(topBuffer, kScreenWidth, kScreenHeight,
                   outBuffer, outWidth, outHeight, 0, 0, m_topScale);

        unsigned smallW = static_cast<unsigned>(kScreenWidth * 0.4f);
        unsigned smallH = static_cast<unsigned>(kScreenHeight * 0.4f);
        float smallScale = 0.4f;
        unsigned botX = outWidth - smallW - 8;
        unsigned botY = outHeight - smallH - 8;

        copyScaled(bottomBuffer, kScreenWidth, kScreenHeight,
                   outBuffer, outWidth, outHeight, botX, botY, smallScale);
        break;
    }
    }
}

} // namespace beiklive::melonds
