#include "core/ThreeDsTitlePaths.hpp"

#include <borealis/core/logger.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* ThreeDsRoot = "sdmc:/GBAStation/3ds";
    constexpr const char* ZeroId = "00000000000000000000000000000000";

    std::string titleRoot()
    {
        return std::string(ThreeDsRoot) + "/sdmc/Nintendo 3DS/" + ZeroId + "/" +
               ZeroId + "/title";
    }

    std::string categoryPath(std::string_view titleId, std::string_view category)
    {
        const std::string normalized = beiklive::three_ds::normalizeTitleId(titleId);
        if (normalized.empty())
            return {};
        return titleRoot() + "/" + std::string(category) + "/" + normalized.substr(8);
    }

    std::string disabledCategoryPath(std::string_view titleId, std::string_view category)
    {
        const std::string normalized = beiklive::three_ds::normalizeTitleId(titleId);
        if (normalized.empty())
            return {};
        return titleRoot() + "/" + std::string(category) + "/disabled_" +
               normalized.substr(8);
    }

    std::string extractTitleIdFromLegacySavePath(const std::string& path)
    {
        std::string normalizedPath = path;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
        std::string lowerPath = normalizedPath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

        constexpr std::string_view marker = "/saves/3ds/";
        const size_t markerPos = lowerPath.rfind(marker);
        if (markerPos == std::string::npos)
            return {};
        const size_t idBegin = markerPos + marker.size();
        const size_t idEnd = normalizedPath.find('/', idBegin);
        const size_t actualEnd = idEnd == std::string::npos ? normalizedPath.size() : idEnd;
        if (actualEnd - idBegin != 16)
            return {};
        return beiklive::three_ds::normalizeTitleId(
            std::string_view(normalizedPath).substr(idBegin, 16));
    }

    bool removeRecursivelyIfExists(const fs::path& path)
    {
        std::error_code ec;
        const bool exists = fs::exists(path, ec);
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] exists failed: path={} code={} error={}",
                path.string(), ec.value(), ec.message());
            return false;
        }
        if (!exists)
        {
            brls::Logger::info("[3DS Delete] skip missing directory: {}", path.string());
            return true;
        }

        std::vector<fs::path> descendants;
        {
            fs::recursive_directory_iterator iterator(
                path, fs::directory_options::skip_permission_denied, ec);
            const fs::recursive_directory_iterator end;
            while (!ec && iterator != end)
            {
                descendants.push_back(iterator->path());
                iterator.increment(ec);
            }
        }
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] directory scan failed: path={} collected={} code={} error={}",
                path.string(), descendants.size(), ec.value(), ec.message());
            return false;
        }

        std::sort(descendants.begin(), descendants.end(),
                  [](const fs::path& lhs, const fs::path& rhs) {
                      return lhs.native().size() > rhs.native().size();
                  });
        brls::Logger::info(
            "[3DS Delete] directory scan complete: path={} entries={}",
            path.string(), descendants.size());

        std::size_t removedCount = 0;
        bool success = true;
        for (const auto& descendant : descendants)
        {
            ec.clear();
            const bool removed = fs::remove(descendant, ec);
            if (ec)
            {
                brls::Logger::warning(
                    "[3DS Delete] remove entry failed: path={} code={} error={}",
                    descendant.string(), ec.value(), ec.message());
                success = false;
                continue;
            }
            if (!removed)
            {
                ec.clear();
                const bool remains = fs::exists(descendant, ec);
                if (ec || remains)
                {
                    brls::Logger::warning(
                        "[3DS Delete] entry remains: path={} code={} error={}",
                        descendant.string(), ec.value(),
                        ec ? ec.message() : "remove returned false");
                    success = false;
                    continue;
                }
            }
            else
            {
                ++removedCount;
            }
        }

        ec.clear();
        const bool rootRemoved = fs::remove(path, ec);
        if (ec || !rootRemoved)
        {
            std::error_code existsError;
            const bool remains = fs::exists(path, existsError);
            if (ec || existsError || remains)
            {
                brls::Logger::warning(
                    "[3DS Delete] remove root failed: path={} code={} error={} "
                    "exists_code={} remains={}",
                    path.string(), ec.value(), ec ? ec.message() : "remove returned false",
                    existsError.value(), remains);
                success = false;
            }
        }
        else
        {
            ++removedCount;
        }

        brls::Logger::info(
            "[3DS Delete] directory removal complete: path={} removed={} success={}",
            path.string(), removedCount, success);
        return success;
    }

    bool removeFileIfExists(const fs::path& path)
    {
        std::error_code ec;
        const bool exists = fs::exists(path, ec);
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] cache exists failed: path={} code={} error={}",
                path.string(), ec.value(), ec.message());
            return false;
        }
        if (!exists)
        {
            brls::Logger::info("[3DS Delete] skip missing cache: {}", path.string());
            return true;
        }

        ec.clear();
        if (fs::is_directory(path, ec))
            return removeRecursivelyIfExists(path);
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] cache type check failed: path={} code={} error={}",
                path.string(), ec.value(), ec.message());
            return false;
        }

        ec.clear();
        const bool removed = fs::remove(path, ec);
        if (ec || !removed)
        {
            brls::Logger::warning(
                "[3DS Delete] remove cache failed: path={} removed={} code={} error={}",
                path.string(), removed, ec.value(), ec ? ec.message() : "remove returned false");
            return false;
        }
        brls::Logger::info("[3DS Delete] removed cache: {}", path.string());
        return true;
    }

    bool startsWithTitleId(const std::string& filename, const std::string& titleId)
    {
        if (filename.size() < titleId.size())
            return false;
        for (size_t index = 0; index < titleId.size(); ++index)
        {
            const auto value = static_cast<unsigned char>(filename[index]);
            if (static_cast<char>(std::toupper(value)) != titleId[index])
                return false;
        }
        return true;
    }

    bool clearShaderRoot(const fs::path& root, const std::string& titleId)
    {
        brls::Logger::info(
            "[3DS Delete] shader scan begin: title_id={} root={}", titleId, root.string());
        std::error_code ec;
        const bool rootExists = fs::exists(root, ec);
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] shader root check failed: root={} code={} error={}",
                root.string(), ec.value(), ec.message());
            return false;
        }
        if (!rootExists)
        {
            brls::Logger::info("[3DS Delete] shader root missing, nothing to remove");
            return true;
        }

        std::vector<fs::path> matches;
        fs::recursive_directory_iterator iterator(
            root, fs::directory_options::skip_permission_denied, ec);
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] shader iterator open failed: root={} code={} error={}",
                root.string(), ec.value(), ec.message());
            return false;
        }
        const fs::recursive_directory_iterator end;
        std::string lastPath;
        while (!ec && iterator != end)
        {
            const fs::path current = iterator->path();
            lastPath = current.string();
            if (startsWithTitleId(current.filename().string(), titleId))
            {
                matches.push_back(current);
                if (iterator->is_directory(ec) && !ec)
                    iterator.disable_recursion_pending();
            }
            if (!ec)
                iterator.increment(ec);
        }
        if (ec)
        {
            brls::Logger::warning(
                "[3DS Delete] shader scan failed: last_path={} code={} error={}",
                lastPath, ec.value(), ec.message());
            return false;
        }

        std::sort(matches.begin(), matches.end(), [](const fs::path& lhs, const fs::path& rhs) {
            return lhs.native().size() > rhs.native().size();
        });
        brls::Logger::info(
            "[3DS Delete] shader scan complete: title_id={} matches={}",
            titleId, matches.size());
        bool success = true;
        for (const auto& path : matches)
            success = removeFileIfExists(path) && success;
        brls::Logger::info(
            "[3DS Delete] shader removal complete: title_id={} success={}",
            titleId, success);
        return success;
    }

    beiklive::three_ds::ShaderCacheStats shaderStatsForRoot(
        const fs::path& root, const std::string& titleId)
    {
        beiklive::three_ds::ShaderCacheStats stats;
        std::error_code ec;
        if (!fs::exists(root, ec)) {
            stats.valid = !ec;
            return stats;
        }

        fs::recursive_directory_iterator iterator(
            root, fs::directory_options::skip_permission_denied, ec);
        const fs::recursive_directory_iterator end;
        while (!ec && iterator != end) {
            const fs::path current = iterator->path();
            if (!startsWithTitleId(current.filename().string(), titleId)) {
                iterator.increment(ec);
                continue;
            }

            if (iterator->is_directory(ec) && !ec) {
                iterator.disable_recursion_pending();
                fs::recursive_directory_iterator child(
                    current, fs::directory_options::skip_permission_denied, ec);
                while (!ec && child != end) {
                    if (child->is_regular_file(ec) && !ec) {
                        const auto size = child->file_size(ec);
                        if (!ec) {
                            ++stats.fileCount;
                            stats.totalBytes += size;
                        }
                    }
                    if (!ec)
                        child.increment(ec);
                }
            } else if (!ec && iterator->is_regular_file(ec) && !ec) {
                const auto size = iterator->file_size(ec);
                if (!ec) {
                    ++stats.fileCount;
                    stats.totalBytes += size;
                }
            }
            if (!ec)
                iterator.increment(ec);
        }
        stats.valid = !ec;
        return stats;
    }

    bool isManagedPath(const fs::path& path)
    {
        std::string value = path.lexically_normal().generic_string();
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        constexpr std::string_view root = "sdmc:/gbastation/3ds/";
        return value.rfind(root, 0) == 0 && value.size() > root.size();
    }
}

namespace beiklive::three_ds
{
    std::string normalizeTitleId(std::string_view titleId)
    {
        if (titleId.rfind("0x", 0) == 0 || titleId.rfind("0X", 0) == 0)
            titleId.remove_prefix(2);
        if (titleId.size() != 16)
            return {};

        std::string normalized;
        normalized.reserve(16);
        for (const unsigned char value : titleId)
        {
            if (!std::isxdigit(value))
                return {};
            normalized.push_back(static_cast<char>(std::toupper(value)));
        }
        return normalized;
    }

    std::string readNcsdTitleId(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return {};

        std::array<unsigned char, 0x110> header{};
        file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
        if (file.gcount() != static_cast<std::streamsize>(header.size()) ||
            header[0x100] != 'N' || header[0x101] != 'C' ||
            header[0x102] != 'S' || header[0x103] != 'D')
            return {};

        unsigned long long mediaId = 0;
        for (int i = 0; i < 8; ++i)
            mediaId |= static_cast<unsigned long long>(header[0x108 + i]) << (i * 8);
        if (mediaId == 0)
            return {};

        std::ostringstream output;
        output << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << mediaId;
        return output.str();
    }

    std::string extractTitleIdFromInstalledPath(const std::string& path)
    {
        std::string normalizedPath = path;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
        std::string lowerPath = normalizedPath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

        constexpr std::string_view marker = "/title/";
        const size_t markerPos = lowerPath.find(marker);
        if (markerPos == std::string::npos)
            return {};
        const size_t highBegin = markerPos + marker.size();
        const size_t highEnd = lowerPath.find('/', highBegin);
        const size_t lowBegin = highEnd == std::string::npos ? std::string::npos : highEnd + 1;
        const size_t lowEnd = lowBegin == std::string::npos ? std::string::npos
                                                            : lowerPath.find('/', lowBegin);
        if (highEnd == std::string::npos || highEnd - highBegin != 8 ||
            lowBegin == std::string::npos)
            return {};

        const size_t actualLowEnd = lowEnd == std::string::npos ? normalizedPath.size() : lowEnd;
        if (actualLowEnd - lowBegin != 8)
            return {};
        return normalizeTitleId(normalizedPath.substr(highBegin, 8) +
                                normalizedPath.substr(lowBegin, 8));
    }

    std::string resolveTitleId(std::string_view storedTitleId, const std::string& path)
    {
        std::string titleId = normalizeTitleId(storedTitleId);
        if (!titleId.empty())
            return titleId;
        titleId = extractTitleIdFromInstalledPath(path);
        if (!titleId.empty())
            return titleId;
        titleId = extractTitleIdFromLegacySavePath(path);
        if (!titleId.empty())
            return titleId;
        return readNcsdTitleId(path);
    }

    std::string baseTitlePath(std::string_view titleId)
    {
        return categoryPath(titleId, "00040000");
    }

    std::string updateTitlePath(std::string_view titleId)
    {
        return categoryPath(titleId, "0004000E");
    }

    std::string dlcTitlePath(std::string_view titleId)
    {
        return categoryPath(titleId, "0004008C");
    }

    std::string saveDataPath(std::string_view titleId)
    {
        const std::string titlePath = baseTitlePath(titleId);
        return titlePath.empty() ? std::string{} : titlePath + "/data";
    }

    std::string exportDirectory()
    {
        return "sdmc:/GBAStation/export/3DS";
    }

    std::string backupDirectory(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        return normalized.empty() ? std::string{}
                                  : "sdmc:/GBAStation/backup/3DS/" + normalized;
    }

    std::string cheatFilePath(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        return normalized.empty() ? std::string{}
                                  : std::string(ThreeDsRoot) + "/cheats/" + normalized + ".txt";
    }

    std::string texturePath(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        return normalized.empty() ? std::string{}
                                  : std::string(ThreeDsRoot) + "/load/textures/" + normalized;
    }

    std::string disabledTexturePath(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        return normalized.empty() ? std::string{}
                                  : std::string(ThreeDsRoot) + "/load/textures/disabled_" + normalized;
    }

    std::string modPath(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        return normalized.empty() ? std::string{}
                                  : std::string(ThreeDsRoot) + "/load/mods/" + normalized;
    }

    std::string disabledModPath(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        return normalized.empty() ? std::string{}
                                  : std::string(ThreeDsRoot) + "/load/mods/disabled_" + normalized;
    }

    std::string disabledUpdateTitlePath(std::string_view titleId)
    {
        return disabledCategoryPath(titleId, "0004000E");
    }

    std::string disabledDlcTitlePath(std::string_view titleId)
    {
        return disabledCategoryPath(titleId, "0004008C");
    }

    bool clearShaderCache(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        if (normalized.empty())
            return false;

        return clearShaderRoot(fs::path(ThreeDsRoot) / "shaders", normalized);
    }

    ShaderCacheStats shaderCacheStats(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        if (normalized.empty())
            return {0, 0, false};
        return shaderStatsForRoot(fs::path(ThreeDsRoot) / "shaders", normalized);
    }

    bool setManagedContentEnabled(const std::string& enabledPath,
                                  const std::string& disabledPath, bool enabled)
    {
        const fs::path source = enabled ? fs::path(disabledPath) : fs::path(enabledPath);
        const fs::path destination = enabled ? fs::path(enabledPath) : fs::path(disabledPath);
        if (!isManagedPath(source) || !isManagedPath(destination) ||
            source.parent_path() != destination.parent_path())
            return false;

        std::error_code ec;
        const bool sourceExists = fs::exists(source, ec);
        if (ec || !sourceExists)
            return false;
        ec.clear();
        const bool destinationExists = fs::exists(destination, ec);
        if (ec || destinationExists)
            return false;
        ec.clear();
        fs::rename(source, destination, ec);
        return !ec;
    }

    bool deleteManagedContent(const std::string& enabledPath,
                              const std::string& disabledPath)
    {
        const fs::path enabled = enabledPath;
        const fs::path disabled = disabledPath;
        if (!isManagedPath(enabled) || !isManagedPath(disabled) ||
            enabled.parent_path() != disabled.parent_path())
            return false;
        const bool enabledRemoved = removeRecursivelyIfExists(enabled);
        const bool disabledRemoved = removeRecursivelyIfExists(disabled);
        return enabledRemoved && disabledRemoved;
    }

    bool deleteInstalledContent(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        if (normalized.empty())
        {
            brls::Logger::warning(
                "[3DS Delete] invalid title id: {}", std::string(titleId));
            return false;
        }

        brls::Logger::info(
            "[3DS Delete] installed content removal begin: title_id={}", normalized);
        bool success = true;
        const std::array<std::string, 5> titlePaths{
            baseTitlePath(normalized),
            updateTitlePath(normalized),
            dlcTitlePath(normalized),
            disabledUpdateTitlePath(normalized),
            disabledDlcTitlePath(normalized),
        };
        for (const auto& path : titlePaths)
        {
            if (!path.empty())
            {
                const bool removed = removeRecursivelyIfExists(path);
                brls::Logger::info(
                    "[3DS Delete] title path result: path={} success={}", path, removed);
                success = removed && success;
            }
        }
        brls::Logger::info(
            "[3DS Delete] installed content removal complete: title_id={} success={}",
            normalized, success);
        return success;
    }

    bool deleteInstalledContentAndShaderCache(std::string_view titleId)
    {
        const bool contentRemoved = deleteInstalledContent(titleId);
        const bool cacheRemoved = clearShaderCache(titleId);
        brls::Logger::info(
            "[3DS Delete] final result: title_id={} content_removed={} cache_removed={} success={}",
            std::string(titleId), contentRemoved, cacheRemoved,
            contentRemoved && cacheRemoved);
        return contentRemoved && cacheRemoved;
    }
}
