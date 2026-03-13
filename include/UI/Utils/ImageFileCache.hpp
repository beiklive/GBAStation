#pragma once

#include <cstdint>
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
 * Usage:
 *   1. First load  → call getBytes() (returns nullptr) → read disk → storeBytes()
 *   2. Next  loads → call getBytes() (returns cached ptr, no disk I/O)
 *   3. Before GameView launch → call clear() to free RAM for the emulator.
 */
class ImageFileCache
{
  public:
    static ImageFileCache& instance();

    /// Returns a pointer to the cached bytes for @p path, or nullptr if not cached.
    const std::vector<uint8_t>* getBytes(const std::string& path) const;

    /// Store raw file bytes for @p path.  Overwrites any existing entry.
    void storeBytes(const std::string& path, std::vector<uint8_t> bytes);

    /// Remove all cached bytes (call before launching GameView).
    void clear();

  private:
    ImageFileCache() = default;
    std::unordered_map<std::string, std::vector<uint8_t>> m_cache;
};

} // namespace beiklive::UI
