#pragma once

#include <string>

namespace beiklive
{
namespace pinyin
{

/// 返回单个 UTF-8 字符（通常为一个汉字）的拼音，无映射返回空串。
/// 内部懒加载 resources/pinyin/pingyin.json，线程安全。
std::string forChar(const std::string& utf8Char);

/// 将文本转换为拼音全拼（紧凑无分隔符）：中文字符转拼音，
/// ASCII 字母/数字原样保留（小写），其余字符忽略。
/// 例：L"塞尔达" -> "saierda"
std::string full(const std::string& utf8Text);

/// 提取文本中每个中文字符的拼音首字母（其余字符忽略）。
/// 例：L"塞尔达" -> "sed"
std::string initials(const std::string& utf8Text);

/// 判断文本是否包含中文字符（有拼音映射的字符）。
bool containsCjk(const std::string& utf8Text);

} // namespace pinyin
} // namespace beiklive
