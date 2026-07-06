#include "nds_stub/NdsGameLayer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"
#include "nds_stub/NdsShaderCatalog.hpp"

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

RectF fitAspectAligned(const RectF& region, float alignX, float alignY)
{
    RectF rect = fitAspect(region);
    alignX = std::clamp(alignX, 0.0f, 1.0f);
    alignY = std::clamp(alignY, 0.0f, 1.0f);
    rect.x = region.x + (region.w - rect.w) * alignX;
    rect.y = region.y + (region.h - rect.h) * alignY;
    return rect;
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

RectF fitIntegerAligned(const RectF& region, float scale, float alignX, float alignY)
{
    RectF rect = fitInteger(region, scale);
    alignX = std::clamp(alignX, 0.0f, 1.0f);
    alignY = std::clamp(alignY, 0.0f, 1.0f);
    rect.x = region.x + (region.w - rect.w) * alignX;
    rect.y = region.y + (region.h - rect.h) * alignY;
    return rect;
}

RectF fitMaxInteger(const RectF& region)
{
    const float scale = std::max(1.0f, std::floor(std::min(region.w / 256.0f, region.h / 192.0f)));
    return fitInteger(region, scale);
}

RectF customCanvasRect(const RectF& bounds,
                       float canvasW,
                       float canvasH,
                       float baseX,
                       float baseY,
                       float scale,
                       float offsetX,
                       float offsetY)
{
    scale = std::clamp(scale, 1.0f, 10.0f);
    const float dstW = std::max(4.0f, std::round(256.0f * scale / 4.0f) * 4.0f);
    const float dstH = std::max(3.0f, dstW * 3.0f / 4.0f);
    const float dstX = baseX + offsetX - (dstW - 256.0f) * 0.5f;
    const float dstY = baseY + offsetY - (dstH - 192.0f) * 0.5f;
    return {
        bounds.x + bounds.w * (dstX / canvasW),
        bounds.y + bounds.h * (dstY / canvasH),
        bounds.w * (dstW / canvasW),
        bounds.h * (dstH / canvasH),
    };
}

bool validRect(const RectF& rect)
{
    return rect.w > 0.0f && rect.h > 0.0f;
}

Gfx::ShaderMode shaderModeFromType(const std::string& type)
{
    if (isDrasticSimpleShaderType(type))
        return Gfx::shaderMode_NdsDrasticSimple;
    if (type == "dot-clear")
        return Gfx::shaderMode_NdsDotClear;
    if (type == "xbrz-freescale")
        return Gfx::shaderMode_NdsXbrzFreescale;
    if (type == "lcd-grid-v2-nds-color")
        return Gfx::shaderMode_NdsLcdGridNdsColor;
    return Gfx::shaderMode_NdsDot;
}

float clampGap(float gap, float available)
{
    const float limit = std::max(0.0f, available - 2.0f);
    return std::clamp(gap, -limit, limit);
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
    clearOverlayTexture();
    if (m_menuFreezeTexture != 0)
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(m_menuFreezeTexture);
        m_menuFreezeTexture = 0;
    }
    m_menuFreezeEnabled = false;
    m_menuFreezeWidth = 0;
    m_menuFreezeHeight = 0;
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

void NdsGameLayer::setOverlayTexture(std::uint32_t texture, int width, int height)
{
    clearOverlayTexture();
    m_overlayTexture = texture;
    m_overlayWidth = width;
    m_overlayHeight = height;
}

void NdsGameLayer::clearOverlayTexture()
{
    if (m_overlayTexture != 0)
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(m_overlayTexture);
        m_overlayTexture = 0;
    }
    m_overlayWidth = 0;
    m_overlayHeight = 0;
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
    m_layout = static_cast<ScreenLayout>(std::clamp(layout, 0, 7));
}

void NdsGameLayer::setShaderType(const std::string& type)
{
    if (isKnownNdsShaderType(type))
    {
        m_shaderType = type;
    }
    else
    {
        m_shaderType = "dot";
    }
}

std::vector<NdsGameLayer::ScreenDrawRect> NdsGameLayer::computeScreenRects() const
{
    std::vector<ScreenDrawRect> rects;
    const RectF bounds = layoutBounds();
    auto source = [&](bool layoutTop) {
        return m_screensSwapped ? !layoutTop : layoutTop;
    };
    auto add = [&](bool layoutTop, const RectF& rect) {
        if (validRect(rect))
            rects.push_back({source(layoutTop), rotateScreenRect(rect, bounds), rect});
    };

    switch (m_layout)
    {
    case ScreenLayout::Horizontal:
    {
        const float gap = clampGap(m_screenGap, bounds.w);
        const float sideW = (bounds.w - gap) * 0.5f;
        const RectF left{bounds.x, bounds.y, sideW, bounds.h};
        const RectF right{bounds.x + sideW + gap, bounds.y, sideW, bounds.h};

        float integerScale = (m_orientation == 1 || m_orientation == 3) ? 1.0f : 2.0f;

        add(true, m_integerScale ? fitIntegerAligned(left, integerScale, 1.0f, 0.5f) : fitAspectAligned(left, 1.0f, 0.5f));
        add(false, m_integerScale ? fitIntegerAligned(right, integerScale, 0.0f, 0.5f) : fitAspectAligned(right, 0.0f, 0.5f));
        break;
    }
    case ScreenLayout::TopPriority:
    {
        const float gap = clampGap(m_screenGap, bounds.w);
        const float topRegionW = m_integerScale ? 768.0f : std::max(1.0f, bounds.w * 0.75f - gap * 0.5f);
        const RectF topRegion{bounds.x, bounds.y, topRegionW, bounds.h};
        RectF top = m_integerScale ? fitIntegerAligned(topRegion, 3.0f, 1.0f, 0.5f) : fitAspectAligned(topRegion, 1.0f, 0.5f);
        const RectF bottomRegion{top.x + top.w + gap, bounds.y, std::max(0.0f, bounds.x + bounds.w - (top.x + top.w + gap)), bounds.h};
        add(true, top);
        add(false, m_integerScale ? fitIntegerAligned(bottomRegion, 2.0f, 0.0f, 0.5f) : fitAspectAligned(bottomRegion, 0.0f, 0.5f));
        break;
    }
    case ScreenLayout::BottomPriority:
    {
        const float gap = clampGap(m_screenGap, bounds.w);
        const float bottomRegionW = m_integerScale ? 768.0f : std::max(1.0f, bounds.w * 0.75f - gap * 0.5f);
        const RectF bottomRegion{bounds.x + bounds.w - bottomRegionW,
                                 bounds.y,
                                 bottomRegionW,
                                 bounds.h};
        RectF bottom = m_integerScale ? fitIntegerAligned(bottomRegion, 3.0f, 0.0f, 0.5f) : fitAspectAligned(bottomRegion, 0.0f, 0.5f);
        const RectF topRegion{bounds.x, bounds.y, std::max(0.0f, bottom.x - bounds.x - gap), bounds.h};
        add(true, m_integerScale ? fitIntegerAligned(topRegion, 2.0f, 1.0f, 0.5f) : fitAspectAligned(topRegion, 1.0f, 0.5f));
        add(false, bottom);
        break;
    }
    case ScreenLayout::HybridHorizontal:
    {
        const float gap = clampGap(m_screenGap, bounds.w);
        constexpr float leftWeight = 10.0f / 3.0f;
        constexpr float rightWeight = 5.0f / 3.0f;
        constexpr float totalWeight = leftWeight + rightWeight;
        const float leftW = (bounds.w - gap) * (leftWeight / totalWeight);
        const float rightW = bounds.w - gap - leftW;
        const RectF left{bounds.x, bounds.y, leftW, bounds.h};
        const RectF right{bounds.x + leftW + gap, bounds.y, rightW, bounds.h};
        const float verticalGap = clampGap(m_screenGap, right.h);
        const float rightScreenH = (right.h - verticalGap) * 0.5f;
        const RectF rightTop{right.x, right.y, right.w, rightScreenH};
        const RectF rightBottom{right.x, right.y + rightScreenH + verticalGap, right.w, rightScreenH};

        add(true, m_integerScale ? fitIntegerAligned(left, 2.0f, 1.0f, 0.5f) : fitAspectAligned(left, 1.0f, 0.5f));
        add(true, m_integerScale ? fitIntegerAligned(rightTop, 1.0f, 0.0f, 1.0f) : fitAspectAligned(rightTop, 0.0f, 1.0f));
        add(false, m_integerScale ? fitIntegerAligned(rightBottom, 1.0f, 0.0f, 0.0f) : fitAspectAligned(rightBottom, 0.0f, 0.0f));
        break;
    }
    case ScreenLayout::SingleTop:
    {
        add(true, m_integerScale ? fitMaxInteger(bounds) : fitAspect(bounds));
        break;
    }
    case ScreenLayout::SingleBottom:
    {
        add(false, m_integerScale ? fitMaxInteger(bounds) : fitAspect(bounds));
        break;
    }
    case ScreenLayout::Custom:
    {
        const bool portraitCanvas = m_orientation == 1 || m_orientation == 3;
        const float canvasW = portraitCanvas ? 720.0f : 1280.0f;
        const float canvasH = portraitCanvas ? 1280.0f : 720.0f;
        const float topBaseX = portraitCanvas ? (canvasW - 256.0f) * 0.5f : 224.0f;
        const float topBaseY = portraitCanvas ? (canvasH * 0.5f - 192.0f) : 264.0f;
        const float bottomBaseX = portraitCanvas ? topBaseX : 800.0f;
        const float bottomBaseY = portraitCanvas ? (canvasH * 0.5f) : 264.0f;
        add(true, customCanvasRect(bounds,
                                   canvasW,
                                   canvasH,
                                   topBaseX,
                                   topBaseY,
                                   m_customLayout.topScale,
                                   m_customLayout.topOffsetX,
                                   m_customLayout.topOffsetY));
        add(false, customCanvasRect(bounds,
                                    canvasW,
                                    canvasH,
                                    bottomBaseX,
                                    bottomBaseY,
                                    m_customLayout.bottomScale,
                                    m_customLayout.bottomOffsetX,
                                    m_customLayout.bottomOffsetY));
        break;
    }
    case ScreenLayout::Vertical:
    default:
    {
        const float gap = clampGap(m_screenGap, bounds.h);
        const float sideH = (bounds.h - gap) * 0.5f;
        const RectF upper{bounds.x, bounds.y, bounds.w, sideH};
        const RectF lower{bounds.x, bounds.y + sideH + gap, bounds.w, sideH};

        float integerScale = (m_orientation == 1 || m_orientation == 3) ? 2.0f : 1.0f;

        add(true, m_integerScale ? fitIntegerAligned(upper, integerScale, 0.5f, 1.0f) : fitAspectAligned(upper, 0.5f, 1.0f));
        add(false, m_integerScale ? fitIntegerAligned(lower, integerScale, 0.5f, 0.0f) : fitAspectAligned(lower, 0.5f, 0.0f));
        break;
    }
    }
    return rects;
}

RectF NdsGameLayer::layoutBounds() const
{
    if (m_orientation == 1 || m_orientation == 3)
    {
        const float cx = kScreenWidth * 0.5f;
        const float cy = kScreenHeight * 0.5f;
        return {cx - kScreenHeight * 0.5f,
                cy - kScreenWidth * 0.5f,
                kScreenHeight,
                kScreenWidth};
    }
    return {0.0f, 0.0f, kScreenWidth, kScreenHeight};
}

RectF NdsGameLayer::rotateScreenRect(const RectF& rect, const RectF& layoutRect) const
{
    if (m_orientation == 0)
        return rect;

    const RectF oriented{0.0f, 0.0f, kScreenWidth, kScreenHeight};
    const float relX = rect.x - layoutRect.x;
    const float relY = rect.y - layoutRect.y;

    if (m_orientation == 2)
    {
        return {oriented.x + (layoutRect.w - relX - rect.w),
                oriented.y + (layoutRect.h - relY - rect.h),
                rect.w,
                rect.h};
    }
    if (m_orientation == 1)
    {
        return {oriented.x + (layoutRect.h - relY - rect.h),
                oriented.y + relX,
                rect.h,
                rect.w};
    }
    if (m_orientation == 3)
    {
        return {oriented.x + relY,
                oriented.y + (layoutRect.w - relX - rect.w),
                rect.h,
                rect.w};
    }
    return rect;
}

bool NdsGameLayer::mapPointToUnrotated(float x, float y, const ScreenDrawRect& item, float& outX, float& outY) const
{
    const RectF& oriented = item.rect;
    const RectF& layout = item.layoutRect;
    if (oriented.w <= 0.0f || oriented.h <= 0.0f ||
        x < oriented.x || y < oriented.y ||
        x >= oriented.x + oriented.w || y >= oriented.y + oriented.h)
        return false;

    const float rx = x - oriented.x;
    const float ry = y - oriented.y;
    if (m_orientation == 1)
    {
        outX = layout.x + ry;
        outY = layout.y + (layout.h - rx);
    }
    else if (m_orientation == 3)
    {
        outX = layout.x + (layout.w - ry);
        outY = layout.y + rx;
    }
    else if (m_orientation == 2)
    {
        outX = layout.x + (layout.w - rx);
        outY = layout.y + (layout.h - ry);
    }
    else
    {
        outX = layout.x + rx;
        outY = layout.y + ry;
    }
    return true;
}

bool NdsGameLayer::mapNdsPointToScreen(float ndsX, float ndsY, const ScreenDrawRect& item, float& outX, float& outY) const
{
    const RectF& oriented = item.rect;
    if (oriented.w <= 0.0f || oriented.h <= 0.0f)
        return false;

    const float u = std::clamp(ndsX / 255.0f, 0.0f, 1.0f);
    const float v = std::clamp(ndsY / 191.0f, 0.0f, 1.0f);
    if (m_orientation == 1)
    {
        outX = oriented.x + oriented.w * (1.0f - v);
        outY = oriented.y + oriented.h * u;
    }
    else if (m_orientation == 3)
    {
        outX = oriented.x + oriented.w * v;
        outY = oriented.y + oriented.h * (1.0f - u);
    }
    else if (m_orientation == 2)
    {
        outX = oriented.x + oriented.w * (1.0f - u);
        outY = oriented.y + oriented.h * (1.0f - v);
    }
    else
    {
        outX = oriented.x + oriented.w * u;
        outY = oriented.y + oriented.h * v;
    }
    return true;
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

bool NdsGameLayer::ndsPointToScreen(bool sourceTop, float ndsX, float ndsY, float& outX, float& outY) const
{
    for (const auto& item : computeScreenRects())
    {
        if (item.sourceTop == sourceTop)
            return mapNdsPointToScreen(ndsX, ndsY, item, outX, outY);
    }
    return false;
}

bool NdsGameLayer::readTouch(u16& outX, u16& outY) const
{
    HidTouchScreenState state {};
    if (!hidGetTouchScreenStates(&state, 1) || state.count == 0)
        return false;

    const float sx = static_cast<float>(state.touches[0].x);
    const float sy = static_cast<float>(state.touches[0].y);

    for (const auto& item : computeScreenRects())
    {
        if (item.sourceTop)
            continue;

        float ux = 0.0f;
        float uy = 0.0f;
        if (!mapPointToUnrotated(sx, sy, item, ux, uy))
            continue;

        outX = static_cast<u16>(std::clamp((ux - item.layoutRect.x) * kDsWidth / item.layoutRect.w, 0.0f, 255.0f));
        outY = static_cast<u16>(std::clamp((uy - item.layoutRect.y) * kDsHeight / item.layoutRect.h, 0.0f, 191.0f));
        return true;
    }
    return false;
}

bool NdsGameLayer::captureCurrentFrameRgba(std::vector<std::uint8_t>& outRgba,
                                           int& outWidth,
                                           int& outHeight) const
{
    if (refreshCaptureCache())
    {
        outRgba = m_lastCaptureRgba;
        outWidth = m_lastCaptureWidth;
        outHeight = m_lastCaptureHeight;
        return true;
    }

    if (m_lastCaptureRgba.empty() || m_lastCaptureWidth <= 0 || m_lastCaptureHeight <= 0)
        return false;
    outRgba = m_lastCaptureRgba;
    outWidth = m_lastCaptureWidth;
    outHeight = m_lastCaptureHeight;
    return true;
}

bool NdsGameLayer::refreshCaptureCache() const
{
    if (!m_renderer)
        return false;

    std::vector<std::uint8_t> top;
    std::vector<std::uint8_t> bottom;
    if (!m_renderer->TakeCapturedFramebufferRGBA(top, bottom) &&
        !m_renderer->ReadFramebufferRGBA(top, bottom))
        return false;

    const std::vector<std::uint8_t>& upper = m_screensSwapped ? bottom : top;
    const std::vector<std::uint8_t>& lower = m_screensSwapped ? top : bottom;
    constexpr int srcW = 256;
    constexpr int srcH = 192;
    std::vector<std::uint8_t> rgba(static_cast<size_t>(srcW) * srcH * 2 * 4, 0);

    for (int y = 0; y < srcH; ++y)
    {
        auto* dst = rgba.data() + static_cast<size_t>(y) * srcW * 4;
        const auto* upperRow = upper.data() + static_cast<size_t>(y) * srcW * 4;
        std::copy(upperRow, upperRow + srcW * 4, dst);
    }
    for (int y = 0; y < srcH; ++y)
    {
        auto* dst = rgba.data() + static_cast<size_t>(y + srcH) * srcW * 4;
        const auto* lowerRow = lower.data() + static_cast<size_t>(y) * srcW * 4;
        std::copy(lowerRow, lowerRow + srcW * 4, dst);
    }
    m_lastCaptureRgba = std::move(rgba);
    m_lastCaptureWidth = srcW;
    m_lastCaptureHeight = srcH * 2;
    return true;
}

void NdsGameLayer::requestDeferredCapture() const
{
    if (m_renderer)
        m_renderer->RequestFramebufferCapture();
}

bool NdsGameLayer::refreshMenuFreezeTexture()
{
    std::vector<std::uint8_t> rgba;
    int width = 0;
    int height = 0;
    if (!captureCurrentFrameRgba(rgba, width, height) || rgba.empty() || width <= 0 || height <= 0)
        return false;

    if (m_menuFreezeTexture != 0 && (m_menuFreezeWidth != width || m_menuFreezeHeight != height))
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(m_menuFreezeTexture);
        m_menuFreezeTexture = 0;
        m_menuFreezeWidth = 0;
        m_menuFreezeHeight = 0;
    }

    if (m_menuFreezeTexture == 0)
    {
        m_menuFreezeTexture = Gfx::TextureCreate(static_cast<u32>(width),
                                                 static_cast<u32>(height),
                                                 DkImageFormat_RGBA8_Unorm);
        m_menuFreezeWidth = width;
        m_menuFreezeHeight = height;
    }

    if (m_menuFreezeTexture == 0)
        return false;

    Gfx::TextureUpload(m_menuFreezeTexture,
                       0,
                       0,
                       static_cast<u32>(width),
                       static_cast<u32>(height),
                       rgba.data(),
                       static_cast<u32>(width * 4));
    m_menuFreezeEnabled = true;
    return true;
}

void NdsGameLayer::drawScreenTexture(const ScreenDrawRect& item,
                                     std::uint32_t texture,
                                     const RectF& sourceRect) const
{
    const Gfx::Vector2f subPosition{sourceRect.x, sourceRect.y};
    const Gfx::Vector2f subSize{sourceRect.w, sourceRect.h};
    if (m_orientation == 0)
    {
        Gfx::DrawRectangle(texture,
                           {item.rect.x, item.rect.y},
                           {item.rect.w, item.rect.h},
                           subPosition,
                           subSize,
                           {1.0f, 1.0f, 1.0f, 1.0f});
        return;
    }

    const Gfx::Vector2f tl{item.rect.x, item.rect.y};
    const Gfx::Vector2f tr{item.rect.x + item.rect.w, item.rect.y};
    const Gfx::Vector2f bl{item.rect.x, item.rect.y + item.rect.h};
    const Gfx::Vector2f br{item.rect.x + item.rect.w, item.rect.y + item.rect.h};
    Gfx::Vector2f p0 = tl;
    Gfx::Vector2f p1 = tr;
    Gfx::Vector2f p2 = bl;
    Gfx::Vector2f p3 = br;
    if (m_orientation == 1)
    {
        p0 = tr;
        p1 = br;
        p2 = tl;
        p3 = bl;
    }
    else if (m_orientation == 2)
    {
        p0 = br;
        p1 = bl;
        p2 = tr;
        p3 = tl;
    }
    else if (m_orientation == 3)
    {
        p0 = bl;
        p1 = tl;
        p2 = br;
        p3 = tr;
    }
    Gfx::DrawRectangle(texture,
                       p0,
                       p1,
                       p2,
                       p3,
                       subPosition,
                       subSize);
}

void NdsGameLayer::drawScreens() const
{
    if (!m_renderer)
        return;

    const float liveSrcWidth = static_cast<float>(m_renderer->GetFramebufferWidth());
    const float liveSrcHeight = static_cast<float>(m_renderer->GetFramebufferHeight());
    const auto rects = computeScreenRects();
    const bool useFreeze = m_menuFreezeEnabled && m_menuFreezeTexture != 0 &&
                           m_menuFreezeWidth >= kDsWidth &&
                           m_menuFreezeHeight >= kDsHeight * 2;

    const bool useShader = m_shaderEnabled;
    const u32 screenSampler = (useShader ? Gfx::sampler_Nearest :
                              (m_linearFiltering ? Gfx::sampler_Linear : Gfx::sampler_Nearest)) |
                              Gfx::sampler_ClampToEdge;
    Gfx::SetSampler(screenSampler);
    Gfx::SetNdsShaderParams(m_shaderParams);
    Gfx::SetShaderMode(useShader ? shaderModeFromType(m_shaderType) : Gfx::shaderMode_Default);
    if (m_waitForFramebufferReady)
        Gfx::WaitForFenceReady(m_renderer->FramebufferReady[GPU::FrontBuffer]);
    for (const auto& item : rects)
    {
        const int sourceScreen = item.sourceTop ? 0 : 1;
        if (useFreeze)
        {
            drawScreenTexture(item,
                              m_menuFreezeTexture,
                              {0.0f,
                               static_cast<float>(sourceScreen * kDsHeight),
                               static_cast<float>(kDsWidth),
                               static_cast<float>(kDsHeight)});
        }
        else
        {
            const u32 texture = m_framebufferTextures[GPU::FrontBuffer][sourceScreen];
            drawScreenTexture(item,
                              texture,
                              {0.0f, 0.0f, liveSrcWidth, liveSrcHeight});
        }
    }
    Gfx::SetShaderMode(Gfx::shaderMode_Default);
    Gfx::SetSampler(Gfx::sampler_Nearest | Gfx::sampler_ClampToEdge);

    if (m_overlayEnabled && m_overlayTexture != 0 && m_overlayWidth > 0 && m_overlayHeight > 0)
    {
        Gfx::SetSampler(Gfx::sampler_Linear | Gfx::sampler_ClampToEdge);
        Gfx::DrawRectangle(m_overlayTexture,
                           {0.0f, 0.0f},
                           {static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)},
                           {0.0f, 0.0f},
                           {static_cast<float>(m_overlayWidth), static_cast<float>(m_overlayHeight)},
                           {1.0f, 1.0f, 1.0f, 1.0f});
    }
    Gfx::SetSampler(Gfx::sampler_Nearest | Gfx::sampler_ClampToEdge);
    Gfx::SignalFence(m_renderer->FramebufferPresented[GPU::FrontBuffer]);
}

} // namespace beiklive::nds_stub
