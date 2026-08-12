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

        bool load(const std::string& path);
        /// 解绑当前背景；全局缓存保留，供下一个页面零开销复用。
        void clear();
        bool isLoaded() const;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

    public:
        struct Frame
        {
            int texture = 0;
            uint32_t delayMs = 100;
        };

        struct SharedAnimation;

    private:
        std::shared_ptr<SharedAnimation> m_animation;
    };
} // namespace beiklive
