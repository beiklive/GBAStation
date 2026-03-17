#pragma once

#include <borealis.hpp>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"
#include "UI/Utils/Utils.hpp"
#include "UI/Utils/ProImage.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  GameLibraryEntry  –  游戏库单条目数据
// ─────────────────────────────────────────────────────────────────────────────
struct GameLibraryEntry {
    std::string fileName;    ///< 文件名（含后缀）
    std::string gamePath;    ///< 完整游戏路径
    std::string logoPath;    ///< logo 图片路径（可为空）
    std::string displayName; ///< 显示名称（来自 NameMappingManager 或文件名）
    std::string lastOpen;    ///< 上次游玩时间字符串（"从未游玩" 表示未玩过）
    int         totalTime = 0;  ///< 总游玩时长（秒）
    int         playCount = 0;  ///< 游玩次数
};

// ─────────────────────────────────────────────────────────────────────────────
//  GameLibraryItem  –  游戏库网格的单个元素
//  纵向 Box：封面图（ProImage 异步加载）+ 标题 Label
// ─────────────────────────────────────────────────────────────────────────────
class GameLibraryItem : public brls::Box
{
public:
    explicit GameLibraryItem(const GameLibraryEntry& entry);

    /// 按 A 键 / 点击激活时触发
    std::function<void(const GameLibraryEntry&)> onActivated;
    /// 按 X 键时触发（显示选项菜单）
    std::function<void(const GameLibraryEntry&)> onOptions;

    const GameLibraryEntry& getEntry() const { return m_entry; }

    /// 更新封面图片路径并重新加载
    void updateCover(const std::string& newLogoPath);
    /// 更新标题显示
    void updateTitle(const std::string& newTitle);

    void onChildFocusGained(brls::View* directChild, brls::View* focusedView) override;
    void onChildFocusLost(brls::View* directChild, brls::View* focusedView) override;
    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

private:
    GameLibraryEntry          m_entry;
    beiklive::UI::ProImage*   m_coverImage = nullptr;
    brls::Label*              m_label      = nullptr;

    bool  m_focused        = false;  ///< 当前是否处于焦点状态
    float m_scale          = 1.0f;   ///< 聚焦缩放比，由 draw() 平滑插值

    bool  m_clickAnimating = false;  ///< 是否正在播放点击弹性动画
    float m_clickT         = 0.0f;   ///< 点击动画已播放时间（秒）
    float m_clickScale     = 1.0f;   ///< 点击动画缩放比

    void triggerClickBounce();
};

// ─────────────────────────────────────────────────────────────────────────────
//  GameLibraryPage  –  游戏库主页面
//  结构：BrowserHeader（游戏数量） + ScrollingFrame（5列网格） + BottomBar
//  Y 键：弹出排序方式 Dropdown
// ─────────────────────────────────────────────────────────────────────────────
class GameLibraryPage : public beiklive::UI::BBox
{
public:
    /// 排序方式枚举
    enum class SortMode {
        ByLastOpen,  ///< 按最近游玩时间排序（lastopen 字段，DESC）
        ByTotalTime, ///< 按总游玩时长排序（totaltime 字段，DESC）
        ByName       ///< 按游戏名称排序（displayName，ASC）
    };

    GameLibraryPage();

    /// 游戏被激活时（用于外部注入启动逻辑，如 StartPageView 设置）
    std::function<void(const GameLibraryEntry&)> onGameSelected;
    /// 游戏 X 键选项回调（用于外部覆盖，若不设置则页面内部处理）
    std::function<void(const GameLibraryEntry&)> onGameOptions;

private:
    beiklive::UI::BrowserHeader* m_header   = nullptr;
    brls::ScrollingFrame*        m_scroll   = nullptr;
    brls::Box*                   m_gridBox  = nullptr;

    std::vector<GameLibraryEntry> m_entries; ///< 游戏数据列表
    SortMode m_sortMode = SortMode::ByLastOpen;

    /// 从 gamedataManager / NameMappingManager 收集所有游戏条目
    void loadEntries();
    /// 按当前 m_sortMode 对 m_entries 排序
    void sortEntries();
    /// 清空并重建网格，焦点移到第一个元素
    void rebuildGrid();
    /// 弹出排序方式 Dropdown
    void showSortDropdown();
    /// 更新标题栏游戏数量显示
    void updateHeader();

    /// 单个游戏被激活时的内部处理
    void onItemActivated(const GameLibraryEntry& entry);
    /// 单个游戏 X 键时的内部处理（选项菜单）
    void onItemOptions(const GameLibraryEntry& entry);
};
