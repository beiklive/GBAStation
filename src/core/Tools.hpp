#pragma once

#include "common.h"
#include <filesystem>
#include <string>
#include <cstdint>
#include <fstream>

namespace fs = std::filesystem;

namespace beiklive::tools {

// 获取文件扩展名（小写形式，不含点号）
std::string getFileExtension(const fs::path& path);

// 主函数：根据文件路径判断类型
beiklive::enums::FileType getFileType(const fs::path& path);

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
std::string getIconPath(beiklive::enums::FileType type);
// 根据文件路径自动检测类型并返回图标资源路径
std::string getIconPath(const std::string& path);
std::string getDefaultLogoPath(beiklive::enums::EmuPlatform platform);
// 获取系统逻辑磁盘驱动器列表（Windows: C:\、D:\ 等；其他平台: {"/"}）
std::vector<std::string> getLogicalDrives();

// 检查文件或目录是否存在
bool isFileExists(const std::string& path);

// 计算文件的 CRC32 校验值
uint32_t crc32(const std::string& path);

// 获取当前时间戳字符串（格式：YYYY-MM-DD HH:MM:SS）
std::string getTimestampString();

// 获取文件最后修改时间的字符串（格式：YYYY-MM-DD HH:MM:SS，失败时返回空字符串）
std::string getFileModTimeStr(const std::string& path);

} // namespace beiklive::tools