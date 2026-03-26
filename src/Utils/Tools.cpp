#include "Tools.hpp"

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
    if (!fs::exists(path) || !fs::is_directory(path))
        return 0;
    size_t count = 0;
    for ([[maybe_unused]] const auto& entry : fs::directory_iterator(path))
        ++count;
    return count;
}

std::string getFileSizeString(const fs::path& path) {
    if (!fs::exists(path) || !fs::is_regular_file(path))
        return "0 B";

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

std::string getIconPath(const std::string& path) {
    beiklive::enums::FileType type = getFileType(path);
    std::string path_prefix = "img/ui/" +
                               std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");

    switch (type) {
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

} // namespace beiklive::tools