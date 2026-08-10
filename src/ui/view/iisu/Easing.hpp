#pragma once

#include <algorithm>
#include <cmath>

namespace beiklive
{
    namespace easing
    {
        inline float clamp01(float t)
        {
            return std::max(0.f, std::min(1.f, t));
        }

        /// 快速开始，慢慢停止
        inline float easeOutCubic(float t)
        {
            t = clamp01(t);
            return 1.f - std::pow(1.f - t, 3.f);
        }

        inline float easeInOutCubic(float t)
        {
            t = clamp01(t);
            return t < 0.5f
                ? 4.f * t * t * t
                : 1.f - std::pow(-2.f * t + 2.f, 3.f) * 0.5f;
        }

        /// 超出再回弹（弹跳感）
        inline float easeOutBack(float t)
        {
            t = clamp01(t);
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.f;
            const float v = t - 1.f;
            return 1.f + c3 * v * v * v + c1 * v * v;
        }
    } // namespace easing
} // namespace beiklive
