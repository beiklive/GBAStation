#pragma once
#include <borealis.hpp>
namespace beiklive
{
    /// 游戏画面缩放模式
    enum class ScreenMode : int
    {
        Fit          = 0,  ///< 保持宽高比（比例模式，默认）
        Fill         = 1,  ///< 拉伸填满，不保持宽高比
        IntegerScale = 2,  ///< 整数倍率缩放
        FreeScale    = 3,  ///< 自由缩放（使用 customScale）
    };

    /// 绘制矩形，用于 computeDisplayRect() 结果
    struct DisplayRect
    {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    /// 根据画面模式计算游戏帧在视图区域内的绘制矩形
    inline DisplayRect computeDisplayRect(ScreenMode mode,
                                          float viewX, float viewY,
                                          float viewW, float viewH,
                                          unsigned gameW, unsigned gameH,
                                          float customScale = 1.0f,
                                          float xOffset = 0.0f,
                                          float yOffset = 0.0f)
    {
        DisplayRect r;
        const float gw = static_cast<float>(gameW);
        const float gh = static_cast<float>(gameH);
        if (gw <= 0.f || gh <= 0.f) return r;

        float scale = 1.0f;
        switch (mode)
        {
            case ScreenMode::Fit:
                scale = std::min(viewW / gw, viewH / gh);
                break;
            case ScreenMode::Fill:
                r.w = viewW;
                r.h = viewH;
                r.x = viewX + xOffset;
                r.y = viewY + yOffset;
                return r;
            case ScreenMode::IntegerScale:
            {
                float s = std::min(viewW / gw, viewH / gh);
                scale = std::max(1.0f, std::floor(s));
                break;
            }
            case ScreenMode::FreeScale:
                scale = customScale;
                break;
        }

        r.w = gw * scale;
        r.h = gh * scale;
        r.x = viewX + (viewW - r.w) * 0.5f + xOffset;
        r.y = viewY + (viewH - r.h) * 0.5f + yOffset;
        return r;
    }

} // namespace beiklive (ScreenMode)

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
        GBA_ROM, // GBA文件
        GBC_ROM, // GBC文件
        GB_ROM,  // GB文件

        // 上面的顺序必须与EmuPlatform保持一致，方便后续通过平台类型直接转换为文件类型

        DRIVE, // 磁盘驱动器（Windows: C:\、D:\ 等）
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

    // 游戏 条目结构体，包含游戏路径、显示标题、封面路径等字段, 用于在游戏列表中显示和管理游戏信息
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
        std::string savePath;       // 游戏专属存档路径（空=使用全局默认）
        std::string screenShotPath; // 游戏截图路径
        std::string logoPath;       // 游戏封面图片路径
        std::string cheatPath;      // 金手指文件路径
        std::string overlayPath;    // 游戏专属遮罩图片路径
        std::string shaderPath;     // 游戏专属着色器预设路径

        bool overlayEnabled = false; // 是否启用游戏专属遮罩（使用全局设置初始化）
        bool shaderEnabled = false;  // 是否启用游戏专属着色器（使用全局设置初始化）

        int displayMode = 0;
        float integerAspectRatio = 1.0f;
        float customScale = 1.0f;
        float customOffsetX = 0.0f;
        float customOffsetY = 0.0f;

        std::vector<std::string> shaderParaNames; // 着色器参数名称列表
        std::vector<float> shaderParaValues;      // 着色器参数值列表
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
    /// 金手指条目
    struct CheatEntry {
        std::string desc;    ///< 金手指名称
        std::string code;    ///< 金手指代码
        bool        enabled = true; ///< 是否启用
    };
    struct RetroNameMap
    {
        const char *name;
        int id;
    };

    // 方向键
    #define UP_FLAG        0x00000001
    #define DOWN_FLAG      0x00000002
    #define LEFT_FLAG      0x00000004
    #define RIGHT_FLAG     0x00000008

    // ABXY
    #define A_FLAG         0x00000010
    #define B_FLAG         0x00000020
    #define X_FLAG         0x00000040
    #define Y_FLAG         0x00000080

    // 功能键
    #define BACK_FLAG      0x00000100
    #define PLAY_FLAG      0x00000200

    // 肩键
    #define LB_FLAG        0x00000400
    #define RB_FLAG        0x00000800

    // 摇杆按压
    #define LS_CLK_FLAG    0x00001000
    #define RS_CLK_FLAG    0x00002000

    // 存储键值 pad.retro.xxx  = [[], []]
    static const RetroNameMap k_retroNames[] = {
        { "a",      RETRO_DEVICE_ID_JOYPAD_A      },
        { "b",      RETRO_DEVICE_ID_JOYPAD_B      },
        { "x",      RETRO_DEVICE_ID_JOYPAD_X      },
        { "y",      RETRO_DEVICE_ID_JOYPAD_Y      },
        { "up",     RETRO_DEVICE_ID_JOYPAD_UP     },
        { "down",   RETRO_DEVICE_ID_JOYPAD_DOWN   },
        { "left",   RETRO_DEVICE_ID_JOYPAD_LEFT   },
        { "right",  RETRO_DEVICE_ID_JOYPAD_RIGHT  },
        { "l",      RETRO_DEVICE_ID_JOYPAD_L      },
        { "r",      RETRO_DEVICE_ID_JOYPAD_R      },
        { "l2",     RETRO_DEVICE_ID_JOYPAD_L2     },
        { "r2",     RETRO_DEVICE_ID_JOYPAD_R2     },
        { "l3",     RETRO_DEVICE_ID_JOYPAD_L3     },
        { "r3",     RETRO_DEVICE_ID_JOYPAD_R3     },
        { "start",  RETRO_DEVICE_ID_JOYPAD_START  },
        { "select", RETRO_DEVICE_ID_JOYPAD_SELECT },
    };
    // 存储键值 pad.emu.xxx = [[], []]
    enum EmuFunctionKey {
        EMU_A         , 
        EMU_B         , 
        EMU_X         , 
        EMU_Y         , 
        EMU_UP        , 
        EMU_DOWN      , 
        EMU_LEFT      , 
        EMU_RIGHT     , 
        EMU_L         , 
        EMU_R         , 
        EMU_L2        , 
        EMU_R2        , 
        EMU_L3        , 
        EMU_R3        , 
        EMU_START     , 
        EMU_SELECT    , 
        EMU_FAST_FORWARD,               // 快进
        EMU_REWIND,                     // 倒带
        EMU_QUICK_SAVE,                 // 快速保存
        EMU_QUICK_LOAD,                 // 快速读取
        EMU_OPEN_MENU,                  // 打开菜单
        EMU_MUTE,                       // 静音
        EMU_FUNCTION_KEY_COUNT
    };
    enum class TriggerType
    {
        PRESS,        // 刚按下（默认触发一次）
        LONG_PRESS,   // 长按触发一次
        HOLD,         // 按住持续触发
        RELEASE       // 松开触发一次
    };
    static const RetroNameMap k_emuNames[] = {
        { "fastforward",    EMU_FAST_FORWARD    },
        { "rewind",         EMU_REWIND          },
        { "quicksave",      EMU_QUICK_SAVE      },
        { "quickload",      EMU_QUICK_LOAD      },
        { "menu",           EMU_OPEN_MENU       },
        { "mute",           EMU_MUTE            },
    };

    enum GameInputPad
    {
        STATE_PAD_LT              = brls::BUTTON_LT    ,
        STATE_PAD_LB              = brls::BUTTON_LB    ,
        STATE_PAD_LSB             = brls::BUTTON_LSB   ,
        STATE_PAD_UP              = brls::BUTTON_UP    ,
        STATE_PAD_RIGHT           = brls::BUTTON_RIGHT ,
        STATE_PAD_DOWN            = brls::BUTTON_DOWN  ,
        STATE_PAD_LEFT            = brls::BUTTON_LEFT  ,
        STATE_PAD_BACK            = brls::BUTTON_BACK  ,
        STATE_PAD_START           = brls::BUTTON_START ,
        STATE_PAD_RSB             = brls::BUTTON_RSB   ,
        STATE_PAD_Y               = brls::BUTTON_Y     ,
        STATE_PAD_B               = brls::BUTTON_B     ,
        STATE_PAD_A               = brls::BUTTON_A     ,
        STATE_PAD_X               = brls::BUTTON_X     ,
        STATE_PAD_RB              = brls::BUTTON_RB    ,
        STATE_PAD_RT              = brls::BUTTON_RT    ,
        STATE_PAD_BUTTON_MAX      = brls::_BUTTON_MAX  ,
        STATE_PAD_LEFT_STICK_X    ,
        STATE_PAD_LEFT_STICK_Y    ,
        STATE_PAD_RIGHT_STICK_X   ,
        STATE_PAD_RIGHT_STICK_Y 
    };
    // 手柄字符值与 GameInputPad 的映射表
    static const RetroNameMap k_gameInputNames[] = {
        { "A",           STATE_PAD_A               },
        { "B",           STATE_PAD_B               },
        { "X",           STATE_PAD_X               },
        { "Y",           STATE_PAD_Y               },
        { "UP",          STATE_PAD_UP              },
        { "DOWN",        STATE_PAD_DOWN            },
        { "LEFT",        STATE_PAD_LEFT            },
        { "RIGHT",       STATE_PAD_RIGHT           },
        { "LB",          STATE_PAD_LB               },
        { "RB",          STATE_PAD_RB               },
        { "LT",          STATE_PAD_LT              },
        { "RT",          STATE_PAD_RT              },
        { "LSB",         STATE_PAD_LSB              },
        { "RSB",         STATE_PAD_RSB              },
        { "START",       STATE_PAD_START           },
        { "BACK",        STATE_PAD_BACK          },
        { "LEFTSTICKX",  STATE_PAD_LEFT_STICK_X    },
        { "LEFTSTICKY",  STATE_PAD_LEFT_STICK_Y    },
        { "RIGHTSTICKX", STATE_PAD_RIGHT_STICK_X   },
        { "RIGHTSTICKY", STATE_PAD_RIGHT_STICK_Y   },
    };


} // namespace beiklive
