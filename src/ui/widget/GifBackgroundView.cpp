#include "GifBackgroundView.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>

#include "core/common.h"
#include "ui/view/iisu/GifDecoder.hpp"

namespace beiklive
{
    struct GifBackgroundView::SharedAnimation
    {
        ~SharedAnimation()
        {
            // The weak cache deliberately does not own animations.  Once the
            // last background view drops its reference, release every GPU
            // image while the NanoVG context is still available.
            NVGcontext* vg = brls::Application::getNVGContext();
            if (!vg)
                return;
            for (const auto& frame : frames) {
                if (frame.texture > 0)
                    nvgDeleteImage(vg, frame.texture);
            }
        }

        std::string path;
        std::vector<Frame> frames;
        size_t currentFrame = 0;
        float elapsedMs = 0.f;
        bool looping = true;
        std::chrono::steady_clock::time_point lastTick{};
    };

    namespace
    {
        using SharedAnimation = GifBackgroundView::SharedAnimation;

        // The cache indexes currently live animations but must not keep old
        // backgrounds alive after every Box has switched to another image.
        std::unordered_map<std::string, std::weak_ptr<SharedAnimation>> g_backgroundGifCache;

        std::shared_ptr<SharedAnimation> decodeAnimation(const std::string& path)
        {
            NVGcontext* vg = brls::Application::getNVGContext();
            if (!vg || path.empty())
                return nullptr;

#ifdef __SWITCH__
            constexpr int kMaxGifEdge = 384;
#else
            constexpr int kMaxGifEdge = 512;
#endif
            constexpr size_t kMaxGifFrames = 60;

            GifDecoded decoded;
            if (!GifDecoder::decode(path, decoded, kMaxGifEdge, kMaxGifFrames))
                return nullptr;

            auto animation = std::make_shared<SharedAnimation>();
            animation->path = path;
            animation->frames.reserve(decoded.frames.size());
            for (const auto& source : decoded.frames) {
                const int texture = nvgCreateImageRGBA(
                    vg, static_cast<int>(decoded.width), static_cast<int>(decoded.height),
                    NVG_IMAGE_PREMULTIPLIED, source.rgba.data());
                if (texture <= 0) {
                    // SharedAnimation's destructor releases textures already
                    // created for earlier frames.
                    return nullptr;
                }
                animation->frames.push_back({texture, std::max<uint32_t>(1, source.delayMs)});
            }
            if (animation->frames.empty())
                return nullptr;
            animation->looping = decoded.looping;
            animation->lastTick = std::chrono::steady_clock::now();
            return animation;
        }

        void advanceAnimation(SharedAnimation& animation)
        {
            if (animation.frames.size() < 2)
                return;

            const auto now = std::chrono::steady_clock::now();
            if (animation.lastTick.time_since_epoch().count() == 0) {
                animation.lastTick = now;
                return;
            }
            const float deltaMs = std::min(250.f, std::chrono::duration<float, std::milli>(
                now - animation.lastTick).count());
            animation.lastTick = now;
            const float speed = std::clamp(
                GET_SETTING_KEY_FLOAT(SettingKey::KEY_UI_BG_GIF_SPEED, 1.f), 0.1f, 4.f);
            animation.elapsedMs += deltaMs * speed;

            size_t guard = 0;
            while (animation.elapsedMs >= animation.frames[animation.currentFrame].delayMs) {
                animation.elapsedMs -= static_cast<float>(animation.frames[animation.currentFrame].delayMs);
                if (animation.currentFrame + 1 < animation.frames.size()) {
                    ++animation.currentFrame;
                } else if (animation.looping) {
                    animation.currentFrame = 0;
                } else {
                    animation.elapsedMs = 0.f;
                    break;
                }
                if (++guard > 120)
                    break;
            }
        }
    } // namespace

    void GifBackgroundView::clear()
    {
        m_animation.reset();
    }

    bool GifBackgroundView::load(const std::string& path)
    {
        if (m_animation && m_animation->path == path)
            return true;

        if (const auto found = g_backgroundGifCache.find(path);
            found != g_backgroundGifCache.end()) {
            if (auto cached = found->second.lock()) {
                m_animation = std::move(cached);
                return true;
            }
            g_backgroundGifCache.erase(found);
        }

        auto decoded = decodeAnimation(path);
        if (!decoded)
            return false;
        m_animation = decoded;
        g_backgroundGifCache[path] = decoded;
        return true;
    }

    bool GifBackgroundView::isLoaded() const
    {
        return m_animation && !m_animation->frames.empty();
    }

    void GifBackgroundView::frame(brls::FrameContext* ctx)
    {
        brls::View::frame(ctx);
        if (!m_animation || getVisibility() != brls::Visibility::VISIBLE)
            return;
        advanceAnimation(*m_animation);
        invalidate();
    }

    void GifBackgroundView::draw(NVGcontext* vg, float x, float y, float width,
                                 float height, brls::Style style,
                                 brls::FrameContext* ctx)
    {
        (void)style;
        (void)ctx;
        if (!vg || !m_animation || m_animation->frames.empty())
            return;

        int imageWidth = 0;
        int imageHeight = 0;
        const int texture = m_animation->frames[m_animation->currentFrame].texture;
        nvgImageSize(vg, texture, &imageWidth, &imageHeight);
        if (imageWidth <= 0 || imageHeight <= 0)
            return;

        const float scale = std::max(width / imageWidth, height / imageHeight);
        const float drawWidth = imageWidth * scale;
        const float drawHeight = imageHeight * scale;
        const float drawX = x + (width - drawWidth) * 0.5f;
        const float drawY = y + (height - drawHeight) * 0.5f;
        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, width, height);
        nvgBeginPath(vg);
        nvgRect(vg, drawX, drawY, drawWidth, drawHeight);
        nvgFillPaint(vg, nvgImagePattern(vg, drawX, drawY, drawWidth, drawHeight,
                                         0.f, texture, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }
} // namespace beiklive
