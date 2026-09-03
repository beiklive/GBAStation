#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

namespace beiklive::archive {

struct Entry {
    std::string name;
    std::uint64_t size = 0;
};

bool isArchive(const std::filesystem::path& path);
std::vector<Entry> list(const std::filesystem::path& path);
bool extract(const std::filesystem::path& archivePath,
             const std::string& memberName,
             const std::filesystem::path& outputPath);

} // namespace beiklive::archive
