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
    const float borderWidth = std::max(2.0f, width * 1.8f);
    const float outerPad = borderWidth;
    const Vector2f innerPos = pos + Vector2f{outerPad, outerPad};
    const Vector2f innerSize = size - Vector2f{outerPad * 2.0f, outerPad * 2.0f};
    const float perimeter = std::max(1.0f, innerSize.X * 2.0f + innerSize.Y * 2.0f);

    for (int i = 3; i >= 1; --i)
    {
        const float p = static_cast<float>(i) * 4.0f;
        const float a = 0.045f * static_cast<float>(i);
        drawRect(pos - Vector2f{p, p},
                 size + Vector2f{p * 2.0f, p * 2.0f + 2.0f},
                 {0.0f, 0.0f, 0.0f, a},
                 true);
    }

    auto drawFlowSegment = [&](Vector2f segmentPos, Vector2f segmentSize, float pathCenter) {
        const float lutPos = (pathCenter / perimeter) + animationOffset;
        drawRect(segmentPos, segmentSize, gradientFocusColor(lutPos, alpha));
    };

    constexpr int horizontalSegments = 36;
    constexpr int verticalSegments = 8;
    const float topW = innerSize.X / horizontalSegments;
    const float sideH = innerSize.Y / verticalSegments;

    for (int i = 0; i < horizontalSegments; ++i)
    {
        const float x = i * topW;
        drawFlowSegment({innerPos.X + x, innerPos.Y - borderWidth},
                        {topW + 0.75f, borderWidth},
                        (i + 0.5f) * topW);

        const float bottomX = innerSize.X - (i + 1) * topW;
        drawFlowSegment({innerPos.X + bottomX, innerPos.Y + innerSize.Y},
                        {topW + 0.75f, borderWidth},
                        innerSize.X + innerSize.Y + (i + 0.5f) * topW);
    }

    for (int i = 0; i < verticalSegments; ++i)
    {
        const float y = i * sideH;
        drawFlowSegment({innerPos.X + innerSize.X, innerPos.Y + y},
                        {borderWidth, sideH + 0.75f},
                        innerSize.X + (i + 0.5f) * sideH);

        const float leftY = innerSize.Y - (i + 1) * sideH;
        drawFlowSegment({innerPos.X - borderWidth, innerPos.Y + leftY},
                        {borderWidth, sideH + 0.75f},
                        innerSize.X * 2.0f + innerSize.Y + (i + 0.5f) * sideH);
    }
}

} // namespace beiklive::nds_stub::ui
