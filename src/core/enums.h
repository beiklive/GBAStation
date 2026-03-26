#pragma once

namespace beiklive::enums
{
    // ROM 平台类型
    enum class EmuPlatform
    {
        NONE,
        GBA,
        GBC,
        GB
    };
    // 文件类型,用于文件浏览器
    enum class FileType
    {
        NONE,
        DIRECTORY,
        NORMAL_FILE,
        IMAGE_FILE, // PNG后缀
        ZIP_FILE,   // ZIP后缀
        GBA_ROM,    // GBA文件
        GBC_ROM,    // GBC文件
        GB_ROM,     // GB文件

    };
    // 主题布局类型 默认为switch布局，后续会添加天马布局
    enum class ThemeLayout
    {
        DEFAULT,
        SWITCH,
    };

    // 白名单/黑名单过滤模式
    enum class FilterMode
    {
        None,      // 不过滤，显示所有文件和文件夹
        Whitelist, // 只显示特定扩展名的文件（和所有文件夹）
        Blacklist  // 显示所有文件和文件夹，但隐藏特定扩展名的文件
    };





} // namespace beiklive
