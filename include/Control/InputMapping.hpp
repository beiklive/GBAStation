#pragma once

#include <string>
#include <vector>

#include "Utils/ConfigManager.hpp"

namespace beiklive {

// ============================================================
// InputMappingConfig
//
// Centralises all key-binding configuration for the emulator:
//   - Game button map  (gamepad + keyboard → retro joypad ID)
//   - Emulator hotkeys (fast-forward, rewind, quick-save, …)
//   - Fast-forward / rewind settings loaded together with bindings
//
// Lookup tables, scancode parsers and config I/O live in
// src/Control/InputMapping.cpp.  GameView holds one instance and
// delegates all config loading to this class.
// ============================================================

class InputMappingConfig
{
public:

    // ----------------------------------------------------------------
    // Emulator system function-key identifiers
    // ----------------------------------------------------------------
    enum class Hotkey : int
    {
        FastForward,        ///< 快进（保持）
        Rewind,             ///< 倒带（保持）
        QuickSave,              ///< 快速保存
        QuickLoad,              ///< 快速读取
        OpenMenu,               ///< 打开菜单
        Mute,                   ///< 静音
        Pause,                  ///< 暂停
        OpenCheatMenu,          ///< 打开金手指菜单
        OpenShaderMenu,         ///< 打开着色器菜单
        Screenshot,             ///< 截屏
        ExitGame,               ///< 退出游戏

        _Count                  ///< Sentinel – always last
    };

    // ----------------------------------------------------------------
    // A keyboard key combo: primary scancode + optional modifier keys.
    // Parsed from strings like "F5", "CTRL+F5", "SHIFT+Z".
    // ----------------------------------------------------------------
    struct KeyCombo
    {
        int  scancode = -1;     ///< BrlsKeyboardScancode; -1 = unbound
        bool ctrl     = false;
        bool shift    = false;
        bool alt      = false;

        bool isBound() const { return scancode >= 0; }
    };

    // ----------------------------------------------------------------
    // One game-button binding: retro joypad ID mapped to a gamepad
    // button and/or a keyboard key.
    // ----------------------------------------------------------------
    struct GameButtonEntry
    {
        unsigned retroId    = 0;   ///< RETRO_DEVICE_ID_JOYPAD_*
        int      padButton  = -1;  ///< brls::ControllerButton; -1 = unbound
        int      kbScancode = -1;  ///< BrlsKeyboardScancode; -1 = unbound
    };

    // ----------------------------------------------------------------
    // One emulator hotkey binding: keyboard combo + gamepad button.
    // Both may be unbound (-1 / isBound() == false).
    // ----------------------------------------------------------------
    struct HotkeyBinding
    {
        int      padButton = -1;    ///< brls::ControllerButton; -1 = unbound

        bool isPadBound() const { return padButton >= 0; }
    };

    // ----------------------------------------------------------------
    // Fast-forward / rewind settings (loaded together with bindings)
    // ----------------------------------------------------------------
    float    ffMultiplier     = 4.0f;   ///< Speed multiplier (fastforward.multiplier)
    bool     ffMute           = true;   ///< Mute audio during FF (fastforward.mute)
    bool     ffToggleMode     = false;  ///< false = hold, true = toggle (fastforward.mode)
    bool     rewindEnabled    = false;  ///< rewind.enabled
    unsigned rewindBufSize    = 3600;   ///< rewind.bufferSize
    unsigned rewindStep       = 2;      ///< rewind.step
    bool     rewindMute       = false;  ///< rewind.mute
    bool     rewindToggleMode = false;  ///< false = hold, true = toggle (rewind.mode)

    // ----------------------------------------------------------------
    // Config I/O
    // ----------------------------------------------------------------

    /// Write defaults for all keys into @a cfg (only when key absent).
    void setDefaults(ConfigManager& cfg);

    /// Load all settings and bindings from @a cfg.
    void load(const ConfigManager& cfg);

    // ----------------------------------------------------------------
    // Accessors
    // ----------------------------------------------------------------

    const std::vector<GameButtonEntry>& gameButtonMap() const
    { return m_gameButtonMap; }

    const HotkeyBinding& hotkeyBinding(Hotkey h) const;

    // ----------------------------------------------------------------
    // Static parsers – reusable by a future settings UI
    // ----------------------------------------------------------------

    /// Parse a keyboard scancode: integer ("88") or named ("X", "TAB").
    static int     parseKeyboardScancode(const std::string& s);

    /// Parse a gamepad button: integer or named ("A", "LB", "RT").
    static int     parseGamepadButton(const std::string& s);

    /// Parse a key-combo string: "F5", "CTRL+F5", "SHIFT+Z", "none".
    static KeyCombo parseKeyCombo(const std::string& s);

    /// Return the config key name for a hotkey's gamepad binding.
    /// e.g. Hotkey::QuickSave  → "hotkey.quicksave.pad"
    static const char* hotkeyPadConfigKey(Hotkey h);

    /// Return a human-readable display name for a hotkey (UTF-8).
    static const char* hotkeyDisplayName(Hotkey h);

private:
    std::vector<GameButtonEntry> m_gameButtonMap;
    HotkeyBinding                m_hotkeys[static_cast<int>(Hotkey::_Count)];

    void loadGameButtonMap(const ConfigManager& cfg);
    void loadHotkeyBindings(const ConfigManager& cfg);
    void loadFfRewindSettings(const ConfigManager& cfg);
};

} // namespace beiklive
