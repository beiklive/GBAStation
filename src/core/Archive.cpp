#include "Archive.hpp"

#include "Tools.hpp"
#include "miniz.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <fcntl.h>
#include <mgba-util/vfs.h>

namespace beiklive::archive {
namespace {

std::string lowerExt(const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

bool isDirectoryName(const std::string& name)
{
    return name.empty() || name.back() == '/';
}

std::vector<Entry> listZip(const std::filesystem::path& path)
{
    std::vector<Entry> result;
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, path.string().c_str(), 0))
        return result;

    const mz_uint count = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&zip, i, &stat) ||
            isDirectoryName(stat.m_filename))
            continue;
        result.push_back({stat.m_filename, stat.m_uncomp_size});
    }
    mz_zip_reader_end(&zip);
    return result;
}

std::vector<Entry> list7z(const std::filesystem::path& path)
{
    std::vector<Entry> result;
    struct VDir* dir = VDirOpen7z(path.string().c_str(), O_RDONLY);
    if (!dir)
        return result;
    dir->rewind(dir);
    while (auto* item = dir->listNext(dir)) {
        if (item->type(item) == VFS_FILE) {
            // The 7z VFS intentionally exposes only names. Size is not needed
            // by the launcher, so leave it at zero for this backend.
            result.push_back({item->name(item), 0});
        }
    }
    dir->close(dir);
    return result;
}

bool extractZip(const std::filesystem::path& archivePath,
                const std::string& memberName,
                const std::filesystem::path& outputPath)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, archivePath.string().c_str(), 0))
        return false;
    const int index = mz_zip_reader_locate_file(&zip, memberName.c_str(), nullptr, 0);
    bool ok = index >= 0 && mz_zip_reader_extract_to_file(
        &zip, static_cast<mz_uint>(index), outputPath.string().c_str(), 0);
    mz_zip_reader_end(&zip);
    return ok;
}

bool extract7z(const std::filesystem::path& archivePath,
               const std::string& memberName,
               const std::filesystem::path& outputPath)
{
    struct VDir* dir = VDirOpen7z(archivePath.string().c_str(), O_RDONLY);
    if (!dir)
        return false;
    struct VFile* file = dir->openFile(dir, memberName.c_str(), O_RDONLY);
    if (!file) {
        dir->close(dir);
        return false;
    }

    std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
    bool ok = out.good();
    std::vector<char> buffer(128 * 1024);
    while (ok) {
        const ssize_t got = file->read(file, buffer.data(), buffer.size());
        if (got < 0) { ok = false; break; }
        if (got == 0) break;
        out.write(buffer.data(), got);
        if (!out.good()) { ok = false; break; }
    }
    file->close(file);
    dir->close(dir);
    if (!ok) {
        std::error_code ec;
        std::filesystem::remove(outputPath, ec);
    }
    return ok;
}

} // namespace

bool isArchive(const std::filesystem::path& path)
{
    const auto ext = lowerExt(path);
    return ext == ".zip" || ext == ".7z";
}

std::vector<Entry> list(const std::filesystem::path& path)
{
    const auto ext = lowerExt(path);
    if (ext == ".zip") return listZip(path);
    if (ext == ".7z") return list7z(path);
    return {};
}

bool extract(const std::filesystem::path& archivePath,
             const std::string& memberName,
             const std::filesystem::path& outputPath)
{
    std::error_code ec;
    std::filesystem::create_directories(outputPath.parent_path(), ec);
    const auto ext = lowerExt(archivePath);
    if (ext == ".zip") return extractZip(archivePath, memberName, outputPath);
    if (ext == ".7z") return extract7z(archivePath, memberName, outputPath);
    return false;
}

} // namespace beiklive::archive
