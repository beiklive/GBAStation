#include "Control/InputMapping.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// 引入 borealis 的 ControllerButton 和 BrlsKeyboardScancode 类型。
#include <borealis.hpp>

// 引入 libretro 手柄按键 ID。
#include "third_party/mgba/src/platform/libretro/libretro.h"

namespace beiklive {

// ============================================================
// 内部查找表
// ============================================================

// ---- 手柄按键名称表 ------------------------------
// 支持配置项如 handle.a = A 或 handle.a = 13
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
    { "START", brls::BUTTON_START },
    { "RSB",   brls::BUTTON_RSB   },
    { "Y",     brls::BUTTON_Y     },
    { "B",     brls::BUTTON_B     },
    { "A",     brls::BUTTON_A     },
    { "X",     brls::BUTTON_X     },
    { "RB",    brls::BUTTON_RB    },
    { "RT",    brls::BUTTON_RT    },
};

// ---- Retro 按键名称 → RETRO_DEVICE_ID_JOYPAD_* ----------
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

// ---- 默认游戏按键映射表（手柄）-----------------------
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

// ---- 热键元数据表 ------------------------------------
// 将 Hotkey 枚举映射到配置键名和默认值。
struct HotkeyMeta {
    const char* padKey;     ///< 手柄绑定的配置键名
    const char* padDefault; ///< 手柄默认值（"none" 表示未绑定）
    const char* displayName; ///< 显示名称（UTF-8）
};

static const HotkeyMeta k_hotkeyMeta[] = {
    // 快进（保持）
    {
        "handle.fastforward",
        "none",
        "\xe5\xbf\xab\xe8\xbf\x9b"  // 快进
    },
    // 倒带（保持）
    {
        "handle.rewind",
        "none",
        "\xe5\x80\x92\xe5\xb8\xa6"  // 倒带
    },
    // 快速保存
    {
        "hotkey.quicksave.pad",
        "none",
        "\xe5\xbf\xab\xe9\x80\x9f\xe4\xbf\x9d\xe5\xad\x98"  // 快速保存
    },
    // 快速读取
    {
        "hotkey.quickload.pad",
        "none",
        "\xe5\xbf\xab\xe9\x80\x9f\xe8\xaf\xbb\xe5\x8f\x96"  // 快速读取
    },
    // 打开菜单
    {
        "hotkey.menu.pad",
        "none",
        "\xe6\x89\x93\xe5\xbc\x80\xe8\x8f\x9c\xe5\x8d\x95"  // 打开菜单
    },
    // 静音
    {
        "hotkey.mute.pad",
        "none",
        "\xe9\x9d\x99\xe9\x9f\xb3"  // 静音
    },
    // 暂停
    {
        "hotkey.pause.pad",
        "none",
        "\xe6\x9a\x82\xe5\x81\x9c"  // 暂停
    },
    // 打开金手指菜单
    {
        "hotkey.cheat_menu.pad",
        "none",
        "\xe6\x89\x93\xe5\xbc\x80\xe9\x87\x91\xe6\x89\x8b\xe6\x8c\x87\xe8\x8f\x9c\xe5\x8d\x95"  // 打开金手指菜单
    },
    // 打开着色器菜单
    {
        "hotkey.shader_menu.pad",
        "none",
        "\xe6\x89\x93\xe5\xbc\x80\xe7\x9d\x80\xe8\x89\xb2\xe5\x99\xa8\xe8\x8f\x9c\xe5\x8d\x95"  // 打开着色器菜单
    },
    // 截屏
    {
        "hotkey.screenshot.pad",
        "none",
        "\xe6\x88\xaa\xe5\xb1\x8f"  // 截屏
    },
    // 退出游戏
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
// 静态解析辅助函数
// ============================================================

/// 将字符串转为大写（原地）。
static std::string toUpper(const std::string& s)
{
    std::string u(s.size(), '\0');
    std::transform(s.begin(), s.end(), u.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return u;
}

// ============================================================
// InputMappingConfig – 静态公共解析函数
// ============================================================

int InputMappingConfig::parseGamepadButton(const std::string& s)
{
    std::string upper = toUpper(s);
    for (const auto& bn : k_brlsBtnNames)
        if (upper == bn.name) return static_cast<int>(bn.btn);
    try { return std::stoi(s); } catch (...) {}
    return -1;
}

// ============================================================
// 热键元数据访问函数
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
// HotkeyBinding 访问函数
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
// setDefaults – 向 cfg 写入所有配置项的默认值
// ============================================================

void InputMappingConfig::setDefaults(ConfigManager& cfg)
{
    using CV = ConfigValue;

    // ---- 快进 -----------------------------------------------
    cfg.SetDefault("fastforward.enabled",    CV(std::string("true")));
    cfg.SetDefault("fastforward.multiplier", CV(4.0f));
    cfg.SetDefault("fastforward.mute",       CV(std::string("true")));
    cfg.SetDefault("fastforward.mode",       CV(std::string("hold")));

    // ---- 倒带 -----------------------------------------------
    cfg.SetDefault("rewind.enabled",    CV(std::string("false")));
    cfg.SetDefault("rewind.bufferSize", CV(3600));
    cfg.SetDefault("rewind.step",       CV(2));
    cfg.SetDefault("rewind.mute",       CV(std::string("false")));
    cfg.SetDefault("rewind.mode",       CV(std::string("hold")));

    // ---- 游戏按键映射：手柄 ---------------------------------
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

    // ---- 模拟器热键 -----------------------------------------
    for (int i = 0; i < static_cast<int>(Hotkey::_Count); ++i) {
        cfg.SetDefault(k_hotkeyMeta[i].padKey,
                       CV(std::string(k_hotkeyMeta[i].padDefault)));
    }
}

// ============================================================
// load – 从 cfg 读取所有设置和绑定
// ============================================================

void InputMappingConfig::load(const ConfigManager& cfg)
{
    loadFfRewindSettings(cfg);  // 读取倒带和快进的设置
    loadGameButtonMap(cfg);     // 读取游戏按键映射设置
    loadHotkeyBindings(cfg);    // 读取功能热键绑定设置
}

// ---- 私有辅助函数 ----------------------------------------

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
    // 读取手柄按键绑定：整数或命名字符串。
    auto getCfgPad = [&](const std::string& key, brls::ControllerButton def) -> int {
        auto v = cfg.Get(key);
        if (v) {
            if (auto i = v->AsInt())    return *i;
            if (auto s = v->AsString()) return parseGamepadButton(*s);
        }
        return static_cast<int>(def);
    };

    m_gameButtonMap.clear();

    // 读取默认手柄映射，并用配置值覆盖。
    for (const auto& defPad : k_defaultButtonMap) {
        unsigned retroId = defPad.retroId;

        // 查找该 retro 按键的配置键名后缀。
        for (const auto& rn : k_retroNames) {
            if (rn.id != retroId) continue;

            // 手柄绑定
            std::string padKey = std::string("handle.") + rn.name;
            int padBtn = getCfgPad(padKey, defPad.brl);

            m_gameButtonMap.push_back({ retroId,
                (padBtn >= 0 && padBtn < static_cast<int>(brls::_BUTTON_MAX)) ? padBtn : -1,
                });
            break;
        }
    }

    // 同时为 NAV 按键（方向键别名，仅手柄）添加条目。
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
        // --- 手柄绑定（支持组合键，如 "LB+START"）---
        {
            std::string val = k_hotkeyMeta[i].padDefault;
            auto v = cfg.Get(k_hotkeyMeta[i].padKey);

            if (v) {
                if (auto s = v->AsString()) val = *s;
                else if (auto n = v->AsInt()) val = std::to_string(*n);
            }

            hk.padButtons.clear();
            if (toUpper(val) != "NONE" && !val.empty()) {
                // 按 '+' 分割，支持多键组合（如 "LB+START"）
                std::vector<std::string> parts;
                std::string part;
                for (char c : val) {
                    if (c == '+') {
                        if (!part.empty()) { parts.push_back(part); part.clear(); }
                    } else {
                        part += c;
                    }
                }
                if (!part.empty()) parts.push_back(part);

                for (const auto& p : parts) {
                    int btn = parseGamepadButton(p);
                    if (btn >= 0)
                        hk.padButtons.push_back(btn);
                }
            }
        }
    }
}

} // namespace beiklive
