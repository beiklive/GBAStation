#include "core/ThreeDsTitlePaths.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* ThreeDsRoot = "sdmc:/GBAStation/3ds";
    constexpr const char* ZeroId = "00000000000000000000000000000000";

    std::string titleRoot()
    {
        return std::string(ThreeDsRoot) + "/Nintendo 3DS/" + ZeroId + "/" + ZeroId + "/title";
    }

    std::string categoryPath(std::string_view titleId, std::string_view category)
    {
        const std::string normalized = beiklive::three_ds::normalizeTitleId(titleId);
        if (normalized.empty())
            return {};
        return titleRoot() + "/" + std::string(category) + "/" + normalized.substr(8);
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
        if (!fs::exists(path, ec))
            return !ec;
        ec.clear();
        fs::remove_all(path, ec);
        return !ec;
    }

    bool removeFileIfExists(const fs::path& path)
    {
        std::error_code ec;
        if (!fs::exists(path, ec))
            return !ec;
        ec.clear();
        if (fs::is_directory(path, ec))
            return removeRecursivelyIfExists(path);
        ec.clear();
        return fs::remove(path, ec) && !ec;
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
        return categoryPath(titleId, "0004000e");
    }

    std::string dlcTitlePath(std::string_view titleId)
    {
        return categoryPath(titleId, "0004008c");
    }

    std::string saveDataPath(std::string_view titleId)
    {
        const std::string base = baseTitlePath(titleId);
        return base.empty() ? std::string{} : base + "/data";
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

    bool deleteInstalledContentAndShaderCache(std::string_view titleId)
    {
        const std::string normalized = normalizeTitleId(titleId);
        if (normalized.empty())
            return false;

        bool success = true;
        std::unordered_set<std::string> titlePaths{
            baseTitlePath(normalized),
            updateTitlePath(normalized),
            dlcTitlePath(normalized),
        };
        const std::string originalCategory = normalized.substr(0, 8);
        titlePaths.insert(categoryPath(normalized, originalCategory));
        if (originalCategory == "00040002")
            titlePaths.insert(categoryPath(normalized, "00040002"));
        for (const auto& path : titlePaths)
        {
            if (!path.empty())
                success = removeRecursivelyIfExists(path) && success;
        }

        const fs::path shaderRoot = std::string(ThreeDsRoot) + "/shaders";
        const std::vector<fs::path> exactCachePaths{
            shaderRoot / "opengl" / "precompiled" / "separable" / (normalized + ".bin"),
            shaderRoot / "opengl" / "precompiled" / "conventional" / (normalized + ".bin"),
            shaderRoot / "opengl" / "transferable" / (normalized + ".bin"),
        };
        for (const auto& path : exactCachePaths)
            success = removeFileIfExists(path) && success;

        const fs::path transferable = shaderRoot / "vulkan" / "transferable";
        for (const char* stage : {"vs", "fs", "gs", "pl"})
        {
            success = removeFileIfExists(transferable / (normalized + "_" + stage + ".vkch")) && success;
            success = removeFileIfExists(transferable / (normalized + "_" + stage + "_temp.vkch")) && success;
        }

        const fs::path pipeline = shaderRoot / "vulkan" / "pipeline";
        std::error_code ec;
        for (fs::directory_iterator it(pipeline, ec), end; !ec && it != end; it.increment(ec))
        {
            const std::string filename = it->path().filename().string();
            if (filename.rfind(normalized, 0) == 0)
                success = removeFileIfExists(it->path()) && success;
        }
        if (ec && fs::exists(pipeline))
            success = false;
        return success;
    }
}
