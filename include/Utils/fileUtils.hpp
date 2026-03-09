#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>  // for std::size_t
#include "Utils/strUtils.hpp"

// 对文件和目录的操作
namespace beiklive::file
{
namespace fs = std::filesystem;
enum class PathType
{
    Directory,
    File,
    NotExist
};
enum class FileType
{
    Normal,
    Image,
    Zip,
    GBA,
    GB
};
// 排序方式枚举（移除了时间相关选项）
enum class SortBy
{
    None, // 不排序，按目录迭代器默认顺序（通常为文件系统顺序）
    NameAsc, // 按名称升序（A-Z）
    NameDesc, // 按名称降序（Z-A）
    TypeThenName, // 先按类型（目录在前，文件在后），同类型内按名称升序
    DirFirst, // 目录在前，文件在后，不额外排序
    FileFirst // 文件在前，目录在后，不额外排序
};
/**
 * 获取指定路径下的所有文件和目录的完整路径，并按指定方式排序
 * 
 * @param path 目标目录路径
 * @param sort_by 排序方式（默认不排序）
 * @return 包含所有直接子项完整路径的 vector，按指定顺序排列
 */
std::vector<std::string> listDir(std::string path, SortBy sort_by);
std::vector<std::string> listDir(std::string path);

/**
 * 判断路径是否存在，并区分是文件还是目录。
 */
PathType getPathType(const std::string& path);

std::string getParentPath(const std::string& path);

bool is_root_directory(const std::string& path_str);

fs::path get_executable_path();

std::string TransStrToRes(const std::string& path);

} // namespace beiklive
