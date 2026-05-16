#pragma once

#include <atomic>
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

        std::function<void(const beiklive::GameEntry&)> onGameSelected;

    private:
        static constexpr int PAGE_SIZE = 21;

        beiklive::GridBox* m_grid = nullptr;
        std::vector<beiklive::GameEntry> m_entries;
        int                   m_visibleCount = 0;
        bool                  m_loadingMore  = false;
        PlatformFilter        m_platformFilter = PlatformFilter::ALL;
        std::string           m_searchTerm;
        bool                  m_isSearching = false;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        std::vector<beiklive::GridItem*> m_itemPool;

        void _loadAndShowEntries();
        void _filterEntries();
        void _rebuildGrid();
        void _loadNextPage();
        void _reloadEntries();
        void _showFilterDropdown();
        void _recycleVisibleItems();
        void _freeItemPool();
        void _updateHeader();

        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        void _hideGameOptionsPanel();

        static PlatformBadgeColor _platformBadge(int platform);
        static std::string _formatPlayTime(int seconds);
        static GridItemData _buildItemData(const beiklive::GameEntry& entry);

        int _currentFocusedIndex = -1;
        std::atomic<bool> m_alive{true};
    };

} // namespace beiklive
