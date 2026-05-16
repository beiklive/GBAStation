#pragma once

#include <atomic>
#include "core/common.h"
#include "core/Tools.hpp"
#include "ui/utils/Box.hpp"
#include "ui/utils/RecyclingGrid.hpp"
#include "ui/utils/RecyclingGridDataSource.hpp"
#include "ui/utils/GameOptionsSidebar.hpp"

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
        };

        GameLibraryPage();
        ~GameLibraryPage();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        std::function<void(const beiklive::GameEntry&)> onGameSelected;

    private:
        class LibGridCell : public RecyclingGridItem {
        public:
            LibGridCell();
            void prepareForReuse() override;
            void setEntry(const beiklive::GameEntry& entry);
            void setLogoPath(const std::string& path);
            void setGameTitle(const std::string& title);
            beiklive::GameEntry m_entry;
        private:
            brls::Image* m_cover = nullptr;
            brls::Label* m_title = nullptr;
        };

        class GameLibraryDS : public RecyclingGridDataSource {
        public:
            GameLibraryDS(GameLibraryPage* page) : m_page(page) {}
            size_t getItemCount() const override;
            RecyclingGridItem* cellForRow(RecyclingGrid* grid, size_t index) override;
            float heightForRow(RecyclingGrid* grid, size_t index) override { return 260.0f; }
        private:
            GameLibraryPage* m_page;
        };

        static constexpr int PAGE_SIZE = 21;

        beiklive::RecyclingGrid* m_grid = nullptr;
        std::vector<beiklive::GameEntry> m_entries;
        GameLibraryDS* m_dataSource = nullptr;
        int                   m_visibleCount = 0;
        bool                  m_loadingMore  = false;
        PlatformFilter        m_platformFilter = PlatformFilter::ALL;
        std::string           m_searchTerm;
        bool                  m_isSearching = false;
        beiklive::GameOptionsSidebar* m_gameOptionsSidebar = nullptr;

        void _loadAndShowEntries();
        void _filterEntries();
        void _loadNextPage();
        void _reloadEntries();
        void _showFilterDropdown();
        void _updateHeader();

        void _showGameOptionsPanel(const beiklive::GameEntry& entry);
        void _hideGameOptionsPanel();

        int _currentFocusedIndex = -1;
        std::atomic<bool> m_alive{true};
    };

} // namespace beiklive
