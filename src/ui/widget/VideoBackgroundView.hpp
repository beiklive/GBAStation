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

        // The selected video is process-global: page changes attach to the
        // same decoder, NanoVG texture and playback clock instead of opening
        // the file again.  A new non-video background releases this cache.
        static bool hasCachedVideo(const std::string& path);
        static void clearCachedVideo();
        bool isCurrentCachedVideo(const std::string& path) const;
        // Used while an emulator core owns CPU time. The decoded texture is
        // retained, but the worker stops filling frames until UI resumes.
        static void setSharedPlaybackPaused(bool paused);

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

    public:
        struct SharedVideo;

    private:
        std::shared_ptr<SharedVideo> m_video;
    };
} // namespace beiklive
