#pragma once

#include <algorithm>
#include <cmath>
#include <borealis.hpp>
#include "core/common.h"

namespace beiklive
{
namespace ui
{

static constexpr float GRADIENT_FOCUS_FLOW_CYCLE_MS = 1800.0f;
static constexpr float GRADIENT_FOCUS_BRIGHTNESS    = 1.0f;

inline float gradientFocusAnimationOffset(float seconds)
{
    return fmodf((seconds * 1000.0f) / GRADIENT_FOCUS_FLOW_CYCLE_MS, 1.0f);
}

inline int getGradientFocusBorderImage(NVGcontext* vg)
{
    static NVGcontext* cachedContext = nullptr;
    static int cachedImage = 0;

    if (cachedContext != vg)
    {
        cachedContext = vg;
        cachedImage = 0;
    }

    if (cachedImage == 0)
    {
        std::string path = BK_RES("img/ui/border_gradient.png");
        cachedImage = nvgCreateImage(vg, path.c_str(), 0);
    }

    return cachedImage;
}

inline void drawGradientFocusBorder(
    NVGcontext* vg,
    float x,
    float y,
    float w,
    float h,
    float radius,
    float borderWidth,
    float alpha,
    float animationOffset)
{
    if (!vg || alpha <= 0.0f || w <= 0.0f || h <= 0.0f || borderWidth <= 0.0f)
        return;

    float borderAlpha = std::max(0.0f, std::min(alpha * GRADIENT_FOCUS_BRIGHTNESS, 1.0f));

#if defined(BOREALIS_USE_OPENGL) || defined(BOREALIS_USE_DEKO3D)
    int gradientImage = getGradientFocusBorderImage(vg);
    if (gradientImage != 0)
    {
        NVGpaint borderPaint = nvgBoxGradientLUT(
            vg,
            x,
            y,
            w,
            h,
            radius,
            borderWidth,
            gradientImage,
            borderAlpha,
            animationOffset);

        nvgBeginPath(vg);
        nvgRect(vg, x - borderWidth, y - borderWidth, w + borderWidth * 2.0f, h + borderWidth * 2.0f);
        nvgRoundedRect(vg, x, y, w, h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, borderPaint);
        nvgFill(vg);
        return;
    }
#endif

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, radius);
    nvgStrokeColor(vg, nvgRGBA(79, 193, 255, static_cast<unsigned char>(borderAlpha * 255.0f)));
    nvgStrokeWidth(vg, borderWidth);
    nvgStroke(vg);
}

} // namespace ui
} // namespace beiklive
