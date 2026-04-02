#include "Tools.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

// 如果 BK_RES 宏在其他头文件中定义，这里无需额外包含；否则可能需要包含相关头文件
// 假设 BK_RES 已在 core/common.h 中定义，此处已通过 file_tools.hpp 包含

namespace beiklive::tools {

std::string getFileExtension(const fs::path& path) {
    std::string ext = path.extension().string();
    if (ext.empty() || ext[0] != '.')
        return "";
    ext.erase(0, 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

beiklive::enums::FileType getFileType(const fs::path& path) {
    if (!fs::exists(path))
        return beiklive::enums::FileType::NONE;
    if (fs::is_directory(path))
        return beiklive::enums::FileType::DIRECTORY;

    std::string ext = getFileExtension(path);

    if (ext == "png")
        return beiklive::enums::FileType::IMAGE_FILE;
    if (ext == "zip")
        return beiklive::enums::FileType::ZIP_FILE;
    if (ext == "gba")
        return beiklive::enums::FileType::GBA_ROM;
    if (ext == "gbc")
        return beiklive::enums::FileType::GBC_ROM;
    if (ext == "gb")
        return beiklive::enums::FileType::GB_ROM;

    return beiklive::enums::FileType::NORMAL_FILE;
}

std::string getFileName(const fs::path& path) {
    return path.filename().string();
}

std::string getFileNameWithoutExtension(const std::string& filenameWithExt) {
    fs::path p(filenameWithExt);
    return p.stem().string();
}

size_t countEntries(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec))
        return 0;
    size_t count = 0;
    for ([[maybe_unused]] const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec) break;
        ++count;
    }
    return count;
}

std::string getFileSizeString(const fs::path& path) {
    if (!fs::exists(path) || !fs::is_regular_file(path))
        return " ";

    std::uintmax_t size = fs::file_size(path);
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unitIndex = 0;
    double readableSize = static_cast<double>(size);

    while (readableSize >= 1024.0 && unitIndex < 5) {
        readableSize /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << readableSize << " " << units[unitIndex];
    return oss.str();
}

std::string getParentPath(const std::string& path) {
    fs::path p(path);
    return p.parent_path().string();
}

std::string getIconPath(beiklive::enums::FileType type) {
    std::string path_prefix = "img/ui/" +
                               std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");
    switch (type) {
        case beiklive::enums::FileType::NONE:
        case beiklive::enums::FileType::DRIVE:
        case beiklive::enums::FileType::DIRECTORY:
            return BK_RES(path_prefix + "wenjianjia_64.png");
        case beiklive::enums::FileType::IMAGE_FILE:
            return BK_RES(path_prefix + "tupian.png");
        case beiklive::enums::FileType::ZIP_FILE:
            return BK_RES(path_prefix + "zip.png");
        case beiklive::enums::FileType::GBA_ROM:
            return BK_RES(path_prefix + "icon_gba.png");
        case beiklive::enums::FileType::GBC_ROM:
            return BK_RES(path_prefix + "icon_gb.png");
        case beiklive::enums::FileType::GB_ROM:
            return BK_RES(path_prefix + "icon_gb.png");
        default:
            return BK_RES(path_prefix + "wenjian.png");
    }
}
std::string getDefaultLogoPath(beiklive::enums::EmuPlatform platform)
{
    std::string path_prefix = "img/ui/";
    switch (platform)
    {
        case beiklive::enums::EmuPlatform::EmuGBA:
            return BK_RES(path_prefix + "icon_gba.png");
        case beiklive::enums::EmuPlatform::EmuGBC:
        case beiklive::enums::EmuPlatform::EmuGB:
            return BK_RES(path_prefix + "icon_gb.png");
        default:
            return BK_RES(path_prefix + "icon_gba.png");
    }
}
std::string getIconPath(const std::string& path) {
    return getIconPath(getFileType(path));
}

std::vector<std::string> getLogicalDrives() {
#ifdef _WIN32
    char buffer[256] = {};
    DWORD len = GetLogicalDriveStringsA(sizeof(buffer) - 1, buffer);
    std::vector<std::string> drives;
    for (char* p = buffer; len && *p; p += std::strlen(p) + 1)
        drives.push_back(std::string(p));
    return drives;
#else
    return {"/"};
#endif
}

bool isFileExists(const std::string& path) {
    return fs::exists(fs::path(path));
}


uint32_t crc32(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;

    uint32_t crc = 0xFFFFFFFF;
    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount())
    {
        for (long long int i = 0; i < file.gcount(); i++)
        {
            crc ^= (uint8_t)buffer[i];
            for (int j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }

    return ~crc;
}


std::string getTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time_t);
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%y-%m-%d %H-%M-%S");
    return oss.str();
}

std::string getFileModTimeStr(const std::string& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return "";
    // 将 file_time_type 转换为 system_clock::time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() +
        std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    char buf[64];
    std::tm* tm = std::localtime(&tt);
    if (!tm) return "";
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buf);
}

} // namespace beiklive::tools