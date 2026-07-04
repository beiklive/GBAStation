#include "nds_stub/NdsGameLayer.hpp"

#include <algorithm>

#include "../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub {

namespace {

constexpr float kAspect = 4.0f / 3.0f;

RectF fitAspect(const RectF& region)
{
    float w = region.w;
    float h = w / kAspect;
    if (h > region.h)
    {
        h = region.h;
        w = h * kAspect;
    }
    return {region.x + (region.w - w) * 0.5f,
            region.y + (region.h - h) * 0.5f,
            w,
            h};
}

RectF fitInteger(const RectF& region, float scale)
{
    const float w = 256.0f * scale;
    const float h = 192.0f * scale;
    return {region.x + (region.w - w) * 0.5f,
            region.y + (region.h - h) * 0.5f,
            w,
            h};
}

bool validRect(const RectF& rect)
{
    return rect.w > 0.0f && rect.h > 0.0f;
}

} // namespace

void NdsGameLayer::init(GPU2D::DekoRenderer* renderer)
{
    m_renderer = renderer;
    for (int front = 0; front < 2; ++front)
    {
        for (int screen = 0; screen < 2; ++screen)
        {
            m_framebufferTextures[front][screen] =
                Gfx::TextureCreateExternal(m_renderer->GetFramebufferTextureWidth(),
                                           m_renderer->GetFramebufferTextureHeight(),
                                           m_renderer->GetFramebuffer(front, screen));
        }
    }
}

void NdsGameLayer::deinit()
{
    for (int front = 0; front < 2; ++front)
    {
        for (int screen = 0; screen < 2; ++screen)
        {
            if (m_framebufferTextures[front][screen])
            {
                Gfx::TextureDelete(m_framebufferTextures[front][screen]);
                m_framebufferTextures[front][screen] = 0;
            }
        }
    }
    m_renderer = nullptr;
}

RectF NdsGameLayer::topRect() const
{
    return firstRectForSource(true);
}

RectF NdsGameLayer::bottomRect() const
{
    return firstRectForSource(false);
}

RectF NdsGameLayer::touchRect() const
{
    return firstRectForSource(false);
}

void NdsGameLayer::setScreenLayout(int layout)
{
    m_layout = static_cast<ScreenLayout>(std::clamp(layout, 0, 5));
}

std::vector<NdsGameLayer::ScreenDrawRect> NdsGameLayer::computeScreenRects() const
{
    std::vector<ScreenDrawRect> rects;
    auto source = [&](bool layoutTop) {
        return m_screensSwapped ? !layoutTop : layoutTop;
    };
    auto add = [&](bool layoutTop, const RectF& rect) {
        if (validRect(rect))
            rects.push_back({source(layoutTop), rect});
    };

    switch (m_layout)
    {
    case ScreenLayout::Horizontal:
    {
        const RectF left{0.0f, 0.0f, kScreenWidth * 0.5f, kScreenHeight};
        const RectF right{kScreenWidth * 0.5f, 0.0f, kScreenWidth * 0.5f, kScreenHeight};
        add(true, m_integerScale ? fitInteger(left, 2.0f) : fitAspect(left));
        add(false, m_integerScale ? fitInteger(right, 2.0f) : fitAspect(right));
        break;
    }
    case ScreenLayout::TopPriority:
    {
        const RectF topRegion{0.0f, 0.0f, m_integerScale ? 768.0f : 960.0f, kScreenHeight};
        RectF top = m_integerScale ? fitInteger(topRegion, 3.0f) : fitAspect(topRegion);
        const RectF bottomRegion{top.x + top.w, 0.0f, std::max(0.0f, kScreenWidth - (top.x + top.w)), kScreenHeight};
        add(true, top);
        add(false, m_integerScale ? fitInteger(bottomRegion, 2.0f) : fitAspect(bottomRegion));
        break;
    }
    case ScreenLayout::BottomPriority:
    {
        const RectF bottomRegion{m_integerScale ? 512.0f : 320.0f, 0.0f,
                                 m_integerScale ? 768.0f : 960.0f, kScreenHeight};
        RectF bottom = m_integerScale ? fitInteger(bottomRegion, 3.0f) : fitAspect(bottomRegion);
        const RectF topRegion{0.0f, 0.0f, std::max(0.0f, bottom.x), kScreenHeight};
        add(true, m_integerScale ? fitInteger(topRegion, 2.0f) : fitAspect(topRegion));
        add(false, bottom);
        break;
    }
    case ScreenLayout::HybridHorizontal:
    {
        const RectF left{0.0f, 0.0f, kScreenWidth * 0.7f, kScreenHeight};
        const RectF right{kScreenWidth * 0.7f, 0.0f, kScreenWidth * 0.3f, kScreenHeight};
        const RectF rightTop{right.x, right.y, right.w, right.h * 0.5f};
        const RectF rightBottom{right.x, right.y + right.h * 0.5f, right.w, right.h * 0.5f};
        add(true, m_integerScale ? fitInteger(left, 3.0f) : fitAspect(left));
        add(true, m_integerScale ? fitInteger(rightTop, 1.0f) : fitAspect(rightTop));
        add(false, m_integerScale ? fitInteger(rightBottom, 1.0f) : fitAspect(rightBottom));
        break;
    }
    case ScreenLayout::Custom:
    {
        const RectF left{0.0f, 0.0f, kScreenWidth * 0.5f, kScreenHeight};
        const RectF right{kScreenWidth * 0.5f, 0.0f, kScreenWidth * 0.5f, kScreenHeight};
        add(true, fitAspect(left));
        add(false, fitAspect(right));
        break;
    }
    case ScreenLayout::Vertical:
    default:
    {
        const RectF upper{0.0f, 0.0f, kScreenWidth, kScreenHeight * 0.5f};
        const RectF lower{0.0f, kScreenHeight * 0.5f, kScreenWidth, kScreenHeight * 0.5f};
        add(true, m_integerScale ? fitInteger(upper, 1.0f) : fitAspect(upper));
        add(false, m_integerScale ? fitInteger(lower, 1.0f) : fitAspect(lower));
        break;
    }
    }
    return rects;
}

RectF NdsGameLayer::firstRectForSource(bool sourceTop) const
{
    for (const auto& item : computeScreenRects())
    {
        if (item.sourceTop == sourceTop)
            return item.rect;
    }
    return {};
}

bool NdsGameLayer::readTouch(u16& outX, u16& outY) const
{
    HidTouchScreenState state {};
    if (!hidGetTouchScreenStates(&state, 1) || state.count == 0)
        return false;

    const RectF rect = touchRect();
    const float sx = static_cast<float>(state.touches[0].x);
    const float sy = static_cast<float>(state.touches[0].y);
    if (sx < rect.x || sx >= rect.x + rect.w ||
        sy < rect.y || sy >= rect.y + rect.h)
        return false;

    outX = static_cast<u16>(std::clamp((sx - rect.x) * kDsWidth / rect.w, 0.0f, 255.0f));
    outY = static_cast<u16>(std::clamp((sy - rect.y) * kDsHeight / rect.h, 0.0f, 191.0f));
    return true;
}

bool NdsGameLayer::captureCurrentFrameRgba(std::vector<std::uint8_t>& outRgba,
                                           int& outWidth,
                                           int& outHeight) const
{
    if (!m_renderer)
        return false;

    std::vector<std::uint8_t> top;
    std::vector<std::uint8_t> bottom;
    if (!m_renderer->ReadFramebufferRGBA(top, bottom))
        return false;

    const std::vector<std::uint8_t>& upper = m_screensSwapped ? bottom : top;
    const std::vector<std::uint8_t>& lower = m_screensSwapped ? top : bottom;
    constexpr int srcW = 256;
    constexpr int srcH = 192;
    outWidth = srcW;
    outHeight = srcH * 2;
    outRgba.assign(static_cast<size_t>(outWidth) * outHeight * 4, 0);

    for (int y = 0; y < srcH; ++y)
    {
        auto* dst = outRgba.data() + static_cast<size_t>(y) * outWidth * 4;
        const auto* upperRow = upper.data() + static_cast<size_t>(y) * srcW * 4;
        std::copy(upperRow, upperRow + srcW * 4, dst);
    }
    for (int y = 0; y < srcH; ++y)
    {
        auto* dst = outRgba.data() + static_cast<size_t>(y + srcH) * outWidth * 4;
        const auto* lowerRow = lower.data() + static_cast<size_t>(y) * srcW * 4;
        std::copy(lowerRow, lowerRow + srcW * 4, dst);
    }
    return true;
}

void NdsGameLayer::drawScreens() const
{
    if (!m_renderer)
        return;

    const float srcWidth = static_cast<float>(m_renderer->GetFramebufferWidth());
    const float srcHeight = static_cast<float>(m_renderer->GetFramebufferHeight());
    const auto rects = computeScreenRects();

    Gfx::SetSampler((m_linearFiltering ? Gfx::sampler_Linear : Gfx::sampler_Nearest) |
                    Gfx::sampler_ClampToEdge);
    if (m_waitForFramebufferReady)
        Gfx::WaitForFenceReady(m_renderer->FramebufferReady[GPU::FrontBuffer]);
    for (const auto& item : rects)
    {
        const int sourceScreen = item.sourceTop ? 0 : 1;
        Gfx::DrawRectangle(m_framebufferTextures[GPU::FrontBuffer][sourceScreen],
                           {item.rect.x, item.rect.y},
                           {item.rect.w, item.rect.h},
                           {0.0f, 0.0f},
                           {srcWidth, srcHeight},
                           {1.0f, 1.0f, 1.0f, 1.0f});
    }
    Gfx::SignalFence(m_renderer->FramebufferPresented[GPU::FrontBuffer]);
}

} // namespace beiklive::nds_stub
