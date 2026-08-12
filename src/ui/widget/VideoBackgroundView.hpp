#pragma once

#include <borealis.hpp>

#include <memory>
#include <string>

namespace beiklive
{
    /// Full-screen MP4 background. Decoder state, texture and timeline are
    /// shared by every Box using the same file.
    class VideoBackgroundView : public brls::View
    {
    public:
        VideoBackgroundView() = default;
        ~VideoBackgroundView() override = default;

        bool load(const std::string& path);
        void clear();
        bool isLoaded() const;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

    public:
        struct SharedVideo;

    private:
        std::shared_ptr<SharedVideo> m_video;
    };
} // namespace beiklive
