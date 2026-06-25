#pragma once

#include <atomic>
#include "core/common.h"
#include "core/Tools.hpp"
#include "ui/widget/Box.hpp"
#include "ui/view/RecyclingGrid.hpp"
#include "ui/view/RecyclingGridDataSource.hpp"
#include "ui/view/GameOptionsSidebar.hpp"

namespace beiklive
{
    class GameLibraryPage : public beiklive::Box
    {
    public:
enum class PlatformFilter : int
{
    ALL = 0,
    GBA = (int)beiklive::enums::EmuPlatform::EmuGBA,
    GBC = (int)beiklive::enums::EmuPlatform::EmuGBC,
    GB  = (int)beiklive::enums::EmuPlatform::EmuGB,
    NES = (int)beiklive::enums::EmuPlatform::EmuNES,
    SNES = (int)beiklive::enums::EmuPlatform::EmuSNES,
    NDS = (int)beiklive::enums::EmuPlatform::EmuNDS,
    FAVORITE = 999,
};

        enum class SortMode : int
        {
            LAST_PLAYED = 0,
            PLAY_TIME,
            FIRST_LETTER,
        };

        GameLibraryPage();
        ~GameLibraryPage();

        void willAppear(bool resetState) override;

        std::function<void(const beiklive::GameEntry&)> onGameSelected;

    private:
        class GameLibraryDS : public GameGridDataSource {
        public:
            GameLibraryDS(class GameLibraryPage* page) : m_page(page) {}
            size_t getItemCount() override;
            void populateItem(GridDrawItem& item, size_t index) override;
            void onItemSelected(size_t index) override;
            void clearData() override;
        private:
            GameLibraryPage* m_page;
        };

        static constexpr int PAGE_SIZE = 21;

        GameGridView* m_grid = nullptr;
        std::vector<beiklive::GameEntry> m_entries;
        GameLibraryDS* m_dataSource = nullptr;
        int m_visibleCount = 0;
        bool m_loadingMore = false;
        PlatformFilter m_platformFilter = PlatformFilter::ALL;
        SortMode m_sortMode = SortMode::LAST_PLAYED;
        std::string m_searchTerm;
        bool m_isSearching = false;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        void _loadAndShowEntries();
        void _filterEntries();
        void _loadNextPage();
        void _reloadEntries();
        void _showFilterDropdown();
        void _showSortSelector();
        void _updateHeader();

        static std::string _titleToSortKey(const std::string& title);
        static std::string _formatPlayTime(int seconds);

        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        void _hideGameOptionsPanel();
        void _showMultiSelectSidebar();

        int _currentFocusedIndex = -1;
        bool m_firstAppear = true;
        std::atomic<bool> m_alive{true};
    };

} // namespace beiklive
