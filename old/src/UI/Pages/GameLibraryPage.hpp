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
//  GameLibraryListItem  –  游戏库列表的单行视图（类似 FileListItemView）
//  横向 Box：左侧强调色 + 封面图标 + 游戏名称 + 右侧时间信息
// ─────────────────────────────────────────────────────────────────────────────
class GameLibraryListItem : public brls::Box
{
public:
    explicit GameLibraryListItem(const GameLibraryEntry& entry, int index);

    /// 按 A 键 / 点击激活时触发
    std::function<void(int)> onActivated;
    /// 按 X 键时触发（显示选项菜单）
    std::function<void(int)> onOptions;
    /// 获得焦点时触发（用于更新详情面板和标题栏）
    std::function<void(int)> onFocused;

    const GameLibraryEntry& getEntry() const { return m_entry; }
    int getIndex() const { return m_index; }

    /// 更新封面图片路径并重新加载
    void updateCover(const std::string& newLogoPath);
    /// 更新标题显示
    void updateTitle(const std::string& newTitle);

    void onFocusGained() override;
    void onFocusLost() override;

private:
    GameLibraryEntry          m_entry;
    int                       m_index      = -1;
    brls::Rectangle*          m_accent     = nullptr;  ///< 左侧焦点强调色矩形
    beiklive::UI::ProImage*   m_icon       = nullptr;  ///< 封面图标（小图）
    brls::Label*              m_nameLabel  = nullptr;  ///< 游戏显示名称
    brls::Label*              m_infoLabel  = nullptr;  ///< 右侧游玩时间信息
};

// ─────────────────────────────────────────────────────────────────────────────
//  GameLibraryPage  –  游戏库主页面
//  结构：BrowserHeader + 横向内容区（网格 + 右侧详情面板）+ BottomBar
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
    beiklive::UI::BrowserHeader* m_header      = nullptr;
    brls::ScrollingFrame*        m_scroll      = nullptr;
    brls::Box*                   m_itemsBox    = nullptr;  ///< 列表条目直接父容器

    // ── 右侧详情面板组件 ─────────────────────────────────────────────────────
    brls::Box*                   m_detailPanel     = nullptr;  ///< 详情面板容器
    beiklive::UI::ProImage*      m_detailThumb     = nullptr;  ///< 封面缩略图
    brls::Label*                 m_detailName      = nullptr;  ///< 显示名称
    brls::Label*                 m_detailFileName  = nullptr;  ///< 文件名
    brls::Label*                 m_detailLastOpen  = nullptr;  ///< 上次游玩时间
    brls::Label*                 m_detailTotalTime = nullptr;  ///< 游玩总时长
    brls::Label*                 m_detailPlatform  = nullptr;  ///< 平台名称

    std::vector<GameLibraryEntry> m_entries; ///< 游戏数据列表
    SortMode m_sortMode = SortMode::ByLastOpen;

    /// 构建右侧详情面板
    void buildDetailPanel();
    /// 用指定游戏条目更新详情面板内容
    void updateDetailPanel(const GameLibraryEntry& entry);
    /// 清空详情面板（无焦点时）
    void clearDetailPanel();

    /// 从 gamedataManager / NameMappingManager 收集所有游戏条目
    void loadEntries();
    /// 按当前 m_sortMode 对 m_entries 排序
    void sortEntries();
    /// 清空并重建列表，焦点移到第一个元素
    void rebuildList();
    /// 弹出排序方式 Dropdown
    void showSortDropdown();
    /// 更新标题栏游戏数量显示
    void updateHeader();
    /// 切换条目时实时更新标题栏显示游戏名
    void updateHeaderFocused(const GameLibraryEntry& entry);

    /// 单个游戏被激活时的内部处理
    void onItemActivated(const GameLibraryEntry& entry);
    /// 单个游戏 X 键时的内部处理（选项菜单）
    void onItemOptions(const GameLibraryEntry& entry);
};
