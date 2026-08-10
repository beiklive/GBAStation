#include "TextureManager.hpp"

#include "ImageLoader.hpp"

namespace beiklive
{
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
            m_cache.erase(found);
        }
    }

    void TextureManager::clear(NVGcontext* vg)
    {
        if (vg) {
            for (const auto& pair : m_cache) {
                if (pair.second.imageHandle > 0)
                    nvgDeleteImage(vg, pair.second.imageHandle);
            }
        }
        m_cache.clear();
    }
} // namespace beiklive
