#pragma once

#include "core/common.h"
#include "core/Tools.hpp"
#include "ui/utils/Box.hpp"
#include "ui/utils/GridBox.hpp"
#include "ui/utils/GridItem.hpp"
#include "ui/utils/DetailCell.hpp"
#include "ui/utils/GameOptionsSidebar.hpp"

namespace beiklive
{
    /**
     * GameLibraryPage – 游戏库主页面
     *
     * 布局：左侧分类列表（DetailCell）+ 右侧 2 列 GridBox
     *
     * 功能：
     *   - 按分类筛选：全部游戏 / 收藏 / GBA / GBC / GB
     *   - 按最近游玩（默认）/ 游玩时长 / 游戏名称 三种方式排序
     *   - Y 键弹出排序方式 Dropdown
     *   - ZR 键收藏/取消收藏
     */
    class GameLibraryPage : public beiklive::Box
    {
    public:
        /// 排序方式
        enum class SortMode
        {
            ByLastPlayed,  ///< 按最近游玩时间（默认）
            ByPlayTime,    ///< 按总游玩时长
            ByName,        ///< 按游戏名称
        };

        /// 分类筛选类型
        enum class FilterType
        {
            All,        ///< 全部游戏
            Favourite,  ///< 仅收藏
            GBA,        ///< 仅 GBA
            GBC,        ///< 仅 GBC
            GB,         ///< 仅 GB
        };

        GameLibraryPage();
        ~GameLibraryPage() = default;

        /// 游戏被激活（启动）时触发
        std::function<void(const beiklive::GameEntry&)> onGameSelected;

    private:
        beiklive::GridBox*    m_grid      = nullptr;
        std::vector<beiklive::GameEntry> m_entries;        // 全部游戏
        std::vector<beiklive::GameEntry> m_filteredEntries; // 当前筛选后的游戏
        SortMode              m_sortMode  = SortMode::ByLastPlayed;
        FilterType            m_filterType = FilterType::All;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        // 左侧侧边栏
        brls::Box* m_sidebar = nullptr;
        beiklive::DetailCell* m_allCell     = nullptr;
        beiklive::DetailCell* m_favCell     = nullptr;
        beiklive::DetailCell* m_gbaCell     = nullptr;
        beiklive::DetailCell* m_gbcCell     = nullptr;
        beiklive::DetailCell* m_gbCell      = nullptr;

        void _loadAndShowEntries();
        void _sortEntries();
        void _applyFilter();
        void _rebuildGrid();
        void _updateSidebar();
        void _updateSortDropdown();
        void _reloadEntries();
        void _showSortDropdown();

        /// 显示游戏选项侧边栏
        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        /// 关闭游戏选项侧边栏
        void _hideGameOptionsPanel();

        /// 将 GameEntry 的平台字段转换为徽标颜色枚举
        static PlatformBadgeColor _platformBadge(int platform);

        /// 将游戏时长（秒）格式化为可读字符串
        static std::string _formatPlayTime(int seconds);

        int _currentFocusedIndex = -1;
    };

} // namespace beiklive
