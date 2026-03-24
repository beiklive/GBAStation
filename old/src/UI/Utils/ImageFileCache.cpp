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
    // 移动到 LRU 链表头部（最近使用）
    m_lruList.splice(m_lruList.begin(), m_lruList, it->second.second);
    return &it->second.first;
}

void ImageFileCache::storeBytes(const std::string& path, std::vector<uint8_t> bytes)
{
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        // 更新已有条目并移至链表头部
        it->second.first = std::move(bytes);
        m_lruList.splice(m_lruList.begin(), m_lruList, it->second.second);
        return;
    }

    // 容量已满时淘汰最久未使用的条目
    if (m_cache.size() >= MAX_ENTRIES) {
        std::string lruKey = m_lruList.back(); // 复制 key，防止 pop_back 后引用失效
        m_lruList.pop_back();
        m_cache.erase(lruKey);
    }

    // 在链表头部插入新条目（最近使用）
    m_lruList.push_front(path);
    m_cache.emplace(path, std::make_pair(std::move(bytes), m_lruList.begin()));
}

void ImageFileCache::clear()
{
    m_cache.clear();
    m_lruList.clear();
}

} // namespace beiklive::UI
