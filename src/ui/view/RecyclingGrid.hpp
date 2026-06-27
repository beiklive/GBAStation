#pragma once

#include <borealis.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <chrono>

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

    void setDataSource(GameGridDataSource* source);
    GameGridDataSource* getDataSource() const { return m_dataSource; }

    void reloadData();
    void notifyDataChanged();
    void clearData();

    void setDefaultCellFocus(size_t index);
    size_t getDefaultCellFocus() const { return m_defaultCellFocus; }

    int getSelectedIndex() const { return m_selectedIndex; }

    void onNextPage(std::function<void()> callback) { m_nextPageCallback = std::move(callback); }
    void setFocusChangeCallback(std::function<void(int)> callback) { m_focusChangeCallback = std::move(callback); }
    void setInteractionDisabled(bool disabled) { m_interactionDisabled = disabled; }
    void setTitleFontSize(int opt);

    void setMultiSelectMode(bool on);
    bool isMultiSelectMode() const { return m_multiSelectMode; }
    void toggleDeleteSelection(size_t index);
    void selectAllForDelete(size_t count);
    const std::unordered_set<int>& getDeleteSelection() const { return m_selectedForDelete; }
    void clearDeleteSelection();
    void setItemFavourite(size_t index, bool fav);
    void setItemTitle(size_t index, const std::string& title);
    void setItemImagePath(size_t index, const std::string& path);

    void setPadding(float top, float right, float bottom, float left);

    int spanCount = 3;
    float estimatedRowHeight = 120.f;
    float estimatedRowSpace = 8.f;

private:
    GameGridDataSource* m_dataSource = nullptr;

    std::vector<GridDrawItem> m_items;

    int m_selectedIndex = 0;
    uint64_t m_selectedGameId = 0;
    size_t m_defaultCellFocus = 0;

    float m_scrollY = 0.f;
    float m_targetScrollY = 0.f;
    float m_maxScrollY = 0.f;

    int m_visibleStartRow = 0;
    int m_visibleEndRow = 0;

    float m_paddingTop = 0.f;
    float m_paddingRight = 0.f;
    float m_paddingLeft = 0.f;

    bool m_focusMoved = false;
    bool m_isLayouted = false;
    bool m_requestNextPage = false;
    bool m_interactionDisabled = false;
    bool m_multiSelectMode = false;
    bool m_wasFocused = false;

    float m_shakeTime = 0.f;
    float m_shakeDir = 0.f;
    float m_focusBorderAnimTime = 0.f;

    std::function<void()> m_nextPageCallback;
    std::function<void(int)> m_focusChangeCallback;

    std::unordered_map<std::string, int> m_textureCache;
    std::unordered_set<int> m_selectedForDelete;

    int m_fontId = -1;
    int m_titleFontSize = 16;
    int m_favIconHandle = -1;

    std::chrono::steady_clock::time_point m_lastFrameTime;

    bool m_prevUp = false;
    bool m_prevDown = false;
    bool m_prevLeft = false;
    bool m_prevRight = false;
    bool m_prevA = false;
    bool m_prevStickUp = false;
    bool m_prevStickDown = false;
    bool m_prevStickLeft = false;
    bool m_prevStickRight = false;
    float m_holdUpTime = 0.f;
    float m_holdDownTime = 0.f;
    float m_holdLeftTime = 0.f;
    float m_holdRightTime = 0.f;
    float m_holdUpRepeat = 0.f;
    float m_holdDownRepeat = 0.f;
    float m_holdLeftRepeat = 0.f;
    float m_holdRightRepeat = 0.f;

    static constexpr float HOLD_INITIAL_DELAY = 0.3f;
    static constexpr float HOLD_REPEAT = 0.08f;
    static constexpr float HOLD_REPEAT_FAST = 0.03f;
    static constexpr float HOLD_ACCEL_TIME = 1.5f;

    void _updateVisibleRange();
    void _updateFocusAnimation(float delta);
    void _updateMarquee(float delta);
    void _updateScrollPhysics(float delta);
    void _ensureSelectedVisible();
    void _loadTextures(NVGcontext* vg);
    void _evictTextures();
    void _handleInput(float dt);

    void _moveUp();
    void _moveDown();
    void _moveLeft();
    void _moveRight();
    bool _tryMoveUp();
    bool _tryMoveDown();
    bool _tryMoveLeft();
    bool _tryMoveRight();
    void _captureInputState();

    void _drawItem(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, bool focused, int idx);
    void _drawImage(NVGcontext* vg, const GridDrawItem& item, float x, float y, float imageSize);
    void _drawBadge(NVGcontext* vg, const GridDrawItem& item, float x, float y);
    void _drawTitle(NVGcontext* vg, const GridDrawItem& item, float x, float y, float maxWidth, bool focused);
    void _drawSubText(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth);
    void _drawPlayTime(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth);
    void _drawEmptyItem(NVGcontext* vg, float x, float y, float w, float h);
    void _drawScrollbar(NVGcontext* vg, float x, float y, float w, float h);
    void _drawFavourite(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, float sx, float sy);

    float _getItemX(int col);
    float _getItemY(int row);
    float _getItemWidth();
    float _getRowHeight();
    int _getRowCount();

    NVGcolor _getBadgeColor(PlatformBadgeColor color) const;
};
