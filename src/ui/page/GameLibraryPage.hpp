#pragma once

#include "core/common.h"
#include "core/Tools.hpp"
#include "ui/utils/Box.hpp"
#include "ui/utils/GridBox.hpp"
#include "ui/utils/GridItem.hpp"
#include "ui/utils/GameOptionsSidebar.hpp"

namespace beiklive
{
    /**
     * GameLibraryPage – 游戏库主页面
     *
     * 布局：与 FileListPage 相同的 Header + BottomBar 结构，
     * 主视图为 GridBox（3 列），每个格子使用 GridItem（GAME_LIBRARY 模式）。
     *
     * 功能：
     *   - 按最近游玩（默认）/ 游玩时长 / 游戏名称 三种方式排序
     *   - Y 键弹出排序方式 Dropdown
     *   - 每个 GridItem 按 X 键触发设置面板（暂时 log 占位）
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

        GameLibraryPage();
        ~GameLibraryPage() = default;

        /// 游戏被激活（启动）时触发
        std::function<void(const beiklive::GameEntry&)> onGameSelected;

    private:
        beiklive::GridBox*    m_grid      = nullptr;
        std::vector<beiklive::GameEntry> m_entries;
        SortMode              m_sortMode  = SortMode::ByLastPlayed;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        void _loadAndShowEntries();
        void _sortEntries();
        void _rebuildGrid();
        void _reloadEntries();
        void _showSortDropdown();
        void _updateHeader();

        /// 显示游戏选项侧边栏
        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        /// 关闭游戏选项侧边栏
        void _hideGameOptionsPanel();

        /// 将 GameEntry 的平台字段转换为徽标颜色枚举
        static PlatformBadgeColor _platformBadge(int platform);

        /// 将游戏时长（秒）格式化为可读字符串
        static std::string _formatPlayTime(int seconds);

        int _currentFocusedIndex = -1; // 当前焦点所在的游戏索引（-1 表示无焦点）
    };

} // namespace beiklive
