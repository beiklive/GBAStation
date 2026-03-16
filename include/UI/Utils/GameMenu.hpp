#pragma once

#include "common.hpp"
#include <borealis.hpp>
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_detail.hpp>
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
    /// 状态槽位信息（用于面板显示）
    struct StateSlotInfo {
        bool        exists    = false; ///< 状态文件是否存在
        std::string thumbPath;         ///< 缩略图路径（存在时非空）
    };

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

    /// 设置当前游戏文件名，用于从 gamedataManager 读取/写入游戏专属遮罩路径。
    /// 须在菜单首次显示之前调用。
    void setGameFileName(const std::string& fileName);

    /// 设置遮罩路径变更回调。用户选择新遮罩后在 UI 线程调用，参数为新的完整遮罩路径。
    void setOverlayChangedCallback(std::function<void(const std::string&)> cb)
    {
        m_overlayChangedCallback = std::move(cb);
    }

    /// 设置保存状态回调。用户确认保存后在 UI 线程调用，参数为槽号（0-9）。
    void setSaveStateCallback(std::function<void(int)> cb) { m_saveStateCallback = std::move(cb); }

    /// 设置读取状态回调。用户确认读取后在 UI 线程调用，参数为槽号（0-9）。
    void setLoadStateCallback(std::function<void(int)> cb) { m_loadStateCallback = std::move(cb); }

    /// 设置状态槽位信息查询回调。参数为槽号（0-9），返回槽位信息（是否存在、缩略图路径）。
    void setStateInfoCallback(std::function<StateSlotInfo(int)> cb) { m_stateInfoCallback = std::move(cb); }

    /// 重置保存/读取状态面板预览图为 NoData 状态。须在 UI 线程调用。
    void refreshStatePanels();

private:
    /// 构建保存/读取状态槽位面板内容（10个槽位按钮）。
    /// 槽位按钮获得焦点时，自动更新对应预览图（m_save/loadPreviewImage）。
    void buildStatePanel(bool isSave, brls::Box* container);

    std::function<void()>               m_closeCallback;
    std::function<void(const std::string&)> m_overlayChangedCallback;
    std::function<void(int, bool)>      m_cheatToggleCallback;
    std::function<void(int)>            m_saveStateCallback;     ///< 确认保存状态时调用
    std::function<void(int)>            m_loadStateCallback;     ///< 确认读取状态时调用
    std::function<StateSlotInfo(int)>   m_stateInfoCallback;     ///< 查询槽位信息
    std::vector<CheatEntry>             m_cheats;
    std::string                         m_romFileName;
    brls::ScrollingFrame*               m_cheatScrollFrame       = nullptr;
    brls::Box*                          m_cheatItemBox           = nullptr;
    brls::ScrollingFrame*               m_displayScrollFrame     = nullptr;
    brls::DetailCell*                   m_overlayPathCell        = nullptr;
    brls::Box*                          m_saveStatePanel         = nullptr; ///< 保存状态面板外层容器（横向：列表+预览）
    brls::Box*                          m_loadStatePanel         = nullptr; ///< 读取状态面板外层容器（横向：列表+预览）
    brls::Box*                          m_saveStateItemBox       = nullptr; ///< 保存状态条目容器（直接用 Box，不用 ScrollingFrame）
    brls::Box*                          m_loadStateItemBox       = nullptr; ///< 读取状态条目容器（直接用 Box，不用 ScrollingFrame）
    brls::Image*                        m_savePreviewImage       = nullptr; ///< 保存状态预览图（右侧）
    brls::Image*                        m_loadPreviewImage       = nullptr; ///< 读取状态预览图（右侧）
    brls::Label*                        m_saveNoDataLabel        = nullptr; ///< 保存状态无数据提示
    brls::Label*                        m_loadNoDataLabel        = nullptr; ///< 读取状态无数据提示
};
