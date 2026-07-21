#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/enums.h"

namespace beiklive
{

    struct FilePickerLocation
    {
        std::string startPath;
        std::string filename;
    };

    /// 获取修改游戏封面时文件选取器的初始位置。
    /// 内置默认封面从游戏存档目录打开，自定义封面则定位到当前文件。
    FilePickerLocation getGameCoverPickerLocation(const GameEntry& entry);

    /// 打开文件选取器 Activity，仅显示指定扩展名的文件
    /// @param extensions 允许的文件扩展名列表（不含点号，如 {"png", "jpg"}）
    /// @param onSelected 选中文件后的回调，参数为完整路径；选择器会在回调后播放关闭动画
    /// @param startPath  初始目录路径（空串则从驱动器列表开始）
    /// @param filename   打开目录后默认滚动聚焦的文件名（空串则使用默认焦点）
    void openFilePicker(
        const std::vector<std::string>& extensions,
        std::function<void(const std::string&)> onSelected,
        const std::string& startPath = "",
        const std::string& filename = "");

} // namespace beiklive
