#pragma once

#include <borealis.hpp>
#include <vector>
#include <map>
#include <functional>
#include <string>

#include "RecyclingGridItem.hpp"
#include "RecyclingGridDataSource.hpp"

class RecyclingGridContentBox : public brls::Box {
public:
    RecyclingGridContentBox(class RecyclingGrid* recycler);
    brls::View* getNextFocus(brls::FocusDirection direction, brls::View* currentView) override;

private:
    RecyclingGrid* m_recycler;
};

class RecyclingGrid : public brls::ScrollingFrame {
public:
    RecyclingGrid();
    ~RecyclingGrid() override;

    void registerCell(const std::string& identifier, std::function<RecyclingGridItem*()> allocation);

    RecyclingGridItem* dequeueReusableCell(const std::string& identifier);

    void setDataSource(RecyclingGridDataSource* source);
    RecyclingGridDataSource* getDataSource() const;

    void reloadData();
    void notifyDataChanged();
    void clearData();

    RecyclingGridItem* getGridItemByIndex(size_t index);
    size_t getItemCount() const;

    void setDefaultCellFocus(size_t index);
    size_t getDefaultCellFocus() const;

    brls::View* getNextCellFocus(brls::FocusDirection direction, brls::View* currentView);

    void onNextPage(const std::function<void()>& callback = nullptr);

    void setPaddingTop(float top);
    void setPaddingBottom(float bottom);
    void setPaddingLeft(float left);

    void setFocusChangeCallback(std::function<void(size_t)> callback);

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void onLayout() override;
    void onChildFocusGained(View* directChild, View* focusedView) override;
    brls::View* getDefaultFocus() override;

    int spanCount = 3;
    float estimatedRowHeight = 120;
    float estimatedRowSpace = 8;

    static brls::View* create();

private:
    RecyclingGridDataSource* m_dataSource = nullptr;
    bool m_layouted = false;
    float m_oldWidth = -1;
    bool m_requestNextPage = false;

    size_t visibleMin = 0, visibleMax = 0;
    size_t m_defaultCellFocus = 0;

    float m_paddingTop = 0, m_paddingRight = 0, m_paddingBottom = 0, m_paddingLeft = 0;

    std::function<void()> m_nextPageCallback;
    std::function<void(size_t)> m_focusChangeCallback;

    RecyclingGridContentBox* m_contentBox = nullptr;
    brls::Rect m_renderedFrame;
    std::vector<float> m_cellHeightCache;

    std::map<std::string, std::vector<RecyclingGridItem*>*> m_queueMap;
    std::map<std::string, std::function<RecyclingGridItem*()>> m_allocationMap;

    bool checkWidth();

    void queueReusableCell(RecyclingGridItem* cell);
    void itemsRecyclingLoop();
    void addCellAt(size_t index, bool downSide);
    void removeCell(brls::View* view);

    size_t getRowCount() const;
    float getHeightByCellIndex(size_t index, size_t start = 0) const;
};
