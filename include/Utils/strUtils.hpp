#pragma once
#include <string>
#include <algorithm>
#include <cctype>

namespace beiklive::string
{
/**
 * 获取文件后缀（最后一个点之后的部分，不含点）
 */
std::string getFileSuffix(std::string filename);
/**
 * 忽略英文大小写的字符串比较
 */
bool iequals(const std::string& a, const std::string& b);
/**
 * 判断字符串是否为路径字符串（包含 '/' 或 '\\'）
 */
bool isPathString(const std::string& str);
/**
 * 从路径字符串中提取文件名（纯字符串操作，不依赖 filesystem）
 * 支持 '/' 和 '\\' 作为分隔符，若路径以分隔符结尾则返回空字符串。
 */
std::string extractFileName(const std::string& path);


} // namespace beiklive
