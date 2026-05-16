#pragma once

#include <borealis.hpp>
#include <functional>
#include <string>

#include "core/common.h"
#include "RecyclingGridItem.hpp"

namespace beiklive
{

    enum class GridItemMode
    {
        GAME_LIBRARY,
        SAVE_STATE,
    };

    enum class PlatformBadgeColor
    {
        GBA,
        GBC,
        GB,
        NONE,
    };

    class GridItem : public RecyclingGridItem
    {
    public:
        static constexpr float ITEM_HEIGHT = 120.f;

        explicit GridItem(GridItemMode mode, int index = 0);
        ~GridItem() = default;

        void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

        void prepareForReuse() override;

        void setImageLayer(const std::string& path, bool visible);
        void setImageLayerDeferred(const std::string& path, bool visible);

        void setImagePath(const std::string& path);
        void setImagePathDeferred(const std::string& path);

        void setBadge(const std::string& text, PlatformBadgeColor color);

        void setTitle(const std::string& title);
        void setSubText(const std::string& text);
        void setPlayTime(const std::string& text);

        void setEmpty(const std::string& slotName);
        void setDataLoaded();

        int           getIndex() const { return m_index; }
        GridItemMode  getMode()  const { return m_mode;  }
        bool          isEmpty()  const { return m_isEmpty; }

        static void cancelDeferredLoads();

        std::function<void(int index)> onItemClicked;
        std::function<void(int index)> onItemFocused;

        std::function<bool(int index)> isFavourite;
        std::function<void(int index)> toggleFavourite;

    private:
        GridItemMode m_mode;
        int          m_index;
        bool         m_isEmpty = true;
        bool         m_showImageLayer = false;
        brls::Label* m_emptyLabel = nullptr;

        brls::Box*   m_dataLayout  = nullptr;
        brls::Image* m_image       = nullptr;
        brls::Image* m_imageLayer  = nullptr;
        brls::Box*   m_rightBox    = nullptr;

        brls::Box*   m_row1        = nullptr;
        brls::Box*   m_badgeBox    = nullptr;
        brls::Label* m_badgeLabel  = nullptr;
        brls::Label* m_titleLabel  = nullptr;

        brls::Label* m_subLabel    = nullptr;
        brls::Label* m_playLabel   = nullptr;

        void _initLayout();

        static NVGcolor _getBadgeColor(PlatformBadgeColor color);

        void onFocusGained() override;
        void onFocusLost()   override;

        void _updateFavouriteHint();
    };

} // namespace beiklive
