#pragma once

#include "core/enums.h"

#include <string>

namespace beiklive::input_mapping
{
    inline constexpr unsigned kPlatformGbFamily = 1u << 0;
    inline constexpr unsigned kPlatformNes = 1u << 1;
    inline constexpr unsigned kPlatformSfc = 1u << 2;
    inline constexpr unsigned kPlatformNds = 1u << 3;
    inline constexpr unsigned kPlatformThreeDs = 1u << 4;
    inline constexpr unsigned kPlatformAll = kPlatformGbFamily | kPlatformNes | kPlatformSfc | kPlatformNds | kPlatformThreeDs;

    struct GameButtonDefault
    {
        const char* label;
        const char* suffix;
        const char* defaultValue;
        unsigned platformMask;
    };

    inline constexpr GameButtonDefault kGameButtonDefaults[] = {
        {"A键", "a", "PAD_A", kPlatformAll},
        {"B键", "b", "PAD_B", kPlatformAll},
        {"X键", "x", "PAD_X", kPlatformSfc | kPlatformNds | kPlatformThreeDs},
        {"Y键", "y", "PAD_Y", kPlatformSfc | kPlatformNds | kPlatformThreeDs},
        {"方向键上", "up", "PAD_UP", kPlatformAll},
        {"方向键下", "down", "PAD_DOWN", kPlatformAll},
        {"方向键左", "left", "PAD_LEFT", kPlatformAll},
        {"方向键右", "right", "PAD_RIGHT", kPlatformAll},
        {"L键", "l", "PAD_LB", kPlatformGbFamily | kPlatformSfc | kPlatformNds | kPlatformThreeDs},
        {"R键", "r", "PAD_RB", kPlatformGbFamily | kPlatformSfc | kPlatformNds | kPlatformThreeDs},
        {"ZL键", "l2", "PAD_LT", kPlatformThreeDs},
        {"ZR键", "r2", "PAD_RT", kPlatformThreeDs},
        {"L3键", "l3", "PAD_LSB", 0},
        {"R3键", "r3", "PAD_RSB", 0},
        {"开始键", "start", "PAD_START", kPlatformAll},
        {"选择键", "select", "PAD_BACK", kPlatformAll},
        {"左摇杆上", "lstick_up", "PAD_LEFTSTICKUP", kPlatformThreeDs},
        {"左摇杆下", "lstick_down", "PAD_LEFTSTICKDOWN", kPlatformThreeDs},
        {"左摇杆左", "lstick_left", "PAD_LEFTSTICKLEFT", kPlatformThreeDs},
        {"左摇杆右", "lstick_right", "PAD_LEFTSTICKRIGHT", kPlatformThreeDs},
        {"右摇杆上", "rstick_up", "PAD_RIGHTSTICKUP", kPlatformThreeDs},
        {"右摇杆下", "rstick_down", "PAD_RIGHTSTICKDOWN", kPlatformThreeDs},
        {"右摇杆左", "rstick_left", "PAD_RIGHTSTICKLEFT", kPlatformThreeDs},
        {"右摇杆右", "rstick_right", "PAD_RIGHTSTICKRIGHT", kPlatformThreeDs},
    };

    struct HotkeyDefault
    {
        const char* key;
        const char* label;
        const char* defaultValue;
        bool hiddenOnNds = false;
        bool hiddenOnThreeDs = false;
    };

    inline constexpr HotkeyDefault kHotkeyDefaults[] = {
        {"handle.fastforward", "快进", "PAD_LSB", false, false},
        {"handle.rewind", "倒带", "none", true, true},
        {"hotkey.quicksave.pad", "快速保存", "none", false, false},
        {"hotkey.quickload.pad", "快速读取", "none", false, false},
        {"hotkey.screenshot.pad", "截图", "none", false, false},
        {"hotkey.menu.pad", "打开菜单", "PAD_LT+PAD_RT", false, false},
        {"hotkey.mute.pad", "静音", "none", false, false},
        {"hotkey.pause.pad", "暂停", "none", false, false},
    };

    inline constexpr HotkeyDefault kPointerHotkeys[] = {
        {"hotkey.pointer_mode.pad", "指针模式切换", "none", false},
        {"hotkey.pointer_click.pad", "指针点击", "none", false},
        {"hotkey.swap_screens.pad", "交换上下屏", "none", false},
        {"hotkey.mic_input.pad", "模拟麦克风输入", "none", false},
    };

    inline constexpr const char* kTurboAKey = "handle.a_turbo";
    inline constexpr const char* kTurboBKey = "handle.b_turbo";
    inline constexpr const char* kTurboADefault = "none";
    inline constexpr const char* kTurboBDefault = "none";

    inline bool usesLegacyGbFamilyFallback(const std::string& prefix)
    {
        return prefix == "gbc." || prefix == "gb.";
    }

    inline std::string platformPrefix(int platform)
    {
        switch (static_cast<beiklive::enums::EmuPlatform>(platform))
        {
        case beiklive::enums::EmuPlatform::EmuGBC:
            return "gbc.";
        case beiklive::enums::EmuPlatform::EmuGB:
            return "gb.";
        case beiklive::enums::EmuPlatform::EmuNES:
            return "nes.";
        case beiklive::enums::EmuPlatform::EmuSNES:
            return "sfc.";
        case beiklive::enums::EmuPlatform::EmuNDS:
            return "nds.";
        case beiklive::enums::EmuPlatform::Emu3DS:
            return "3ds.";
        default:
            return "";
        }
    }

    inline unsigned platformMaskForPlatform(int platform)
    {
        switch (static_cast<beiklive::enums::EmuPlatform>(platform))
        {
        case beiklive::enums::EmuPlatform::EmuNES:
            return kPlatformNes;
        case beiklive::enums::EmuPlatform::EmuSNES:
            return kPlatformSfc;
        case beiklive::enums::EmuPlatform::EmuNDS:
            return kPlatformNds;
        case beiklive::enums::EmuPlatform::Emu3DS:
            return kPlatformThreeDs;
        case beiklive::enums::EmuPlatform::EmuGBA:
        case beiklive::enums::EmuPlatform::EmuGBC:
        case beiklive::enums::EmuPlatform::EmuGB:
        default:
            return kPlatformGbFamily;
        }
    }

    inline unsigned platformMaskForPrefix(const std::string& prefix)
    {
        if (prefix == "nes.")
            return kPlatformNes;
        if (prefix == "sfc.")
            return kPlatformSfc;
        if (prefix == "nds.")
            return kPlatformNds;
        if (prefix == "3ds.")
            return kPlatformThreeDs;
        return kPlatformGbFamily;
    }

    inline std::string makeKey(const std::string& prefix, const std::string& key)
    {
        return prefix + key;
    }

    inline std::string makeHandleKey(const std::string& prefix, const std::string& suffix)
    {
        return prefix + "handle." + suffix;
    }

    inline const char* defaultHandleValue(const std::string& suffix, const char* fallback = "none")
    {
        for (const auto& entry : kGameButtonDefaults)
        {
            if (suffix == entry.suffix)
                return entry.defaultValue;
        }
        return fallback;
    }
}
