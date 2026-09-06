#include "VideoBackgroundView.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/common.h"
#include "ui/utils/BackgroundAudioPlayer.hpp"
#include "ui/utils/BKAudioPlayer.hpp"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
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
        }

        std::string path;
        std::thread decoder;
        std::atomic_bool stop{false};
        std::atomic<State> state{State::Loading};
        // The initial scan must never monopolize an SD card forever because
        // of a malformed MP4 index or a key frame placed far into the file.
        std::atomic_bool firstFrameQueued{false};
        std::atomic_bool initialLoadTimedOut{false};

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
        std::atomic_bool audioReady{false};
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
        // The map may hold one replacement while it is preloading; only
        // g_activeVideo is allowed to be presented by page views.
        std::unordered_map<std::string, std::shared_ptr<SharedVideo>> g_videoCache;
        std::shared_ptr<SharedVideo> g_activeVideo;
        uint64_t g_activeVideoGeneration = 0;
        std::atomic_bool g_videoPlaybackPaused{false};
        // Audio decoding can still be finishing an FFmpeg packet while the
        // GamePage transition stops the output device. Keep a separate gate
        // so that such a packet cannot start the background player again.
        std::atomic_bool g_backgroundAudioSuspended{false};
        BackgroundAudioPlayer g_backgroundAudio;
        std::mutex g_backgroundAudioMutex;

        void stopBackgroundAudio()
        {
            std::lock_guard<std::mutex> lock(g_backgroundAudioMutex);
            g_backgroundAudio.stop();
        }

        bool ensureBackgroundAudio()
        {
            std::lock_guard<std::mutex> lock(g_backgroundAudioMutex);
            if (g_backgroundAudioSuspended.load(std::memory_order_acquire) ||
                g_videoPlaybackPaused.load(std::memory_order_acquire) ||
                !GET_SETTING_KEY_INT(SettingKey::KEY_UI_BG_VIDEO_AUDIO, 0))
                return false;
            if (!g_backgroundAudio.isRunning()) {
                if (BKAudioPlayer::isAnyPlaying())
                    return false;
                if (!g_backgroundAudio.start(48000, 2))
                    return false;
            }
            g_backgroundAudio.setVolume(static_cast<float>(std::clamp(
                GET_SETTING_KEY_INT(SettingKey::KEY_UI_BG_VIDEO_VOLUME, 60), 0, 200)) / 100.0f);
            return g_backgroundAudio.isRunning();
        }

#ifdef __SWITCH__
        // Streaming no longer duplicates the file in RAM. Keep a generous
        // upper bound to prevent a misplaced movie file from monopolizing SD
        // I/O; decode/output dimensions remain bounded below.
        constexpr size_t kMaxVideoBytes = 128 * 1024 * 1024;
        // Decode higher-resolution source files only when they stay within a
        // bounded software-decoding budget; the RGBA texture is still 720p.
        constexpr int kMaxSourceWidth = 1920;
        constexpr int kMaxSourceHeight = 1080;
        constexpr int kMaxVideoEdge = 720;
#else
        constexpr size_t kMaxVideoBytes = 128 * 1024 * 1024;
        constexpr int kMaxSourceWidth = 3840;
        constexpr int kMaxSourceHeight = 2160;
        constexpr int kMaxVideoEdge = 720;
#endif
        constexpr double kFallbackFrameDuration = 1.0 / 30.0;
        constexpr double kMinFrameDuration = 1.0 / 60.0;
        constexpr double kMaxFrameDuration = 1.0;
        constexpr auto kInitialFrameDeadline = std::chrono::seconds(15);
        constexpr int kAvioBufferSize = 256 * 1024;

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

        // Keep FFmpeg on custom AVIO (the direct file protocol has proved
        // unreliable on libnx), but do not copy the whole MP4 into RAM before
        // decoding.  FFmpeg can now seek to `moov` and read only the packets
        // required for the first key frame.
        struct FileInput
        {
            std::ifstream file;
            int64_t size = 0;
            SharedVideo* video = nullptr;
            std::chrono::steady_clock::time_point firstFrameDeadline{};
            uint64_t bytesRead = 0;
            uint64_t nextProgressLog = 1024 * 1024;

            bool open(const std::string& path)
            {
                file.open(path, std::ios::binary | std::ios::ate);
                if (!file)
                    return false;
                const std::streamoff length = file.tellg();
                if (length < 12 || static_cast<uint64_t>(length) > kMaxVideoBytes)
                    return false;
                size = static_cast<int64_t>(length);
                file.seekg(0, std::ios::beg);
                return static_cast<bool>(file);
            }

            bool interrupted()
            {
                if (!video || video->stop.load(std::memory_order_acquire))
                    return true;
                if (!video->firstFrameQueued.load(std::memory_order_acquire) &&
                    std::chrono::steady_clock::now() >= firstFrameDeadline) {
                    video->initialLoadTimedOut.store(true, std::memory_order_release);
                    return true;
                }
                return false;
            }
        };

        int readFile(void* opaque, uint8_t* destination, int requested)
        {
            auto* input = static_cast<FileInput*>(opaque);
            if (!input || input->interrupted())
                return AVERROR_EXIT;
            if (!input->file || requested <= 0)
                return AVERROR_EOF;
            // A 256 KiB AVIO buffer avoids thousands of costly SD reads for
            // a normal video while the interrupt callback still makes the
            // operation cancellable between bounded reads.
            const int countRequested = std::min(requested, kAvioBufferSize);
            input->file.read(reinterpret_cast<char*>(destination), countRequested);
            const std::streamsize count = input->file.gcount();
            if (count > 0) {
                input->bytesRead += static_cast<uint64_t>(count);
                if (input->bytesRead >= input->nextProgressLog &&
                    !input->video->firstFrameQueued.load(std::memory_order_acquire)) {
                    brls::Logger::debug("MP4: initial scan '{}' read {} KiB", input->video->path,
                                        input->bytesRead / 1024);
                    input->nextProgressLog += 1024 * 1024;
                }
            }
            return count > 0 ? static_cast<int>(count) : AVERROR_EOF;
        }

        int64_t seekFile(void* opaque, int64_t offset, int whence)
        {
            auto* input = static_cast<FileInput*>(opaque);
            if (!input || input->interrupted())
                return AVERROR(EINVAL);
            if (whence == AVSEEK_SIZE)
                return input->size;
            const int origin = whence & ~AVSEEK_FORCE;
            // FFmpeg commonly seeks immediately after a short final read.
            // Clear eof/fail before asking the stream for its current offset.
            input->file.clear();
            const std::streampos position = input->file.tellg();
            const int64_t base = origin == SEEK_SET ? 0 :
                origin == SEEK_CUR ? (position < 0 ? -1 : static_cast<int64_t>(position)) :
                origin == SEEK_END ? input->size : -1;
            const int64_t target = base < 0 ? -1 : base + offset;
            if (target < 0 || target > input->size)
                return AVERROR(EINVAL);
            input->file.clear();
            input->file.seekg(target, std::ios::beg);
            return input->file ? target : AVERROR(EINVAL);
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
            const auto loadStarted = std::chrono::steady_clock::now();
            brls::Logger::info("MP4: decoder thread loading '{}'", video->path);
            FileInput input;
            input.video = video;
            input.firstFrameDeadline = std::chrono::steady_clock::now() + kInitialFrameDeadline;
            if (!input.open(video->path)) {
                brls::Logger::warning("MP4: cannot open '{}' or its size exceeds {} bytes",
                                      video->path, kMaxVideoBytes);
                video->state.store(VideoState::Failed, std::memory_order_release);
                return;
            }
            brls::Logger::info("MP4: streaming '{}' ({} bytes); decoding first frame without full preload",
                               video->path, input.size);
            AVIOContext* io = nullptr;
            AVFormatContext* format = nullptr;
            AVCodecContext* codec = nullptr;
            AVCodecContext* audioCodec = nullptr;
            AVFrame* frame = nullptr;
            AVFrame* audioFrame = nullptr;
            AVPacket* packet = nullptr;
            SwsContext* sws = nullptr;
            SwrContext* swr = nullptr;
            uint8_t* ioBuffer = nullptr;

            const auto cleanup = [&]() {
                sws_freeContext(sws);
                av_packet_free(&packet);
                av_frame_free(&frame);
                av_frame_free(&audioFrame);
                avcodec_free_context(&codec);
                avcodec_free_context(&audioCodec);
                avformat_close_input(&format);
                avio_context_free(&io);
                swr_free(&swr);
            };
            const auto fail = [&](const char* stage, int error = 0) {
                if (video->stop.load(std::memory_order_acquire)) {
                    brls::Logger::info("MP4: '{}' load cancelled", video->path);
                    video->state.store(VideoState::Stopped, std::memory_order_release);
                    cleanup();
                    return;
                }
                if (video->initialLoadTimedOut.load(std::memory_order_acquire)) {
                    brls::Logger::warning("MP4: '{}' did not produce a first frame within {} seconds; "
                                          "use a fast-start MP4 with an early key frame",
                                          video->path, kInitialFrameDeadline.count());
                    video->state.store(VideoState::Failed, std::memory_order_release);
                    cleanup();
                    return;
                }
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

            ioBuffer = static_cast<uint8_t*>(av_malloc(kAvioBufferSize));
            if (!ioBuffer) {
                fail("AVIO allocation");
                return;
            }
            io = avio_alloc_context(ioBuffer, kAvioBufferSize, 0, &input,
                                    readFile, nullptr, seekFile);
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
            format->interrupt_callback.callback = [](void* opaque) {
                return static_cast<FileInput*>(opaque)->interrupted() ? 1 : 0;
            };
            format->interrupt_callback.opaque = &input;

            const AVInputFormat* mov = av_find_input_format("mov");
            AVDictionary* options = nullptr;
            av_dict_set(&options, "probesize", "262144", 0);
            av_dict_set(&options, "analyzeduration", "0", 0);
            int result = mov ? avformat_open_input(&format, nullptr, mov, &options) : AVERROR_DEMUXER_NOT_FOUND;
            av_dict_free(&options);
            if (result < 0) {
                fail("MP4 container open", result);
                return;
            }
            brls::Logger::info("MP4: container opened '{}' after {} ms ({} KiB read)",
                               video->path,
                               std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - loadStarted).count(),
                               input.bytesRead / 1024);
            int streamIndex = -1;
            int audioStreamIndex = -1;
            for (unsigned int index = 0; index < format->nb_streams; ++index) {
                if (format->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    streamIndex = static_cast<int>(index);
                } else if (format->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                           audioStreamIndex < 0) {
                    audioStreamIndex = static_cast<int>(index);
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
            audioFrame = av_frame_alloc();
            packet = av_packet_alloc();
            if (!frame || !audioFrame || !packet) {
                fail("FFmpeg frame allocation");
                return;
            }

            if (audioStreamIndex >= 0) {
                AVStream* audioStream = format->streams[audioStreamIndex];
                const AVCodec* audioDecoder = avcodec_find_decoder(audioStream->codecpar->codec_id);
                if (audioDecoder) {
                    audioCodec = avcodec_alloc_context3(audioDecoder);
                    if (audioCodec && avcodec_parameters_to_context(audioCodec, audioStream->codecpar) >= 0 &&
                        avcodec_open2(audioCodec, audioDecoder, nullptr) >= 0 &&
                        audioCodec->sample_rate > 0 && audioCodec->ch_layout.nb_channels > 0) {
                        AVChannelLayout outputLayout = AV_CHANNEL_LAYOUT_STEREO;
                        if (swr_alloc_set_opts2(&swr, &outputLayout, AV_SAMPLE_FMT_S16, 48000,
                                                &audioCodec->ch_layout, audioCodec->sample_fmt,
                                                audioCodec->sample_rate, 0, nullptr) >= 0 &&
                            swr_init(swr) >= 0) {
                            video->audioReady.store(true, std::memory_order_release);
                            brls::Logger::info("MP4: audio decoder opened '{}': {} @ {}Hz, {} channels",
                                               video->path, avcodec_get_name(audioCodec->codec_id),
                                               audioCodec->sample_rate, audioCodec->ch_layout.nb_channels);
                        } else {
                            swr_free(&swr);
                            avcodec_free_context(&audioCodec);
                        }
                    } else {
                        avcodec_free_context(&audioCodec);
                    }
                }
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
                    const bool queueFirstFrame = !video->firstFrameQueued.load(
                        std::memory_order_acquire);
                    std::vector<uint8_t> rgba(static_cast<size_t>(outputWidth) * outputHeight * 4);
                    uint8_t* destination[] = {rgba.data(), nullptr, nullptr, nullptr};
                    int stride[] = {outputWidth * 4, 0, 0, 0};
                    sws_scale(sws, frame->data, frame->linesize, 0, frame->height,
                              destination, stride);
                    lastTimelinePts = timelinePts;
                    if (!enqueueFrame(*video, std::move(rgba), timelinePts))
                        break;
                    const bool isFirstQueuedFrame = !video->firstFrameQueued.exchange(
                        true, std::memory_order_acq_rel);
                    video->state.store(VideoState::Ready, std::memory_order_release);
                    if (isFirstQueuedFrame) {
                        brls::Logger::info("MP4: first frame queued for '{}' after {} ms ({} KiB read)",
                                           video->path,
                                           std::chrono::duration_cast<std::chrono::milliseconds>(
                                               std::chrono::steady_clock::now() - loadStarted).count(),
                                           input.bytesRead / 1024);
                    }
                    continue;
                }
                if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                    fail("video frame decode", result);
                    return;
                }
                if (result == AVERROR_EOF) {
                    if (av_seek_frame(format, -1, 0, AVSEEK_FLAG_BACKWARD) < 0) {
                        fail("loop seek");
                        return;
                    }
                    avcodec_flush_buffers(codec);
                    if (audioCodec) {
                        avcodec_flush_buffers(audioCodec);
                        if (swr)
                            swr_convert(swr, nullptr, 0, nullptr, 0);
                    }
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
                if (packet->stream_index == audioStreamIndex && audioCodec && swr) {
                    result = avcodec_send_packet(audioCodec, packet);
                    av_packet_unref(packet);
                    if (result < 0 && result != AVERROR(EAGAIN)) {
                        fail("audio packet decode", result);
                        return;
                    }
                    while ((result = avcodec_receive_frame(audioCodec, audioFrame)) == 0) {
                        const int outputSamples = static_cast<int>(av_rescale_rnd(
                            swr_get_delay(swr, audioCodec->sample_rate) + audioFrame->nb_samples,
                            48000, audioCodec->sample_rate, AV_ROUND_UP));
                        if (outputSamples <= 0)
                            continue;
                        std::vector<int16_t> pcm(static_cast<size_t>(outputSamples) * 2);
                        uint8_t* output[] = {reinterpret_cast<uint8_t*>(pcm.data()), nullptr};
                        const int converted = swr_convert(swr, output, outputSamples,
                                                          const_cast<const uint8_t* const*>(audioFrame->extended_data),
                                                          audioFrame->nb_samples);
                        if (converted <= 0 || !ensureBackgroundAudio())
                            continue;
                        pcm.resize(static_cast<size_t>(converted) * 2);
                        g_backgroundAudio.pushSamples(pcm.data(), static_cast<size_t>(converted));
                    }
                    if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                        fail("audio frame decode", result);
                        return;
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

        void retireVideo(std::shared_ptr<SharedVideo> video)
        {
            if (!video)
                return;
            if (g_activeVideo == video)
                stopBackgroundAudio();
            video->stop.store(true, std::memory_order_release);
            video->queueWake.notify_all();

            // NanoVG resources belong to the UI thread. The next decoder
            // must not start until this one has fully stopped: on Switch,
            // concurrently probing two MP4s can terminate the process before
            // FFmpeg reaches its first frame. The outgoing texture is already
            // transparent when this is called, so a short join here is safer
            // than an asynchronous overlapping decoder.
            if (NVGcontext* vg = brls::Application::getNVGContext(); vg && video->texture > 0) {
                nvgDeleteImage(vg, video->texture);
                video->texture = 0;
                video->textureCreated = false;
                video->textureReady.store(false, std::memory_order_release);
            }
            if (video->decoder.joinable()) {
                brls::Logger::info("MP4: stopping outgoing decoder '{}' before replacement", video->path);
                video->decoder.join();
                brls::Logger::info("MP4: outgoing decoder '{}' stopped", video->path);
            }
        }

        void setActiveVideo(const std::shared_ptr<SharedVideo>& video)
        {
            if (g_activeVideo == video)
                return;
            g_activeVideo = video;
            ++g_activeVideoGeneration;
        }
    }

    void VideoBackgroundView::clear()
    {
        m_observedGeneration = 0;
    }

    bool VideoBackgroundView::hasCachedVideo(const std::string& path)
    {
        return !path.empty() && g_videoCache.find(path) != g_videoCache.end();
    }

    bool VideoBackgroundView::preload(const std::string& path)
    {
        if (path.empty() || !std::filesystem::exists(path))
            return false;
        if (const auto found = g_videoCache.find(path); found != g_videoCache.end())
            return found->second->state.load(std::memory_order_acquire) != VideoState::Failed;

        auto video = std::make_shared<SharedVideo>();
        video->path = path;
        video->decoder = std::thread(decodeLoop, video.get());
        g_videoCache[path] = std::move(video);
        brls::Logger::info("MP4: preloading replacement background '{}'", path);
        return true;
    }

    void VideoBackgroundView::keepCachedVideo(const std::string& path)
    {
        for (auto iterator = g_videoCache.begin(); iterator != g_videoCache.end();) {
            if (iterator->first == path) {
                ++iterator;
                continue;
            }
            retireVideo(std::move(iterator->second));
            iterator = g_videoCache.erase(iterator);
        }
    }

    void VideoBackgroundView::clearCachedVideo()
    {
        // A page may still own a stale view after a new media type was
        // selected. Retire the cached player asynchronously: joining FFmpeg
        // on the UI thread makes a background switch visibly stall.
        stopBackgroundAudio();
        for (auto& [path, video] : g_videoCache) {
            (void)path;
            retireVideo(std::move(video));
        }
        g_videoCache.clear();
        setActiveVideo(nullptr);
    }

    void VideoBackgroundView::shutdownSharedVideo()
    {
        // main() calls this while the NanoVG context still belongs to the UI
        // thread. Unlike a background replacement, application exit may wait
        // briefly for in-flight storage I/O to finish.
        // Set the gate before stopping the device so a decoder that is
        // finishing an audio packet cannot recreate the output during exit.
        g_backgroundAudioSuspended.store(true, std::memory_order_release);
        stopBackgroundAudio();
        for (auto& [path, video] : g_videoCache) {
            (void)path;
            if (!video)
                continue;
            video->stop.store(true, std::memory_order_release);
            video->queueWake.notify_all();
            if (NVGcontext* vg = brls::Application::getNVGContext(); vg && video->texture > 0) {
                nvgDeleteImage(vg, video->texture);
                video->texture = 0;
                video->textureCreated = false;
                video->textureReady.store(false, std::memory_order_release);
            }
            if (video->decoder.joinable())
                video->decoder.join();
        }
        g_videoCache.clear();
        setActiveVideo(nullptr);
    }

    bool VideoBackgroundView::isCurrentCachedVideo(const std::string& path) const
    {
        return !path.empty() && g_activeVideo && g_activeVideo->path == path &&
            g_activeVideo->state.load(std::memory_order_acquire) != VideoState::Failed &&
            g_activeVideo->state.load(std::memory_order_acquire) != VideoState::Stopped;
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

    void VideoBackgroundView::setSharedAudioSuspended(bool suspended)
    {
        g_backgroundAudioSuspended.store(suspended, std::memory_order_release);
        if (suspended)
            stopBackgroundAudio();
        else if (!g_videoPlaybackPaused.load(std::memory_order_acquire) && g_activeVideo)
            ensureBackgroundAudio();
    }

    bool VideoBackgroundView::load(const std::string& path)
    {
        if (path.empty() || !std::filesystem::exists(path)) {
            brls::Logger::warning("MP4: selected path does not exist: '{}'", path);
            return false;
        }
        std::shared_ptr<SharedVideo> video;
        if (const auto found = g_videoCache.find(path); found != g_videoCache.end()) {
            video = found->second;
        } else {
            // Only one configured video background can be active. Existing
            // page views follow g_activeVideo on their next UI frame, so they
            // cannot keep displaying this retired player after a replacement.
            clearCachedVideo();
            video = std::make_shared<SharedVideo>();
            video->path = path;
            video->decoder = std::thread(decodeLoop, video.get());
            g_videoCache[path] = video;
        }

        setActiveVideo(video);
        m_observedGeneration = g_activeVideoGeneration;
        brls::Logger::info("MP4: active background committed '{}' (generation {})",
                           path, g_activeVideoGeneration);
        return video->state.load(std::memory_order_acquire) != VideoState::Failed;
    }

    bool VideoBackgroundView::isLoaded() const
    {
        return g_activeVideo &&
            g_activeVideo->state.load(std::memory_order_acquire) == VideoState::Ready &&
            g_activeVideo->textureReady.load(std::memory_order_acquire);
    }

    void VideoBackgroundView::frame(brls::FrameContext* ctx)
    {
        brls::View::frame(ctx);
        if (m_observedGeneration != g_activeVideoGeneration) {
            m_observedGeneration = g_activeVideoGeneration;
            brls::Logger::debug("MP4: view attached active generation {}", m_observedGeneration);
            invalidate();
        }

        const auto video = g_activeVideo;
        if (!GET_SETTING_KEY_INT(SettingKey::KEY_UI_BG_VIDEO_AUDIO, 0) &&
            g_backgroundAudio.isRunning())
            stopBackgroundAudio();
        if (!video || getVisibility() != brls::Visibility::VISIBLE ||
            video->state.load(std::memory_order_acquire) != VideoState::Ready)
            return;
        if (g_videoPlaybackPaused.load(std::memory_order_acquire))
            return;
        NVGcontext* vg = brls::Application::getNVGContext();
        if (!vg)
            return;

        if (!video->textureCreated) {
            if (takeFrame(*video, true, 0.0, vg)) {
                brls::Logger::info("MP4: first texture uploaded for '{}'", video->path);
                invalidate();
            }
            return;
        }
        const float speed = std::clamp(
            GET_SETTING_KEY_FLOAT(SettingKey::KEY_UI_BG_GIF_SPEED, 1.f), 0.1f, 4.f);
        const auto now = std::chrono::steady_clock::now();
        const double delta = std::min(0.050, std::max(0.0,
            std::chrono::duration<double>(now - video->lastPresentationTick).count()));
        video->lastPresentationTick = now;
        video->presentationElapsed += delta * speed;
        bool uploaded = false;
        while (takeFrame(*video, false, video->presentationElapsed, vg))
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
        const auto video = g_activeVideo;
        if (!vg || !video || !video->textureCreated || video->texture <= 0 ||
            video->width <= 0 || video->height <= 0)
            return;
        const float scale = std::max(width / video->width, height / video->height);
        const float drawWidth = video->width * scale;
        const float drawHeight = video->height * scale;
        const float drawX = x + (width - drawWidth) * 0.5f;
        const float drawY = y + (height - drawHeight) * 0.5f;
        nvgSave(vg);
        // View::setAlpha() is not applied automatically to custom NanoVG
        // drawing. Use the inherited alpha so Box can fade the first startup
        // frame in instead of presenting it at full opacity immediately.
        nvgGlobalAlpha(vg, getAlpha());
        nvgIntersectScissor(vg, x, y, width, height);
        nvgBeginPath(vg);
        nvgRect(vg, drawX, drawY, drawWidth, drawHeight);
        nvgFillPaint(vg, nvgImagePattern(vg, drawX, drawY, drawWidth, drawHeight,
                                         0.f, video->texture, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }
} // namespace beiklive
