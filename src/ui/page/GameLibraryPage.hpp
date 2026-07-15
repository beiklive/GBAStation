#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
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
    THREEDS = (int)beiklive::enums::EmuPlatform::Emu3DS,
    FAVORITE = 999,
};

        enum class SortMode : int
        {
            LAST_PLAYED = 0,
            PLAY_TIME,
            FIRST_LETTER,
        };

        struct PreparedData
        {
            std::vector<beiklive::GameEntry> entries;
            std::vector<PlatformFilter> filters;
            bool ready = false;
        };

        GameLibraryPage();
        explicit GameLibraryPage(PreparedData preparedData);
        ~GameLibraryPage();

        static PreparedData prepareInitialData();

        void willAppear(bool resetState) override;
        void resetLaunchOverlay();

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

        GameGridView* m_libraryView = nullptr;
        std::vector<beiklive::GameEntry> m_entries;
        GameLibraryDS* m_dataSource = nullptr;
        int m_visibleCount = 0;
        PlatformFilter m_platformFilter = PlatformFilter::ALL;
        std::vector<PlatformFilter> m_availableFilters{PlatformFilter::ALL};
        std::unordered_map<int, int> m_filterFocusIndices;
        int m_platformAnimationDirection = 0;
        SortMode m_sortMode = SortMode::LAST_PLAYED;
        std::string m_searchTerm;
        bool m_isSearching = false;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        void _loadAndShowEntries();
        void _filterEntries();
        void _reloadEntries(uint64_t requestGeneration = 0,
                            bool useFastSnapshot = false);
        void _presentReloadedEntries(
            uint64_t requestGeneration,
            std::vector<beiklive::GameEntry> entries,
            std::vector<PlatformFilter> filters,
            PlatformFilter resolvedFilter,
            bool favoriteFallback,
            bool isSearching,
            const std::string& searchTerm);
        void _schedulePlatformReload();
        void _showFilterDropdown();
        void _showSortSelector();
        void _updateHeader();
        void _rebuildAvailableFilters(const std::vector<beiklive::GameEntry>& entries);
        static std::vector<PlatformFilter> _buildAvailableFilters(
            const std::vector<beiklive::GameEntry>& entries);
        static void _filterAndSortEntries(std::vector<beiklive::GameEntry>& entries,
                                          PlatformFilter platformFilter,
                                          SortMode sortMode,
                                          bool isSearching,
                                          const std::string& searchTerm);
        void _cyclePlatformFilter(int direction);
        int _savedFocusIndex() const;

        static std::string _titleToSortKey(const std::string& title);
        static std::string _formatPlayTime(int seconds);

        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        void _hideGameOptionsPanel();
        void _closeGameOptionsPanelAnimated(std::function<void()> completion,
                                            bool launchTransition = false);
        void _showMultiSelectSidebar();
        void _deleteEntriesAsync(std::vector<int> indices, bool deleteRomFiles);
        void _openGameDataPage(const beiklive::GameEntry& entry);

        int _currentFocusedIndex = -1;
        bool m_firstAppear = true;
        bool m_isClosing = false;
        bool m_hasPlatformReloadDelay = false;
        size_t m_platformReloadDelayId = 0;
        std::shared_ptr<std::atomic<bool>> m_aliveToken =
            std::make_shared<std::atomic<bool>>(true);
        std::atomic<uint64_t> m_reloadGeneration{0};
        PreparedData m_deferredPreparedData;
        bool m_hasPresentedInitialData = false;
    };

} // namespace beiklive
