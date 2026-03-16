#pragma once

#include "common.hpp"
#include <borealis.hpp>
#include <functional>
#include <string>
#include <vector>

/// 金手指条目
struct CheatEntry {
    std::string desc;    ///< 金手指名称
    std::string code;    ///< 金手指代码
    bool        enabled = true; ///< 是否启用
};

class GameMenu: public brls::Box
{
public:
    GameMenu();
    ~GameMenu();

    /// 设置"返回游戏"回调。菜单关闭时在 UI 线程调用。
    void setCloseCallback(std::function<void()> cb) { m_closeCallback = std::move(cb); }

    /// 设置金手指列表并刷新 cheatbox 显示。
    /// 须在主（UI）线程调用。
    void setCheats(const std::vector<CheatEntry>& cheats);

    /// 设置金手指切换回调。切换某条金手指的启用状态时调用，参数为（索引, 新状态）。
    void setCheatToggleCallback(std::function<void(int, bool)> cb)
    {
        m_cheatToggleCallback = std::move(cb);
    }

private:
    std::function<void()>         m_closeCallback;
    std::function<void(int, bool)> m_cheatToggleCallback;
    std::vector<CheatEntry>      m_cheats;     ///< 当前金手指列表（副本，用于 UI 显示）
    brls::Box*                   m_cheatbox = nullptr; ///< 金手指内容区域
};
