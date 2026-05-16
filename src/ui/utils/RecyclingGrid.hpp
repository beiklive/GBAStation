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
    struct CellItem {
        RecyclingGridItem* cell = nullptr;
        int row = 0;
        int col = 0;
    };

    void itemsRecyclingLoop();
    void addCellsForRow(int row);
    void recycleOutOfRangeRows();
    void setupNavigation();
    int getRowIndex(size_t itemIndex) const;
    int getItemCount() const;

    size_t getCellStartIndex();
    size_t getCellEndIndex();
    float getContentHeightForRows(int rows) const;

    int m_spanCount;
    float m_itemHeight;
    float m_itemSpace;
    float m_estimatedRowHeight;

    RecyclingGridDataSource* m_dataSource = nullptr;

    std::map<std::string, std::function<RecyclingGridItem*()>> m_allocationMap;
    std::map<std::string, std::vector<RecyclingGridItem*>*> m_queueMap;

    std::map<int, std::vector<CellItem>> m_attachedRows;

    brls::Box* m_contentBox = nullptr;

    int m_visibleMinRow = 0;
    int m_visibleMaxRow = 0;
    size_t m_itemCount = 0;
    bool m_requestNextPage = false;
    int m_preFetchLine = 1;

    bool m_focusOnReload = false;
};

} // namespace beiklive
