#pragma once

#include <borealis.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace beiklive
{
    /// 全屏 GIF 背景视图。纹理和播放时钟由所有实例全局共享。
    class GifBackgroundView : public brls::View
    {
    public:
        GifBackgroundView() = default;
        ~GifBackgroundView() override = default;

        // Interactive selections start decoding immediately. Startup callers
        // may retain the small defer to keep their first frame responsive.
        bool load(const std::string& path, bool immediate = false);
        /// Unbind the current view without stopping the process-global
        /// animation. This keeps frame progress stable across page changes.
        void clear();
        bool isLoaded() const;
        const std::string& path() const;
        static bool hasCachedAnimation(const std::string& path);
        static void clearCachedAnimation();
        bool isCurrentCachedAnimation(const std::string& path) const;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

    public:
        struct SharedAnimation;

    private:
        std::shared_ptr<SharedAnimation> m_animation;
    };
} // namespace beiklive
