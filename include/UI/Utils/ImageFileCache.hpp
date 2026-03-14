#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace beiklive::UI {

/**
 * Single-threaded file-byte cache for image resources.
 *
 * Must only be accessed from the main (render) thread.
 * Background loading threads should pass their result back via brls::sync()
 * and then call storeBytes() from the main thread.
 *
 * Evicts the least-recently-used entry when the cache exceeds MAX_ENTRIES.
 *
 * Usage:
 *   1. First load  → call getBytes() (returns nullptr) → read disk → storeBytes()
 *   2. Next  loads → call getBytes() (returns cached ptr, no disk I/O)
 *   3. Before GameView launch → call clear() to free RAM for the emulator.
 */
class ImageFileCache
{
  public:
    /// Maximum number of image file entries kept in memory simultaneously.
    static constexpr size_t MAX_ENTRIES = 30;

    static ImageFileCache& instance();

    /// Returns a pointer to the cached bytes for @p path, or nullptr if not cached.
    /// Accessing an entry promotes it to most-recently-used in the eviction order.
    const std::vector<uint8_t>* getBytes(const std::string& path) const;

    /// Store raw file bytes for @p path.  Overwrites any existing entry.
    /// Evicts the least-recently-used entry if the cache is full.
    void storeBytes(const std::string& path, std::vector<uint8_t> bytes);

    /// Remove all cached bytes (call before launching GameView).
    void clear();

  private:
    ImageFileCache() = default;

    // LRU tracking: front = most recently used, back = least recently used
    mutable std::list<std::string> m_lruList;
    std::unordered_map<std::string, std::pair<std::vector<uint8_t>, std::list<std::string>::iterator>> m_cache;
};

} // namespace beiklive::UI
