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
    // Move to front of LRU list (most recently used)
    m_lruList.splice(m_lruList.begin(), m_lruList, it->second.second);
    return &it->second.first;
}

void ImageFileCache::storeBytes(const std::string& path, std::vector<uint8_t> bytes)
{
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        // Update existing entry and move to front
        it->second.first = std::move(bytes);
        m_lruList.splice(m_lruList.begin(), m_lruList, it->second.second);
        return;
    }

    // Evict LRU entry if at capacity
    if (m_cache.size() >= MAX_ENTRIES) {
        std::string lruKey = m_lruList.back(); // copy before pop_back invalidates the reference
        m_lruList.pop_back();
        m_cache.erase(lruKey);
    }

    // Insert new entry at the front (most recently used)
    m_lruList.push_front(path);
    m_cache.emplace(path, std::make_pair(std::move(bytes), m_lruList.begin()));
}

void ImageFileCache::clear()
{
    m_cache.clear();
    m_lruList.clear();
}

} // namespace beiklive::UI
