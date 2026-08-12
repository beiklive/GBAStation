#include "VideoBackgroundView.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core/common.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace beiklive
{
    struct VideoBackgroundView::SharedVideo
    {
        ~SharedVideo()
        {
            NVGcontext* vg = brls::Application::getNVGContext();
            if (vg && texture > 0)
                nvgDeleteImage(vg, texture);
            sws_freeContext(sws);
            av_packet_free(&packet);
            av_frame_free(&frame);
            avcodec_free_context(&codec);
            avformat_close_input(&format);
        }

        std::string path;
        AVFormatContext* format = nullptr;
        AVCodecContext* codec = nullptr;
        AVFrame* frame = nullptr;
        AVPacket* packet = nullptr;
        SwsContext* sws = nullptr;
        int streamIndex = -1;
        AVRational timeBase{0, 1};
        int width = 0;
        int height = 0;
        int texture = 0;
        std::vector<uint8_t> rgba;
        std::chrono::steady_clock::time_point nextFrameAt{};
        double lastPts = -1.0;
        double defaultFrameDuration = 1.0 / 30.0;
        bool hasFrame = false;
    };

    namespace
    {
        using SharedVideo = VideoBackgroundView::SharedVideo;
        std::unordered_map<std::string, std::weak_ptr<SharedVideo>> g_backgroundVideoCache;

        constexpr double kMinFrameDuration = 1.0 / 240.0;
        constexpr double kMaxFrameDuration = 0.25;

        int scaledDimension(int sourceWidth, int sourceHeight, bool horizontal)
        {
#ifdef __SWITCH__
            constexpr int kMaxVideoEdge = 384;
#else
            constexpr int kMaxVideoEdge = 512;
#endif
            const int longestEdge = std::max(sourceWidth, sourceHeight);
            if (longestEdge <= kMaxVideoEdge)
                return horizontal ? sourceWidth : sourceHeight;
            const double scale = static_cast<double>(kMaxVideoEdge) / longestEdge;
            // YUV decoders and scalers are happiest with even dimensions.
            return std::max(2, static_cast<int>(std::lround(
                (horizontal ? sourceWidth : sourceHeight) * scale)) & ~1);
        }

        bool uploadFrame(SharedVideo& video)
        {
            uint8_t* destination[] = {video.rgba.data(), nullptr, nullptr, nullptr};
            int destinationStride[] = {video.width * 4, 0, 0, 0};
            sws_scale(video.sws, video.frame->data, video.frame->linesize, 0,
                      video.codec->height, destination, destinationStride);
            if (video.texture > 0) {
                nvgUpdateImage(brls::Application::getNVGContext(), video.texture,
                               video.rgba.data());
                return true;
            }
            return false;
        }

        bool seekToBeginning(SharedVideo& video)
        {
            if (av_seek_frame(video.format, video.streamIndex, 0, AVSEEK_FLAG_BACKWARD) < 0)
                return false;
            avcodec_flush_buffers(video.codec);
            av_packet_unref(video.packet);
            video.lastPts = -1.0;
            return true;
        }

        // Decodes one displayed frame. This runs on the UI thread, so no
        // locks are needed for the shared decoder and NanoVG texture.
        bool decodeNextFrame(SharedVideo& video, double& duration)
        {
            for (int attempts = 0; attempts < 4096; ++attempts) {
                const int receive = avcodec_receive_frame(video.codec, video.frame);
                if (receive == 0) {
                    const int64_t pts = video.frame->best_effort_timestamp;
                    const double currentPts = pts == AV_NOPTS_VALUE
                        ? -1.0 : pts * av_q2d(video.timeBase);
                    duration = video.defaultFrameDuration;
                    if (video.lastPts >= 0.0 && currentPts > video.lastPts)
                        duration = std::clamp(currentPts - video.lastPts,
                                              kMinFrameDuration, kMaxFrameDuration);
                    if (currentPts >= 0.0)
                        video.lastPts = currentPts;
                    return uploadFrame(video);
                }
                if (receive != AVERROR(EAGAIN) && receive != AVERROR_EOF)
                    return false;

                if (receive == AVERROR_EOF) {
                    if (!seekToBeginning(video))
                        return false;
                }

                const int read = av_read_frame(video.format, video.packet);
                if (read < 0) {
                    const int flush = avcodec_send_packet(video.codec, nullptr);
                    if (flush < 0 && flush != AVERROR_EOF)
                        return false;
                    continue;
                }
                if (video.packet->stream_index != video.streamIndex) {
                    av_packet_unref(video.packet);
                    continue;
                }
                const int send = avcodec_send_packet(video.codec, video.packet);
                av_packet_unref(video.packet);
                if (send < 0 && send != AVERROR(EAGAIN))
                    return false;
            }
            return false;
        }

        std::shared_ptr<SharedVideo> openVideo(const std::string& path)
        {
            NVGcontext* vg = brls::Application::getNVGContext();
            if (!vg || path.empty())
                return nullptr;

            auto video = std::make_shared<SharedVideo>();
            video->path = path;
            if (avformat_open_input(&video->format, path.c_str(), nullptr, nullptr) < 0 ||
                avformat_find_stream_info(video->format, nullptr) < 0)
                return nullptr;

            video->streamIndex = av_find_best_stream(video->format, AVMEDIA_TYPE_VIDEO,
                                                      -1, -1, nullptr, 0);
            if (video->streamIndex < 0)
                return nullptr;
            AVStream* stream = video->format->streams[video->streamIndex];
            const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!decoder)
                return nullptr;
            video->codec = avcodec_alloc_context3(decoder);
            if (!video->codec ||
                avcodec_parameters_to_context(video->codec, stream->codecpar) < 0 ||
                avcodec_open2(video->codec, decoder, nullptr) < 0)
                return nullptr;

            video->frame = av_frame_alloc();
            video->packet = av_packet_alloc();
            if (!video->frame || !video->packet || video->codec->width <= 0 ||
                video->codec->height <= 0)
                return nullptr;
            video->width = scaledDimension(video->codec->width, video->codec->height, true);
            video->height = scaledDimension(video->codec->width, video->codec->height, false);
            video->rgba.resize(static_cast<size_t>(video->width) * video->height * 4);
            video->sws = sws_getContext(video->codec->width, video->codec->height,
                                        video->codec->pix_fmt, video->width, video->height,
                                        AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!video->sws)
                return nullptr;
            video->texture = nvgCreateImageRGBA(vg, video->width, video->height,
                                                NVG_IMAGE_PREMULTIPLIED, video->rgba.data());
            if (video->texture <= 0)
                return nullptr;

            video->timeBase = stream->time_base;
            const AVRational frameRate = av_guess_frame_rate(video->format, stream, nullptr);
            if (frameRate.num > 0 && frameRate.den > 0)
                video->defaultFrameDuration = std::clamp(av_q2d(av_inv_q(frameRate)),
                                                         kMinFrameDuration, kMaxFrameDuration);
            double duration = video->defaultFrameDuration;
            if (!decodeNextFrame(*video, duration))
                return nullptr;
            video->hasFrame = true;
            video->nextFrameAt = std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(duration));
            return video;
        }

        void advanceVideo(SharedVideo& video)
        {
            if (!video.hasFrame)
                return;
            const float speed = std::clamp(
                GET_SETTING_KEY_FLOAT(SettingKey::KEY_UI_BG_GIF_SPEED, 1.f), 0.1f, 4.f);
            const auto now = std::chrono::steady_clock::now();
            size_t decoded = 0;
            while (now >= video.nextFrameAt && decoded++ < 8) {
                double duration = video.defaultFrameDuration;
                if (!decodeNextFrame(video, duration))
                    return;
                duration /= speed;
                video.nextFrameAt += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(duration));
            }
            // Avoid a long decode burst after the app was suspended.
            if (decoded >= 8)
                video.nextFrameAt = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(video.defaultFrameDuration / speed));
        }
    } // namespace

    void VideoBackgroundView::clear()
    {
        m_video.reset();
    }

    bool VideoBackgroundView::load(const std::string& path)
    {
        if (m_video && m_video->path == path)
            return true;
        if (const auto found = g_backgroundVideoCache.find(path);
            found != g_backgroundVideoCache.end()) {
            if (auto cached = found->second.lock()) {
                m_video = std::move(cached);
                return true;
            }
            g_backgroundVideoCache.erase(found);
        }
        auto decoded = openVideo(path);
        if (!decoded)
            return false;
        m_video = decoded;
        g_backgroundVideoCache[path] = decoded;
        return true;
    }

    bool VideoBackgroundView::isLoaded() const
    {
        return m_video && m_video->hasFrame && m_video->texture > 0;
    }

    void VideoBackgroundView::frame(brls::FrameContext* ctx)
    {
        brls::View::frame(ctx);
        if (!m_video || getVisibility() != brls::Visibility::VISIBLE)
            return;
        advanceVideo(*m_video);
        invalidate();
    }

    void VideoBackgroundView::draw(NVGcontext* vg, float x, float y, float width,
                                   float height, brls::Style style,
                                   brls::FrameContext* ctx)
    {
        (void)style;
        (void)ctx;
        if (!vg || !isLoaded())
            return;
        const float scale = std::max(width / m_video->width, height / m_video->height);
        const float drawWidth = m_video->width * scale;
        const float drawHeight = m_video->height * scale;
        const float drawX = x + (width - drawWidth) * 0.5f;
        const float drawY = y + (height - drawHeight) * 0.5f;
        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, width, height);
        nvgBeginPath(vg);
        nvgRect(vg, drawX, drawY, drawWidth, drawHeight);
        nvgFillPaint(vg, nvgImagePattern(vg, drawX, drawY, drawWidth, drawHeight,
                                         0.f, m_video->texture, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }
} // namespace beiklive
