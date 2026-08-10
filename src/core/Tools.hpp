#pragma once

#include "common.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive::tools {

/// 获取 Switch 校准数据中的设备 ID，并缓存结果；非 Switch 平台返回 0。
uint64_t getDeviceId();

/// 在 URL 后追加 device_id 查询参数。
std::string appendDeviceIdParameter(const std::string& url);

// 获取文件扩展名（小写形式，不含点号）
std::string getFileExtension(const fs::path& path);

// 主函数：根据文件路径判断类型
beiklive::enums::FileType getFileType(const fs::path& path);

// 检测压缩包内容。ZIP 会枚举内部文件；7Z 会校验签名并轻量探测常见 ROM 文件名标记。
beiklive::enums::FileType detectArchiveFileType(const fs::path& path);

// 根据文件类型映射游戏平台；无法映射返回 -1。
int platformFromFileType(beiklive::enums::FileType type);

// 根据真实文件路径判断游戏平台；会对 ZIP/7Z 做内容检测。
int detectGamePlatform(const fs::path& path);
std::vector<int> candidatePlatformsForExtension(const std::string& ext);

// 传入文件路径，提取文件名（包含扩展名）
std::string getFileName(const fs::path& path);

// 传入“文件名+后缀”，提取文件名称部分（去除最后一个扩展名）
std::string getFileNameWithoutExtension(const std::string& filenameWithExt);

// 统计目录下所有直接子项（文件和目录）的总数量
size_t countEntries(const fs::path& path);

// 获取文件大小并转换为可读的字符串（B/KB/MB/GB等）
std::string getFileSizeString(const fs::path& path);

// 返回给定路径的父路径（字符串形式）
std::string getParentPath(const std::string& path);

// 根据文件类型返回对应的图标资源路径（已知类型时直接传入，跳过文件系统探测）
// 注意：此函数需要在 UI 线程调用，因为它依赖 brls::Application::getPlatform()
std::string getIconPath(beiklive::enums::FileType type);
// 根据文件路径自动检测类型并返回图标资源路径
std::string getIconPath(const std::string& path);
// 获取当前主题的图标路径前缀（如 "img/ui/light/"），必须在 UI 线程调用
std::string getIconPathPrefix();
// 根据文件类型和预先计算的图标前缀返回图标路径（可在后台线程调用）
std::string getIconPathWithPrefix(beiklive::enums::FileType type, const std::string& prefix);
std::string getDefaultLogoPath(beiklive::enums::EmuPlatform platform);
std::string getDefaultLogoPath(beiklive::enums::EmuPlatform platform, const std::string& romPath);
bool tryUseNdsInternalIconCover(beiklive::GameEntry& entry);
/// 启用存档截图封面时，将默认封面替换为即时存档 0 的截图。
bool tryUseSavestateThumbnailCover(beiklive::GameEntry& entry);
// 获取系统逻辑磁盘驱动器列表（Windows: C:\、D:\ 等；其他平台: {"/"}）
std::vector<std::string> getLogicalDrives();

// 检查文件或目录是否存在
bool isFileExists(const std::string& path);

// 获取 GBA ROM 的游戏 ID（前4字节 ASCII），失败时返回空字符串
std::string readGbaGameID(const std::string& path);

// 获取当前时间戳字符串（存储格式：yy-mm-dd HH-MM-SS，适合字符串比较排序）
std::string getTimestampString();

// 将存储格式时间戳（"26-03-31 09-38-11"）转换为显示格式（"26-03-31 09时38分"）
// 若解析失败则原样返回（兼容旧格式）
std::string formatTimestampForDisplay(const std::string& ts);

// 获取文件最后修改时间的字符串（格式：YYYY-MM-DD HH:MM:SS，失败时返回空字符串）
std::string getFileModTimeStr(const std::string& path);

// ── 按键字符串解析 ──────────────────────────────────────────────────────────

/// 将按键名称字符串（如 "A"、"LB+START"）解析为 GameInputPad ID 列表。
/// "+" 分隔表示组合键（同时按下），大小写不敏感。
/// 若字符串为 "none" 或空，返回空列表。
std::vector<int> parsePadCombo(const std::string& combo);

/// 将键盘按键字符串（如 "A"、"SPACE+ENTER"）解析为 BrlsKeyboardScancode 列表。
/// "+" 分隔表示组合键，大小写不敏感。
std::vector<int> parseKbdCombo(const std::string& combo);

/// 将多 combo 字符串（逗号分隔，如 "A,LB+A"）解析为多组 combo。
/// 外层 vector 为各组合（OR 关系），内层为各按键（AND 关系）。
/// "none" 或空字符串返回空列表。
std::vector<std::vector<int>> parseMultiCombo(const std::string& val);

// ── 平台工具 ────────────────────────────────────────────────────────────────

/// 将 EmuPlatform 枚举值转为平台显示名称（"GBA"/"GBC"/"GB"）
std::string platformName(int platform);

/// 根据平台返回对应的遮罩全局配置键
std::string platformOverlayKey(int platform);

/// 根据平台返回对应的着色器全局配置键
std::string platformShaderKey(int platform);

/// 根据平台默认遮罩路径判断新游戏是否应自动启用遮罩
bool shouldAutoEnableOverlayForPlatform(int platform);

/// 根据平台默认着色器路径判断新游戏是否应自动启用着色器
bool shouldAutoEnableShaderForPlatform(int platform);

/// 根据平台返回对应的徽章显示文本
std::string platformBadgeName(int platform);

/// 构建游戏默认存档目录：GBAStation/saves/<平台名称>/<游戏文件名>/
std::string defaultGameSavePath(int platform, const std::string& romPath);

// ── 存档路径工具 ────────────────────────────────────────────────────────────

/// 返回存档槽位名称："自动存档" (slot=0) 或 "槽位 N" (slot>0)
std::string slotName(int slot);

/// 构建即时存档文件路径
std::string getStatePath(const std::string& saveDir, const std::string& romPath, int slot);

/// 构建即时存档缩略图路径 (.png)
std::string getStateThumbPath(const std::string& saveDir, const std::string& romPath, int slot);

/// 检查指定槽位的存档文件是否存在
bool stateExists(const std::string& saveDir, const std::string& romPath, int slot);

// ── 时间工具 ────────────────────────────────────────────────────────────────

/// 将游玩时长（秒）格式化为可读字符串（"X 小时 X 分钟" / "不到 1 分钟"）
std::string formatPlayTime(int totalSeconds);

/// 将版本号字符串（如 "v3.2.1"）转为整数（如 3002001），每段占三位十进制，用于版本比较
int versionCode(const std::string& version);

} // namespace beiklive::tools
