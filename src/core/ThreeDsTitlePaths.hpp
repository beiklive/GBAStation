#pragma once

#include <string>
#include <string_view>

namespace beiklive::three_ds
{
    std::string normalizeTitleId(std::string_view titleId);
    std::string readNcsdTitleId(const std::string& path);
    std::string extractTitleIdFromInstalledPath(const std::string& path);
    std::string resolveTitleId(std::string_view storedTitleId, const std::string& path);

    std::string baseTitlePath(std::string_view titleId);
    std::string updateTitlePath(std::string_view titleId);
    std::string dlcTitlePath(std::string_view titleId);
    std::string saveDataPath(std::string_view titleId);
    std::string exportDirectory();
    std::string backupDirectory(std::string_view titleId);
    std::string cheatFilePath(std::string_view titleId);
    std::string texturePath(std::string_view titleId);
    std::string disabledTexturePath(std::string_view titleId);
    std::string modPath(std::string_view titleId);
    std::string disabledModPath(std::string_view titleId);
    std::string disabledUpdateTitlePath(std::string_view titleId);
    std::string disabledDlcTitlePath(std::string_view titleId);

    bool clearShaderCache(std::string_view titleId);
    bool setManagedContentEnabled(const std::string& enabledPath,
                                  const std::string& disabledPath, bool enabled);
    bool deleteManagedContent(const std::string& enabledPath,
                              const std::string& disabledPath);
    bool deleteInstalledContent(std::string_view titleId);
    bool deleteInstalledContentAndShaderCache(std::string_view titleId);
}
