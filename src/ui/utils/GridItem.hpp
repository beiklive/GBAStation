#pragma once

#include <borealis.hpp>
#include <string>

#include "core/common.h"
#include "RecyclingGridItem.hpp"

namespace beiklive {

enum class GridItemMode {
    GAME_LIBRARY,
    SAVE_STATE,
};

class GridItem : public brls::Box {
public:
    static constexpr float ITEM_HEIGHT = 120.f;

    explicit GridItem(GridItemMode mode, int index = 0);
    ~GridItem() = default;

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

    void setImagePath(const std::string& path);
    void setBadge(const std::string& text, PlatformBadgeColor color);
    void setTitle(const std::string& title);
    void setSubText(const std::string& text);
    void setPlayTime(const std::string& text);
    void setSubTextEmpty();
    void setEmpty(const std::string& slotName);
    void setDataLoaded();

    int getIndex() const { return m_index; }
    GridItemMode getMode() const { return m_mode; }
    bool isEmpty() const { return m_isEmpty; }

    static void populateFromGameEntry(GridDrawItem& item, const GameEntry& entry,
                                       GridItemMode mode = GridItemMode::GAME_LIBRARY);
    static void populateEmpty(GridDrawItem& item, const std::string& slotName = "空");
    static std::string formatPlayTime(int seconds);
    static std::string formatSubText(const GameEntry& entry, GridItemMode mode);

private:
    GridItemMode m_mode;
    int m_index;
    bool m_isEmpty = true;

    brls::Label* m_emptyLabel = nullptr;
    brls::Box* m_dataLayout = nullptr;
    brls::Image* m_image = nullptr;
    brls::Box* m_rightBox = nullptr;
    brls::Box* m_row1 = nullptr;
    brls::Box* m_badgeBox = nullptr;
    brls::Label* m_badgeLabel = nullptr;
    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_subLabel = nullptr;
    brls::Label* m_playLabel = nullptr;

    void _initLayout();
    static NVGcolor _getBadgeColor(PlatformBadgeColor color);
};

} // namespace beiklive
