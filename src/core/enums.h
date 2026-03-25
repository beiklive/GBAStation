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
        IMAGE_FILE,
        ZIP_FILE,
        GBA_ROM,
        GB_ROM,

    };

    enum class ThemeLayout
    {
        SWITCH,
    };







} // namespace beiklive
