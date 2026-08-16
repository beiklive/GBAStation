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
    inline constexpr unsigned kPlatformGenesis = 1u << 5;
    inline constexpr unsigned kPlatformArcade = 1u << 6;
    inline constexpr unsigned kPlatformDreamcast = 1u << 7;
    inline constexpr unsigned kPlatformPsp = 1u << 8;
    inline constexpr unsigned kPlatformPs1 = 1u << 9;
    inline constexpr unsigned kPlatformSaturn = 1u << 10;
    inline constexpr unsigned kPlatformDolphin = 1u << 11;
    inline constexpr unsigned kPlatformAll = kPlatformGbFamily | kPlatformNes | kPlatformSfc | kPlatformNds | kPlatformThreeDs | kPlatformGenesis | kPlatformArcade | kPlatformDreamcast | kPlatformPsp | kPlatformPs1 | kPlatformSaturn | kPlatformDolphin;
    inline constexpr unsigned kPlatformExplicitRightStick =
        kPlatformGbFamily | kPlatformNes | kPlatformSfc | kPlatformThreeDs | kPlatformGenesis;

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
        {"X键", "x", "PAD_X", kPlatformSfc | kPlatformNds | kPlatformThreeDs | kPlatformGenesis | kPlatformArcade | kPlatformDreamcast | kPlatformPsp | kPlatformSaturn | kPlatformDolphin},
        {"Y键", "y", "PAD_Y", kPlatformSfc | kPlatformNds | kPlatformThreeDs | kPlatformGenesis | kPlatformArcade | kPlatformDreamcast | kPlatformPsp | kPlatformSaturn | kPlatformDolphin},
        {"C键", "c", "PAD_X", kPlatformSaturn},
        {"Z键", "z", "PAD_RB", kPlatformSaturn},
        {"方向键上", "up", "PAD_UP", kPlatformAll},
        {"方向键下", "down", "PAD_DOWN", kPlatformAll},
        {"方向键左", "left", "PAD_LEFT", kPlatformAll},
        {"方向键右", "right", "PAD_RIGHT", kPlatformAll},
        {"L键", "l", "PAD_LB", kPlatformGbFamily | kPlatformSfc | kPlatformNds | kPlatformThreeDs | kPlatformGenesis | kPlatformArcade | kPlatformDreamcast | kPlatformPsp | kPlatformSaturn | kPlatformDolphin},
        {"R键", "r", "PAD_RB", kPlatformGbFamily | kPlatformSfc | kPlatformNds | kPlatformThreeDs | kPlatformGenesis | kPlatformArcade | kPlatformDreamcast | kPlatformPsp | kPlatformSaturn | kPlatformDolphin},
        // ZL/ZR only exist on the 3DS; the external cores use these physical
        // buttons for their own inputs (arcade button 7/8 on FBNeo).
        {"ZL键", "l2", "PAD_LT", kPlatformThreeDs | kPlatformArcade | kPlatformPs1 | kPlatformSaturn | kPlatformDolphin},
        {"ZR键", "r2", "PAD_RT", kPlatformThreeDs | kPlatformArcade | kPlatformPs1 | kPlatformSaturn | kPlatformDolphin},
        {"开始键", "start", "PAD_START", kPlatformAll},
        {"选择键", "select", "PAD_BACK", kPlatformAll},
        {"左摇杆上", "lstick_up", "PAD_LEFTSTICKUP", kPlatformThreeDs},
        {"左摇杆下", "lstick_down", "PAD_LEFTSTICKDOWN", kPlatformThreeDs},
        {"左摇杆左", "lstick_left", "PAD_LEFTSTICKLEFT", kPlatformThreeDs},
        {"左摇杆右", "lstick_right", "PAD_LEFTSTICKRIGHT", kPlatformThreeDs},
        {"右摇杆上", "rstick_up", "PAD_RIGHTSTICKUP", kPlatformExplicitRightStick},
        {"右摇杆下", "rstick_down", "PAD_RIGHTSTICKDOWN", kPlatformExplicitRightStick},
        {"右摇杆左", "rstick_left", "PAD_RIGHTSTICKLEFT", kPlatformExplicitRightStick},
        {"右摇杆右", "rstick_right", "PAD_RIGHTSTICKRIGHT", kPlatformExplicitRightStick},
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
        {"hotkey.swap_screens.pad", "交换上下屏", "none", false, false},
    };

    // These are exclusive to the NDS host.  Unlike a physical microphone,
    // DraStic accepts a transient white-noise feed while the mapping is held.
    inline constexpr HotkeyDefault kNdsSpecialHotkeys[] = {
        {"hotkey.mic_input.pad", "模拟麦克风输入", "PAD_LT+PAD_Y", false},
    };

    inline constexpr const char* kTurboAKey = "handle.a_turbo";
    inline constexpr const char* kTurboBKey = "handle.b_turbo";
    inline constexpr const char* kTurboADefault = "none";
    inline constexpr const char* kTurboBDefault = "none";

    inline bool showsHotkeyForPrefix(const std::string& prefix,
                                     const HotkeyDefault& entry,
                                     bool nds)
    {
        if ((nds && entry.hiddenOnNds) ||
            (prefix == "3ds." && entry.hiddenOnThreeDs))
            return false;

        if (prefix == "arcade.")
        {
            return std::string(entry.key) == "handle.fastforward" ||
                   std::string(entry.key) == "handle.rewind" ||
                   std::string(entry.key) == "hotkey.quicksave.pad" ||
                   std::string(entry.key) == "hotkey.quickload.pad" ||
                   std::string(entry.key) == "hotkey.menu.pad";
        }

        if (prefix == "dc." || prefix == "psp.")
        {
            return std::string(entry.key) != "hotkey.screenshot.pad" &&
                   std::string(entry.key) != "hotkey.mute.pad" &&
                   std::string(entry.key) != "hotkey.pause.pad";
        }

        return true;
    }

    inline bool showsTurboBindingsForPrefix(const std::string& prefix)
    {
        return prefix != "arcade." && prefix != "dc." && prefix != "psp.";
    }

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
        case beiklive::enums::EmuPlatform::EmuGenesis:
            return "md.";
        case beiklive::enums::EmuPlatform::EmuArcade:
            return "arcade.";
        case beiklive::enums::EmuPlatform::EmuDreamcast:
            return "dc.";
        case beiklive::enums::EmuPlatform::EmuPSP:
            return "psp.";
        case beiklive::enums::EmuPlatform::EmuPS1:
            return "ps1.";
        case beiklive::enums::EmuPlatform::EmuSaturn:
            return "saturn.";
        case beiklive::enums::EmuPlatform::EmuDolphin:
            return "dolphin.";
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
        case beiklive::enums::EmuPlatform::EmuGenesis:
            return kPlatformGenesis;
        case beiklive::enums::EmuPlatform::EmuArcade:
            return kPlatformArcade;
        case beiklive::enums::EmuPlatform::EmuDreamcast:
            return kPlatformDreamcast;
        case beiklive::enums::EmuPlatform::EmuPSP:
            return kPlatformPsp;
        case beiklive::enums::EmuPlatform::EmuPS1:
            return kPlatformPs1;
        case beiklive::enums::EmuPlatform::EmuSaturn:
            return kPlatformSaturn;
        case beiklive::enums::EmuPlatform::EmuDolphin:
            return kPlatformDolphin;
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
        if (prefix == "md.")
            return kPlatformGenesis;
        if (prefix == "arcade.")
            return kPlatformArcade;
        if (prefix == "dc.")
            return kPlatformDreamcast;
        if (prefix == "psp.")
            return kPlatformPsp;
        if (prefix == "ps1.")
            return kPlatformPs1;
        if (prefix == "saturn.")
            return kPlatformSaturn;
        if (prefix == "dolphin.")
            return kPlatformDolphin;
        return kPlatformGbFamily;
    }

    inline const char* gameButtonLabelForPrefix(const std::string& prefix,
                                                const GameButtonDefault& entry)
    {
        if (prefix == "saturn.")
        {
            const std::string suffix = entry.suffix;
            if (suffix == "a") return "Saturn A键";
            if (suffix == "b") return "Saturn B键";
            if (suffix == "c") return "Saturn C键";
            if (suffix == "x") return "Saturn X键";
            if (suffix == "y") return "Saturn Y键";
            if (suffix == "z") return "Saturn Z键";
            if (suffix == "l") return "Saturn L键";
            if (suffix == "r") return "Saturn R键";
        }
        if (prefix != "md.")
            return entry.label;

        const std::string suffix = entry.suffix;
        if (suffix == "y") return "MD A键";
        if (suffix == "b") return "MD B键";
        if (suffix == "a") return "MD C键";
        if (suffix == "l") return "MD X键";
        if (suffix == "x") return "MD Y键";
        if (suffix == "r") return "MD Z键";
        if (suffix == "select") return "MD Mode键";
        return entry.label;
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

    inline bool isRightStickMapping(const std::string& suffix)
    {
        return suffix.rfind("rstick_", 0) == 0;
    }

    inline bool requiresExplicitRightStickMapping(const std::string& prefix)
    {
        return prefix.empty() || prefix == "gbc." || prefix == "gb." ||
               prefix == "nes." || prefix == "sfc." || prefix == "md." ||
               prefix == "arcade." || prefix == "dc." || prefix == "psp.";
    }

    inline const char* defaultHandleValueForPrefix(const std::string& prefix,
                                                   const std::string& suffix,
                                                   const char* fallback = "none")
    {
        if (prefix == "saturn.")
        {
            if (suffix == "a") return "PAD_B";
            if (suffix == "b") return "PAD_A";
            if (suffix == "c") return "PAD_X";
            if (suffix == "x") return "PAD_Y";
            if (suffix == "y") return "PAD_LB";
            if (suffix == "z") return "PAD_RB";
            if (suffix == "l") return "PAD_LT";
            if (suffix == "r") return "PAD_RT";
        }
        if (requiresExplicitRightStickMapping(prefix) && isRightStickMapping(suffix))
            return "none";
        return defaultHandleValue(suffix, fallback);
    }
}
