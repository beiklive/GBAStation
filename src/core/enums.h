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


namespace beiklive // 结构体
{



    // 游戏条目结构体，包含游戏路径、显示标题、封面路径等字段, 用于在游戏列表中显示和管理游戏信息
    struct GameEntry
    {
        std::string path;                                       // 游戏文件路径
        std::string title;                                      // 显示标题（默认为映射名）
        int playCount = 0;                                      // 玩过的次数
        int playTime = 0;                                       // 玩过的总时间（单位：秒）
        int platform = (int)beiklive::enums::EmuPlatform::NONE; // 游戏平台（如 GBA、GBC、GB）
        std::string lastPlayed;                                 // 上次玩的时间(时间戳字符串)
        int crc32 = 0;                                          // 游戏文件的 CRC32 校验值（用于唯一标识游戏）
        
        // 游戏独立设置相关
        std::string savePath;                                  // 游戏专属存档路径（空=使用全局默认） 
        std::string screenShotPath;                             // 游戏截图路径
        std::string logoPath;                                   // 游戏封面图片路径
        std::string cheatPath;                                  // 金手指文件路径
        std::string overlayPath;                                // 游戏专属遮罩图片路径
        std::string shaderPath;                                 // 游戏专属着色器预设路径

        bool overlayEnabled = false;                                  // 是否启用游戏专属遮罩（使用全局设置初始化）
        bool shaderEnabled = false;                                   // 是否启用游戏专属着色器（使用全局设置初始化）

        int displayMode = 0;
        float integerAspectRatio = 1.0f;
        float customScale = 1.0f;
        float customOffsetX = 0.0f;
        float customOffsetY = 0.0f;
        
        std::vector<std::string> shaderParaNames; // 着色器参数名称列表
        std::vector<float> shaderParaValues;       // 着色器参数值列表

    };

    typedef std::vector<GameEntry> GameList; // 游戏列表类型定义

    // 列表元素数据
    struct ListItem
    {
        std::string text;     // 标题
        std::string subText;  // 子标题
        std::string iconPath; // 图标路径
        std::string data;     // 额外数据（如游戏路径）
    };

    typedef std::vector<ListItem> ListItemList; // 列表数据类型定义

    struct DirListData
    {
        std::string fileName;               // 文件名（不含路径）
        std::string fullPath;               // 完整路径
        std::string iconPath;               // 图标路径
        beiklive::enums::FileType itemType; // 文件类型
        std::string fileSize;               // 文件大小（字节），目录为0
        size_t childCount;                  // 子项数量，仅目录有效，文件为0
    };

} // namespace beiklive
