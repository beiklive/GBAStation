#pragma once

#include <cstdint>
#include <vector>

namespace beiklive::melonds
{

struct ScreenLayout
{
    unsigned layoutMode = 0;
    float topScale = 1.0f;
    float bottomScale = 1.0f;
    float gap = 0.0f;
};

class FrameComposer
{
public:
    FrameComposer();

    void setLayout(unsigned mode);

    void setScales(float top, float bottom);

    void setGap(float gap);

    void compose(const uint32_t* topBuffer,
                 const uint32_t* bottomBuffer,
                 unsigned screenW, unsigned screenH,
                 uint32_t* outBuffer,
                 unsigned outWidth, unsigned outHeight);

    const uint32_t* composedBuffer() const { return m_outputBuffer.data(); }
    unsigned composedWidth() const;
    unsigned composedHeight() const;

    static constexpr unsigned kScreenWidth = 256;
    static constexpr unsigned kScreenHeight = 192;

private:
    unsigned m_layoutMode = 0;
    float m_topScale = 1.0f;
    float m_bottomScale = 1.0f;
    float m_gap = 0.0f;

    std::vector<uint32_t> m_outputBuffer;
    unsigned m_outWidth = 256;
    unsigned m_outHeight = 384;

    void updateOutputSize();
};

} // namespace beiklive::melonds
