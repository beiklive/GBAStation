#include "nds_stub/ui/UiPrimitives.hpp"

namespace beiklive::nds_stub::ui {

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
    const float animationOffset = gradientFocusAnimationOffset();
    const float alpha = 1.0f;
    const float perimeter = size.X * 2.0f + size.Y * 2.0f;

    auto drawFlowSegment = [&](Vector2f segmentPos, Vector2f segmentSize, float pathCenter) {
        const float lutPos = (pathCenter / perimeter) + animationOffset;
        drawRect(segmentPos, segmentSize, gradientFocusColor(lutPos, alpha));
    };

    constexpr int horizontalSegments = 28;
    constexpr int verticalSegments = 6;
    const float topW = size.X / horizontalSegments;
    const float sideH = size.Y / verticalSegments;

    for (int i = 0; i < horizontalSegments; ++i)
    {
        const float x = i * topW;
        drawFlowSegment({pos.X + x, pos.Y}, {topW + 0.75f, width}, (i + 0.5f) * topW);

        const float bottomX = size.X - (i + 1) * topW;
        drawFlowSegment({pos.X + bottomX, pos.Y + size.Y - width}, {topW + 0.75f, width},
                        size.X + size.Y + (i + 0.5f) * topW);
    }

    for (int i = 0; i < verticalSegments; ++i)
    {
        const float y = i * sideH;
        drawFlowSegment({pos.X + size.X - width, pos.Y + y}, {width, sideH + 0.75f},
                        size.X + (i + 0.5f) * sideH);

        const float leftY = size.Y - (i + 1) * sideH;
        drawFlowSegment({pos.X, pos.Y + leftY}, {width, sideH + 0.75f},
                        size.X * 2.0f + size.Y + (i + 0.5f) * sideH);
    }

    drawRect({pos.X + 10.0f, pos.Y + (size.Y - 40.0f) * 0.5f}, {5.0f, 40.0f},
             gradientFocusColor(animationOffset, 1.0f));
}

} // namespace beiklive::nds_stub::ui
