#pragma once

#include <functional>

namespace beiklive
{
    /// 动画：统一由 AnimationManager 驱动，update 回调收到进度 t ∈ [0,1]
    struct Animation
    {
        float duration = 0.f;
        float elapsed = 0.f;
        std::function<void(float)> update;      // 进度回调
        std::function<void()> onFinished;       // 结束回调
        bool finished = false;
    };
} // namespace beiklive
