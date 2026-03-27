#pragma once

namespace beiklive::enums
{
    // ROM 平台类型
    enum class EmuPlatform
    {
        NONE,
        EmuGBA,
        EmuGBC,
        EmuGB
    };
    // 文件类型,用于文件浏览器
    enum class FileType
    {
        NONE,
        GBA_ROM,    // GBA文件
        GBC_ROM,    // GBC文件
        GB_ROM,     // GB文件


        // 上面的顺序必须与EmuPlatform保持一致，方便后续通过平台类型直接转换为文件类型


        DRIVE,      // 磁盘驱动器（Windows: C:\、D:\ 等）
        DIRECTORY,
        NORMAL_FILE,
        IMAGE_FILE, // PNG后缀
        ZIP_FILE    // ZIP后缀

    };
    // 主题布局类型 默认为switch布局，后续会添加天马布局
    enum class ThemeLayout
    {
        DEFAULT_THEME,
        SWITCH_THEME
    };

    // 白名单/黑名单过滤模式
    enum class FilterMode
    {
        None,      // 不过滤，显示所有文件和文件夹
        Whitelist, // 只显示特定扩展名的文件（和所有文件夹）
        Blacklist  // 显示所有文件和文件夹，但隐藏特定扩展名的文件
    };





} // namespace beiklive
