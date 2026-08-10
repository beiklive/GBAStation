#pragma once

#include <string>

namespace beiklive
{

/// 获取 3DS ROM（CIA/CCI/CXI）内置大图标的缓存 PNG 路径（cache/3ds_icons/）。
/// 缓存键包含路径/大小/修改时间，ROM 变化时自动失效。
std::string GetThreeDsIconCachePath(const std::string& romPath);

/// 提取/读取 3DS ROM 内置大图标（SMDH 48x48），缓存为 PNG 后返回路径。
/// 解析失败返回空字符串。
std::string GetOrCreateThreeDsIconPath(const std::string& romPath);

/// 提取 3DS ROM 的名称（SMDH TITLE，简中>英文>日文优先），失败返回空字符串。
std::string ExtractThreeDsTitle(const std::string& romPath);

} // namespace beiklive
