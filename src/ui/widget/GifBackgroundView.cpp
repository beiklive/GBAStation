#include "GifBackgroundView.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <unordered_map>

#include "core/common.h"
#include "core/ThreadPool.hpp"
#include "ui/view/iisu/GifDecoder.hpp"

namespace beiklive
{
    struct GifBackgroundView::SharedAnimation
    {
        ~SharedAnimation()
        {
            NVGcontext* vg = brls::Application::getNVGContext();
            if (vg && texture > 0)
                nvgDeleteImage(vg, texture);
        }

        std::string path;
        std::vector<GifFrame> frames;
        uint32_t width = 0;
        uint32_t height = 0;
        int texture = 0;
        size_t currentFrame = 0;
        float elapsedMs = 0.f;
        bool looping = true;
        bool ready = false;
        bool failed = false;
        // Decoding uses the worker pool. A view can be replaced while that
        // work is still in flight, so the completion callback must not touch
        // NanoVG for an animation no longer owned by a background layer.
        std::atomic_bool cancelled{false};
        std::chrono::steady_clock::time_point lastTick{};
    };

    namespace
    {
        using SharedAnimation = GifBackgroundView::SharedAnimation;

        // Background pages are short lived, while the selected animation is
        // a process-global visual.  Keep it alive so a page change preserves
        // both its decoded frames and its playback position.
        std::unordered_map<std::string, std::shared_ptr<SharedAnimation>> g_backgroundGifCache;

        void decodeAnimationAsync(const std::shared_ptr<SharedAnimation>& animation)
        {
#ifdef __SWITCH__
            constexpr int kMaxGifEdge = 384;
            constexpr size_t kMaxGifFrames = 16;
#else
            constexpr int kMaxGifEdge = 512;
            constexpr size_t kMaxGifFrames = 24;
#endif

            ThreadPool::instance().enqueue([animation, path = animation->path]() {
                GifDecoded decoded;
                const bool decodedOk = GifDecoder::decode(path, decoded, kMaxGifEdge, kMaxGifFrames);
                brls::sync([animation, decodedOk, decoded = std::move(decoded)]() mutable {
                    if (animation->cancelled.load(std::memory_order_acquire))
                        return;
                    if (!decodedOk || decoded.frames.empty()) {
                        animation->failed = true;
                        brls::Logger::warning("Unable to decode GIF background: {}", animation->path);
                        return;
                    }

                    NVGcontext* vg = brls::Application::getNVGContext();
                    if (!vg) {
                        animation->failed = true;
                        return;
                    }
                    const int texture = nvgCreateImageRGBA(
                        vg, static_cast<int>(decoded.width), static_cast<int>(decoded.height),
                        NVG_IMAGE_PREMULTIPLIED, decoded.frames.front().rgba.data());
                    if (texture <= 0) {
                        animation->failed = true;
                        brls::Logger::warning("Unable to create GIF background texture: {}", animation->path);
                        return;
                    }

                    animation->width = decoded.width;
                    animation->height = decoded.height;
                    animation->frames = std::move(decoded.frames);
                    animation->texture = texture;
                    animation->looping = decoded.looping;
                    animation->currentFrame = 0;
                    animation->elapsedMs = 0.f;
                    animation->lastTick = std::chrono::steady_clock::now();
                    animation->ready = true;
                });
            });
        }

        bool advanceAnimation(SharedAnimation& animation)
        {
            if (animation.frames.size() < 2)
                return false;

            const auto now = std::chrono::steady_clock::now();
            if (animation.lastTick.time_since_epoch().count() == 0) {
                animation.lastTick = now;
                return false;
            }
            const float deltaMs = std::min(250.f, std::chrono::duration<float, std::milli>(
                now - animation.lastTick).count());
            animation.lastTick = now;
            const float speed = std::clamp(
                GET_SETTING_KEY_FLOAT(SettingKey::KEY_UI_BG_GIF_SPEED, 1.f), 0.1f, 4.f);
            animation.elapsedMs += deltaMs * speed;

            size_t guard = 0;
            bool changed = false;
            while (animation.elapsedMs >= animation.frames[animation.currentFrame].delayMs) {
                animation.elapsedMs -= static_cast<float>(animation.frames[animation.currentFrame].delayMs);
                if (animation.currentFrame + 1 < animation.frames.size()) {
                    ++animation.currentFrame;
                    changed = true;
                } else if (animation.looping) {
                    animation.currentFrame = 0;
                    changed = true;
                } else {
                    animation.elapsedMs = 0.f;
                    break;
                }
                if (++guard > 120)
                    break;
            }
            return changed;
        }

        void uploadCurrentFrame(SharedAnimation& animation)
        {
            if (!animation.ready || animation.texture <= 0 ||
                animation.currentFrame >= animation.frames.size())
                return;
            NVGcontext* vg = brls::Application::getNVGContext();
            if (vg)
                nvgUpdateImage(vg, animation.texture,
                               animation.frames[animation.currentFrame].rgba.data());
        }
    } // namespace

    void GifBackgroundView::clear()
    {
        m_animation.reset();
    }

    bool GifBackgroundView::hasCachedAnimation(const std::string& path)
    {
        return !path.empty() && g_backgroundGifCache.find(path) != g_backgroundGifCache.end();
    }

    void GifBackgroundView::clearCachedAnimation()
    {
        for (auto& [path, animation] : g_backgroundGifCache) {
            (void)path;
            animation->cancelled.store(true, std::memory_order_release);
        }
        g_backgroundGifCache.clear();
    }

    bool GifBackgroundView::load(const std::string& path, bool immediate)
    {
        if (m_animation && m_animation->path == path &&
            !m_animation->cancelled.load(std::memory_order_acquire) &&
            g_backgroundGifCache.find(path) != g_backgroundGifCache.end() &&
            g_backgroundGifCache[path] == m_animation)
            return true;

        if (const auto found = g_backgroundGifCache.find(path);
            found != g_backgroundGifCache.end()) {
            if (!found->second->cancelled.load(std::memory_order_acquire)) {
                m_animation = found->second;
                return true;
            }
            g_backgroundGifCache.erase(found);
        }

        clearCachedAnimation();

        auto animation = std::make_shared<SharedAnimation>();
        animation->path = path;
        m_animation = animation;
        g_backgroundGifCache[path] = animation;
        if (immediate) {
            decodeAnimationAsync(animation);
        } else {
            // Let the first screen settle before using CPU and storage
            // bandwidth for a potentially large persisted background.
            std::weak_ptr<SharedAnimation> pending = animation;
            brls::delay(350, [pending]() {
                if (auto animation = pending.lock())
                    decodeAnimationAsync(animation);
            });
        }
        return true;
    }

    bool GifBackgroundView::isCurrentCachedAnimation(const std::string& path) const
    {
        const auto found = g_backgroundGifCache.find(path);
        return m_animation && found != g_backgroundGifCache.end() &&
            found->second == m_animation &&
            !m_animation->cancelled.load(std::memory_order_acquire);
    }

    bool GifBackgroundView::isLoaded() const
    {
        return m_animation && m_animation->ready && m_animation->texture > 0;
    }

    const std::string& GifBackgroundView::path() const
    {
        static const std::string empty;
        return m_animation ? m_animation->path : empty;
    }

    void GifBackgroundView::frame(brls::FrameContext* ctx)
    {
        brls::View::frame(ctx);
        if (!m_animation || getVisibility() != brls::Visibility::VISIBLE)
            return;
        if (!m_animation->ready)
            return;
        if (advanceAnimation(*m_animation))
            uploadCurrentFrame(*m_animation);
        invalidate();
    }

    void GifBackgroundView::draw(NVGcontext* vg, float x, float y, float width,
                                 float height, brls::Style style,
                                 brls::FrameContext* ctx)
    {
        (void)style;
        (void)ctx;
        if (!vg || !isLoaded())
            return;

        if (m_animation->width == 0 || m_animation->height == 0)
            return;

        const float scale = std::max(width / m_animation->width, height / m_animation->height);
        const float drawWidth = m_animation->width * scale;
        const float drawHeight = m_animation->height * scale;
        const float drawX = x + (width - drawWidth) * 0.5f;
        const float drawY = y + (height - drawHeight) * 0.5f;
        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, width, height);
        nvgBeginPath(vg);
        nvgRect(vg, drawX, drawY, drawWidth, drawHeight);
        nvgFillPaint(vg, nvgImagePattern(vg, drawX, drawY, drawWidth, drawHeight,
                                         0.f, m_animation->texture, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }
} // namespace beiklive
