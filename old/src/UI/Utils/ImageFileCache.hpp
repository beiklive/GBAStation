#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace beiklive::UI {

/**
 * 图片资源的单线程文件字节缓存。
 *
 * 只能在主（渲染）线程访问。
 * 后台加载线程应通过 brls::sync() 将结果回传，再在主线程调用 storeBytes()。
 *
 * 超出 MAX_ENTRIES 时淘汰最近最少使用的条目。
 *
 * 用法：
 *   1. 首次加载 → getBytes()（返回 nullptr）→ 读磁盘 → storeBytes()
 *   2. 后续加载 → getBytes()（返回缓存指针，无磁盘 I/O）
 *   3. 启动 GameView 前 → clear() 释放内存供模拟器使用
 */
class ImageFileCache
{
  public:
    /// 内存中最多缓存的图片条目数。
    static constexpr size_t MAX_ENTRIES = 30;

    static ImageFileCache& instance();

    /// 返回 path 对应的缓存字节指针，未缓存则返回 nullptr。
    /// 访问时将该条目提升为最近使用。
    const std::vector<uint8_t>* getBytes(const std::string& path) const;

    /// 存储 path 的原始文件字节，已存在则覆盖。
    /// 缓存满时淘汰最久未使用的条目。
    void storeBytes(const std::string& path, std::vector<uint8_t> bytes);

    /// 清空所有缓存（启动 GameView 前调用）。
    void clear();

  private:
    ImageFileCache() = default;

    // LRU 追踪：front = 最近使用，back = 最久未使用
    mutable std::list<std::string> m_lruList;
    std::unordered_map<std::string, std::pair<std::vector<uint8_t>, std::list<std::string>::iterator>> m_cache;
};

} // namespace beiklive::UI
