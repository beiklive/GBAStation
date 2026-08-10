#pragma once

#include <vector>

#include "Animation.hpp"

namespace beiklive
{
    /// 动画管理器：添加/更新/清理动画，不要让 Widget 自己计时
    class AnimationManager
    {
    public:
        void add(Animation anim);
        void update(float delta);
        void clear();

        bool isRunning() const { return !m_animations.empty(); }

    private:
        std::vector<Animation> m_animations;
    };
} // namespace beiklive
