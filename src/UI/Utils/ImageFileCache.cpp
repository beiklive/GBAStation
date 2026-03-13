#include "UI/Utils/ImageFileCache.hpp"

namespace beiklive::UI {

ImageFileCache& ImageFileCache::instance()
{
    static ImageFileCache s_instance;
    return s_instance;
}

const std::vector<uint8_t>* ImageFileCache::getBytes(const std::string& path) const
{
    auto it = m_cache.find(path);
    if (it == m_cache.end())
        return nullptr;
    return &it->second;
}

void ImageFileCache::storeBytes(const std::string& path, std::vector<uint8_t> bytes)
{
    m_cache[path] = std::move(bytes);
}

void ImageFileCache::clear()
{
    m_cache.clear();
}

} // namespace beiklive::UI
