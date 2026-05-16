#pragma once

#include <borealis.hpp>
#include <vector>
#include <map>
#include <functional>
#include <string>

#include "RecyclingGridItem.hpp"
#include "RecyclingGridDataSource.hpp"

namespace beiklive {

class RecyclingGrid : public brls::ScrollingFrame {
public:
    RecyclingGrid(int spanCount = 3, float itemHeight = 120.0f, float itemSpace = 10.0f);

    void registerCell(const std::string& identifier, std::function<RecyclingGridItem*()> factory);

    RecyclingGridItem* dequeueReusableCell(const std::string& identifier);

    void setDataSource(RecyclingGridDataSource* source);
    void reloadData();
    void notifyDataChanged();

    RecyclingGridItem* getGridItemByIndex(size_t index);

    void setSpanCount(int count);
    int getSpanCount() const { return m_spanCount; }

    void setItemHeight(float height);
    float getItemHeight() const { return m_itemHeight; }

    void setItemSpace(float space);
    float getItemSpace() const { return m_itemSpace; }

    std::function<void()> onNextPage;

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

    void onFocusGained() override;
    void onFocusLost() override;

private:
    void itemsRecyclingLoop();
    void addCellAt(size_t startIndex, bool below);
    void recycleOutOfRangeCells();
    void setupNavigation();
    int getRowIndex(size_t itemIndex) const;
    int getItemCount() const;

    size_t getCellStartIndex() const;
    size_t getCellEndIndex() const;
    float getContentHeightForRows(int rows) const;
    float getYForItem(size_t index) const;

    int m_spanCount;
    float m_itemHeight;
    float m_itemSpace;
    float m_estimatedRowHeight;

    RecyclingGridDataSource* m_dataSource = nullptr;

    std::map<std::string, std::function<RecyclingGridItem*()>> m_allocationMap;
    std::map<std::string, std::vector<RecyclingGridItem*>*> m_queueMap;

    std::vector<RecyclingGridItem*> m_attachedItems;

    brls::Box* m_contentBox = nullptr;

    size_t m_visibleMin = 0;
    size_t m_visibleMax = 0;
    size_t m_itemCount = 0;
    bool m_requestNextPage = false;
    int m_preFetchLine = 1;

    bool m_focusOnReload = false;
};

} // namespace beiklive
