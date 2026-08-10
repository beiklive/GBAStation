#include "AnimationManager.hpp"

#include <algorithm>

#include "Easing.hpp"

namespace beiklive
{
    void AnimationManager::add(Animation anim)
    {
        if (anim.duration <= 0.f)
            return;
        m_animations.push_back(std::move(anim));
    }

    void AnimationManager::update(float delta)
    {
        for (auto& anim : m_animations) {
            if (anim.finished)
                continue;
            anim.elapsed += delta;
            const float t = easing::clamp01(anim.elapsed / anim.duration);
            if (anim.update)
                anim.update(t);
            if (anim.elapsed >= anim.duration) {
                anim.finished = true;
                if (anim.onFinished)
                    anim.onFinished();
            }
        }

        m_animations.erase(
            std::remove_if(m_animations.begin(), m_animations.end(),
                           [](const Animation& anim) {
                               return anim.finished;
                           }),
            m_animations.end());
    }

    void AnimationManager::clear()
    {
        m_animations.clear();
    }
} // namespace beiklive
