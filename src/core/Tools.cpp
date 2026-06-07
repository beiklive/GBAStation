#include "Tools.hpp"
#include "enums.h"

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
    if (ext == "nes" || ext == "fds")
        return beiklive::enums::FileType::NES_ROM;
    if (ext == "sfc" || ext == "smc")
        return beiklive::enums::FileType::SNES_ROM;
    if (ext == "md" || ext == "gen" || ext == "bin" || ext == "smd"
        || ext == "sms" || ext == "gg" || ext == "sg" || ext == "cue")
        return beiklive::enums::FileType::GENESIS_ROM;

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
    // 注意：DARK 主题返回 "light/" 前缀（浅色图标适合暗色背景），LIGHT 主题返回 "dark/" 前缀
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

std::string getIconPathPrefix() {
    // 必须在 UI 线程调用，返回主题相关图标路径前缀
    // 注意：Switch 默认使用暗色主题（DARK），因此返回 "light/" 前缀（浅色图标适合暗色背景）
    return "img/ui/" + std::string(
        (brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK)
        ? "light/" : "dark/");
}

std::string getIconPathWithPrefix(beiklive::enums::FileType type, const std::string& prefix) {
    // 使用预计算的前缀（可在后台线程调用，无需访问 UI API）
    switch (type) {
        case beiklive::enums::FileType::NONE:
        case beiklive::enums::FileType::DRIVE:
        case beiklive::enums::FileType::DIRECTORY:
            return BK_RES(prefix + "wenjianjia_64.png");
        case beiklive::enums::FileType::IMAGE_FILE:
            return BK_RES(prefix + "tupian.png");
        case beiklive::enums::FileType::ZIP_FILE:
            return BK_RES(prefix + "zip.png");
        case beiklive::enums::FileType::GBA_ROM:
            return BK_RES(prefix + "icon_gba.png");
        case beiklive::enums::FileType::GBC_ROM:
            return BK_RES(prefix + "icon_gb.png");
        case beiklive::enums::FileType::GB_ROM:
            return BK_RES(prefix + "icon_gb.png");
        default:
            return BK_RES(prefix + "wenjian.png");
    }
}
std::string getDefaultLogoPath(beiklive::enums::EmuPlatform platform)
{
    std::string path_prefix = "img/ui/";
    switch (platform)
    {
        case beiklive::enums::EmuPlatform::EmuGBA:
            return BK_RES(path_prefix + "gba.png");
        case beiklive::enums::EmuPlatform::EmuGBC:
            return BK_RES(path_prefix + "gbc.png");
        case beiklive::enums::EmuPlatform::EmuGB:
            return BK_RES(path_prefix + "gb.png");
        case beiklive::enums::EmuPlatform::EmuNES:
            return BK_RES(path_prefix + "gba.png");
        case beiklive::enums::EmuPlatform::EmuSNES:
            return BK_RES(path_prefix + "gba.png");
        case beiklive::enums::EmuPlatform::EmuGenesis:
            return BK_RES(path_prefix + "gba.png");
        default:
            return BK_RES(path_prefix + "gba.png");
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
    static uint32_t table[256];
    static bool initialized = false;

    if (!initialized)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t c = i;

            for (int j = 0; j < 8; j++)
            {
                c = (c & 1)
                    ? (0xEDB88320 ^ (c >> 1))
                    : (c >> 1);
            }

            table[i] = c;
        }

        initialized = true;
    }

    FILE* fp = fopen(path.c_str(), "rb");

    if (!fp)
        return 0;

    uint32_t crc = 0xFFFFFFFF;

    uint8_t buffer[16384];

    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            crc = table[
                (crc ^ buffer[i]) & 0xFF
            ] ^ (crc >> 8);
        }
    }

    fclose(fp);

    return ~crc;
}

std::string crc32ToHex(uint32_t crc)
{
    static const char* hex = "0123456789ABCDEF";

    std::string result(8, '0');

    for (int i = 7; i >= 0; i--)
    {
        result[i] = hex[crc & 0xF];
        crc >>= 4;
    }

    return result;
}


std::string getTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time_t);
    char buf[64];
    // 存储格式使用 "yy-mm-dd HH-MM-SS"，便于字符串字典序排序
    std::strftime(buf, sizeof(buf), "%y-%m-%d %H-%M-%S", now_tm);
    return std::string(buf);
}

std::string formatTimestampForDisplay(const std::string& ts) {
    if (ts.empty()) return ts;
    // 解析存储格式 "26-03-31 09-38-11"，转为显示格式 "26-03-31 09时38分"
    int year, month, day, hour, min, sec;
    if (std::sscanf(ts.c_str(), "%d-%d-%d %d-%d-%d", &year, &month, &day, &hour, &min, &sec) == 6
        && month >= 1 && month <= 12
        && day   >= 1 && day   <= 31
        && hour  >= 0 && hour  <= 23
        && min   >= 0 && min   <= 59
        && sec   >= 0 && sec   <= 59) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02d-%02d-%02d %02d时%02d分", year, month, day, hour, min);
        return std::string(buf);
    }
    // 解析失败时原样返回（兼容旧格式数据）
    return ts;
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

// ── 按键字符串解析 ──────────────────────────────────────────────────────────

/// 将单个 combo 字符串（如 "LB+START"）解析为 GameInputPad ID 列表。
/// 按 '+' 分割各按键名，大小写不敏感。
/// "none" 或空字符串返回空列表。
std::vector<int> parsePadCombo(const std::string& combo)
{
    // 转大写以实现大小写不敏感
    std::string upper;
    upper.reserve(combo.size());
    for (char c : combo)
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));

    if (upper.empty() || upper == "NONE")
        return {};

    std::vector<int> result;
    // 按 '+' 分割
    std::istringstream iss(upper);
    std::string part;
    while (std::getline(iss, part, '+')) {
        if (part.empty()) continue;
        // trim 首尾空格
        size_t s = 0, e = part.size();
        while (s < e && part[s] == ' ') ++s;
        while (e > s && part[e - 1] == ' ') --e;
        if (s >= e) continue;
        std::string name = part.substr(s, e - s);
        // 在 k_gameInputNames 中查找
        for (const auto& entry : beiklive::k_gameInputNames) {
            if (entry.name == name) {
                result.push_back(entry.id);
                break;
            }
        }
    }
    return result;
}

/// 将键盘按键字符串（如 "A"、"SPACE+ENTER"）解析为 BrlsKeyboardScancode 列表。
std::vector<int> parseKbdCombo(const std::string& combo)
{
    std::string upper;
    upper.reserve(combo.size());
    for (char c : combo)
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));

    if (upper.empty() || upper == "NONE")
        return {};

    std::vector<int> result;
    std::istringstream iss(upper);
    std::string part;
    while (std::getline(iss, part, '+')) {
        if (part.empty()) continue;
        // trim 首尾空格
        size_t s = 0, e = part.size();
        while (s < e && part[s] == ' ') ++s;
        while (e > s && part[e - 1] == ' ') --e;
        if (s >= e) continue;
        std::string name = part.substr(s, e - s);
        for (const auto& entry : beiklive::k_kbdInputNames) {
            if (entry.name == name) {
                result.push_back(entry.id);
                break;
            }
        }
    }
    return result;
}

/// 将多 combo 字符串（逗号分隔，如 "A,LB+A"）解析为多组 combo。
/// 外层 vector 为各组合（OR 关系），内层为各按键 ID（AND 关系）。
/// "none" 或空字符串返回空列表。
std::vector<std::vector<int>> parseMultiCombo(const std::string& val)
{
    if (val.empty()) return {};

    std::vector<std::vector<int>> result;
    std::istringstream iss(val);
    std::string comboStr;
    while (std::getline(iss, comboStr, '|')) {
        if (comboStr.empty()) continue;
        auto combo = parsePadCombo(comboStr);
        if (combo.empty())
            combo = parseKbdCombo(comboStr);
        if (!combo.empty())
            result.push_back(std::move(combo));
    }
    return result;
}

// ── 平台工具 ──────────────────────────────────────────────────────────────

std::string platformName(int platform) {
    switch (static_cast<beiklive::enums::EmuPlatform>(platform)) {
        case beiklive::enums::EmuPlatform::EmuGBA: return "GBA";
        case beiklive::enums::EmuPlatform::EmuGBC: return "GBC";
        case beiklive::enums::EmuPlatform::EmuGB:  return "GB";
        case beiklive::enums::EmuPlatform::EmuNES: return "FC";
        case beiklive::enums::EmuPlatform::EmuSNES: return "SFC";
        case beiklive::enums::EmuPlatform::EmuGenesis: return "MD";
        default: return "";
    }
}

std::string platformOverlayKey(int platform) {
    switch (static_cast<beiklive::enums::EmuPlatform>(platform)) {
        case beiklive::enums::EmuPlatform::EmuGBA: return beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBA_PATH;
        case beiklive::enums::EmuPlatform::EmuGBC: return beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBC_PATH;
        case beiklive::enums::EmuPlatform::EmuGB:  return beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GB_PATH;
        case beiklive::enums::EmuPlatform::EmuNES: return beiklive::SettingKey::KEY_DISPLAY_OVERLAY_NES_PATH;
        case beiklive::enums::EmuPlatform::EmuSNES: return beiklive::SettingKey::KEY_DISPLAY_OVERLAY_SNES_PATH;
        case beiklive::enums::EmuPlatform::EmuGenesis: return beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GENESIS_PATH;
        default: return "";
    }
}

std::string platformShaderKey(int platform) {
    switch (static_cast<beiklive::enums::EmuPlatform>(platform)) {
        case beiklive::enums::EmuPlatform::EmuGBA: return beiklive::SettingKey::KEY_DISPLAY_SHADER_GBA_PATH;
        case beiklive::enums::EmuPlatform::EmuGBC: return beiklive::SettingKey::KEY_DISPLAY_SHADER_GBC_PATH;
        case beiklive::enums::EmuPlatform::EmuGB:  return beiklive::SettingKey::KEY_DISPLAY_SHADER_GB_PATH;
        case beiklive::enums::EmuPlatform::EmuNES: return beiklive::SettingKey::KEY_DISPLAY_SHADER_NES_PATH;
        case beiklive::enums::EmuPlatform::EmuSNES: return beiklive::SettingKey::KEY_DISPLAY_SHADER_SNES_PATH;
        case beiklive::enums::EmuPlatform::EmuGenesis: return beiklive::SettingKey::KEY_DISPLAY_SHADER_GENESIS_PATH;
        default: return "";
    }
}

std::string platformBadgeName(int platform) {
    switch (static_cast<beiklive::enums::EmuPlatform>(platform)) {
        case beiklive::enums::EmuPlatform::EmuGBA: return "GBA";
        case beiklive::enums::EmuPlatform::EmuGBC: return "GBC";
        case beiklive::enums::EmuPlatform::EmuGB:  return "GB";
        case beiklive::enums::EmuPlatform::EmuNES: return "FC";
        case beiklive::enums::EmuPlatform::EmuSNES: return "SFC";
        case beiklive::enums::EmuPlatform::EmuGenesis: return "MD";
        default: return "";
    }
}

// ── 存档路径工具 ──────────────────────────────────────────────────────────

std::string slotName(int slot) {
    return (slot == 0) ? "自动存档" : "槽位 " + std::to_string(slot);
}

std::string getStatePath(const std::string& saveDir, const std::string& romPath, int slot) {
    std::string stem = std::filesystem::path(romPath).stem().string();
    std::string sep;
    if (!saveDir.empty() && saveDir.back() != '/' && saveDir.back() != '\\')
        sep = "/";
    return saveDir + sep + stem + ".ss" + std::to_string(slot);
}

std::string getStateThumbPath(const std::string& saveDir, const std::string& romPath, int slot) {
    return getStatePath(saveDir, romPath, slot) + ".png";
}

bool stateExists(const std::string& saveDir, const std::string& romPath, int slot) {
    std::error_code ec;
    return std::filesystem::exists(getStatePath(saveDir, romPath, slot), ec);
}

// ── 时间工具 ──────────────────────────────────────────────────────────────

std::string formatPlayTime(int totalSeconds) {
    if (totalSeconds < 60) return "不到 1 分钟";
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    if (hours > 0)
        return std::to_string(hours) + " 小时 " + std::to_string(minutes) + " 分钟";
    return std::to_string(minutes) + " 分钟";
}

int versionCode(const std::string& version) {
    // 去除前缀 "v" 或 "V"
    std::string v = version;
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V'))
        v = v.substr(1);
    // 按 '.' 分割，取前三段，不足补 0
    int parts[3] = {0, 0, 0};
    size_t pos = 0;
    for (int i = 0; i < 3; ++i) {
        auto dot = v.find('.', pos);
        std::string seg = (dot == std::string::npos) ? v.substr(pos) : v.substr(pos, dot - pos);
        parts[i] = std::stoi(seg.empty() ? "0" : seg);
        if (dot == std::string::npos) break;
        pos = dot + 1;
    }
    return parts[0] * 1000000 + parts[1] * 1000 + parts[2];
}


std::string readGbaGameID(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
        return "";

    // Game ID 位于 0xAC
    file.seekg(0xAC, std::ios::beg);

    char gameId[4];

    if (!file.read(gameId, 4))
        return "";

    return std::string(gameId, 4);
}


} // namespace beiklive::tools