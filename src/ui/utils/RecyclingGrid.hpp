#pragma once

#include <borealis.hpp>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <unordered_map>

#include "RecyclingGridItem.hpp"
#include "RecyclingGridDataSource.hpp"

class GameGridView : public brls::View {
public:
    GameGridView();
    ~GameGridView() override;

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void frame(brls::FrameContext* ctx) override;
    void onLayout() override;
    void willAppear(bool resetState = false) override;

    void setDataSource(GameGridDataSource* source);
    GameGridDataSource* getDataSource() const { return m_dataSource; }

    void reloadData();
    void notifyDataChanged();
    void clearData();

    void setDefaultCellFocus(size_t index);
    size_t getDefaultCellFocus() const { return m_defaultCellFocus; }

    const GridDrawItem* getGridItemByIndex(size_t index) const;
    size_t getItemCount() const;
    int getSelectedIndex() const { return m_selectedIndex; }

    void onNextPage(std::function<void()> callback) { m_nextPageCallback = std::move(callback); }
    void setFocusChangeCallback(std::function<void(int)> callback) { m_focusChangeCallback = std::move(callback); }

    void setPadding(float top, float right, float bottom, float left);

    int spanCount = 3;
    float estimatedRowHeight = 120.f;
    float estimatedRowSpace = 8.f;

    static brls::View* create();

private:
    GameGridDataSource* m_dataSource = nullptr;

    std::vector<GridDrawItem> m_items;
    std::vector<int> m_visibleIndices;

    int m_selectedIndex = 0;
    uint64_t m_selectedGameId = 0;
    size_t m_defaultCellFocus = 0;

    float m_scrollY = 0.f;
    float m_targetScrollY = 0.f;
    float m_velocityY = 0.f;
    float m_maxScrollY = 0.f;

    int m_visibleStartRow = 0;
    int m_visibleEndRow = 0;

    float m_paddingTop = 0.f;
    float m_paddingRight = 0.f;
    float m_paddingBottom = 0.f;
    float m_paddingLeft = 0.f;

    bool m_focusMoved = false;
    bool m_isLayouted = false;
    bool m_requestNextPage = false;

    std::function<void()> m_nextPageCallback;
    std::function<void(int)> m_focusChangeCallback;

    std::unordered_map<std::string, int> m_textureCache;

    void _updateVisibleRange();
    void _updateFocusAnimation(float delta);
    void _updateMarquee(float delta);
    void _updateScrollPhysics(float delta);
    void _ensureSelectedVisible();
    void _loadTextures(NVGcontext* vg);
    void _evictTextures();

    void _drawItem(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, bool focused);
    void _drawImage(NVGcontext* vg, const GridDrawItem& item, float x, float y, float imageSize);
    void _drawBadge(NVGcontext* vg, const GridDrawItem& item, float x, float y);
    void _drawTitle(NVGcontext* vg, const GridDrawItem& item, float x, float y, float maxWidth, bool focused);
    void _drawSubText(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth);
    void _drawPlayTime(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth);
    void _drawEmptyItem(NVGcontext* vg, float x, float y, float w, float h);
    void _drawScrollbar(NVGcontext* vg, float x, float y, float w, float h);

    float _getItemX(int col);
    float _getItemY(int row);
    float _getItemWidth();
    float _getRowHeight();
    int _getRowCount();
    void _navigateFocus(brls::FocusDirection direction);

    NVGcolor _getBadgeColor(PlatformBadgeColor color) const;

    uint64_t m_lastFrameTime = 0;
    int m_fontId = -1;
};
