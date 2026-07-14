#pragma once

#include <algorithm>
#include <cmath>

namespace beiklive::pico8_transition
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float SHORTCUT_LEFT_OFFSET = -3.f;
    constexpr float SHORTCUT_WIDTH = 225.f;
    constexpr float SHORTCUT_HEIGHT = 95.f;
    constexpr float SHORTCUT_BOTTOM_MARGIN = 90.f;
    constexpr float SHORTCUT_L_CENTER_X = 37.f;
    constexpr float SHORTCUT_LOGO_X = 78.f;
    constexpr float SHORTCUT_LOGO_WIDTH = 112.f;
    constexpr float SHORTCUT_LOGO_HEIGHT = 40.f;
    constexpr float CENTER_LOGO_SCALE = 2.f;
    constexpr float TRANSITION_DURATION = 0.43f;

    struct Geometry
    {
        float shortcutX = 0.f;
        float shortcutY = 0.f;
        float shortcutWidth = SHORTCUT_WIDTH;
        float shortcutHeight = SHORTCUT_HEIGHT;
        float keyCenterX = 0.f;
        float keyCenterY = 0.f;
        float logoX = 0.f;
        float logoY = 0.f;
        float logoWidth = SHORTCUT_LOGO_WIDTH;
        float logoHeight = SHORTCUT_LOGO_HEIGHT;
        float centerLogoX = 0.f;
        float centerLogoY = 0.f;
        float centerLogoWidth = SHORTCUT_LOGO_WIDTH * CENTER_LOGO_SCALE;
        float centerLogoHeight = SHORTCUT_LOGO_HEIGHT * CENTER_LOGO_SCALE;
    };

    struct LogoPose
    {
        float x = 0.f;
        float y = 0.f;
        float width = 0.f;
        float height = 0.f;
        float rotation = 0.f;
    };

    inline float clamp01(float value)
    {
        return std::max(0.f, std::min(1.f, value));
    }

    inline float easeOutBack(float value)
    {
        value = clamp01(value);
        constexpr float c1 = 1.18f;
        constexpr float c3 = c1 + 1.f;
        const float shifted = value - 1.f;
        return 1.f + c3 * shifted * shifted * shifted +
            c1 * shifted * shifted;
    }

    inline Geometry geometry(float x, float y, float width, float height)
    {
        Geometry result;
        result.shortcutX = x + SHORTCUT_LEFT_OFFSET;
        result.shortcutY = y + height - SHORTCUT_BOTTOM_MARGIN - SHORTCUT_HEIGHT;
        result.keyCenterX = result.shortcutX + SHORTCUT_L_CENTER_X;
        result.keyCenterY = result.shortcutY + SHORTCUT_HEIGHT * 0.5f;
        result.logoX = result.shortcutX + SHORTCUT_LOGO_X;
        result.logoY = result.shortcutY +
            (SHORTCUT_HEIGHT - SHORTCUT_LOGO_HEIGHT) * 0.5f;
        result.centerLogoX = x +
            (width - result.centerLogoWidth) * 0.5f;
        result.centerLogoY = y +
            (height - result.centerLogoHeight) * 0.5f;
        return result;
    }

    inline LogoPose logoPose(const Geometry& geometry, float transition)
    {
        transition = clamp01(transition);
        const float travel = easeOutBack(transition);
        const float arc = -24.f * std::sin(PI * transition);

        LogoPose pose;
        pose.x = geometry.logoX +
            (geometry.centerLogoX - geometry.logoX) * travel;
        pose.y = geometry.logoY +
            (geometry.centerLogoY - geometry.logoY) * travel + arc;
        pose.width = geometry.logoWidth +
            (geometry.centerLogoWidth - geometry.logoWidth) * travel;
        pose.height = geometry.logoHeight +
            (geometry.centerLogoHeight - geometry.logoHeight) * travel;
        pose.rotation = 0.045f * std::sin(2.f * PI * transition);
        return pose;
    }
}
