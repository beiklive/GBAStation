#pragma once

#include <string>

namespace beiklive
{
namespace psp_meta
{

/// 提取 PSP ROM（ISO/CSO/PBP/已解包目录）的真实游戏标题（PARAM.SFO TITLE）。
/// 解析失败返回空字符串，调用方应回退到文件名。
std::string ExtractTitle(const std::string& path);

/// 提取 PSP ROM 的 ICON0.PNG 并保存为 <cacheDir>/<stem>.icon0.png。
/// 成功返回封面文件路径，失败返回空字符串。
std::string ExtractIcon0(const std::string& path, const std::string& cacheDir);

} // namespace psp_meta
} // namespace beiklive
