#include "nds_stub/ui/UiPrimitives.hpp"

#include <array>
#include <cstdio>
#include <string>

#include "nds_stub/StubLog.hpp"
#include "stb/stb_image.h"

namespace beiklive::nds_stub::ui {

namespace {

constexpr float kGradientFocusBorderWidthScale = 2.0f;

struct HighlightGradientTexture {
    bool attempted = false;
    std::uint32_t texture = 0;
    int width = 0;
    int height = 0;
};

HighlightGradientTexture gHighlightGradient;

bool fileExists(const char* path)
{
    FILE* fp = std::fopen(path, "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

const HighlightGradientTexture& getHighlightGradientTexture()
{
    if (gHighlightGradient.attempted)
        return gHighlightGradient;

    gHighlightGradient.attempted = true;

    constexpr std::array<const char*, 4> paths = {
        "romfs:/img/ui/border_gradient.png",
        "sdmc:/GBAStation/resources/img/ui/border_gradient.png",
        "/GBAStation/resources/img/ui/border_gradient.png",
        "resources/img/ui/border_gradient.png",
    };

    const char* openedPath = nullptr;
    for (const char* path : paths)
    {
        if (fileExists(path))
        {
            openedPath = path;
            break;
        }
    }

    if (!openedPath)
    {
        appendStubLog("GBAStationNDSStub: highlight gradient image not found");
        return gHighlightGradient;
    }

    int comp = 0;
    unsigned char* pixels = stbi_load(openedPath,
                                      &gHighlightGradient.width,
                                      &gHighlightGradient.height,
                                      &comp,
                                      4);
    if (!pixels ||
        gHighlightGradient.width <= 0 ||
        gHighlightGradient.height <= 0 ||
        gHighlightGradient.width > 4096 ||
        gHighlightGradient.height > 4096)
    {
        if (pixels)
            stbi_image_free(pixels);
        appendStubLog("GBAStationNDSStub: highlight gradient decode failed path=%s reason=%s",
                      openedPath,
                      stbi_failure_reason() ? stbi_failure_reason() : "(null)");
        gHighlightGradient.width = 0;
        gHighlightGradient.height = 0;
        return gHighlightGradient;
    }

    gHighlightGradient.texture = Gfx::TextureCreate(static_cast<u32>(gHighlightGradient.width),
                                                    static_cast<u32>(gHighlightGradient.height),
                                                    DkImageFormat_RGBA8_Unorm);
    Gfx::TextureUpload(gHighlightGradient.texture,
                       0,
                       0,
                       static_cast<u32>(gHighlightGradient.width),
                       static_cast<u32>(gHighlightGradient.height),
                       pixels,
                       static_cast<u32>(gHighlightGradient.width * 4));
    stbi_image_free(pixels);

    appendStubLog("GBAStationNDSStub: highlight gradient loaded path=%s size=%dx%d tex=%u",
                  openedPath,
                  gHighlightGradient.width,
                  gHighlightGradient.height,
                  gHighlightGradient.texture);
    return gHighlightGradient;
}

void drawFallbackGradientBorder(Vector2f pos, Vector2f size, float width)
{
    const float animationOffset = gradientFocusAnimationOffset();
    const Color c1 = gradientFocusColor(animationOffset + 0.16f, 1.0f);
    const float borderWidth = std::max(2.0f, width);

    drawBorder(pos - Vector2f{borderWidth, borderWidth},
               size + Vector2f{borderWidth * 2.0f, borderWidth * 2.0f},
               borderWidth,
               {c1.R, c1.G, c1.B, 0.95f});
}

void drawGradientQuad(const HighlightGradientTexture& gradient,
                      Vector2f p0,
                      Vector2f p1,
                      Vector2f p2,
                      Vector2f p3,
                      float uvStart,
                      float uvLength,
                      Color tint)
{
    Gfx::DrawRectangle(gradient.texture,
                       p0,
                       p1,
                       p2,
                       p3,
                       {uvStart, 0.0f},
                       {std::max(1.0f, uvLength), static_cast<float>(gradient.height)},
                       tint);
}

} // namespace

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float easeOutCubic(float t)
{
    t = clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float easeOutQuart(float t)
{
    t = clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv * inv;
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float animationProgress(std::uint64_t startTick, float durationMs)
{
    if (startTick == 0)
        return 1.0f;

    const std::uint64_t elapsedTicks = armGetSystemTick() - startTick;
    const double elapsedMs = static_cast<double>(armTicksToNs(elapsedTicks)) / 1000000.0;
    return clamp01(static_cast<float>(elapsedMs / durationMs));
}

float gradientFocusAnimationOffset()
{
    const double ms = static_cast<double>(armTicksToNs(armGetSystemTick())) / 1000000.0;
    return std::fmod(static_cast<float>(ms) / kGradientFocusFlowCycleMs, 1.0f);
}

Color mixColor(Color a, Color b, float t)
{
    t = clamp01(t);
    return {
        lerp(a.R, b.R, t),
        lerp(a.G, b.G, t),
        lerp(a.B, b.B, t),
        lerp(a.A, b.A, t),
    };
}

Color gradientFocusColor(float offset, float alpha)
{
    offset = offset - std::floor(offset);

    struct Stop {
        float pos;
        Color color;
    };

    constexpr int stopCount = 6;
    const Stop stops[stopCount] = {
        {0.00f, {0.31f, 0.76f, 1.00f, 1.0f}},
        {0.18f, {0.25f, 0.95f, 0.86f, 1.0f}},
        {0.38f, {0.72f, 0.46f, 1.00f, 1.0f}},
        {0.58f, {1.00f, 0.42f, 0.82f, 1.0f}},
        {0.78f, {0.38f, 0.63f, 1.00f, 1.0f}},
        {1.00f, {0.31f, 0.76f, 1.00f, 1.0f}},
    };

    Color color = stops[0].color;
    for (int i = 0; i < stopCount - 1; ++i)
    {
        if (offset >= stops[i].pos && offset <= stops[i + 1].pos)
        {
            const float localT = (offset - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            color = mixColor(stops[i].color, stops[i + 1].color, localT);
            break;
        }
    }

    color.R *= kGradientFocusBrightness;
    color.G *= kGradientFocusBrightness;
    color.B *= kGradientFocusBrightness;
    color.A = alpha;
    return color;
}

void drawRect(Vector2f pos, Vector2f size, Color color, bool cool)
{
    Gfx::DrawRectangle(pos, size, color, cool);
}

void drawLine(Vector2f pos, Vector2f size, Color color)
{
    Gfx::DrawRectangle(pos, size, color);
}

void drawBorder(Vector2f pos, Vector2f size, float width, Color color)
{
    drawRect(pos, {size.X, width}, color);
    drawRect({pos.X, pos.Y + size.Y - width}, {size.X, width}, color);
    drawRect(pos, {width, size.Y}, color);
    drawRect({pos.X + size.X - width, pos.Y}, {width, size.Y}, color);
}

void drawGradientBorder(Vector2f pos, Vector2f size, float width)
{
    const HighlightGradientTexture& gradient = getHighlightGradientTexture();
    if (gradient.texture == 0 || gradient.width <= 0 || gradient.height <= 0)
    {
        drawFallbackGradientBorder(pos, size, width);
        return;
    }

    const float animationOffset = gradientFocusAnimationOffset();
    const float borderWidth = std::max(4.0f, width * kGradientFocusBorderWidthScale);
    const float x = pos.X;
    const float y = pos.Y;
    const float w = size.X;
    const float h = size.Y;
    const float uvOffset = animationOffset * static_cast<float>(gradient.width);
    const Color borderTint{1.0f, 1.0f, 1.0f, 0.98f};
    const float topLen = std::max(1.0f, w);
    const float sideLen = std::max(1.0f, h);
    float uv = uvOffset;

    Gfx::SetSampler(Gfx::sampler_Linear | Gfx::sampler_Repeat);

    drawGradientQuad(gradient,
                     {x - borderWidth, y - borderWidth},
                     {x + w + borderWidth, y - borderWidth},
                     {x - borderWidth, y},
                     {x + w + borderWidth, y},
                     uv,
                     topLen + borderWidth * 2.0f,
                     borderTint);
    uv += topLen + borderWidth * 2.0f;

    drawGradientQuad(gradient,
                     {x + w, y},
                     {x + w, y + h},
                     {x + w + borderWidth, y},
                     {x + w + borderWidth, y + h},
                     uv,
                     sideLen,
                     borderTint);
    uv += sideLen;

    drawGradientQuad(gradient,
                     {x + w + borderWidth, y + h},
                     {x - borderWidth, y + h},
                     {x + w + borderWidth, y + h + borderWidth},
                     {x - borderWidth, y + h + borderWidth},
                     uv,
                     topLen + borderWidth * 2.0f,
                     borderTint);
    uv += topLen + borderWidth * 2.0f;

    drawGradientQuad(gradient,
                     {x - borderWidth, y + h},
                     {x - borderWidth, y},
                     {x, y + h},
                     {x, y},
                     uv,
                     sideLen,
                     borderTint);

    Gfx::SetSampler(Gfx::sampler_Nearest | Gfx::sampler_ClampToEdge);
}

} // namespace beiklive::nds_stub::ui
