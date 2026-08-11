#include "TextureManager.hpp"

#include <algorithm>

#include "GifDecoder.hpp"
#include "ImageLoader.hpp"

namespace beiklive
{
    namespace
    {
        bool endsWithGif(const std::string& path)
        {
            if (path.size() < 4)
                return false;
            auto lower = [](char c) {
                return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
            };
            const size_t p = path.size() - 4;
            return lower(path[p]) == '.' && lower(path[p + 1]) == 'g' &&
                   lower(path[p + 2]) == 'i' && lower(path[p + 3]) == 'f';
        }
    } // namespace

    int TextureManager::loadTexture(NVGcontext* vg, const std::string& path)
    {
        if (!vg || path.empty())
            return 0;

        // 已缓存：复用（含引用计数为 0 的延迟删除条目）
        auto found = m_cache.find(path);
        if (found != m_cache.end() && found->second.imageHandle > 0) {
            ++found->second.refCount;
            return found->second.imageHandle;
        }

        // GIF：全帧解码，单纹理逐帧更新
        if (endsWithGif(path)) {
#ifdef __SWITCH__
            constexpr int kMaxGifEdge = 384;
#else
            constexpr int kMaxGifEdge = 512;
#endif
            constexpr size_t kMaxGifFrames = 60;

            GifDecoded decoded;
            if (GifDecoder::decode(path, decoded, kMaxGifEdge, kMaxGifFrames)) {
                TextureResource resource;
                resource.path = path;
                resource.width = static_cast<int>(decoded.width);
                resource.height = static_cast<int>(decoded.height);
                resource.refCount = 1;
                resource.isGif = true;
                resource.gifLooping = decoded.looping;
                resource.gifDelaysMs.reserve(decoded.frames.size());
                resource.gifFrameHandles.reserve(decoded.frames.size());
                for (auto& frame : decoded.frames) {
                    resource.gifDelaysMs.push_back(frame.delayMs);
                    const int handle = nvgCreateImageRGBA(
                        vg, resource.width, resource.height, 0,
                        frame.rgba.data());
                    if (handle <= 0)
                        break;
                    resource.gifFrameHandles.push_back(handle);
                }
                if (resource.gifFrameHandles.empty())
                    return 0;
                resource.imageHandle = resource.gifFrameHandles.front();
                m_cache[path] = std::move(resource);
                return m_cache[path].imageHandle;
            }
            // 解码失败：回退到静态首帧（stb_image 可解 GIF 第一帧）
        }

        std::vector<uint8_t> pixels;
        int w = 0;
        int h = 0;
        if (!ImageLoader::load(path, pixels, w, h) || w <= 0 || h <= 0)
            return 0;

        TextureResource resource;
        resource.path = path;
        resource.width = w;
        resource.height = h;
        resource.refCount = 1;
        resource.imageHandle =
            nvgCreateImageRGBA(vg, w, h, 0, pixels.data());
        if (resource.imageHandle <= 0)
            return 0;

        m_cache[path] = std::move(resource);
        return m_cache[path].imageHandle;
    }

    void TextureManager::releaseTexture(NVGcontext* vg, const std::string& path)
    {
        auto found = m_cache.find(path);
        if (found == m_cache.end())
            return;

        auto& resource = found->second;
        if (resource.refCount > 0)
            --resource.refCount;
        if (resource.refCount > 0)
            return;

        // vg 不可用时（如析构阶段）保留条目，由 clear() 统一删除
        if (vg && resource.imageHandle > 0) {
            nvgDeleteImage(vg, resource.imageHandle);
            // GIF 每帧独立纹理
            for (size_t i = 1; i < resource.gifFrameHandles.size(); ++i) {
                if (resource.gifFrameHandles[i] > 0)
                    nvgDeleteImage(vg, resource.gifFrameHandles[i]);
            }
            m_cache.erase(found);
        }
    }

    void TextureManager::clear(NVGcontext* vg)
    {
        if (vg) {
            for (const auto& pair : m_cache) {
                if (pair.second.imageHandle > 0)
                    nvgDeleteImage(vg, pair.second.imageHandle);
                // GIF 每帧独立纹理
                for (size_t i = 1; i < pair.second.gifFrameHandles.size(); ++i) {
                    if (pair.second.gifFrameHandles[i] > 0)
                        nvgDeleteImage(vg, pair.second.gifFrameHandles[i]);
                }
            }
        }
        m_cache.clear();
    }

    bool TextureManager::isGifTexture(const std::string& path) const
    {
        auto found = m_cache.find(path);
        return found != m_cache.end() && found->second.isGif;
    }

    size_t TextureManager::gifFrameCount(const std::string& path) const
    {
        auto found = m_cache.find(path);
        if (found == m_cache.end() || !found->second.isGif)
            return 0;
        return found->second.gifFrameHandles.size();
    }

    bool TextureManager::gifLooping(const std::string& path) const
    {
        auto found = m_cache.find(path);
        if (found == m_cache.end() || !found->second.isGif)
            return true;
        return found->second.gifLooping;
    }

    uint32_t TextureManager::gifDelayMs(const std::string& path,
                                        size_t frame) const
    {
        auto found = m_cache.find(path);
        if (found == m_cache.end() || !found->second.isGif ||
            frame >= found->second.gifDelaysMs.size())
            return 100;
        return found->second.gifDelaysMs[frame];
    }

    int TextureManager::gifFrameTexture(const std::string& path,
                                        size_t frame) const
    {
        auto found = m_cache.find(path);
        if (found == m_cache.end() || !found->second.isGif)
            return 0;
        if (frame >= found->second.gifFrameHandles.size())
            return found->second.imageHandle;
        return found->second.gifFrameHandles[frame];
    }
} // namespace beiklive
