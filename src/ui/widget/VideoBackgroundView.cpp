#include "VideoBackgroundView.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/common.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

namespace beiklive
{
    struct VideoBackgroundView::SharedVideo
    {
        enum class State : uint8_t { Loading, Ready, Failed, Stopped };
        static constexpr size_t kQueueSize = 3;

        struct Frame
        {
            std::vector<uint8_t> rgba;
            double pts = 0.0;
        };

        ~SharedVideo()
        {
            stop.store(true, std::memory_order_release);
            queueWake.notify_all();
            if (decoder.joinable())
                decoder.join();
            if (NVGcontext* vg = brls::Application::getNVGContext(); vg && texture > 0)
                nvgDeleteImage(vg, texture);
        }

        std::string path;
        std::thread decoder;
        std::atomic_bool stop{false};
        std::atomic<State> state{State::Loading};

        // Accessed only by the UI thread after the worker publishes Ready.
        int sourceWidth = 0;
        int sourceHeight = 0;
        int width = 0;
        int height = 0;
        int texture = 0;
        bool textureCreated = false;
        // Published after the UI thread has successfully created the one
        // NanoVG texture.  Box uses this to begin its fade only when there is
        // actual GPU content to show, rather than merely a decoded CPU frame.
        std::atomic_bool textureReady{false};
        bool clockStarted = false;
        double firstPts = 0.0;
        // The display clock advances only while a visible UI page presents
        // this shared texture. It deliberately freezes while navigation or a
        // modal obscures every consumer, preventing a return from consuming a
        // burst of "overdue" queued frames.
        double presentationElapsed = 0.0;
        std::chrono::steady_clock::time_point lastPresentationTick{};

        // One producer (decoder) and one consumer (UI). FFmpeg and NanoVG
        // never cross this boundary: only completed RGBA frames do.
        std::mutex queueMutex;
        std::condition_variable queueWake;
        std::array<Frame, kQueueSize> queue;
        size_t readIndex = 0;
        size_t writeIndex = 0;
        size_t queued = 0;
    };

    namespace
    {
        using SharedVideo = VideoBackgroundView::SharedVideo;
        using VideoState = SharedVideo::State;

        // This intentionally owns the active background. Views themselves
        // come and go with Borealis pages, but background playback must not.
        std::unordered_map<std::string, std::shared_ptr<SharedVideo>> g_videoCache;
        std::atomic_bool g_videoPlaybackPaused{false};

#ifdef __SWITCH__
        constexpr size_t kMaxVideoBytes = 16 * 1024 * 1024;
        // Decode higher-resolution source files only when they stay within a
        // bounded software-decoding budget; the RGBA texture is still 720p.
        constexpr int kMaxSourceWidth = 1920;
        constexpr int kMaxSourceHeight = 1080;
        constexpr int kMaxVideoEdge = 720;
#else
        constexpr size_t kMaxVideoBytes = 64 * 1024 * 1024;
        constexpr int kMaxSourceWidth = 3840;
        constexpr int kMaxSourceHeight = 2160;
        constexpr int kMaxVideoEdge = 720;
#endif
        constexpr double kFallbackFrameDuration = 1.0 / 30.0;
        constexpr double kMinFrameDuration = 1.0 / 60.0;
        constexpr double kMaxFrameDuration = 1.0;

        const char* errorText(int error, char (&buffer)[AV_ERROR_MAX_STRING_SIZE])
        {
            return av_strerror(error, buffer, sizeof(buffer)) < 0 ? "unknown FFmpeg error" : buffer;
        }

        int scaleDimension(int sourceWidth, int sourceHeight, bool horizontal)
        {
            const int longest = std::max(sourceWidth, sourceHeight);
            if (longest <= kMaxVideoEdge)
                return horizontal ? sourceWidth : sourceHeight;
            const double scale = static_cast<double>(kMaxVideoEdge) / longest;
            return std::max(2, static_cast<int>(std::lround(
                (horizontal ? sourceWidth : sourceHeight) * scale)) & ~1);
        }

        bool readFile(const std::string& path, std::vector<uint8_t>& output)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                brls::Logger::warning("MP4: cannot open '{}'", path);
                return false;
            }
            const std::streamoff size = file.tellg();
            if (size < 12 || static_cast<uint64_t>(size) > kMaxVideoBytes) {
                brls::Logger::warning("MP4: '{}' has invalid size {} (limit {} bytes)",
                                      path, size, kMaxVideoBytes);
                return false;
            }
            output.resize(static_cast<size_t>(size));
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(output.data()), size);
            if (file.gcount() != size) {
                brls::Logger::warning("MP4: partial read for '{}': {} / {} bytes", path,
                                      file.gcount(), size);
                output.clear();
                return false;
            }
            return true;
        }

        struct MemoryInput
        {
            const std::vector<uint8_t>* data = nullptr;
            size_t offset = 0;
        };

        int readMemory(void* opaque, uint8_t* destination, int requested)
        {
            auto* input = static_cast<MemoryInput*>(opaque);
            if (!input || !input->data || requested <= 0 || input->offset >= input->data->size())
                return AVERROR_EOF;
            const size_t count = std::min(static_cast<size_t>(requested),
                                          input->data->size() - input->offset);
            std::memcpy(destination, input->data->data() + input->offset, count);
            input->offset += count;
            return static_cast<int>(count);
        }

        int64_t seekMemory(void* opaque, int64_t offset, int whence)
        {
            auto* input = static_cast<MemoryInput*>(opaque);
            if (!input || !input->data)
                return AVERROR(EINVAL);
            if (whence == AVSEEK_SIZE)
                return static_cast<int64_t>(input->data->size());
            const int origin = whence & ~AVSEEK_FORCE;
            const int64_t base = origin == SEEK_SET ? 0 :
                origin == SEEK_CUR ? static_cast<int64_t>(input->offset) :
                origin == SEEK_END ? static_cast<int64_t>(input->data->size()) : -1;
            const int64_t target = base < 0 ? -1 : base + offset;
            if (target < 0 || target > static_cast<int64_t>(input->data->size()))
                return AVERROR(EINVAL);
            input->offset = static_cast<size_t>(target);
            return target;
        }

        bool enqueueFrame(SharedVideo& video, std::vector<uint8_t>&& rgba, double pts)
        {
            std::unique_lock<std::mutex> lock(video.queueMutex);
            video.queueWake.wait(lock, [&]() {
                return video.stop.load(std::memory_order_acquire) ||
                    (!g_videoPlaybackPaused.load(std::memory_order_acquire) &&
                     video.queued < SharedVideo::kQueueSize);
            });
            if (video.stop.load(std::memory_order_acquire))
                return false;
            auto& slot = video.queue[video.writeIndex];
            slot.rgba = std::move(rgba);
            slot.pts = pts;
            video.writeIndex = (video.writeIndex + 1) % SharedVideo::kQueueSize;
            ++video.queued;
            lock.unlock();
            video.queueWake.notify_all();
            return true;
        }

        void decodeLoop(SharedVideo* video)
        {
            std::vector<uint8_t> source;
            brls::Logger::info("MP4: decoder thread loading '{}'", video->path);
            if (!readFile(video->path, source)) {
                video->state.store(VideoState::Failed, std::memory_order_release);
                return;
            }
            brls::Logger::info("MP4: read {} bytes from '{}'", source.size(), video->path);

            MemoryInput input{&source};
            AVIOContext* io = nullptr;
            AVFormatContext* format = nullptr;
            AVCodecContext* codec = nullptr;
            AVFrame* frame = nullptr;
            AVPacket* packet = nullptr;
            SwsContext* sws = nullptr;
            uint8_t* ioBuffer = nullptr;

            const auto cleanup = [&]() {
                sws_freeContext(sws);
                av_packet_free(&packet);
                av_frame_free(&frame);
                avcodec_free_context(&codec);
                avformat_close_input(&format);
                avio_context_free(&io);
            };
            const auto fail = [&](const char* stage, int error = 0) {
                if (error < 0) {
                    char text[AV_ERROR_MAX_STRING_SIZE];
                    brls::Logger::warning("MP4: {} failed for '{}': {}", stage, video->path,
                                          errorText(error, text));
                } else {
                    brls::Logger::warning("MP4: {} failed for '{}'", stage, video->path);
                }
                video->state.store(VideoState::Failed, std::memory_order_release);
                cleanup();
            };

            ioBuffer = static_cast<uint8_t*>(av_malloc(32 * 1024));
            if (!ioBuffer) {
                fail("AVIO allocation");
                return;
            }
            io = avio_alloc_context(ioBuffer, 32 * 1024, 0, &input,
                                    readMemory, nullptr, seekMemory);
            if (!io) {
                av_free(ioBuffer);
                ioBuffer = nullptr;
                fail("AVIO context allocation");
                return;
            }
            ioBuffer = nullptr; // AVIO owns its buffer after successful creation.
            format = avformat_alloc_context();
            if (!format) {
                fail("format context allocation");
                return;
            }
            format->pb = io;
            format->flags |= AVFMT_FLAG_CUSTOM_IO;

            const AVInputFormat* mov = av_find_input_format("mov");
            int result = mov ? avformat_open_input(&format, nullptr, mov, nullptr) : AVERROR_DEMUXER_NOT_FOUND;
            if (result < 0) {
                fail("MP4 container open", result);
                return;
            }
            int streamIndex = -1;
            for (unsigned int index = 0; index < format->nb_streams; ++index) {
                if (format->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    streamIndex = static_cast<int>(index);
                    break;
                }
            }
            if (streamIndex < 0) {
                fail("video stream selection");
                return;
            }
            AVStream* stream = format->streams[streamIndex];
            const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!decoder) {
                brls::Logger::warning("MP4: '{}' uses '{}', but this build has no decoder for it",
                                      video->path, avcodec_get_name(stream->codecpar->codec_id));
                video->state.store(VideoState::Failed, std::memory_order_release);
                cleanup();
                return;
            }
            codec = avcodec_alloc_context3(decoder);
            result = codec ? avcodec_parameters_to_context(codec, stream->codecpar) : AVERROR(ENOMEM);
            if (result < 0) {
                fail("video stream parameters", result);
                return;
            }
            result = avcodec_open2(codec, decoder, nullptr);
            if (result < 0) {
                fail("video decoder open", result);
                return;
            }
            frame = av_frame_alloc();
            packet = av_packet_alloc();
            if (!frame || !packet) {
                fail("FFmpeg frame allocation");
                return;
            }

            const AVRational rate = av_guess_frame_rate(format, stream, nullptr);
            const double nominalDuration = rate.num > 0 && rate.den > 0
                ? std::clamp(av_q2d(av_inv_q(rate)), kMinFrameDuration, kMaxFrameDuration)
                : kFallbackFrameDuration;
            const double framesPerSecond = 1.0 / nominalDuration;
            if (framesPerSecond < 1.0 || framesPerSecond > 60.0) {
                brls::Logger::warning("MP4: '{}' is {:.2f} FPS; supported range is 1-60 FPS",
                                      video->path, framesPerSecond);
                video->state.store(VideoState::Failed, std::memory_order_release);
                cleanup();
                return;
            }
            brls::Logger::info("MP4: decoder opened '{}': {} @ {:.2f} fps; awaiting first frame dimensions",
                               video->path, avcodec_get_name(codec->codec_id), 1.0 / nominalDuration);

            bool sentEof = false;
            bool segmentStart = true;
            bool outputConfigured = false;
            int outputWidth = 0;
            int outputHeight = 0;
            double rawSegmentStart = 0.0;
            double loopOffset = 0.0;
            double lastTimelinePts = 0.0;
            while (!video->stop.load(std::memory_order_acquire)) {
                {
                    std::unique_lock<std::mutex> lock(video->queueMutex);
                    video->queueWake.wait(lock, [&]() {
                        return video->stop.load(std::memory_order_acquire) ||
                            !g_videoPlaybackPaused.load(std::memory_order_acquire);
                    });
                }
                if (video->stop.load(std::memory_order_acquire))
                    break;
                result = avcodec_receive_frame(codec, frame);
                if (result == 0) {
                    // Some MP4 codecs, notably HEVC, do not expose complete
                    // dimensions until their sequence header is decoded.
                    // AVFrame is therefore the first authoritative source.
                    if (!outputConfigured) {
                        if (frame->width <= 0 || frame->height <= 0) {
                            brls::Logger::warning("MP4: '{}' decoded invalid frame dimensions {}x{}",
                                                  video->path, frame->width, frame->height);
                            video->state.store(VideoState::Failed, std::memory_order_release);
                            cleanup();
                            return;
                        }
                        if (frame->width > kMaxSourceWidth || frame->height > kMaxSourceHeight) {
                            brls::Logger::warning("MP4: '{}' is {}x{}; source limit is {}x{} (output is scaled to 720p)",
                                                  video->path, frame->width, frame->height,
                                                  kMaxSourceWidth, kMaxSourceHeight);
                            video->state.store(VideoState::Failed, std::memory_order_release);
                            cleanup();
                            return;
                        }
                        outputWidth = scaleDimension(frame->width, frame->height, true);
                        outputHeight = scaleDimension(frame->width, frame->height, false);
                        {
                            std::lock_guard<std::mutex> lock(video->queueMutex);
                            video->sourceWidth = frame->width;
                            video->sourceHeight = frame->height;
                            video->width = outputWidth;
                            video->height = outputHeight;
                        }
                        outputConfigured = true;
                        brls::Logger::info("MP4: first frame '{}': {} {}x{}, output {}x{} ({})",
                                           video->path, avcodec_get_name(codec->codec_id),
                                           frame->width, frame->height, outputWidth, outputHeight,
                                           av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)));
                    }
                    const auto sourceFormat = static_cast<AVPixelFormat>(frame->format);
                    if (sourceFormat == AV_PIX_FMT_NONE || !av_pix_fmt_desc_get(sourceFormat)) {
                        brls::Logger::warning("MP4: '{}' returned an unsupported pixel format '{}'",
                                              video->path, av_get_pix_fmt_name(sourceFormat));
                        video->state.store(VideoState::Failed, std::memory_order_release);
                        cleanup();
                        return;
                    }
                    // Pixel format is authoritative only on a decoded frame.
                    // Reuse or rebuild the conversion context here so H.264,
                    // HEVC (including 10-bit), MPEG-4, VP8/VP9 and AV1 can
                    // all feed the same RGBA/NanoVG upload path.
                    sws = sws_getCachedContext(sws, frame->width, frame->height,
                                               sourceFormat, outputWidth, outputHeight,
                                               AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR,
                                               nullptr, nullptr, nullptr);
                    if (!sws) {
                        fail("RGBA conversion allocation");
                        return;
                    }
                    const int64_t timestamp = frame->best_effort_timestamp;
                    const double rawPts = timestamp == AV_NOPTS_VALUE
                        ? (segmentStart ? 0.0 : lastTimelinePts + nominalDuration)
                        : timestamp * av_q2d(stream->time_base);
                    if (segmentStart) {
                        rawSegmentStart = rawPts;
                        segmentStart = false;
                    }
                    const double timelinePts = loopOffset + std::max(0.0, rawPts - rawSegmentStart);
                    std::vector<uint8_t> rgba(static_cast<size_t>(outputWidth) * outputHeight * 4);
                    uint8_t* destination[] = {rgba.data(), nullptr, nullptr, nullptr};
                    int stride[] = {outputWidth * 4, 0, 0, 0};
                    sws_scale(sws, frame->data, frame->linesize, 0, frame->height,
                              destination, stride);
                    lastTimelinePts = timelinePts;
                    if (!enqueueFrame(*video, std::move(rgba), timelinePts))
                        break;
                    video->state.store(VideoState::Ready, std::memory_order_release);
                    continue;
                }
                if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                    fail("video frame decode", result);
                    return;
                }
                if (result == AVERROR_EOF) {
                    if (av_seek_frame(format, streamIndex, 0, AVSEEK_FLAG_BACKWARD) < 0) {
                        fail("loop seek");
                        return;
                    }
                    avcodec_flush_buffers(codec);
                    av_packet_unref(packet);
                    sentEof = false;
                    segmentStart = true;
                    loopOffset = lastTimelinePts + nominalDuration;
                    continue;
                }

                result = av_read_frame(format, packet);
                if (result < 0) {
                    if (!sentEof) {
                        sentEof = true;
                        result = avcodec_send_packet(codec, nullptr);
                        if (result < 0 && result != AVERROR_EOF) {
                            fail("decoder drain", result);
                            return;
                        }
                    }
                    continue;
                }
                if (packet->stream_index != streamIndex) {
                    av_packet_unref(packet);
                    continue;
                }
                result = avcodec_send_packet(codec, packet);
                av_packet_unref(packet);
                if (result < 0 && result != AVERROR(EAGAIN)) {
                    fail("video packet decode", result);
                    return;
                }
            }
            cleanup();
            if (video->state.load(std::memory_order_acquire) != VideoState::Failed)
                video->state.store(VideoState::Stopped, std::memory_order_release);
        }

        bool takeFrame(SharedVideo& video, bool first, double elapsed, NVGcontext* vg)
        {
            std::unique_lock<std::mutex> lock(video.queueMutex);
            if (video.queued == 0)
                return false;
            const auto& frame = video.queue[video.readIndex];
            if (!first && frame.pts - video.firstPts > elapsed)
                return false;

            if (first) {
                video.texture = nvgCreateImageRGBA(vg, video.width, video.height,
                                                   NVG_IMAGE_PREMULTIPLIED, frame.rgba.data());
                if (video.texture <= 0)
                    return false;
                video.textureCreated = true;
                video.textureReady.store(true, std::memory_order_release);
                video.firstPts = frame.pts;
                video.presentationElapsed = 0.0;
                video.lastPresentationTick = std::chrono::steady_clock::now();
                video.clockStarted = true;
            } else {
                nvgUpdateImage(vg, video.texture, frame.rgba.data());
            }
            video.readIndex = (video.readIndex + 1) % SharedVideo::kQueueSize;
            --video.queued;
            lock.unlock();
            video.queueWake.notify_all();
            return true;
        }
    }

    void VideoBackgroundView::clear()
    {
        m_video.reset();
    }

    bool VideoBackgroundView::hasCachedVideo(const std::string& path)
    {
        return !path.empty() && g_videoCache.find(path) != g_videoCache.end();
    }

    void VideoBackgroundView::clearCachedVideo()
    {
        // A page may still own a stale view after a new media type was
        // selected. Stop its worker before dropping the process-global root;
        // otherwise it keeps decoding in the background and can later be
        // mistaken for the newly selected video on that page.
        for (auto& [path, video] : g_videoCache) {
            (void)path;
            video->stop.store(true, std::memory_order_release);
            video->queueWake.notify_all();
        }
        g_videoCache.clear();
    }

    bool VideoBackgroundView::isCurrentCachedVideo(const std::string& path) const
    {
        const auto found = g_videoCache.find(path);
        return m_video && found != g_videoCache.end() && found->second == m_video &&
            m_video->state.load(std::memory_order_acquire) != VideoState::Failed &&
            m_video->state.load(std::memory_order_acquire) != VideoState::Stopped;
    }

    void VideoBackgroundView::setSharedPlaybackPaused(bool paused)
    {
        const bool previous = g_videoPlaybackPaused.exchange(paused, std::memory_order_acq_rel);
        if (previous == paused)
            return;
        for (auto& [path, video] : g_videoCache) {
            (void)path;
            video->queueWake.notify_all();
        }
        brls::Logger::info("MP4: shared background playback {}", paused ? "paused" : "resumed");
    }

    bool VideoBackgroundView::load(const std::string& path)
    {
        if (path.empty() || !std::filesystem::exists(path)) {
            brls::Logger::warning("MP4: selected path does not exist: '{}'", path);
            return false;
        }
        if (m_video && m_video->path == path)
            return m_video->state.load(std::memory_order_acquire) != VideoState::Failed;
        if (const auto found = g_videoCache.find(path); found != g_videoCache.end()) {
            m_video = found->second;
            return m_video->state.load(std::memory_order_acquire) != VideoState::Failed;
        }

        // Only one configured video background can be active. Existing page
        // views may still hold the old shared object, so merely erasing the
        // map is insufficient: explicitly wake and stop its worker first.
        clearCachedVideo();

        auto video = std::make_shared<SharedVideo>();
        video->path = path;
        video->decoder = std::thread(decodeLoop, video.get());
        m_video = video;
        g_videoCache[path] = video;
        return true;
    }

    bool VideoBackgroundView::isLoaded() const
    {
        return m_video &&
            m_video->state.load(std::memory_order_acquire) == VideoState::Ready &&
            m_video->textureReady.load(std::memory_order_acquire);
    }

    void VideoBackgroundView::frame(brls::FrameContext* ctx)
    {
        brls::View::frame(ctx);
        if (!m_video || getVisibility() != brls::Visibility::VISIBLE ||
            m_video->state.load(std::memory_order_acquire) != VideoState::Ready)
            return;
        if (g_videoPlaybackPaused.load(std::memory_order_acquire))
            return;
        NVGcontext* vg = brls::Application::getNVGContext();
        if (!vg)
            return;

        if (!m_video->textureCreated) {
            if (takeFrame(*m_video, true, 0.0, vg))
                invalidate();
            return;
        }
        const float speed = std::clamp(
            GET_SETTING_KEY_FLOAT(SettingKey::KEY_UI_BG_GIF_SPEED, 1.f), 0.1f, 4.f);
        const auto now = std::chrono::steady_clock::now();
        const double delta = std::min(0.050, std::max(0.0,
            std::chrono::duration<double>(now - m_video->lastPresentationTick).count()));
        m_video->lastPresentationTick = now;
        m_video->presentationElapsed += delta * speed;
        bool uploaded = false;
        while (takeFrame(*m_video, false, m_video->presentationElapsed, vg))
            uploaded = true;
        if (uploaded)
            invalidate();
    }

    void VideoBackgroundView::draw(NVGcontext* vg, float x, float y, float width,
                                   float height, brls::Style style,
                                   brls::FrameContext* ctx)
    {
        (void)style;
        (void)ctx;
        if (!vg || !m_video || !m_video->textureCreated || m_video->texture <= 0 ||
            m_video->width <= 0 || m_video->height <= 0)
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
