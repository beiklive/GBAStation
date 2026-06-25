#pragma once
#include <borealis.hpp>
namespace beiklive
{
    /// 核心类型标识符，用于选择静态链接的 libretro 核心。
    enum class CoreType {
        Mgba,
        Fceumm,
        Snes9x,
        Nestopia,
        Snes9x2005
    };

    /// 游戏画面缩放模式
    enum class ScreenMode : int
    {
        Fit          = 0,  ///< 保持宽高比（比例模式，默认取最大比例）
        Fill         = 1,  ///< 拉伸填满，不保持宽高比
        IntegerScale = 2,  ///< 整数倍率缩放，默认最大整数倍
        FreeScale    = 3,  ///< 自由缩放（使用 customScale）
        FourThree    = 4,  ///< 4:3 模式，高度按窗口高度，宽度按 4:3 等比换算
    };

    /// 可视化倒带缩略图压缩策略
    enum class RewindThumbCompression : int
    {
        NearestNeighbor = 0, ///< 最近邻采样（速度快，质量较低）
        Bilinear        = 1, ///< 双线性插值（速度适中，质量较高）
    };

    /// 绘制矩形，用于 computeDisplayRect() 结果
    struct DisplayRect
    {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };


} // namespace beiklive (ScreenMode)

namespace beiklive::enums
{
    // ROM 平台类型
    enum class EmuPlatform
    {
        NONE,
        EmuGBA,
        EmuGBC,
        EmuGB,
        EmuNES,
        EmuSNES,
        EmuNDS
    };
    // 文件类型,用于文件浏览器
    enum class FileType
    {
        NONE,
        GBA_ROM, // GBA文件
        GBC_ROM, // GBC文件
        GB_ROM,  // GB文件
        NES_ROM, // NES/Famicom文件
        SNES_ROM,// SNES/SFC文件
        NDS_ROM, // NDS文件

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
        std::string path = "";                                  // 游戏文件路径
        std::string title = "";                                 // 显示标题（默认为映射名）
        int playCount = 0;                                      // 玩过的次数
        int playTime = 0;                                       // 玩过的总时间（单位：秒）
        int platform = (int)beiklive::enums::EmuPlatform::NONE; // 游戏平台（如 GBA、GBC、GB）
        std::string core = "";                                  // 使用的模拟核心（空=按平台默认）
        std::string lastPlayed = "";                            // 上次玩的时间(时间戳字符串)
        int crc32 = 0;                                          // 游戏文件的 CRC32 校验值（用于唯一标识游戏）
        bool favourite = false;                                 // 是否收藏

        // 游戏独立设置相关
        std::string savePath = "";       // 游戏专属存档路径（空=使用全局默认）
        std::string screenShotPath = ""; // 游戏截图路径
        std::string logoPath = "";       // 游戏封面图片路径
        std::string cheatPath = "";      // 金手指文件路径
        std::string overlayPath = "";    // 游戏专属遮罩图片路径
        std::string shaderPath = "";     // 游戏专属着色器预设路径

        bool overlayEnabled = false; // 是否启用游戏专属遮罩（使用全局设置初始化）
        bool shaderEnabled = false;  // 是否启用游戏专属着色器（使用全局设置初始化）

        int displayMode = 0;
        float integerAspectRatio = 1.0f;
        float customScale = 1.0f;
        float customOffsetX = 0.0f;
        float customOffsetY = 0.0f;
        float ndsTopScale = 1.0f;
        float ndsTopOffsetX = 0.0f;
        float ndsTopOffsetY = 0.0f;
        float ndsBottomScale = 1.0f;
        float ndsBottomOffsetX = 0.0f;
        float ndsBottomOffsetY = 0.0f;
        std::string ndsScreenLayout = ""; // NDS 双屏布局（vertical/horizontal/custom/hybrid/top/bottom）
        std::string ndsScreenOrientation = ""; // NDS 屏幕旋转角度（0/90/180/270，兼容旧方向字符串）
        bool ndsIntegerScale = false; // NDS 自动最大整数倍缩放
        int ndsInternalResolution = 1; // NDS 3D 内部分辨率倍率（1-4）

        std::vector<std::string> shaderParaNames; // 着色器参数名称列表
        std::vector<float> shaderParaValues;      // 着色器参数值列表
    };

    typedef std::vector<GameEntry> GameList; // 游戏列表类型定义

    // 列表元素数据
    struct ListItem
    {
        std::string text ;     // 标题
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
    constexpr uint32_t UP_FLAG    = 0x00000001;
    constexpr uint32_t DOWN_FLAG  = 0x00000002;
    constexpr uint32_t LEFT_FLAG  = 0x00000004;
    constexpr uint32_t RIGHT_FLAG = 0x00000008;

    // ABXY
    constexpr uint32_t A_FLAG = 0x00000010;
    constexpr uint32_t B_FLAG = 0x00000020;
    constexpr uint32_t X_FLAG = 0x00000040;
    constexpr uint32_t Y_FLAG = 0x00000080;

    // 功能键
    constexpr uint32_t BACK_FLAG = 0x00000100;
    constexpr uint32_t PLAY_FLAG = 0x00000200;

    // 肩键
    constexpr uint32_t LB_FLAG = 0x00000400;
    constexpr uint32_t RB_FLAG = 0x00000800;

    // 摇杆按压
    constexpr uint32_t LS_CLK_FLAG = 0x00001000;
    constexpr uint32_t RS_CLK_FLAG = 0x00002000;

    // 存储键值 pad.retro.xxx  = [[], []]
    inline constexpr RetroNameMap k_retroNames[] = {
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
        EMU_A_TURBO,                    // A键连发
        EMU_B_TURBO,                    // B键连发
        EMU_LEFT_STICK_UP,              // 左摇杆向上
        EMU_LEFT_STICK_DOWN,            // 左摇杆向下
        EMU_LEFT_STICK_LEFT,            // 左摇杆向左
        EMU_LEFT_STICK_RIGHT,           // 左摇杆向右
        EMU_RIGHT_STICK_UP,             // 右摇杆向上
        EMU_RIGHT_STICK_DOWN,           // 右摇杆向下
        EMU_RIGHT_STICK_LEFT,           // 右摇杆向左
        EMU_RIGHT_STICK_RIGHT,          // 右摇杆向右
        EMU_NDS_POINTER_MODE,           // NDS 指针模式切换
        EMU_NDS_POINTER_CLICK,          // NDS 指针点击
        EMU_NDS_SWAP_SCREENS,           // NDS 交换上下屏
        EMU_FUNCTION_KEY_COUNT
    };
    enum class TriggerType
    {
        PRESS,        // 刚按下（默认触发一次）
        LONG_PRESS,   // 长按触发一次
        HOLD,         // 按住持续触发
        RELEASE       // 松开触发一次
    };
    inline constexpr RetroNameMap k_emuNames[] = {
        { "fastforward",    EMU_FAST_FORWARD    },
        { "rewind",         EMU_REWIND          },
        { "quicksave",      EMU_QUICK_SAVE      },
        { "quickload",      EMU_QUICK_LOAD      },
        { "menu",           EMU_OPEN_MENU       },
        { "mute",           EMU_MUTE            },
        { "a_turbo",        EMU_A_TURBO         },
        { "b_turbo",        EMU_B_TURBO         },
        { "nds_pointer_mode", EMU_NDS_POINTER_MODE },
        { "nds_pointer_click", EMU_NDS_POINTER_CLICK },
        { "nds_swap_screens", EMU_NDS_SWAP_SCREENS },
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
        STATE_PAD_RIGHT_STICK_Y   ,
        STATE_PAD_LEFT_STICK_UP   ,  // 左摇杆向上
        STATE_PAD_LEFT_STICK_DOWN ,  // 左摇杆向下
        STATE_PAD_LEFT_STICK_LEFT ,  // 左摇杆向左
        STATE_PAD_LEFT_STICK_RIGHT,  // 左摇杆向右
        STATE_PAD_RIGHT_STICK_UP  ,  // 右摇杆向上
        STATE_PAD_RIGHT_STICK_DOWN,  // 右摇杆向下
        STATE_PAD_RIGHT_STICK_LEFT,  // 右摇杆向左
        STATE_PAD_RIGHT_STICK_RIGHT  // 右摇杆向右
    };
    // 手柄字符值与 GameInputPad 的映射表
    inline constexpr RetroNameMap k_gameInputNames[] = {
        { "PAD_A",               STATE_PAD_A                 },
        { "PAD_B",               STATE_PAD_B                 },
        { "PAD_X",               STATE_PAD_X                 },
        { "PAD_Y",               STATE_PAD_Y                 },
        { "PAD_UP",              STATE_PAD_UP                },
        { "PAD_DOWN",            STATE_PAD_DOWN              },
        { "PAD_LEFT",            STATE_PAD_LEFT              },
        { "PAD_RIGHT",           STATE_PAD_RIGHT             },
        { "PAD_LB",              STATE_PAD_LB                },
        { "PAD_RB",              STATE_PAD_RB                },
        { "PAD_LT",              STATE_PAD_LT                },
        { "PAD_RT",              STATE_PAD_RT                },
        { "PAD_ZL",              STATE_PAD_LT                },
        { "PAD_ZR",              STATE_PAD_RT                },
        { "ZL",                  STATE_PAD_LT                },
        { "ZR",                  STATE_PAD_RT                },
        { "PAD_LSB",             STATE_PAD_LSB               },
        { "PAD_RSB",             STATE_PAD_RSB               },
        { "PAD_START",           STATE_PAD_START             },
        { "PAD_BACK",            STATE_PAD_BACK              },
        { "PAD_LEFTSTICKX",      STATE_PAD_LEFT_STICK_X      },
        { "PAD_LEFTSTICKY",      STATE_PAD_LEFT_STICK_Y      },
        { "PAD_RIGHTSTICKX",     STATE_PAD_RIGHT_STICK_X     },
        { "PAD_RIGHTSTICKY",     STATE_PAD_RIGHT_STICK_Y     },
        { "PAD_LEFTSTICKUP",     STATE_PAD_LEFT_STICK_UP     },
        { "PAD_LEFTSTICKDOWN",   STATE_PAD_LEFT_STICK_DOWN   },
        { "PAD_LEFTSTICKLEFT",   STATE_PAD_LEFT_STICK_LEFT   },
        { "PAD_LEFTSTICKRIGHT",  STATE_PAD_LEFT_STICK_RIGHT  },
        { "PAD_RIGHTSTICKUP",    STATE_PAD_RIGHT_STICK_UP    },
        { "PAD_RIGHTSTICKDOWN",  STATE_PAD_RIGHT_STICK_DOWN  },
        { "PAD_RIGHTSTICKLEFT",  STATE_PAD_RIGHT_STICK_LEFT  },
        { "PAD_RIGHTSTICKRIGHT", STATE_PAD_RIGHT_STICK_RIGHT },
    };

    /// 键盘按键名 → BrlsKeyboardScancode
    inline constexpr RetroNameMap k_kbdInputNames[] = {
        { "A",  brls::BRLS_KBD_KEY_A }, { "B", brls::BRLS_KBD_KEY_B },
        { "C",  brls::BRLS_KBD_KEY_C }, { "D", brls::BRLS_KBD_KEY_D },
        { "E",  brls::BRLS_KBD_KEY_E }, { "F", brls::BRLS_KBD_KEY_F },
        { "G",  brls::BRLS_KBD_KEY_G }, { "H", brls::BRLS_KBD_KEY_H },
        { "I",  brls::BRLS_KBD_KEY_I }, { "J", brls::BRLS_KBD_KEY_J },
        { "K",  brls::BRLS_KBD_KEY_K }, { "L", brls::BRLS_KBD_KEY_L },
        { "M",  brls::BRLS_KBD_KEY_M }, { "N", brls::BRLS_KBD_KEY_N },
        { "O",  brls::BRLS_KBD_KEY_O }, { "P", brls::BRLS_KBD_KEY_P },
        { "Q",  brls::BRLS_KBD_KEY_Q }, { "R", brls::BRLS_KBD_KEY_R },
        { "S",  brls::BRLS_KBD_KEY_S }, { "T", brls::BRLS_KBD_KEY_T },
        { "U",  brls::BRLS_KBD_KEY_U }, { "V", brls::BRLS_KBD_KEY_V },
        { "W",  brls::BRLS_KBD_KEY_W }, { "X", brls::BRLS_KBD_KEY_X },
        { "Y",  brls::BRLS_KBD_KEY_Y }, { "Z", brls::BRLS_KBD_KEY_Z },
        { "0",  brls::BRLS_KBD_KEY_0 }, { "1", brls::BRLS_KBD_KEY_1 },
        { "2",  brls::BRLS_KBD_KEY_2 }, { "3", brls::BRLS_KBD_KEY_3 },
        { "4",  brls::BRLS_KBD_KEY_4 }, { "5", brls::BRLS_KBD_KEY_5 },
        { "6",  brls::BRLS_KBD_KEY_6 }, { "7", brls::BRLS_KBD_KEY_7 },
        { "8",  brls::BRLS_KBD_KEY_8 }, { "9", brls::BRLS_KBD_KEY_9 },
        { "F1",  brls::BRLS_KBD_KEY_F1  }, { "F2",  brls::BRLS_KBD_KEY_F2  },
        { "F3",  brls::BRLS_KBD_KEY_F3  }, { "F4",  brls::BRLS_KBD_KEY_F4  },
        { "F5",  brls::BRLS_KBD_KEY_F5  }, { "F6",  brls::BRLS_KBD_KEY_F6  },
        { "F7",  brls::BRLS_KBD_KEY_F7  }, { "F8",  brls::BRLS_KBD_KEY_F8  },
        { "F9",  brls::BRLS_KBD_KEY_F9  }, { "F10", brls::BRLS_KBD_KEY_F10 },
        { "F11", brls::BRLS_KBD_KEY_F11 }, { "F12", brls::BRLS_KBD_KEY_F12 },
        { "SPACE",     brls::BRLS_KBD_KEY_SPACE     },
        { "ENTER",     brls::BRLS_KBD_KEY_ENTER     },
        { "TAB",       brls::BRLS_KBD_KEY_TAB       },
        { "ESC",       brls::BRLS_KBD_KEY_ESCAPE    },
        { "BACKSPACE", brls::BRLS_KBD_KEY_BACKSPACE },
        { "DEL",       brls::BRLS_KBD_KEY_DELETE    },
        { "UP",        brls::BRLS_KBD_KEY_UP        },
        { "DOWN",      brls::BRLS_KBD_KEY_DOWN      },
        { "LEFT",      brls::BRLS_KBD_KEY_LEFT      },
        { "RIGHT",     brls::BRLS_KBD_KEY_RIGHT     },
        { "SHIFT",     brls::BRLS_KBD_KEY_LEFT_SHIFT  },
        { "CTRL",      brls::BRLS_KBD_KEY_LEFT_CONTROL },
        { "ALT",       brls::BRLS_KBD_KEY_LEFT_ALT     },
    };


} // namespace beiklive
