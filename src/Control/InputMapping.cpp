#include "Control/InputMapping.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// Pull in borealis types for ControllerButton and BrlsKeyboardScancode.
#include <borealis.hpp>

// Pull in libretro joypad IDs.
#include "third_party/mgba/src/platform/libretro/libretro.h"

namespace beiklive {

// ============================================================
// Internal lookup tables
// (moved from src/Game/game_view.cpp)
// ============================================================

// ---- Keyboard scancode name table ---------------------------
// Allows config values like  keyboard.a = X  or  keyboard.a = 88
struct KbdKeyName { const char* name; int scancode; };

static const KbdKeyName k_kbdKeyNames[] = {
    // Letters
    { "A", brls::BRLS_KBD_KEY_A }, { "B", brls::BRLS_KBD_KEY_B },
    { "C", brls::BRLS_KBD_KEY_C }, { "D", brls::BRLS_KBD_KEY_D },
    { "E", brls::BRLS_KBD_KEY_E }, { "F", brls::BRLS_KBD_KEY_F },
    { "G", brls::BRLS_KBD_KEY_G }, { "H", brls::BRLS_KBD_KEY_H },
    { "I", brls::BRLS_KBD_KEY_I }, { "J", brls::BRLS_KBD_KEY_J },
    { "K", brls::BRLS_KBD_KEY_K }, { "L", brls::BRLS_KBD_KEY_L },
    { "M", brls::BRLS_KBD_KEY_M }, { "N", brls::BRLS_KBD_KEY_N },
    { "O", brls::BRLS_KBD_KEY_O }, { "P", brls::BRLS_KBD_KEY_P },
    { "Q", brls::BRLS_KBD_KEY_Q }, { "R", brls::BRLS_KBD_KEY_R },
    { "S", brls::BRLS_KBD_KEY_S }, { "T", brls::BRLS_KBD_KEY_T },
    { "U", brls::BRLS_KBD_KEY_U }, { "V", brls::BRLS_KBD_KEY_V },
    { "W", brls::BRLS_KBD_KEY_W }, { "X", brls::BRLS_KBD_KEY_X },
    { "Y", brls::BRLS_KBD_KEY_Y }, { "Z", brls::BRLS_KBD_KEY_Z },
    // Digits
    { "0", brls::BRLS_KBD_KEY_0 }, { "1", brls::BRLS_KBD_KEY_1 },
    { "2", brls::BRLS_KBD_KEY_2 }, { "3", brls::BRLS_KBD_KEY_3 },
    { "4", brls::BRLS_KBD_KEY_4 }, { "5", brls::BRLS_KBD_KEY_5 },
    { "6", brls::BRLS_KBD_KEY_6 }, { "7", brls::BRLS_KBD_KEY_7 },
    { "8", brls::BRLS_KBD_KEY_8 }, { "9", brls::BRLS_KBD_KEY_9 },
    // Special / control keys
    { "SPACE",        brls::BRLS_KBD_KEY_SPACE         },
    { "ENTER",        brls::BRLS_KBD_KEY_ENTER         },
    { "BACKSPACE",    brls::BRLS_KBD_KEY_BACKSPACE      },
    { "TAB",          brls::BRLS_KBD_KEY_TAB            },
    { "ESC",          brls::BRLS_KBD_KEY_ESCAPE         },
    { "ESCAPE",       brls::BRLS_KBD_KEY_ESCAPE         },
    { "GRAVE",        brls::BRLS_KBD_KEY_GRAVE_ACCENT   },
    { "GRAVE_ACCENT", brls::BRLS_KBD_KEY_GRAVE_ACCENT   },
    // Arrow keys
    { "UP",    brls::BRLS_KBD_KEY_UP    },
    { "DOWN",  brls::BRLS_KBD_KEY_DOWN  },
    { "LEFT",  brls::BRLS_KBD_KEY_LEFT  },
    { "RIGHT", brls::BRLS_KBD_KEY_RIGHT },
    // Function keys
    { "F1",  brls::BRLS_KBD_KEY_F1  }, { "F2",  brls::BRLS_KBD_KEY_F2  },
    { "F3",  brls::BRLS_KBD_KEY_F3  }, { "F4",  brls::BRLS_KBD_KEY_F4  },
    { "F5",  brls::BRLS_KBD_KEY_F5  }, { "F6",  brls::BRLS_KBD_KEY_F6  },
    { "F7",  brls::BRLS_KBD_KEY_F7  }, { "F8",  brls::BRLS_KBD_KEY_F8  },
    { "F9",  brls::BRLS_KBD_KEY_F9  }, { "F10", brls::BRLS_KBD_KEY_F10 },
    { "F11", brls::BRLS_KBD_KEY_F11 }, { "F12", brls::BRLS_KBD_KEY_F12 },
    // Modifier keys (useful when specifying raw scancode in settings UI)
    { "LSHIFT",   brls::BRLS_KBD_KEY_LEFT_SHIFT    },
    { "RSHIFT",   brls::BRLS_KBD_KEY_RIGHT_SHIFT   },
    { "LCTRL",    brls::BRLS_KBD_KEY_LEFT_CONTROL  },
    { "RCTRL",    brls::BRLS_KBD_KEY_RIGHT_CONTROL },
    { "LALT",     brls::BRLS_KBD_KEY_LEFT_ALT      },
    { "RALT",     brls::BRLS_KBD_KEY_RIGHT_ALT     },
};

// ---- Gamepad button name table ------------------------------
// Allows config values like  handle.a = A  or  handle.a = 13
struct BrlsBtnName { const char* name; brls::ControllerButton btn; };

static const BrlsBtnName k_brlsBtnNames[] = {
    { "LT",    brls::BUTTON_LT    },
    { "LB",    brls::BUTTON_LB    },
    { "LSB",   brls::BUTTON_LSB   },
    { "UP",    brls::BUTTON_UP    },
    { "RIGHT", brls::BUTTON_RIGHT },
    { "DOWN",  brls::BUTTON_DOWN  },
    { "LEFT",  brls::BUTTON_LEFT  },
    { "BACK",  brls::BUTTON_BACK  },
    { "GUIDE", brls::BUTTON_GUIDE },
    { "START", brls::BUTTON_START },
    { "RSB",   brls::BUTTON_RSB   },
    { "Y",     brls::BUTTON_Y     },
    { "B",     brls::BUTTON_B     },
    { "A",     brls::BUTTON_A     },
    { "X",     brls::BUTTON_X     },
    { "RB",    brls::BUTTON_RB    },
    { "RT",    brls::BUTTON_RT    },
};

// ---- Retro button name → RETRO_DEVICE_ID_JOYPAD_* ----------
struct RetroNameMap { const char* name; unsigned id; };

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

// ---- Default game button map --------------------------------
// borealis ControllerButton → retro joypad ID
struct DefaultButtonMap { brls::ControllerButton brl; unsigned retroId; };

static const DefaultButtonMap k_defaultButtonMap[] = {
    { brls::BUTTON_A,          RETRO_DEVICE_ID_JOYPAD_A      },
    { brls::BUTTON_B,          RETRO_DEVICE_ID_JOYPAD_B      },
    { brls::BUTTON_X,          RETRO_DEVICE_ID_JOYPAD_X      },
    { brls::BUTTON_Y,          RETRO_DEVICE_ID_JOYPAD_Y      },
    { brls::BUTTON_UP,         RETRO_DEVICE_ID_JOYPAD_UP     },
    { brls::BUTTON_DOWN,       RETRO_DEVICE_ID_JOYPAD_DOWN   },
    { brls::BUTTON_LEFT,       RETRO_DEVICE_ID_JOYPAD_LEFT   },
    { brls::BUTTON_RIGHT,      RETRO_DEVICE_ID_JOYPAD_RIGHT  },
    { brls::BUTTON_LB,         RETRO_DEVICE_ID_JOYPAD_L      },
    { brls::BUTTON_RB,         RETRO_DEVICE_ID_JOYPAD_R      },
    { brls::BUTTON_LT,         RETRO_DEVICE_ID_JOYPAD_L2     },
    { brls::BUTTON_RT,         RETRO_DEVICE_ID_JOYPAD_R2     },
    { brls::BUTTON_START,      RETRO_DEVICE_ID_JOYPAD_START  },
    { brls::BUTTON_BACK,       RETRO_DEVICE_ID_JOYPAD_SELECT },
};

// ---- Default keyboard game button map -----------------------
// BrlsKeyboardScancode → retro joypad ID
// NOTE: JOYPAD_X is not listed here (no default kbd binding for it).
//       Users can configure keyboard.x to enable it.
struct DefaultKbMap { int scancode; unsigned retroId; };

static const DefaultKbMap k_defaultKbMap[] = {
    { brls::BRLS_KBD_KEY_X,     RETRO_DEVICE_ID_JOYPAD_A      },
    { brls::BRLS_KBD_KEY_Z,     RETRO_DEVICE_ID_JOYPAD_B      },
    { brls::BRLS_KBD_KEY_A,     RETRO_DEVICE_ID_JOYPAD_Y      },
    { brls::BRLS_KBD_KEY_UP,    RETRO_DEVICE_ID_JOYPAD_UP     },
    { brls::BRLS_KBD_KEY_DOWN,  RETRO_DEVICE_ID_JOYPAD_DOWN   },
    { brls::BRLS_KBD_KEY_LEFT,  RETRO_DEVICE_ID_JOYPAD_LEFT   },
    { brls::BRLS_KBD_KEY_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT  },
    { brls::BRLS_KBD_KEY_Q,     RETRO_DEVICE_ID_JOYPAD_L      },
    { brls::BRLS_KBD_KEY_W,     RETRO_DEVICE_ID_JOYPAD_R      },
    { brls::BRLS_KBD_KEY_E,     RETRO_DEVICE_ID_JOYPAD_L2     },
    { brls::BRLS_KBD_KEY_R,     RETRO_DEVICE_ID_JOYPAD_R2     },
    { brls::BRLS_KBD_KEY_ENTER, RETRO_DEVICE_ID_JOYPAD_START  },
    { brls::BRLS_KBD_KEY_S,     RETRO_DEVICE_ID_JOYPAD_SELECT },
};

// ---- Hotkey metadata table ----------------------------------
// Maps each Hotkey enum to its config key names and default values.
struct HotkeyMeta {
    const char* padKey;     ///< Config key for gamepad binding
    const char* padDefault; ///< Default gamepad value  ("none" = unbound)
    const char* displayName; ///< Human-readable name (UTF-8)
};

static const HotkeyMeta k_hotkeyMeta[] = {
    // FastForwardHold  – kbd: keyboard.fastforward, pad: handle.fastforward (legacy keys)
    {
        "handle.fastforward",
        "none",
        "\xe5\xbf\xab\xe8\xbf\x9b\xef\xbc\x88\xe4\xbf\x9d\xe6\x8c\x81\xef\xbc\x89"  // 快进
    },
    // RewindHold  – kbd: keyboard.rewind, pad: handle.rewind (legacy keys)
    {
        "handle.rewind",
        "none",
        "\xe5\x80\x92\xe5\xb8\xa6\xef\xbc\x88\xe4\xbf\x9d\xe6\x8c\x81\xef\xbc\x89"  // 倒带
    },
    // QuickSave
    {
        "hotkey.quicksave.pad",
        "none",
        "\xe5\xbf\xab\xe9\x80\x9f\xe4\xbf\x9d\xe5\xad\x98"  // 快速保存
    },
    // QuickLoad
    {
        "hotkey.quickload.pad",
        "none",
        "\xe5\xbf\xab\xe9\x80\x9f\xe8\xaf\xbb\xe5\x8f\x96"  // 快速读取
    },
    // OpenMenu
    {
        "hotkey.menu.pad",
        "none",
        "\xe6\x89\x93\xe5\xbc\x80\xe8\x8f\x9c\xe5\x8d\x95"  // 打开菜单
    },
    // Mute
    {
        "hotkey.mute.pad",
        "none",
        "\xe9\x9d\x99\xe9\x9f\xb3"  // 静音
    },
    // Pause
    {
        "hotkey.pause.pad",
        "none",
        "\xe6\x9a\x82\xe5\x81\x9c"  // 暂停
    },
    // OpenCheatMenu
    {
        "hotkey.cheat_menu.pad",
        "none",
        "\xe6\x89\x93\xe5\xbc\x80\xe9\x87\x91\xe6\x89\x8b\xe6\x8c\x87\xe8\x8f\x9c\xe5\x8d\x95"  // 打开金手指菜单
    },
    // OpenShaderMenu
    {
        "hotkey.shader_menu.pad",
        "none",
        "\xe6\x89\x93\xe5\xbc\x80\xe7\x9d\x80\xe8\x89\xb2\xe5\x99\xa8\xe8\x8f\x9c\xe5\x8d\x95"  // 打开着色器菜单
    },
    // Screenshot
    {
        "hotkey.screenshot.pad",
        "none",
        "\xe6\x88\xaa\xe5\xb1\x8f"  // 截屏
    },
    // ExitGame  – kbd: keyboard.exit (legacy key)
    {
        "hotkey.exit_game.pad",
        "none",
        "\xe9\x80\x80\xe5\x87\xba\xe6\xb8\xb8\xe6\x88\x8f"  // 退出游戏
    },
};

static_assert(
    sizeof(k_hotkeyMeta) / sizeof(k_hotkeyMeta[0]) ==
    static_cast<int>(InputMappingConfig::Hotkey::_Count),
    "k_hotkeyMeta size mismatch with Hotkey enum");

// ============================================================
// Static parser helpers
// ============================================================

/// Convert string to upper-case (in-place).
static std::string toUpper(const std::string& s)
{
    std::string u(s.size(), '\0');
    std::transform(s.begin(), s.end(), u.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return u;
}

// ============================================================
// InputMappingConfig – static public parsers
// ============================================================

int InputMappingConfig::parseKeyboardScancode(const std::string& s)
{
    std::string upper = toUpper(s);
    for (const auto& kn : k_kbdKeyNames)
        if (upper == kn.name) return kn.scancode;
    // Fall back to integer
    try { return std::stoi(s); } catch (...) {}
    return -1;
}

int InputMappingConfig::parseGamepadButton(const std::string& s)
{
    std::string upper = toUpper(s);
    for (const auto& bn : k_brlsBtnNames)
        if (upper == bn.name) return static_cast<int>(bn.btn);
    try { return std::stoi(s); } catch (...) {}
    return -1;
}

InputMappingConfig::KeyCombo
InputMappingConfig::parseKeyCombo(const std::string& s)
{
    KeyCombo result;
    if (s.empty() || toUpper(s) == "NONE") return result; // unbound

    // Split on '+' to find modifier tokens and the primary key.
    // e.g. "CTRL+SHIFT+F5"  → ["CTRL", "SHIFT", "F5"]
    std::vector<std::string> parts;
    {
        std::string part;
        for (char c : s) {
            if (c == '+') {
                if (!part.empty()) { parts.push_back(part); part.clear(); }
            } else {
                part += c;
            }
        }
        if (!part.empty()) parts.push_back(part);
    }

    return result;
}

// ============================================================
// Hotkey metadata accessors
// ============================================================

const char* InputMappingConfig::hotkeyPadConfigKey(Hotkey h)
{
    int i = static_cast<int>(h);
    if (i < 0 || i >= static_cast<int>(Hotkey::_Count)) return "";
    return k_hotkeyMeta[i].padKey;
}

const char* InputMappingConfig::hotkeyDisplayName(Hotkey h)
{
    int i = static_cast<int>(h);
    if (i < 0 || i >= static_cast<int>(Hotkey::_Count)) return "";
    return k_hotkeyMeta[i].displayName;
}

// ============================================================
// HotkeyBinding accessor
// ============================================================

const InputMappingConfig::HotkeyBinding&
InputMappingConfig::hotkeyBinding(Hotkey h) const
{
    static HotkeyBinding s_empty{};
    int i = static_cast<int>(h);
    if (i < 0 || i >= static_cast<int>(Hotkey::_Count)) return s_empty;
    return m_hotkeys[i];
}

// ============================================================
// setDefaults – write default values for all keys into cfg
// ============================================================

void InputMappingConfig::setDefaults(ConfigManager& cfg)
{
    using CV = ConfigValue;

    // ---- Fast-forward -----------------------------------------------
    cfg.SetDefault("fastforward.enabled",    CV(std::string("true")));
    cfg.SetDefault("fastforward.multiplier", CV(4.0f));
    cfg.SetDefault("fastforward.mute",       CV(std::string("true")));
    cfg.SetDefault("fastforward.mode",       CV(std::string("hold")));

    // ---- Rewind -----------------------------------------------------
    cfg.SetDefault("rewind.enabled",    CV(std::string("false")));
    cfg.SetDefault("rewind.bufferSize", CV(3600));
    cfg.SetDefault("rewind.step",       CV(2));
    cfg.SetDefault("rewind.mute",       CV(std::string("false")));
    cfg.SetDefault("rewind.mode",       CV(std::string("hold")));

    // ---- Game button map: gamepad -----------------------------------
    cfg.SetDefault("handle.a",      CV(std::string("A")));
    cfg.SetDefault("handle.b",      CV(std::string("B")));
    cfg.SetDefault("handle.x",      CV(std::string("none")));
    cfg.SetDefault("handle.y",      CV(std::string("Y")));
    cfg.SetDefault("handle.up",     CV(std::string("UP")));
    cfg.SetDefault("handle.down",   CV(std::string("DOWN")));
    cfg.SetDefault("handle.left",   CV(std::string("LEFT")));
    cfg.SetDefault("handle.right",  CV(std::string("RIGHT")));
    cfg.SetDefault("handle.start",  CV(std::string("START")));
    cfg.SetDefault("handle.select", CV(std::string("BACK")));
    cfg.SetDefault("handle.l",      CV(std::string("LB")));
    cfg.SetDefault("handle.r",      CV(std::string("RB")));
    cfg.SetDefault("handle.l2",     CV(std::string("LT")));
    cfg.SetDefault("handle.r2",     CV(std::string("RT")));

    // ---- Game button map: keyboard ----------------------------------
    // Populate only when the key is absent so user settings are preserved.
    static constexpr int k_kbMapCount =
        static_cast<int>(sizeof(k_defaultKbMap) / sizeof(k_defaultKbMap[0]));
    for (int i = 0; i < k_kbMapCount; ++i) {
        // Find the retro button name for this mapping.
        for (const auto& rn : k_retroNames) {
            if (rn.id == k_defaultKbMap[i].retroId) {
                std::string kbKey = std::string("keyboard.") + rn.name;
                cfg.SetDefault(kbKey, CV(k_defaultKbMap[i].scancode));
                break;
            }
        }
    }

    // ---- Emulator hotkeys ------------------------------------------
    for (int i = 0; i < static_cast<int>(Hotkey::_Count); ++i) {
        cfg.SetDefault(k_hotkeyMeta[i].padKey,
                       CV(std::string(k_hotkeyMeta[i].padDefault)));
    }
}

// ============================================================
// load – read all settings and bindings from cfg
// ============================================================

void InputMappingConfig::load(const ConfigManager& cfg)
{
    loadFfRewindSettings(cfg);  // 读取倒带和快进的设置
    loadGameButtonMap(cfg);     // 读取游戏按键映射设置
    loadHotkeyBindings(cfg);    // 读取功能热键绑定设置
}

// ---- Private helpers ----------------------------------------

void InputMappingConfig::loadFfRewindSettings(const ConfigManager& cfg)
{
    auto getFloat = [&](const std::string& key, float def) -> float {
        auto v = cfg.Get(key);
        if (v) {
            if (auto f = v->AsFloat()) return *f;
            if (auto i = v->AsInt())   return static_cast<float>(*i);
        }
        return def;
    };
    auto getInt = [&](const std::string& key, int def) -> int {
        auto v = cfg.Get(key);
        if (v) {
            if (auto i = v->AsInt())   return *i;
            if (auto f = v->AsFloat()) return static_cast<int>(*f);
        }
        return def;
    };
    auto getBool = [&](const std::string& key, bool def) -> bool {
        auto v = cfg.Get(key);
        if (v) {
            if (auto s = v->AsString()) return (*s == "true" || *s == "1" || *s == "yes");
            if (auto i = v->AsInt())    return (*i != 0);
        }
        return def;
    };
    auto getString = [&](const std::string& key, const std::string& def) -> std::string {
        auto v = cfg.Get(key);
        if (v) { if (auto s = v->AsString()) return *s; }
        return def;
    };

    ffMultiplier      = getFloat("fastforward.multiplier", 4.0f);
    if (ffMultiplier <= 0.0f) ffMultiplier = 4.0f;
    ffMute            = getBool("fastforward.mute", true);
    ffToggleMode      = (getString("fastforward.mode", "hold") == "toggle");

    rewindEnabled     = getBool("rewind.enabled", false);
    rewindBufSize     = static_cast<unsigned>(getInt("rewind.bufferSize", 3600));
    rewindStep        = static_cast<unsigned>(getInt("rewind.step", 2));
    if (rewindStep == 0) rewindStep = 1;
    rewindMute        = getBool("rewind.mute", false);
    rewindToggleMode  = (getString("rewind.mode", "hold") == "toggle");
}

void InputMappingConfig::loadGameButtonMap(const ConfigManager& cfg)
{
    // Read gamepad button binding: integer or named string value.
    auto getCfgPad = [&](const std::string& key, brls::ControllerButton def) -> int {
        auto v = cfg.Get(key);
        if (v) {
            if (auto i = v->AsInt())    return *i;
            if (auto s = v->AsString()) return parseGamepadButton(*s);
        }
        return static_cast<int>(def);
    };

    // Read keyboard scancode binding – returns the configured scancode for a retro button,
    // or -1 if the key is absent or set to "none".
    auto getCfgKb = [&](const std::string& key, int def) -> int {
        auto v = cfg.Get(key);
        if (v) {
            if (auto s = v->AsString()) {
                if (toUpper(*s) == "NONE" || s->empty()) return -1;
                return parseKeyboardScancode(*s);
            }
            if (auto i = v->AsInt()) return *i;
        }
        return def;
    };

    m_gameButtonMap.clear();

    // Read default gamepad button map and override with config values (if any).
    for (const auto& defPad : k_defaultButtonMap) {
        unsigned retroId = defPad.retroId;

        // Find the retro button's config key suffix.
        for (const auto& rn : k_retroNames) {
            if (rn.id != retroId) continue;

            // Gamepad binding
            std::string padKey = std::string("handle.") + rn.name;
            int padBtn = getCfgPad(padKey, defPad.brl);

            // Keyboard binding
            // Look up the default keyboard scancode from k_defaultKbMap.
            int kbDef = -1;
            for (const auto& dm : k_defaultKbMap) {
                if (dm.retroId == retroId) { kbDef = dm.scancode; break; }
            }
            std::string kbKey = std::string("keyboard.") + rn.name;
            int kbScan = getCfgKb(kbKey, kbDef);

            m_gameButtonMap.push_back({ retroId,
                (padBtn >= 0 && padBtn < static_cast<int>(brls::_BUTTON_MAX)) ? padBtn : -1,
                kbScan,
                });
            break;
        }
    }

    // Also emit entries for NAV buttons (dpad aliases, gamepad only).
    m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_UP,
        static_cast<int>(brls::BUTTON_NAV_UP)});
    m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_DOWN,
        static_cast<int>(brls::BUTTON_NAV_DOWN)});
    m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_LEFT,
        static_cast<int>(brls::BUTTON_NAV_LEFT)});
    m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_RIGHT,
        static_cast<int>(brls::BUTTON_NAV_RIGHT)});

}

void InputMappingConfig::loadHotkeyBindings(const ConfigManager& cfg)
{
    for (int i = 0; i < static_cast<int>(Hotkey::_Count); ++i) {
        HotkeyBinding& hk = m_hotkeys[i];
        // --- gamepad binding ---
        {
            std::string val = k_hotkeyMeta[i].padDefault;
            auto v = cfg.Get(k_hotkeyMeta[i].padKey);

            if (v) {
                if (auto s = v->AsString()) val = *s;
                else if (auto n = v->AsInt()) val = std::to_string(*n);
            }
            if (toUpper(val) == "NONE" || val.empty())
                hk.padButton = -1;
            else
                hk.padButton = parseGamepadButton(val);
        }
    }
}

} // namespace beiklive
