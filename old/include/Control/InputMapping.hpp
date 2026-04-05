#pragma once

#include <string>
#include <vector>

#include "Utils/ConfigManager.hpp"

namespace beiklive {

// ============================================================
// InputMappingConfig
//
// 集中管理模拟器所有按键绑定配置：
//   - 游戏按钮映射（手柄 → retro joypad ID）
//   - 模拟器热键（快进、倒带、快速存档等）
//   - 快进/倒带设置（与绑定一同加载）
//
// 查找表、扫描码解析和配置读写位于 src/Control/InputMapping.cpp。
// GameView 持有一个实例并委托所有配置加载给该类。
// ============================================================

class InputMappingConfig
{
public:

    // ----------------------------------------------------------------
    // 模拟器系统功能键标识符
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

        _Count                  ///< 哨兵值，始终在末尾
    };

    // ----------------------------------------------------------------
    // 游戏按钮绑定：retro joypad ID 映射到手柄按钮。
    // ----------------------------------------------------------------
    struct GameButtonEntry
    {
        unsigned retroId   = 0;   ///< RETRO_DEVICE_ID_JOYPAD_*
        int      padButton = -1;  ///< 手柄按钮；-1表示未绑定
    };

    // ----------------------------------------------------------------
    // 模拟器热键绑定：手柄按键组合（支持多键同时按下）。
    // 空列表表示未绑定；多个按键表示组合键，所有键同时按下才触发。
    // 示例配置：hotkey.menu.pad = LB+START（LB与START同时按下打开菜单）
    // ----------------------------------------------------------------
    struct HotkeyBinding
    {
        std::vector<int> padButtons;  ///< 手柄按键列表；空表示未绑定

        bool isPadBound() const { return !padButtons.empty(); }
    };

    // ----------------------------------------------------------------
    // 快进/倒带设置（与绑定一同加载）
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
    // 配置读写
    // ----------------------------------------------------------------

    /// 将所有键的默认值写入 @a cfg（仅在键不存在时写入）。
    void setDefaults(ConfigManager& cfg);

    /// 从 @a cfg 加载所有设置和绑定。
    void load(const ConfigManager& cfg);

    // ----------------------------------------------------------------
    // 访问器
    // ----------------------------------------------------------------

    const std::vector<GameButtonEntry>& gameButtonMap() const
    { return m_gameButtonMap; }

    const HotkeyBinding& hotkeyBinding(Hotkey h) const;

    // ----------------------------------------------------------------
    // 静态解析器（可供未来设置UI复用）
    // ----------------------------------------------------------------

    /// 解析手柄按钮：整数或名称（"A"、"LB"、"RT"）。
    static int     parseGamepadButton(const std::string& s);

    /// 返回热键手柄绑定对应的配置键名。
    /// 例如 Hotkey::QuickSave → "hotkey.quicksave.pad"
    static const char* hotkeyPadConfigKey(Hotkey h);

    /// 返回热键的可读显示名称（UTF-8）。
    static const char* hotkeyDisplayName(Hotkey h);

private:
    std::vector<GameButtonEntry> m_gameButtonMap;
    HotkeyBinding                m_hotkeys[static_cast<int>(Hotkey::_Count)];

    void loadGameButtonMap(const ConfigManager& cfg);
    void loadHotkeyBindings(const ConfigManager& cfg);
    void loadFfRewindSettings(const ConfigManager& cfg);
};

} // namespace beiklive
