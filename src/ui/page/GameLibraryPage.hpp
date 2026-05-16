#pragma once

#include "core/common.h"
#include "core/Tools.hpp"
#include "ui/utils/Box.hpp"
#include "ui/utils/GridBox.hpp"
#include "ui/utils/GridItem.hpp"
#include "ui/utils/GameOptionsSidebar.hpp"

namespace beiklive
{
    struct GridItemData {
        std::string logoPath;
        std::string badgeText;
        PlatformBadgeColor badgeColor = PlatformBadgeColor::NONE;
        std::string logoLayerPath;
        bool showLogoLayer = false;
        std::string title;
        std::string subText;
        std::string playTime;
    };

    class GameLibraryPage : public beiklive::Box
    {
    public:
        /// 平台分类
        enum class PlatformFilter : int
        {
            ALL = 0,
            GBA = (int)beiklive::enums::EmuPlatform::EmuGBA,
            GBC = (int)beiklive::enums::EmuPlatform::EmuGBC,
            GB  = (int)beiklive::enums::EmuPlatform::EmuGB,
        };

        GameLibraryPage();
        ~GameLibraryPage();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        /// 游戏被激活（启动）时触发
        std::function<void(const beiklive::GameEntry&)> onGameSelected;

    private:
        static constexpr int PAGE_SIZE = 21;

        beiklive::GridBox*    m_grid      = nullptr;
        std::vector<beiklive::GameEntry> m_entries;
        int                   m_visibleCount = 0;
        bool                  m_loadingMore  = false;
        PlatformFilter        m_platformFilter = PlatformFilter::ALL;
        std::string           m_searchTerm;
        bool                  m_isSearching = false;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        void _loadAndShowEntries();
        void _filterEntries();
        void _rebuildGrid();
        void _loadNextPage();
        void _reloadEntries();
        void _showFilterDropdown();
        void _updateHeader();

        /// 显示游戏选项侧边栏
        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        /// 关闭游戏选项侧边栏
        void _hideGameOptionsPanel();

        /// 将 GameEntry 的平台字段转换为徽标颜色枚举
        static PlatformBadgeColor _platformBadge(int platform);

        /// 将游戏时长（秒）格式化为可读字符串
        static std::string _formatPlayTime(int seconds);

        /// 从 GameEntry 构建 GridItemData（纯数据，可在后台线程调用）
        static GridItemData _buildItemData(const beiklive::GameEntry& entry);

        int _currentFocusedIndex = -1; // 当前焦点所在的游戏索引（-1 表示无焦点）
        std::atomic<bool> m_alive{true};
    };

} // namespace beiklive
