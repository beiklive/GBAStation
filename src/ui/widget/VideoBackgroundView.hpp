#pragma once

#include <borealis.hpp>

#include <memory>
#include <string>

namespace beiklive
{
    /// Full-screen MP4 background. Decoder state, texture and timeline are
    /// process-global. Each page view automatically follows the currently
    /// active player instead of retaining a stale player after a replacement.
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
        // Starts decoding a replacement while the current background is
        // fading out. This performs no NanoVG calls.
        static bool preload(const std::string& path);
        static void keepCachedVideo(const std::string& path);
        static void clearCachedVideo();
        // Must run on the UI thread before Borealis tears down its NanoVG
        // context. It stops any outstanding decoder work and releases the
        // shared textures synchronously at application shutdown.
        static void shutdownSharedVideo();
        bool isCurrentCachedVideo(const std::string& path) const;
        // Used while an emulator core owns CPU time. The decoded texture is
        // retained, but the worker stops filling frames until UI resumes.
        static void setSharedPlaybackPaused(bool paused);
        /// Stops/restarts the shared background audio output around GamePage.
        static void setSharedAudioSuspended(bool suspended);
        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

    public:
        struct SharedVideo;

    private:
        // Playback ownership lives entirely in the process-global manager.
        // Views are transient page surfaces and must never retain a decoder
        // or NanoVG texture across a background replacement.
        uint64_t m_observedGeneration = 0;
    };
} // namespace beiklive
