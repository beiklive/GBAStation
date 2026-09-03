#include "RecyclingGrid.hpp"
#include "core/Translation.hpp"
#include "core/common.h"
#include "core/ThreadPool.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"

#include <borealis/core/font.hpp>
#include <borealis/extern/nanovg/stb_image.h>
#include <borealis/views/hint.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    std::string encodeUtf8(char32_t codepoint)
    {
        std::string out;
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return out;
    }

    std::vector<unsigned char> resizeRgba(const unsigned char* source,
                                          int sourceWidth, int sourceHeight,
                                          int targetWidth, int targetHeight)
    {
        std::vector<unsigned char> output(
            static_cast<size_t>(targetWidth) * static_cast<size_t>(targetHeight) * 4);
        for (int y = 0; y < targetHeight; ++y) {
            const int sy = std::min(sourceHeight - 1, y * sourceHeight / targetHeight);
            for (int x = 0; x < targetWidth; ++x) {
                const int sx = std::min(sourceWidth - 1, x * sourceWidth / targetWidth);
                const size_t src = (static_cast<size_t>(sy) * sourceWidth + sx) * 4;
                const size_t dst = (static_cast<size_t>(y) * targetWidth + x) * 4;
                output[dst + 0] = source[src + 0];
                output[dst + 1] = source[src + 1];
                output[dst + 2] = source[src + 2];
                output[dst + 3] = source[src + 3];
            }
        }
        return output;
    }

    float easeOutBack(float value)
    {
        value = std::max(0.f, std::min(1.f, value));
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.f;
        const float t = value - 1.f;
        return 1.f + c3 * t * t * t + c1 * t * t;
    }

    void strokeRoundedRectInside(NVGcontext* vg, float x, float y,
                                 float w, float h, float radius,
                                 float strokeWidth, NVGcolor color)
    {
        const float inset = strokeWidth * 0.5f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + inset, y + inset,
                       std::max(0.f, w - strokeWidth),
                       std::max(0.f, h - strokeWidth),
                       std::max(0.f, radius - inset));
        nvgStrokeColor(vg, color);
        nvgStrokeWidth(vg, strokeWidth);
        nvgStroke(vg);
    }
}

GameGridView::GameGridView()
{
    setFocusable(true);
    setHideHighlightBackground(true);
    setHideHighlightBorder(true);
    setHideClickAnimation(true);
    setBackground(brls::ViewBackground::NONE);
    setClipsToBounds(true);

    m_fontId = brls::Application::getDefaultFont();
    m_materialFontId = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
    m_switchIconFontId = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
    m_lastFrameTime = std::chrono::steady_clock::now();
    m_textureLoader = std::make_shared<TextureLoaderState>();

    m_paddingLeft = 18.f;
    m_paddingRight = 18.f;
    m_paddingTop = 10.f;
    setViewMode(ViewMode::GRID);
}

GameGridView::~GameGridView()
{
    if (m_textureLoader)
        m_textureLoader->alive.store(false);
    delete m_dataSource;
    NVGcontext* vg = brls::Application::getNVGContext();
    if (vg) {
        for (auto& kv : m_textureCache) {
            if (kv.second >= 0) nvgDeleteImage(vg, kv.second);
        }
        if (m_favIconHandle >= 0)
            nvgDeleteImage(vg, m_favIconHandle);
    }
    m_textureCache.clear();
    m_textureLastUsed.clear();
    m_textureMemoryBytes.clear();
    m_textureCacheBytes = 0;
    for (auto& item : m_items) {
        item.textureHandle = -1;
        item.platformTextureHandle = -1;
        item.imageLayerHandle = -1;
    }
}

void GameGridView::setDataSource(GameGridDataSource* source)
{
    if (m_dataSource) delete m_dataSource;
    m_requestNextPage = false;
    m_dataSource = source;
}

void GameGridView::setViewMode(ViewMode mode)
{
    if (m_viewMode == mode && m_isLayouted)
        return;
    const ViewMode previous = m_viewMode;
    m_viewMode = mode;
    if (mode == ViewMode::GRID) {
        spanCount = 5;
        estimatedRowHeight = 260.f;
        estimatedRowSpace = 14.f;
    } else {
        spanCount = 1;
        estimatedRowHeight = 66.f;
        estimatedRowSpace = 6.f;
    }
    m_preferredColumn = m_selectedIndex >= 0 && spanCount > 0
        ? m_selectedIndex % spanCount
        : 0;
    _recalculateScrollBounds();
    _ensureSelectedVisible();
    m_scrollY = m_targetScrollY;
    _updateVisibleRange();
    _populateVisibleItems();
    m_requestedStartRow = -1;
    m_requestedEndRow = -1;
    if (previous != mode)
        startContentTransition(mode == ViewMode::LIST ? 1 : -1, false);
    if (mode == ViewMode::LIST)
        m_detailTransition = 0.f;
}

void GameGridView::toggleViewMode()
{
    setViewMode(m_viewMode == ViewMode::GRID ? ViewMode::LIST : ViewMode::GRID);
}

void GameGridView::setLibraryContext(std::string category, std::string detail)
{
    m_categoryLabel = std::move(category);
    m_detailLabel = std::move(detail);
}

void GameGridView::setPlatformCarousel(std::vector<std::string> labels,
                                       int selected, int direction)
{
    if (labels.empty())
        labels.push_back(L("所有"));
    selected = std::max(0, std::min(selected, static_cast<int>(labels.size()) - 1));

    const bool selectionChanged = labels != m_platformLabels || selected != m_platformIndex;
    if (selectionChanged) {
        if (direction == 0 && labels == m_platformLabels) {
            const int count = static_cast<int>(labels.size());
            const int forward = (selected - m_platformIndex + count) % count;
            const int backward = (m_platformIndex - selected + count) % count;
            direction = forward <= backward ? 1 : -1;
        }
        m_platformSlideDirection = direction < 0 ? -1 : (direction > 0 ? 1 : 0);
        m_platformCarouselDirection = m_platformSlideDirection;
        m_platformTransition = 0.f;
    }
    m_platformLabels = std::move(labels);
    m_platformIndex = selected;
}

void GameGridView::startContentTransition(int direction, bool wholePageSlide)
{
    m_contentSlideDirection = direction < 0 ? -1 : (direction > 0 ? 1 : 0);
    m_contentWholePageSlide = wholePageSlide && m_contentSlideDirection != 0;
    m_contentTransition = 0.f;
}

void GameGridView::restartEntranceAnimation()
{
    m_exitAnimationRunning = false;
    m_pageEntrance = 0.f;
    m_contentTransition = 0.f;
    m_detailTransition = 0.f;
    m_contentSlideDirection = 0;
    m_contentWholePageSlide = false;
    m_lastFrameTime = std::chrono::steady_clock::now();
}

void GameGridView::showLoadingSkeleton()
{
    m_loadingSkeleton = true;
    m_hasPresentedData = false;
    m_selectedIndex = -1;
    m_selectedGameId = 0;
    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
    m_items.assign(m_viewMode == ViewMode::GRID ? 15 : 9, GridDrawItem{});
    _recalculateScrollBounds();
    _updateVisibleRange();
    startContentTransition(0, false);
}

void GameGridView::playLaunchAnimation(size_t index, std::function<void()> completion,
                                       bool startCentered)
{
    if (m_launchAnimationRunning || index >= m_items.size())
        return;
    _populateItem(index);
    m_launchAnimationRunning = true;
    m_launchStartsCentered = startCentered;
    m_launchAnimationTime = 0.f;
    m_launchItemIndex = static_cast<int>(index);
    m_launchCompletion = std::move(completion);
    m_interactionDisabled = true;
}

void GameGridView::resetLaunchAnimation()
{
    m_launchAnimationRunning = false;
    m_launchStartsCentered = false;
    m_launchAnimationTime = 0.f;
    m_launchItemIndex = -1;
    m_launchCompletion = nullptr;
    m_interactionDisabled = false;
}

void GameGridView::playExitAnimation(std::function<void()> completion)
{
    if (m_exitAnimationRunning)
        return;
    m_exitAnimationRunning = true;
    m_interactionDisabled = true;
    m_exitCompletion = std::move(completion);
    m_contentSlideDirection = 0;
    m_pageEntrance = 1.f;
    m_contentTransition = 1.f;
    m_detailTransition = 1.f;
}

void GameGridView::setPadding(float top, float right, float /*bottom*/, float left)
{
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingLeft = left;
}

void GameGridView::setMultiSelectMode(bool on)
{
    m_multiSelectMode = on;
    if (!on) m_selectedForDelete.clear();
}

void GameGridView::toggleDeleteSelection(size_t index)
{
    if (m_selectedForDelete.count(static_cast<int>(index)))
        m_selectedForDelete.erase(static_cast<int>(index));
    else
        m_selectedForDelete.insert(static_cast<int>(index));
}

void GameGridView::selectAllForDelete(size_t count)
{
    m_multiSelectMode = true;
    m_selectedForDelete.clear();
    for (size_t i = 0; i < count; i++)
        m_selectedForDelete.insert(static_cast<int>(i));
}

void GameGridView::deselectAllForDelete()
{
    m_selectedForDelete.clear();
}

bool GameGridView::isAllSelectedForDelete(size_t count) const
{
    return count > 0 && m_selectedForDelete.size() == count;
}

void GameGridView::clearDeleteSelection()
{
    m_selectedForDelete.clear();
    m_multiSelectMode = false;
}

void GameGridView::setItemFavourite(size_t index, bool fav)
{
    if (index < m_items.size())
        m_items[index].favorite = fav;
}

void GameGridView::setItemTitle(size_t index, const std::string& title)
{
    if (index < m_items.size())
        m_items[index].title = title;
}

void GameGridView::setItemImagePath(size_t index, const std::string& path)
{
    if (index < m_items.size()) {
        auto& item = m_items[index];
        item.imagePath = path;
        item.textureHandle = -1;
        item.textureLoading = false;
        item.textureReady = false;
        item.textureFailed = false;
        if (item.platformImageSourcePath.empty()) {
            item.platformImagePath = path;
            item.platformTextureHandle = -1;
            item.platformTextureReady = false;
            item.platformTextureFailed = false;
        }
        m_requestedStartRow = -1;
        m_requestedEndRow = -1;
    }
}

void GameGridView::beginDeleteAnimation(const std::vector<int>& indices)
{
    m_deletingIndices.clear();
    for (int index : indices) {
        if (index >= 0 && static_cast<size_t>(index) < m_items.size())
            m_deletingIndices.insert(index);
    }
    if (m_deletingIndices.empty())
        return;
    m_deleteWaiting = true;
    m_deleteCollapsing = false;
    m_deleteBackendFinished = false;
    m_deleteAnimationTime = 0.f;
    m_deleteCollapseProgress = 0.f;
    m_deleteCompletion = nullptr;
    m_interactionDisabled = true;
}

void GameGridView::completeDeleteAnimation(std::function<void()> completion)
{
    if (m_deletingIndices.empty()) {
        if (completion) completion();
        return;
    }
    m_deleteBackendFinished = true;
    m_deleteCompletion = std::move(completion);
}

void GameGridView::cancelDeleteAnimation()
{
    m_deleteWaiting = false;
    m_deleteCollapsing = false;
    m_deleteBackendFinished = false;
    m_deleteAnimationTime = 0.f;
    m_deleteCollapseProgress = 0.f;
    m_deletingIndices.clear();
    m_deleteCompletion = nullptr;
    m_interactionDisabled = false;
}

void GameGridView::setTitleFontSize(int opt)
{
    switch (opt) {
        case 0: m_titleFontSize = 16; break;
        case 1: m_titleFontSize = 19; break;
        case 2: m_titleFontSize = 22; break;
        default: m_titleFontSize = 16; break;
    }
    for (auto& item : m_items) item.marqueeMaxOffset = 0.f;
}

void GameGridView::setDefaultCellFocus(size_t index)
{
    m_defaultCellFocus = index;
    m_selectedIndex = static_cast<int>(index);
    m_preferredColumn = spanCount > 0 ? m_selectedIndex % spanCount : 0;
}

float GameGridView::_getItemWidth()
{
    float usable = (m_viewMode == ViewMode::LIST ? _getListPaneWidth() : getWidth())
        - m_paddingLeft - m_paddingRight;
    if (usable <= 0.f) return estimatedRowHeight;
    return (usable - estimatedRowSpace * (spanCount - 1)) / spanCount;
}

float GameGridView::_getRowHeight()
{
    return estimatedRowHeight + estimatedRowSpace;
}

float GameGridView::_getItemX(int col)
{
    float itemW = _getItemWidth();
    return m_paddingLeft + col * (itemW + estimatedRowSpace);
}

float GameGridView::_getItemY(int row)
{
    return _getContentTop() + m_paddingTop + row * _getRowHeight() - m_scrollY;
}

int GameGridView::_getRowCount()
{
    if (m_items.empty()) return 0;
    return static_cast<int>((m_items.size() - 1) / spanCount + 1);
}

float GameGridView::_getContentTop() const
{
    return 74.f;
}

float GameGridView::_getFooterHeight() const
{
    return 62.f;
}

float GameGridView::_getViewportHeight()
{
    return std::max(1.f, getHeight() - _getContentTop() - _getFooterHeight() - 6.f);
}

float GameGridView::_getListPaneWidth()
{
    return getWidth() * 0.60f;
}

void GameGridView::_recalculateScrollBounds()
{
    const float totalHeight = m_paddingTop + _getRowCount() * _getRowHeight();
    m_maxScrollY = std::max(0.f, totalHeight - _getViewportHeight());
    m_scrollY = std::max(0.f, std::min(m_scrollY, m_maxScrollY));
    m_targetScrollY = std::max(0.f, std::min(m_targetScrollY, m_maxScrollY));
    _updateVisibleRange();
}

void GameGridView::_populateItem(size_t index)
{
    if (!m_dataSource || index >= m_items.size() || m_items[index].populated)
        return;
    m_dataSource->populateItem(m_items[index], index);
    m_items[index].populated = true;
}

void GameGridView::_populateVisibleItems()
{
    if (!m_dataSource || m_items.empty())
        return;
    const int preloadRows = 1;
    const int start = std::max(0, (m_visibleStartRow - preloadRows) * spanCount);
    const int end = std::min(static_cast<int>(m_items.size()),
        (m_visibleEndRow + preloadRows) * spanCount);
    for (int i = start; i < end; ++i)
        _populateItem(static_cast<size_t>(i));
    if (m_selectedIndex >= 0) {
        _populateItem(static_cast<size_t>(m_selectedIndex));
        if (static_cast<size_t>(m_selectedIndex) < m_items.size())
            m_selectedGameId = m_items[static_cast<size_t>(m_selectedIndex)].gameId;
    }
}

void GameGridView::reloadData()
{
    if (!m_dataSource) return;
    if (!m_isLayouted) m_isLayouted = true;

    const bool applyingReflow = m_reflowPending;
    const int previousSelectedIndex = m_selectedIndex;
    const float previousScrollY = m_scrollY;
    const float previousTargetScrollY = m_targetScrollY;

    m_loadingSkeleton = false;
    size_t count = m_dataSource->getItemCount();
    m_items.clear();
    m_items.resize(count);
    m_hasPresentedData = true;
    m_requestedStartRow = -1;
    m_requestedEndRow = -1;
    m_requestedGameId = 0;
    if (applyingReflow) {
        m_contentTransition = 1.f;
        m_contentWholePageSlide = false;
        m_reflowTransition = 0.f;
        m_reflowPending = false;
    } else {
        startContentTransition(m_platformSlideDirection, m_platformSlideDirection != 0);
    }
    m_platformSlideDirection = 0;
    if (m_viewMode == ViewMode::LIST)
        m_detailTransition = 0.f;

    _recalculateScrollBounds();
    if (applyingReflow) {
        m_scrollY = std::max(0.f, std::min(previousScrollY, m_maxScrollY));
        m_targetScrollY = std::max(
            0.f, std::min(previousTargetScrollY, m_maxScrollY));
    } else {
        m_scrollY = 0.f;
        m_targetScrollY = 0.f;
    }

    if (m_items.empty()) {
        m_selectedIndex = -1;
        m_selectedGameId = 0;
        m_visibleStartRow = 0;
        m_visibleEndRow = 0;
        m_requestNextPage = false;
        return;
    }

    size_t focusIdx = m_defaultCellFocus;
    if (applyingReflow && !m_reflowOrigins.empty()) {
        auto selectedOrigin = std::find(
            m_reflowOrigins.begin(), m_reflowOrigins.end(),
            previousSelectedIndex);
        if (selectedOrigin != m_reflowOrigins.end()) {
            focusIdx = static_cast<size_t>(
                std::distance(m_reflowOrigins.begin(), selectedOrigin));
        } else {
            auto nextOrigin = std::find_if(
                m_reflowOrigins.begin(), m_reflowOrigins.end(),
                [previousSelectedIndex](int origin) {
                    return origin > previousSelectedIndex;
                });
            focusIdx = nextOrigin != m_reflowOrigins.end()
                ? static_cast<size_t>(
                    std::distance(m_reflowOrigins.begin(), nextOrigin))
                : m_reflowOrigins.size() - 1;
        }
    }
    if (focusIdx >= m_items.size()) focusIdx = 0;
    m_selectedIndex = static_cast<int>(focusIdx);
    m_preferredColumn = spanCount > 0 ? m_selectedIndex % spanCount : 0;
    _populateItem(focusIdx);
    m_selectedGameId = m_items[m_selectedIndex].gameId;
    if (m_focusChangeCallback) m_focusChangeCallback(m_selectedIndex);

    _updateVisibleRange();
    if (!applyingReflow) {
        _ensureSelectedVisible();
        m_scrollY = m_targetScrollY;
    }
    _updateVisibleRange();
    _populateVisibleItems();

    m_requestNextPage = false;
}

void GameGridView::notifyDataChanged()
{
    if (!m_dataSource) return;

    size_t oldCount = m_items.size();
    size_t newCount = m_dataSource->getItemCount();
    m_items.resize(newCount);
    for (size_t i = oldCount; i < newCount; i++)
        m_items[i].reset();

    _recalculateScrollBounds();
    _populateVisibleItems();

    m_requestNextPage = false;
    startContentTransition(m_platformSlideDirection, m_platformSlideDirection != 0);
    m_platformSlideDirection = 0;
}

void GameGridView::clearData()
{
    if (m_dataSource) {
        m_dataSource->clearData();
        reloadData();
    }
}

void GameGridView::onLayout()
{
    View::onLayout();
    if (!m_isLayouted && m_dataSource) {
        reloadData();
    } else {
        _recalculateScrollBounds();
        _ensureSelectedVisible();
    }
    m_requestedStartRow = -1;
    m_requestedEndRow = -1;
}

bool GameGridView::_tryMoveUp()
{
    if (m_items.empty()) return false;
    int row = m_selectedIndex / spanCount;
    if (row > 0) {
        const int targetRow = row - 1;
        m_selectedIndex = std::min(
            targetRow * spanCount + m_preferredColumn,
            static_cast<int>(m_items.size()) - 1);
        return true;
    }
    return false;
}

bool GameGridView::_tryMoveDown()
{
    if (m_items.empty()) return false;
    int row = m_selectedIndex / spanCount;
    int maxRow = _getRowCount() - 1;
    if (row < maxRow) {
        const int targetRow = row + 1;
        m_selectedIndex = std::min(
            targetRow * spanCount + m_preferredColumn,
            static_cast<int>(m_items.size()) - 1);
        return true;
    }
    return false;
}

bool GameGridView::_tryMoveLeft()
{
    if (m_items.empty()) return false;
    int col = m_selectedIndex % spanCount;
    if (col > 0) {
        m_selectedIndex -= 1;
        m_preferredColumn = m_selectedIndex % spanCount;
        return true;
    }
    return false;
}

bool GameGridView::_tryMoveRight()
{
    if (m_items.empty()) return false;
    int col = m_selectedIndex % spanCount;
    if (col < spanCount - 1) {
        m_selectedIndex += 1;
        if (m_selectedIndex >= static_cast<int>(m_items.size()))
            m_selectedIndex = static_cast<int>(m_items.size()) - 1;
        m_preferredColumn = m_selectedIndex % spanCount;
        return true;
    }
    return false;
}

void GameGridView::_moveUp()
{
    if (_tryMoveUp()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = -1.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_moveDown()
{
    if (_tryMoveDown()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = 1.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_moveLeft()
{
    if (_tryMoveLeft()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = -2.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_moveRight()
{
    if (_tryMoveRight()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = 2.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

bool GameGridView::_tryMovePageUp()
{
    if (m_viewMode != ViewMode::LIST || m_items.empty())
        return false;
    const int visible = std::max(1, static_cast<int>(std::floor(
        _getViewportHeight() / _getRowHeight())));
    const int step = std::max(1, visible - 1);
    const int next = std::max(0, m_selectedIndex - step);
    if (next == m_selectedIndex)
        return false;
    m_selectedIndex = next;
    m_preferredColumn = 0;
    return true;
}

bool GameGridView::_tryMovePageDown()
{
    if (m_viewMode != ViewMode::LIST || m_items.empty())
        return false;
    const int visible = std::max(1, static_cast<int>(std::floor(
        _getViewportHeight() / _getRowHeight())));
    const int step = std::max(1, visible - 1);
    const int next = std::min(static_cast<int>(m_items.size()) - 1,
                              m_selectedIndex + step);
    if (next == m_selectedIndex)
        return false;
    m_selectedIndex = next;
    m_preferredColumn = 0;
    return true;
}

void GameGridView::_movePageUp()
{
    if (_tryMovePageUp()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = -1.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_movePageDown()
{
    if (_tryMovePageDown()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = 1.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_updateVisibleRange()
{
    if (m_items.empty()) return;

    float rowH = _getRowHeight();
    float viewH = _getViewportHeight();

    m_visibleStartRow = static_cast<int>(std::floor((m_scrollY - m_paddingTop) / rowH));
    m_visibleStartRow = std::max(0, m_visibleStartRow);

    int visibleRows = static_cast<int>(std::ceil(viewH / rowH)) + 1;
    m_visibleEndRow = m_visibleStartRow + visibleRows;
    m_visibleEndRow = std::min(m_visibleEndRow, _getRowCount());
}

void GameGridView::_ensureSelectedVisible()
{
    if (m_items.empty() || m_selectedIndex < 0) return;

    int selRow = m_selectedIndex / spanCount;
    float rowH = _getRowHeight();
    float itemY = m_paddingTop + selRow * rowH;
    float itemCenterY = itemY + estimatedRowHeight * 0.5f;
    float viewH = _getViewportHeight();

    // 焦点到达可滚动区域中线后立即推动内容。首尾通过 clamp 自然停靠，
    // 中段则让焦点稳定在视口中央，网格和列表使用相同规则。
    float target = itemCenterY - viewH * 0.5f;

    target = std::max(0.f, std::min(target, m_maxScrollY));
    m_targetScrollY = target;
}

void GameGridView::_updateScrollPhysics(float delta)
{
    if (m_maxScrollY <= 0.f) {
        m_scrollY = 0.f;
        m_targetScrollY = 0.f;
        return;
    }

    float diff = m_targetScrollY - m_scrollY;
    if (diff > 0.5f)
        m_scrollDirection = 1;
    else if (diff < -0.5f)
        m_scrollDirection = -1;
    if (std::abs(diff) > 0.5f)
        m_scrollY += diff * std::min(1.f, delta * 8.f);
    else
        m_scrollY = m_targetScrollY;

    m_scrollY = std::max(0.f, std::min(m_scrollY, m_maxScrollY));
}

void GameGridView::_updateFocusAnimation(float delta)
{
    const int begin = std::max(0, m_visibleStartRow * spanCount);
    const int end = std::min(static_cast<int>(m_items.size()),
                             m_visibleEndRow * spanCount);
    for (int i = begin; i < end; ++i) {
        auto& item = m_items[static_cast<size_t>(i)];
        bool focused = (i == m_selectedIndex);
        float targetGlow = focused ? 1.f : 0.f;
        item.focusGlow += (targetGlow - item.focusGlow) * std::min(1.f, delta * 12.f);
        item.selected = focused;
    }
}

void GameGridView::_updateMarquee(float delta)
{
    if (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < m_items.size()) {
        auto& item = m_items[m_selectedIndex];
        if (!item.title.empty()) {
            NVGcontext* vg = brls::Application::getNVGContext();
            if (vg) {
                nvgFontSize(vg, m_titleFontSize);
                nvgFontFaceId(vg, m_fontId);
                float bounds[4];
                nvgTextBounds(vg, 0, 0, item.title.c_str(), nullptr, bounds);
                float textW = bounds[2] - bounds[0];
                float textMaxW = m_viewMode == ViewMode::GRID
                    ? std::max(80.f, _getItemWidth() - 24.f)
                    : std::max(100.f, _getItemWidth() - 190.f);
                if (textMaxW < 0.f) textMaxW = 100.f;
                if (textW > textMaxW)
                    item.marqueeMaxOffset = textW - textMaxW;
                else
                    item.marqueeMaxOffset = 0.f;
            }
        }
        if (item.marqueeMaxOffset > 0.f) {
            item.marqueeOffset += delta * 30.f;
            if (item.marqueeOffset > item.marqueeMaxOffset + 24.f)
                item.marqueeOffset = 0.f;
        } else {
            item.marqueeOffset = 0.f;
        }
    }
}

void GameGridView::_requestTexture(const std::string& path,
                                   const std::string& sourcePath,
                                   const std::string& fallbackPath)
{
    if (path.empty() || !m_textureLoader || !m_textureLoader->alive.load())
        return;

    auto state = m_textureLoader;
    uint64_t generation = 0;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        generation = state->generation;
        auto pending = state->pending.find(path);
        if (pending != state->pending.end() && pending->second == generation)
            return;
        if (state->pending.size() + state->ready.size() >= 24)
            return;
        state->pending[path] = generation;
    }

    beiklive::ThreadPool::instance().enqueue([
        state, path, sourcePath, fallbackPath, generation]() {
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (!state->alive.load() || state->generation != generation ||
                state->wanted.count(path) == 0) {
                auto pending = state->pending.find(path);
                if (pending != state->pending.end() && pending->second == generation)
                    state->pending.erase(pending);
                return;
            }
        }

        DecodedTexture decoded;
        decoded.path = path;
        decoded.generation = generation;

        int width = 0;
        int height = 0;
        int channels = 0;
        std::string loadPath = path;
        if (!sourcePath.empty()) {
            const std::string extracted = beiklive::GetOrCreateNdsIconPath(sourcePath);
            if (!extracted.empty())
                loadPath = extracted;
            else if (!fallbackPath.empty())
                loadPath = fallbackPath;
        }
        unsigned char* pixels = stbi_load(
            loadPath.c_str(), &width, &height, &channels, 4);
        if (!pixels || width <= 0 || height <= 0) {
            decoded.failed = true;
        } else {
#ifdef __SWITCH__
            constexpr int maxEdge = 384;
#else
            constexpr int maxEdge = 512;
#endif
            const int longest = std::max(width, height);
            if (longest > maxEdge) {
                const float scale = static_cast<float>(maxEdge) / static_cast<float>(longest);
                decoded.width = std::max(1, static_cast<int>(std::round(width * scale)));
                decoded.height = std::max(1, static_cast<int>(std::round(height * scale)));
                decoded.pixels = resizeRgba(pixels, width, height,
                                            decoded.width, decoded.height);
            } else {
                decoded.width = width;
                decoded.height = height;
                decoded.pixels.assign(
                    pixels,
                    pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
            }
            stbi_image_free(pixels);
        }

        std::lock_guard<std::mutex> lock(state->mutex);
        auto pending = state->pending.find(path);
        if (pending != state->pending.end() && pending->second == generation)
            state->pending.erase(pending);
        if (state->alive.load() && state->generation == generation &&
            state->wanted.count(path) != 0)
            state->ready.push_back(std::move(decoded));
    });
}

void GameGridView::_uploadDecodedTextures(NVGcontext* vg)
{
    if (!vg || !m_textureLoader)
        return;

#ifdef __SWITCH__
    constexpr int maxUploadsPerFrame = 2;
#else
    constexpr int maxUploadsPerFrame = 4;
#endif
    for (int i = 0; i < maxUploadsPerFrame; ++i) {
        DecodedTexture decoded;
        {
            std::lock_guard<std::mutex> lock(m_textureLoader->mutex);
            if (m_textureLoader->ready.empty())
                break;
            decoded = std::move(m_textureLoader->ready.front());
            m_textureLoader->ready.pop_front();
            if (decoded.generation != m_textureLoader->generation ||
                m_textureLoader->wanted.count(decoded.path) == 0)
                continue;
        }

        if (decoded.failed || decoded.pixels.empty()) {
            m_failedTextures.insert(decoded.path);
            continue;
        }
        if (m_textureCache.count(decoded.path) != 0)
            continue;

        int handle = nvgCreateImageRGBA(vg, decoded.width, decoded.height,
                                        0,
                                        decoded.pixels.data());
        if (handle >= 0) {
            const size_t textureBytes = static_cast<size_t>(decoded.width) *
                static_cast<size_t>(decoded.height) * 4u;
            m_textureCache[decoded.path] = handle;
            m_textureLastUsed[decoded.path] = ++m_textureUseTick;
            m_textureMemoryBytes[decoded.path] = textureBytes;
            m_textureCacheBytes += textureBytes;
        } else
            m_failedTextures.insert(decoded.path);
    }
}

void GameGridView::_loadTextures(NVGcontext* vg)
{
    if (!vg) return;

    const int startRow = m_visibleStartRow;
    const int endRow = m_visibleEndRow;
    const uint64_t selectedGameId = m_selectedIndex >= 0 &&
        static_cast<size_t>(m_selectedIndex) < m_items.size()
        ? m_items[static_cast<size_t>(m_selectedIndex)].gameId
        : 0;
    const bool requestSetChanged = startRow != m_requestedStartRow ||
        endRow != m_requestedEndRow || m_viewMode != m_requestedViewMode ||
        m_scrollDirection != m_requestedScrollDirection ||
        (m_viewMode == ViewMode::LIST && selectedGameId != m_requestedGameId);

    if (requestSetChanged && m_textureLoader) {
        std::unordered_set<std::string> wanted;
        const int begin = std::max(0, startRow * spanCount);
        const int end = std::min(static_cast<int>(m_items.size()), endRow * spanCount);
        constexpr int preloadCount = 10;
        const int preloadBegin = m_scrollDirection >= 0
            ? end
            : std::max(0, begin - preloadCount);
        const int preloadEnd = m_scrollDirection >= 0
            ? std::min(static_cast<int>(m_items.size()), end + preloadCount)
            : begin;
        for (int i = preloadBegin; i < preloadEnd; ++i)
            _populateItem(static_cast<size_t>(i));
        auto displayPath = [this](const GridDrawItem& item) -> const std::string& {
            return m_viewMode == ViewMode::LIST
                ? item.platformImagePath
                : item.imagePath;
        };
        for (int i = begin; i < end; ++i) {
            const auto& item = m_items[static_cast<size_t>(i)];
            const std::string& path = displayPath(item);
            if (!path.empty())
                wanted.insert(path);
        }
        for (int i = preloadBegin; i < preloadEnd; ++i) {
            const auto& item = m_items[static_cast<size_t>(i)];
            const std::string& path = displayPath(item);
            if (!path.empty())
                wanted.insert(path);
        }
        if (m_viewMode == ViewMode::LIST && m_selectedIndex >= 0 &&
            static_cast<size_t>(m_selectedIndex) < m_items.size()) {
            const auto& cover = m_items[static_cast<size_t>(m_selectedIndex)].imagePath;
            if (!cover.empty())
                wanted.insert(cover);
        }

        std::lock_guard<std::mutex> lock(m_textureLoader->mutex);
        ++m_textureLoader->generation;
        m_textureLoader->wanted = std::move(wanted);
        m_textureLoader->ready.clear();
        for (auto it = m_textureLoader->pending.begin();
             it != m_textureLoader->pending.end();) {
            if (it->second != m_textureLoader->generation)
                it = m_textureLoader->pending.erase(it);
            else
                ++it;
        }
        m_requestedStartRow = startRow;
        m_requestedEndRow = endRow;
        m_requestedScrollDirection = m_scrollDirection;
        m_requestedViewMode = m_viewMode;
        m_requestedGameId = selectedGameId;
    }

    auto bindTexture = [this](GridDrawItem& item, bool platformDefault) {
        const std::string& path = platformDefault ? item.platformImagePath : item.imagePath;
        int& handle = platformDefault ? item.platformTextureHandle : item.textureHandle;
        bool& ready = platformDefault ? item.platformTextureReady : item.textureReady;
        bool& failed = platformDefault ? item.platformTextureFailed : item.textureFailed;
        if (path.empty() || handle >= 0)
            return;
        auto cached = m_textureCache.find(path);
        if (cached != m_textureCache.end()) {
            handle = cached->second;
            ready = true;
            failed = false;
            m_textureLastUsed[path] = ++m_textureUseTick;
        } else if (m_failedTextures.count(path) != 0) {
            failed = true;
        } else {
            _requestTexture(
                path,
                platformDefault ? item.platformImageSourcePath : "",
                platformDefault ? item.imagePath : "");
        }
    };

    auto loadItemAt = [this, &bindTexture](size_t index) {
        if (index >= m_items.size())
            return;
        auto& item = m_items[index];
        bindTexture(item, m_viewMode == ViewMode::LIST);
    };

    // 先提交整屏请求，再额外提交列表详情封面；旧视口任务通过 generation 失效。
    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < spanCount; col++) {
            size_t index = static_cast<size_t>(row * spanCount + col);
            loadItemAt(index);
        }
    }
    if (m_viewMode == ViewMode::LIST && m_selectedIndex >= 0 &&
        static_cast<size_t>(m_selectedIndex) < m_items.size())
        bindTexture(m_items[static_cast<size_t>(m_selectedIndex)], false);
    constexpr int preloadCount = 10;
    const int visibleBegin = std::max(0, startRow * spanCount);
    const int visibleEnd = std::min(static_cast<int>(m_items.size()), endRow * spanCount);
    const int preloadBegin = m_scrollDirection >= 0
        ? visibleEnd
        : std::max(0, visibleBegin - preloadCount);
    const int preloadEnd = m_scrollDirection >= 0
        ? std::min(static_cast<int>(m_items.size()), visibleEnd + preloadCount)
        : visibleBegin;
    for (int i = preloadBegin; i < preloadEnd; ++i) {
        loadItemAt(static_cast<size_t>(i));
    }
}

void GameGridView::_evictTextures()
{
#ifdef __SWITCH__
    constexpr size_t textureBudgetBytes = 128u * 1024u * 1024u;
#else
    constexpr size_t textureBudgetBytes = 512u * 1024u * 1024u;
#endif
    if (m_textureCacheBytes <= textureBudgetBytes)
        return;

    NVGcontext* vg = brls::Application::getNVGContext();
    if (!vg) return;

    std::unordered_set<std::string> protectedPaths;
    const int retainedRows = m_viewMode == ViewMode::GRID ? 16 : 48;
    const int keepStartRow = std::max(0, m_visibleStartRow - retainedRows);
    const int keepEndRow = std::min(
        _getRowCount(), m_visibleEndRow + retainedRows);
    for (int row = keepStartRow; row < keepEndRow; ++row) {
        for (int col = 0; col < spanCount; ++col) {
            const size_t index = static_cast<size_t>(row * spanCount + col);
            if (index >= m_items.size()) break;
            const auto& item = m_items[index];
            if (!item.imagePath.empty()) protectedPaths.insert(item.imagePath);
            if (!item.imageLayerPath.empty()) protectedPaths.insert(item.imageLayerPath);
            if (!item.platformImagePath.empty()) protectedPaths.insert(item.platformImagePath);
        }
    }

    while (m_textureCacheBytes > textureBudgetBytes) {
        auto victim = m_textureCache.end();
        uint64_t oldestUse = std::numeric_limits<uint64_t>::max();
        for (auto it = m_textureCache.begin(); it != m_textureCache.end(); ++it) {
            if (protectedPaths.count(it->first) != 0)
                continue;
            const auto used = m_textureLastUsed.find(it->first);
            const uint64_t tick = used == m_textureLastUsed.end() ? 0 : used->second;
            if (tick < oldestUse) {
                oldestUse = tick;
                victim = it;
            }
        }
        if (victim == m_textureCache.end())
            break;

        const std::string path = victim->first;
        const int handle = victim->second;
        for (auto& item : m_items) {
            if (item.textureHandle == handle) {
                item.textureHandle = -1;
                item.textureReady = false;
            }
            if (item.imageLayerHandle == handle)
                item.imageLayerHandle = -1;
            if (item.platformTextureHandle == handle) {
                item.platformTextureHandle = -1;
                item.platformTextureReady = false;
            }
        }
        nvgDeleteImage(vg, handle);
        const auto memory = m_textureMemoryBytes.find(path);
        if (memory != m_textureMemoryBytes.end()) {
            m_textureCacheBytes -= std::min(m_textureCacheBytes, memory->second);
            m_textureMemoryBytes.erase(memory);
        }
        m_textureLastUsed.erase(path);
        m_textureCache.erase(victim);
    }
}

void GameGridView::_captureInputState()
{
    auto& state = brls::Application::getControllerState();
    m_prevUp = state.buttons[static_cast<int>(brls::BUTTON_UP)];
    m_prevDown = state.buttons[static_cast<int>(brls::BUTTON_DOWN)];
    m_prevLeft = state.buttons[static_cast<int>(brls::BUTTON_LEFT)];
    m_prevRight = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)];
    m_prevA = state.buttons[static_cast<int>(brls::BUTTON_A)];
    float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
    float lx = state.axes[static_cast<int>(brls::LEFT_X)];
    m_prevStickUp = (ly < -0.3f);
    m_prevStickDown = (ly > 0.3f);
    m_prevStickLeft = (lx < -0.3f);
    m_prevStickRight = (lx > 0.3f);
}

void GameGridView::_handleInput(float dt)
{
    if (m_items.empty()) return;

    bool focused = isFocused();
    if (!focused) {
        m_wasFocused = false;
        return;
    }
    if (!m_wasFocused) {
        _captureInputState();
        m_wasFocused = true;
    }
    if (m_interactionDisabled) return;

    auto& state = brls::Application::getControllerState();

    bool upNow = state.buttons[static_cast<int>(brls::BUTTON_UP)];
    if (upNow && !m_prevUp) {
        m_holdUpTime = 0.f;
        m_holdUpRepeat = 0.f;
        _moveUp();
    }
    if (upNow) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) {
                m_holdUpRepeat -= interval;
                if (_tryMoveUp()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevUp = upNow;

    bool downNow = state.buttons[static_cast<int>(brls::BUTTON_DOWN)];
    if (downNow && !m_prevDown) {
        m_holdDownTime = 0.f;
        m_holdDownRepeat = 0.f;
        _moveDown();
    }
    if (downNow) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) {
                m_holdDownRepeat -= interval;
                if (_tryMoveDown()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevDown = downNow;

    bool leftNow = state.buttons[static_cast<int>(brls::BUTTON_LEFT)];
    if (leftNow && !m_prevLeft) {
        m_holdLeftTime = 0.f;
        m_holdLeftRepeat = 0.f;
        if (m_viewMode == ViewMode::LIST)
            _movePageUp();
        else
            _moveLeft();
    }
    if (leftNow) {
        m_holdLeftTime += dt;
        if (m_holdLeftTime > HOLD_INITIAL_DELAY) {
            m_holdLeftRepeat += dt;
            float interval = m_holdLeftTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdLeftRepeat >= interval) {
                m_holdLeftRepeat -= interval;
                const bool moved = m_viewMode == ViewMode::LIST
                    ? _tryMovePageUp() : _tryMoveLeft();
                if (moved) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevLeft = leftNow;

    bool rightNow = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)];
    if (rightNow && !m_prevRight) {
        m_holdRightTime = 0.f;
        m_holdRightRepeat = 0.f;
        if (m_viewMode == ViewMode::LIST)
            _movePageDown();
        else
            _moveRight();
    }
    if (rightNow) {
        m_holdRightTime += dt;
        if (m_holdRightTime > HOLD_INITIAL_DELAY) {
            m_holdRightRepeat += dt;
            float interval = m_holdRightTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdRightRepeat >= interval) {
                m_holdRightRepeat -= interval;
                const bool moved = m_viewMode == ViewMode::LIST
                    ? _tryMovePageDown() : _tryMoveRight();
                if (moved) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevRight = rightNow;

    // ── Stick (Left & Right) ──
    float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
    float lx = state.axes[static_cast<int>(brls::LEFT_X)];
    float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
    float rx = state.axes[static_cast<int>(brls::RIGHT_X)];

    constexpr float STICK_DEADZONE = 0.3f;
    constexpr float STICK_DOMINANCE = 1.5f;
    float absLX = std::abs(lx), absLY = std::abs(ly);
    float absRX = std::abs(rx), absRY = std::abs(ry);

    auto stickDir = [](float x, float y, float ax, float ay) -> uint8_t {
        if (ax < STICK_DEADZONE && ay < STICK_DEADZONE) return 0;
        if (ax > ay * STICK_DOMINANCE) return (x > 0) ? 2 : 1;
        if (ay > ax * STICK_DOMINANCE) return (y > 0) ? 4 : 3;
        return 0;
    };

    uint8_t dir = 0;
    uint8_t ld = stickDir(lx, ly, absLX, absLY);
    uint8_t rd = stickDir(rx, ry, absRX, absRY);
    if (ld) dir = ld;
    if (rd) dir = rd;

    bool stickUp = (dir == 3), stickDown = (dir == 4);
    bool stickLeft = (dir == 1), stickRight = (dir == 2);

    if (stickUp && !m_prevStickUp) {
        m_holdUpTime = 0.f;
        m_holdUpRepeat = 0.f;
        _moveUp();
    }
    if (stickUp) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) {
                m_holdUpRepeat -= interval;
                if (_tryMoveUp()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickUp = stickUp;

    if (stickDown && !m_prevStickDown) {
        m_holdDownTime = 0.f;
        m_holdDownRepeat = 0.f;
        _moveDown();
    }
    if (stickDown) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) {
                m_holdDownRepeat -= interval;
                if (_tryMoveDown()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickDown = stickDown;

    if (stickLeft && !m_prevStickLeft) {
        m_holdLeftTime = 0.f;
        m_holdLeftRepeat = 0.f;
        if (m_viewMode == ViewMode::LIST)
            _movePageUp();
        else
            _moveLeft();
    }
    if (stickLeft) {
        m_holdLeftTime += dt;
        if (m_holdLeftTime > HOLD_INITIAL_DELAY) {
            m_holdLeftRepeat += dt;
            float interval = m_holdLeftTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdLeftRepeat >= interval) {
                m_holdLeftRepeat -= interval;
                const bool moved = m_viewMode == ViewMode::LIST
                    ? _tryMovePageUp() : _tryMoveLeft();
                if (moved) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickLeft = stickLeft;

    if (stickRight && !m_prevStickRight) {
        m_holdRightTime = 0.f;
        m_holdRightRepeat = 0.f;
        if (m_viewMode == ViewMode::LIST)
            _movePageDown();
        else
            _moveRight();
    }
    if (stickRight) {
        m_holdRightTime += dt;
        if (m_holdRightTime > HOLD_INITIAL_DELAY) {
            m_holdRightRepeat += dt;
            float interval = m_holdRightTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdRightRepeat >= interval) {
                m_holdRightRepeat -= interval;
                const bool moved = m_viewMode == ViewMode::LIST
                    ? _tryMovePageDown() : _tryMoveRight();
                if (moved) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickRight = stickRight;

    if (m_focusMoved) {
        m_selectedGameId = m_items[m_selectedIndex].gameId;
        if (m_viewMode == ViewMode::LIST)
            m_detailTransition = 0.f;
        if (m_focusChangeCallback) m_focusChangeCallback(m_selectedIndex);
    }

    bool aNow = state.buttons[static_cast<int>(brls::BUTTON_A)];
    if (aNow && !m_prevA && m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < m_items.size()) {
        if (m_multiSelectMode)
            toggleDeleteSelection(m_selectedIndex);
        else if (m_dataSource)
            m_dataSource->onItemSelected(m_selectedIndex);
    }
    m_prevA = aNow;
}

void GameGridView::frame(brls::FrameContext* ctx)
{
    View::frame(ctx);

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;
    if (dt <= 0.f)
        dt = 0.016f;
    else
        dt = std::min(dt, 0.033f);

    m_focusBorderAnimTime += dt;
    m_platformTransition = std::min(1.f, m_platformTransition + dt * 8.0f);
    if (m_deleteWaiting)
        m_deleteAnimationTime += dt;
    if (m_deleteWaiting && m_deleteBackendFinished &&
        m_deleteAnimationTime >= 0.42f) {
        m_deleteWaiting = false;
        m_deleteCollapsing = true;
        m_deleteBackendFinished = false;
        m_deleteCollapseProgress = 0.f;
    }
    if (m_deleteCollapsing) {
        m_deleteCollapseProgress = std::min(1.f,
            m_deleteCollapseProgress + dt * 6.5f);
        if (m_deleteCollapseProgress >= 1.f && m_deleteCompletion) {
            m_reflowOrigins.clear();
            size_t validDeleteCount = 0;
            for (int index : m_deletingIndices) {
                if (index >= 0 && static_cast<size_t>(index) < m_items.size())
                    ++validDeleteCount;
            }
            m_reflowOrigins.reserve(m_items.size() - validDeleteCount);
            for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
                if (m_deletingIndices.count(i) == 0)
                    m_reflowOrigins.push_back(i);
            }
            m_reflowFirstMovedIndex = 0;
            while (m_reflowFirstMovedIndex < static_cast<int>(m_reflowOrigins.size()) &&
                   m_reflowOrigins[static_cast<size_t>(m_reflowFirstMovedIndex)] ==
                       m_reflowFirstMovedIndex)
                ++m_reflowFirstMovedIndex;
            m_reflowPending = true;
            m_deleteCollapsing = false;
            m_deletingIndices.clear();
            auto completion = std::move(m_deleteCompletion);
            m_deleteCompletion = nullptr;
            brls::sync(std::move(completion));
        }
    }
    if (m_reflowTransition < 1.f) {
        m_reflowTransition = std::min(1.f, m_reflowTransition + dt * 7.f);
        if (m_reflowTransition >= 1.f) {
            m_reflowOrigins.clear();
            m_interactionDisabled = false;
        }
    }
    if (m_launchAnimationRunning) {
        m_launchAnimationTime += dt;
        if (m_launchAnimationTime >= 1.18f && m_launchCompletion) {
            auto completion = std::move(m_launchCompletion);
            m_launchCompletion = nullptr;
            brls::sync(std::move(completion));
        }
    } else if (m_exitAnimationRunning) {
        m_contentTransition = std::max(0.f, m_contentTransition - dt * 5.0f);
        m_detailTransition = std::max(0.f, m_detailTransition - dt * 5.2f);
        m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 5.2f);
        if (m_pageEntrance <= 0.f && m_contentTransition <= 0.f && m_exitCompletion) {
            auto completion = std::move(m_exitCompletion);
            m_exitCompletion = nullptr;
            brls::sync(std::move(completion));
        }
    } else {
        m_contentTransition = std::min(1.f, m_contentTransition + dt * 5.0f);
        m_detailTransition = std::min(1.f, m_detailTransition + dt * 5.2f);
        m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 5.5f);
        if (m_dataSource)
            _handleInput(dt);
    }

    if (m_shakeTime > 0.f)
        m_shakeTime -= dt;

    if (m_focusMoved) {
        _ensureSelectedVisible();
        m_focusMoved = false;
    }

    _updateScrollPhysics(dt);
    _updateVisibleRange();
    _populateVisibleItems();
    _updateFocusAnimation(dt);
    _updateMarquee(dt);

    NVGcontext* vg = brls::Application::getNVGContext();
    _uploadDecodedTextures(vg);
    _loadTextures(vg);
    _evictTextures();

    size_t preloadIndex = std::max(static_cast<size_t>(spanCount), m_items.size() / 2);
    if (!m_requestNextPage && m_selectedIndex >= 0 &&
        static_cast<size_t>(m_selectedIndex) >= preloadIndex &&
        m_items.size() > 0) {
        if (m_nextPageCallback) {
            m_requestNextPage = true;
            m_nextPageCallback();
        }
    }
}

NVGcolor GameGridView::_getBadgeColor(PlatformBadgeColor color) const
{
    switch (color) {
        case PlatformBadgeColor::GBA:     return nvgRGBA(108, 77,  191, 220);
        case PlatformBadgeColor::GBC:     return nvgRGBA(0,   112, 221, 220);
        case PlatformBadgeColor::GB:      return nvgRGBA(0,   168, 107, 220);
        case PlatformBadgeColor::NES:     return nvgRGBA(218, 41,  28,  220);
        case PlatformBadgeColor::SNES:    return nvgRGBA(160, 100, 180, 220);
        case PlatformBadgeColor::NDS:     return nvgRGBA(54,  150, 190, 220);
        case PlatformBadgeColor::THREEDS: return nvgRGBA(230, 79,  91,  220);
        case PlatformBadgeColor::GENESIS: return nvgRGBA(23, 55, 139, 220);
        case PlatformBadgeColor::ARCADE:  return nvgRGBA(236, 134, 44, 220);
        case PlatformBadgeColor::DREAMCAST: return nvgRGBA(0, 142, 180, 220);
        case PlatformBadgeColor::PSP:     return nvgRGBA(67, 118, 226, 220);
        case PlatformBadgeColor::PS1:     return nvgRGBA(74, 74, 82, 220);
        case PlatformBadgeColor::SATURN:  return nvgRGBA(68, 82, 150, 220);
        case PlatformBadgeColor::DOLPHIN: return nvgRGBA(54, 102, 196, 220);
        default:                          return nvgRGBA(100, 100, 100, 200);
    }
}

void GameGridView::draw(NVGcontext* vg, float x, float y, float w, float h,
                         brls::Style style, brls::FrameContext* ctx)
{
    if (!vg) return;

    nvgSave(vg);
    nvgIntersectScissor(vg, x, y, w, h);
    const float pageEased = 1.f - std::pow(1.f - m_pageEntrance, 3.f);
    const float pageBounce = easeOutBack(m_pageEntrance);
    const float pageAlpha = 0.12f + pageEased * 0.88f;
    nvgGlobalAlpha(vg, pageAlpha);

    _drawToolbar(vg, x,
                 y - (1.f - pageBounce) * _getContentTop(), w);

    const float contentY = y + _getContentTop();
    const float contentH = _getViewportHeight();
    const float paneW = m_viewMode == ViewMode::LIST ? _getListPaneWidth() : w;
    const float contentSlideEased = 1.f -
        std::pow(1.f - m_contentTransition, 3.f);
    const float wholePageOffsetX = m_contentWholePageSlide
        ? static_cast<float>(m_contentSlideDirection) *
            (1.f - contentSlideEased) * w
        : 0.f;
    nvgSave(vg);
    nvgIntersectScissor(vg, x, contentY, paneW, contentH);

    if (m_items.empty()) {
        if (m_hasPresentedData) {
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 22.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(230, 232, 238, 210));
            nvgText(vg, x + paneW * 0.5f + wholePageOffsetX,
                    contentY + contentH * 0.45f,
                    L("当前分类暂无游戏").c_str(), nullptr);
        }
    } else {
        const float itemW = _getItemWidth();
        const float itemH = estimatedRowHeight;
        const int visibleItemCount = std::max(1,
            (m_visibleEndRow - m_visibleStartRow) * spanCount);
        for (int row = m_visibleStartRow; row < m_visibleEndRow; ++row) {
            for (int col = 0; col < spanCount; ++col) {
                const int idx = row * spanCount + col;
                if (idx >= static_cast<int>(m_items.size())) break;

                const auto& item = m_items[static_cast<size_t>(idx)];
                int ordinal = (row - m_visibleStartRow) * spanCount + col;
                if (m_contentSlideDirection < 0)
                    ordinal = visibleItemCount - 1 - ordinal;
                const float delay = m_contentWholePageSlide
                    ? 0.f
                    : std::min(0.32f, ordinal * 0.022f);
                const float localProgress = m_contentWholePageSlide
                    ? m_contentTransition
                    : (m_contentTransition <= delay
                        ? 0.f
                        : std::min(1.f,
                            (m_contentTransition - delay) / (1.f - delay)));
                const float itemEased = m_contentWholePageSlide
                    ? contentSlideEased
                    : 1.f - std::pow(1.f - localProgress, 3.f);
                const float bubbleEased = m_contentWholePageSlide
                    ? 1.f
                    : easeOutBack(localProgress);
                const float enterX = m_contentWholePageSlide
                    ? wholePageOffsetX
                    : static_cast<float>(m_contentSlideDirection) *
                        (1.f - itemEased) * 34.f;
                const float enterY = m_contentWholePageSlide
                    ? 0.f
                    : (m_contentSlideDirection == 0 ? 1.f : 0.35f) *
                        (1.f - itemEased) * 20.f;
                float reflowX = 0.f;
                float reflowY = 0.f;
                if (m_reflowTransition < 1.f &&
                    static_cast<size_t>(idx) < m_reflowOrigins.size()) {
                    const int oldIndex = m_reflowOrigins[static_cast<size_t>(idx)];
                    const int oldRow = oldIndex / spanCount;
                    const int oldCol = oldIndex % spanCount;
                    const float delay = oldIndex == idx
                        ? 0.f
                        : std::min(0.32f,
                            std::max(0, idx - m_reflowFirstMovedIndex) * 0.025f);
                    const float localReflow = m_reflowTransition <= delay
                        ? 0.f
                        : std::min(1.f,
                            (m_reflowTransition - delay) / (1.f - delay));
                    const float reflowEased = 1.f -
                        std::pow(1.f - localReflow, 3.f);
                    reflowX = (_getItemX(oldCol) - _getItemX(col)) *
                        (1.f - reflowEased);
                    reflowY = (_getItemY(oldRow) - _getItemY(row)) *
                        (1.f - reflowEased);
                }
                const float ix = x + _getItemX(col) + enterX + reflowX;
                const float iy = y + _getItemY(row) + enterY + reflowY;
                if (iy + itemH < contentY - 20.f || iy > contentY + contentH + 20.f)
                    continue;
                nvgSave(vg);
                float sourceAlpha = 1.f;
                if (m_launchAnimationRunning && idx == m_launchItemIndex)
                    sourceAlpha = std::max(0.f, 1.f - m_launchAnimationTime * 4.f);
                nvgGlobalAlpha(vg,
                    pageAlpha * (0.14f + 0.86f * itemEased) * sourceAlpha);
                const float bubbleScale = 0.72f + 0.28f * bubbleEased;
                nvgTranslate(vg, ix + itemW * 0.5f, iy + itemH * 0.5f);
                nvgScale(vg, bubbleScale, bubbleScale);
                nvgTranslate(vg, -(ix + itemW * 0.5f), -(iy + itemH * 0.5f));
                _drawItem(vg, item, ix, iy, itemW, itemH,
                          idx == m_selectedIndex, idx);
                nvgRestore(vg);
            }
        }
    }

    _drawScrollbar(vg, x + 1.f + wholePageOffsetX,
                   contentY, paneW, contentH);
    nvgRestore(vg);

    if (m_viewMode == ViewMode::LIST && !m_items.empty()) {
        const float detailX = x + paneW + 10.f + wholePageOffsetX;
        _drawDetailsPanel(vg, detailX, contentY,
                          w - paneW - 24.f, contentH);
    }

    _drawFooter(vg, x,
                y + h - _getFooterHeight() +
                    (1.f - pageBounce) * _getFooterHeight(),
                w, _getFooterHeight());

    if (m_launchAnimationRunning)
        _drawLaunchOverlay(vg, x, y, w, h);

    nvgResetScissor(vg);
    nvgRestore(vg);
}

void GameGridView::_drawItem(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, bool focused, int idx)
{
    if (m_loadingSkeleton) {
        _drawSkeletonItem(vg, x, y, w, h, idx);
        return;
    }

    if (m_deletingIndices.count(idx) != 0) {
        const float collapse = m_deleteCollapsing
            ? 1.f - (1.f - std::pow(1.f - m_deleteCollapseProgress, 3.f))
            : 1.f;
        const float shake = m_deleteWaiting
            ? std::sin(m_deleteAnimationTime * 58.f) * 7.f
            : 0.f;
        const float shakeY = m_deleteWaiting
            ? std::cos(m_deleteAnimationTime * 47.f) * 2.5f
            : 0.f;
        nvgSave(vg);
        nvgTranslate(vg, x + w * 0.5f, y + h * 0.5f);
        nvgScale(vg, collapse, collapse);
        nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.5f));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + shake, y + shakeY, w, h, 12.f);
        nvgFillColor(vg, nvgRGBA(210, 65, 65, 30));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(240, 105, 105, 205));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
        _drawMaterialIcon(vg, beiklive::material::DELETE_ICON,
                          x + w * 0.5f + shake, y + h * 0.5f + shakeY,
                          std::min(58.f, h * 0.35f),
                          nvgRGBA(246, 130, 130, 245),
                          NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgRestore(vg);
        return;
    }

    nvgSave(vg);

    float shakeX = 0.f;
    float shakeY = 0.f;
    if (focused && m_shakeTime > 0.f && m_shakeDir != 0.f) {
        float t = m_shakeTime / 0.3f;
        float decay = t * t;
        float freq = 80.f;
        float amount = std::sin(m_shakeTime * freq) * 6.f * decay;
        float adir = std::abs(m_shakeDir);
        if (adir < 1.5f)
            shakeY = amount * m_shakeDir;
        else
            shakeX = amount * (m_shakeDir > 0.f ? 1.f : -1.f);
    }

    NVGpaint cardShadow = nvgBoxGradient(
        vg, x + shakeX + 3.f, y + shakeY + 3.f,
        w, h, 12.f, 5.f,
        nvgRGBA(0, 0, 0, 72), nvgRGBA(0, 0, 0, 0));
    nvgBeginPath(vg);
    nvgRect(vg, x + shakeX - 2.f, y + shakeY - 2.f,
            w + 10.f, h + 10.f);
    nvgRoundedRect(vg, x + shakeX, y + shakeY, w, h, 12.f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, cardShadow);
    nvgFill(vg);

    if (focused && item.focusGlow > 0.01f) {
        float gx = x + shakeX;
        float gy = y + shakeY;

        beiklive::ui::drawGradientFocusBorder(
            vg,
            gx,
            gy,
            w,
            h,
            12.0f,
            3.0f,
            item.focusGlow,
            beiklive::ui::gradientFocusAnimationOffset(m_focusBorderAnimTime));
    }

    const bool multiSelected = m_multiSelectMode && m_selectedForDelete.count(idx) != 0;

    if (multiSelected || item.favorite) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + shakeX, y + shakeY, w, h, 12.f);
        nvgFillColor(vg, multiSelected
            ? nvgRGBA(80, 132, 255, 65)
            : (focused
                ? nvgRGBA(230, 185, 92, 56)
                : nvgRGBA(224, 166, 87, 34)));
        nvgFill(vg);
    }

    if (m_multiSelectMode) {
        strokeRoundedRectInside(
            vg, x + shakeX, y + shakeY, w, h, 12.f,
            multiSelected ? 2.5f : 1.f,
            multiSelected
                ? nvgRGBA(94, 151, 255, 255)
                : nvgRGBA(127, 139, 162, 210));
    } else if (item.favorite) {
        strokeRoundedRectInside(
            vg, x + shakeX, y + shakeY, w, h, 12.f,
            focused ? 1.5f : 1.f,
            focused
                ? nvgRGBA(245, 200, 112, 235)
                : nvgRGBA(224, 166, 87, 165));
    } else {
        strokeRoundedRectInside(
            vg, x + shakeX, y + shakeY, w, h, 12.f,
            focused ? 1.5f : 1.f,
            focused
                ? nvgRGBA(235, 240, 255, 230)
                : nvgRGBA(255, 255, 255, 65));
    }
    

    if (item.empty) {
        _drawEmptyItem(vg, x, y, w, h);
    } else if (m_viewMode == ViewMode::GRID) {
        const float pad = 10.f;
        const float imageH = h - 100.f;
        _drawImage(vg, item, x + pad, y + pad, w - pad * 2.f, imageH);

        const float titleY = y + h - 79.f;
        _drawTitle(vg, item, x + 12.f, titleY, w - 24.f, focused);
        const float lineX = x + 12.f;
        const float badgeW = _drawBadge(vg, item, lineX, y + h - 51.f);
        const float playTimeX = lineX + badgeW + 10.f;
        _drawPlayTime(vg, item.playTime, playTimeX, y + h - 49.f,
                      std::max(1.f, x + w - 12.f - playTimeX));
        _drawSubText(vg, item.subText, x + 12.f, y + h - 24.f, w - 24.f);
    } else {
        const float imageW = 54.f;
        const float imageH = h - 10.f;
        _drawImage(vg, item, x + 8.f, y + 5.f, imageW, imageH, true);

        const float textX = x + 74.f;
        const float textW = w - 106.f;
        _drawTitle(vg, item, textX, y + 9.f, textW, focused);
        const float badgeW = _drawBadge(vg, item, textX, y + 37.f);
        const float playTimeX = textX + badgeW + 10.f;
        const float playTimeW = std::min(140.f,
            std::max(60.f, textX + textW - playTimeX - 56.f));
        _drawPlayTime(vg, item.playTime, playTimeX, y + 39.f, playTimeW);
        const float subTextX = playTimeX + playTimeW + 10.f;
        _drawSubText(vg, item.subText, subTextX, y + 39.f,
                     std::max(40.f, textX + textW - subTextX));
    }

    if (m_multiSelectMode) {
        _drawMaterialIcon(vg,
            multiSelected ? beiklive::material::CHECK_BOX : beiklive::material::CHECK_BOX_OUTLINE,
            x + w - 15.f + shakeX, y + 15.f + shakeY, 24.f,
            multiSelected ? nvgRGBA(110, 164, 255, 255) : nvgRGBA(190, 196, 210, 230),
            NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    } else if (item.favorite) {
        _drawFavourite(vg, item, x, y, w, h, shakeX, shakeY);
    }

    nvgRestore(vg);
}

void GameGridView::_drawImage(NVGcontext* vg, const GridDrawItem& item,
                              float x, float y, float boxW, float boxH,
                              bool platformDefault)
{
    const int handle = platformDefault ? item.platformTextureHandle : item.textureHandle;
    const bool ready = platformDefault ? item.platformTextureReady : item.textureReady;
    float aspect = item.coverAspect > 0.05f ? item.coverAspect : 0.78f;
    if (handle >= 0 && ready) {
        int imageWidth = 0;
        int imageHeight = 0;
        nvgImageSize(vg, handle, &imageWidth, &imageHeight);
        if (imageWidth > 0 && imageHeight > 0)
            aspect = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
    }

    float drawW = boxW;
    float drawH = drawW / aspect;
    if (drawH > boxH) {
        drawH = boxH;
        drawW = drawH * aspect;
    }
    const float drawX = x + (boxW - drawW) * 0.5f;
    const float drawY = y + (boxH - drawH) * 0.5f;

    nvgBeginPath(vg);
    nvgRoundedRect(vg, drawX, drawY, drawW, drawH, 8.f);

    if (handle >= 0 && ready) {
        NVGpaint paint = nvgImagePattern(vg, drawX, drawY, drawW, drawH,
                                         0.f, handle, 1.f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    } else {
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 22));
        nvgFill(vg);
        _drawMaterialIcon(vg, beiklive::material::IMAGE_PLACEHOLDER,
                          drawX + drawW * 0.5f, drawY + drawH * 0.5f,
                          std::min(38.f, drawH * 0.28f), nvgRGBA(235, 238, 245, 130),
                          NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    }

    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 80));
    nvgStrokeWidth(vg, 0.5f);
    nvgStroke(vg);
}

float GameGridView::_drawBadge(NVGcontext* vg, const GridDrawItem& item, float x, float y)
{
    if (item.badgeText.empty() || item.badgeColor == PlatformBadgeColor::NONE) return 0.f;

    nvgFontSize(vg, 12.f);
    nvgFontFaceId(vg, m_fontId);
    float bounds[4]{};
    nvgTextBounds(vg, 0.f, 0.f, item.badgeText.c_str(), nullptr, bounds);
    const float badgeW = std::max(36.f, (bounds[2] - bounds[0]) + 16.f);
    float badgeH = 20.f;

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, badgeW, badgeH, 4.f);
    nvgFillColor(vg, _getBadgeColor(item.badgeColor));
    nvgFill(vg);

    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, beiklive::uiTextPrimary());
    nvgText(vg, x + badgeW * 0.5f, y + badgeH * 0.5f, item.badgeText.c_str(), nullptr);
    return badgeW;
}

void GameGridView::_drawTitle(NVGcontext* vg, const GridDrawItem& item, float x, float y, float maxWidth, bool focused)
{
    nvgFontSize(vg, m_titleFontSize);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, beiklive::uiTextPrimary());

    nvgSave(vg);
    nvgIntersectScissor(vg, x, y - 3.f, maxWidth, m_titleFontSize + 6.f);

    float adjY = y - (m_titleFontSize - 16.f) * 0.15f;
    if (focused && item.marqueeMaxOffset > 0.f) {
        nvgText(vg, x - item.marqueeOffset, adjY, item.title.c_str(), nullptr);
    } else {
        nvgText(vg, x, adjY, item.title.c_str(), nullptr);
    }

    nvgRestore(vg);
}

void GameGridView::_drawSubText(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth)
{
    if (text.empty()) return;

    nvgFontSize(vg, 14.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, beiklive::uiTextSecondary());

    nvgSave(vg);
    nvgIntersectScissor(vg, x, y - 2.f, std::max(1.f, maxWidth), 22.f);
    nvgText(vg, x, y, text.c_str(), nullptr);
    nvgRestore(vg);
}

void GameGridView::_drawPlayTime(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth)
{
    if (text.empty()) return;

    nvgFontSize(vg, 14.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(128, 179, 255, 255));

    nvgSave(vg);
    nvgIntersectScissor(vg, x, y - 2.f, std::max(1.f, maxWidth), 22.f);
    nvgText(vg, x, y, text.c_str(), nullptr);
    nvgRestore(vg);
}

void GameGridView::_drawEmptyItem(NVGcontext* vg, float x, float y, float w, float h)
{
    nvgFontSize(vg, 16.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(150, 150, 150, 200));
    nvgText(vg, x + w * 0.5f, y + h * 0.5f, "空", nullptr);
}

void GameGridView::_drawFavourite(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, float sx, float sy)
{
    (void)item;
    if (m_favIconHandle < 0) {
        const std::string path = BK_RES("img/ui/light/fa.png");
        m_favIconHandle = nvgCreateImage(vg, path.c_str(), NVG_IMAGE_PREMULTIPLIED);
    }
    if (m_favIconHandle < 0)
        return;

    const float size = m_viewMode == ViewMode::GRID ? 92.f : 56.f;
    const float ix = x + w - size - 2.f + sx;
    const float iy = y + h - size - 2.f + sy;
    nvgSave(vg);
    nvgIntersectScissor(vg, x + sx, y + sy, w, h);
    NVGpaint paint = nvgImagePattern(vg, ix, iy, size, size, 0.f,
                                     m_favIconHandle,
                                     m_viewMode == ViewMode::GRID ? 0.30f : 0.24f);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, ix, iy, size, size, 10.f);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
    nvgRestore(vg);
}

void GameGridView::_drawSkeletonItem(NVGcontext* vg, float x, float y,
                                     float w, float h, int idx)
{
    const float wave = 0.5f + 0.5f * std::sin(
        m_focusBorderAnimTime * 3.4f - static_cast<float>(idx) * 0.42f);
    const unsigned char panelAlpha = static_cast<unsigned char>(13.f + wave * 10.f);
    const unsigned char contentAlpha = static_cast<unsigned char>(20.f + wave * 18.f);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, 10.f);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, panelAlpha));
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                               static_cast<unsigned char>(35.f + wave * 25.f)));
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);

    if (m_viewMode == ViewMode::GRID) {
        const float pad = 10.f;
        const float imageH = h - 100.f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + pad, y + pad, w - pad * 2.f, imageH, 8.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, contentAlpha));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 12.f, y + h - 75.f, w * 0.68f, 13.f, 4.f);
        nvgRoundedRect(vg, x + 12.f, y + h - 48.f, w * 0.34f, 10.f, 4.f);
        nvgRoundedRect(vg, x + 12.f, y + h - 25.f, w * 0.52f, 9.f, 4.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, contentAlpha));
        nvgFill(vg);
    } else {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 8.f, y + 5.f, 54.f, h - 10.f, 7.f);
        nvgRoundedRect(vg, x + 74.f, y + 13.f, w * 0.46f, 13.f, 4.f);
        nvgRoundedRect(vg, x + 74.f, y + 40.f, w * 0.28f, 10.f, 4.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, contentAlpha));
        nvgFill(vg);
    }
}

void GameGridView::_drawLaunchOverlay(NVGcontext* vg, float x, float y,
                                      float w, float h)
{
    if (m_launchItemIndex < 0 ||
        static_cast<size_t>(m_launchItemIndex) >= m_items.size())
        return;

    const auto& item = m_items[static_cast<size_t>(m_launchItemIndex)];
    const float moveProgress = m_launchStartsCentered
        ? 1.f
        : std::min(1.f, m_launchAnimationTime / 0.36f);
    const float moveEased = 1.f - std::pow(1.f - moveProgress, 3.f);
    const float fadeToBlack = std::max(0.f,
        std::min(1.f, (m_launchAnimationTime - 1.f) / 0.18f));

    const int row = m_launchItemIndex / spanCount;
    const int col = m_launchItemIndex % spanCount;
    const float sourceX = x + _getItemX(col);
    const float sourceY = y + _getItemY(row);
    const float sourceW = _getItemWidth();
    const float sourceH = estimatedRowHeight;
    const float targetW = 650.f;
    const float targetH = 260.f;
    const float targetX = x + (w - targetW) * 0.5f;
    const float targetY = y + (h - targetH) * 0.5f - 4.f;
    const float launchFloatY = std::sin(m_launchAnimationTime * 5.2f) *
        5.f * moveEased;

    const float cardX = sourceX + (targetX - sourceX) * moveEased;
    const float cardY = sourceY + (targetY - sourceY) * moveEased + launchFloatY;
    const float cardW = sourceW + (targetW - sourceW) * moveEased;
    const float cardH = sourceH + (targetH - sourceH) * moveEased;

    nvgSave(vg);
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    const float launchMaskAlpha = m_launchStartsCentered
        ? 210.f
        : moveEased * 125.f;
    nvgFillColor(vg, nvgRGBA(0, 0, 0,
        static_cast<unsigned char>(launchMaskAlpha)));
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, cardX, cardY, cardW, cardH, 12.f);
    nvgFillColor(vg, nvgRGBA(28, 31, 38,
        static_cast<unsigned char>(175.f + moveEased * 55.f)));
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
        static_cast<unsigned char>(80.f + moveEased * 80.f)));
    nvgStrokeWidth(vg, 1.2f);
    nvgStroke(vg);

    const float sourceImageX = sourceX + (m_viewMode == ViewMode::GRID ? 10.f : 8.f);
    const float sourceImageY = sourceY + (m_viewMode == ViewMode::GRID ? 10.f : 5.f);
    const float sourceImageW = m_viewMode == ViewMode::GRID
        ? std::max(20.f, sourceW - 20.f)
        : 54.f;
    const float sourceImageH = m_viewMode == ViewMode::GRID
        ? std::max(40.f, sourceH - 100.f)
        : std::max(40.f, sourceH - 10.f);
    const float targetImageX = targetX + 24.f;
    const float targetImageY = targetY + 24.f;
    const float targetImageW = 190.f;
    const float targetImageH = targetH - 48.f;
    const float imageX = sourceImageX + (targetImageX - sourceImageX) * moveEased;
    const float imageY = sourceImageY + (targetImageY - sourceImageY) * moveEased +
        launchFloatY;
    const float imageW = sourceImageW + (targetImageW - sourceImageW) * moveEased;
    const float imageH = sourceImageH + (targetImageH - sourceImageH) * moveEased;
    _drawImage(vg, item, imageX, imageY, imageW, imageH);

    const float textAlpha = std::max(0.f, std::min(1.f,
        (moveProgress - 0.45f) / 0.55f));
    nvgGlobalAlpha(vg, textAlpha);
    const float infoX = targetX + 242.f;
    const float infoW = targetW - 270.f;
    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 28.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, beiklive::uiTextPrimary(0.96f));
    nvgSave(vg);
    nvgIntersectScissor(vg, infoX, targetY + launchFloatY + 48.f,
                        std::max(1.f, infoW), 72.f);
    nvgText(vg, infoX, targetY + launchFloatY + 48.f,
            item.title.c_str(), nullptr);
    nvgRestore(vg);

    const int dotCount = static_cast<int>(m_launchAnimationTime * 5.f) % 4;
    std::string launching = L("启动中") + std::string(static_cast<size_t>(dotCount), '.');
    nvgFontSize(vg, 18.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, beiklive::uiTextSecondary(0.90f));
    nvgText(vg, infoX, targetY + launchFloatY + targetH - 82.f,
            launching.c_str(), nullptr);

    if (moveProgress >= 0.999f) {
        const float barY = targetY + launchFloatY + targetH - 54.f;
        const float barH = 12.f;
        const float transitionTime = m_launchStartsCentered ? 0.f : 0.36f;
        const float loadProgress = std::max(0.f, std::min(
            1.f, (m_launchAnimationTime - transitionTime) /
                std::max(0.01f, 1.f - transitionTime)));
        const float fillW = std::max(barH, infoW * loadProgress);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, infoX, barY, infoW, barH, barH * 0.5f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 26));
        nvgFill(vg);
        if (loadProgress > 0.f) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, infoX, barY, fillW, barH, barH * 0.5f);
            NVGpaint progressPaint = nvgLinearGradient(
                vg, infoX, barY, infoX + infoW, barY,
                nvgRGBA(94, 185, 255, 245),
                nvgRGBA(247, 197, 104, 245));
            nvgFillPaint(vg, progressPaint);
            nvgFill(vg);
        }
    }

    nvgGlobalAlpha(vg, 1.f);
    if (fadeToBlack > 0.f) {
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0,
            static_cast<unsigned char>(fadeToBlack * 255.f)));
        nvgFill(vg);
    }
    nvgRestore(vg);
}

void GameGridView::_drawMaterialIcon(NVGcontext* vg, char32_t icon,
                                     float x, float y, float size,
                                     NVGcolor color, int align)
{
    if (m_materialFontId < 0)
        return;
    const std::string text = encodeUtf8(icon);
    nvgFontFaceId(vg, m_materialFontId);
    nvgFontSize(vg, size);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, color);
    nvgText(vg, x, y, text.c_str(), nullptr);
}

void GameGridView::_drawToolbar(NVGcontext* vg, float x, float y, float w)
{
    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 27.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(245, 247, 252, 255));
    nvgText(vg, x + 24.f, y + 27.f, L("游戏库").c_str(), nullptr);

    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 14.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(236, 238, 244, 185));
    nvgText(vg, x + 25.f, y + 54.f, m_detailLabel.c_str(), nullptr);

    const float centerX = x + w * 0.5f;
    const float centerY = y + 35.f;
    const float spacing = 132.f;
    const float eased = 1.f - std::pow(1.f - m_platformTransition, 3.f);
    const float carouselShift = static_cast<float>(m_platformCarouselDirection) *
        spacing * (1.f - eased);
    const int count = static_cast<int>(m_platformLabels.size());

    _drawSwitchButton(vg, brls::BUTTON_LB, centerX - 258.f, centerY,
                      25.f, nvgRGBA(255, 255, 255, 240));
    _drawSwitchButton(vg, brls::BUTTON_RB, centerX + 258.f, centerY,
                      25.f, nvgRGBA(255, 255, 255, 240));

    if (count > 0) {
        const int firstOffset = count == 1 ? 0 : -2;
        const int lastOffset = count == 1 ? 0 : 2;
        for (int relative = firstOffset; relative <= lastOffset; ++relative) {
            int index = (m_platformIndex + relative) % count;
            if (index < 0) index += count;
            const float labelX = centerX + relative * spacing + carouselShift;
            const float distance = std::abs(labelX - centerX) / spacing;
            if (distance > 1.55f)
                continue;
            const float prominence = std::max(0.f, 1.f - distance);
            const float alpha = 0.42f + prominence * 0.58f;
            if (prominence > 0.55f) {
                const float selectorX = labelX - 52.f;
                const float selectorY = centerY - 21.f;
                constexpr float selectorW = 104.f;
                constexpr float selectorH = 42.f;
                constexpr float selectorRadius = 21.f;
                NVGpaint selectorShadow = nvgBoxGradient(
                    vg, selectorX + 3.f, selectorY + 3.f,
                    selectorW, selectorH, selectorRadius, 5.f,
                    nvgRGBA(0, 0, 0,
                        static_cast<unsigned char>(72.f * prominence)),
                    nvgRGBA(0, 0, 0, 0));
                nvgBeginPath(vg);
                nvgRect(vg, selectorX - 2.f, selectorY - 2.f,
                        selectorW + 10.f, selectorH + 10.f);
                nvgRoundedRect(vg, selectorX, selectorY,
                               selectorW, selectorH, selectorRadius);
                nvgPathWinding(vg, NVG_HOLE);
                nvgFillPaint(vg, selectorShadow);
                nvgFill(vg);

                nvgBeginPath(vg);
                nvgRoundedRect(vg, selectorX, selectorY,
                               selectorW, selectorH, selectorRadius);
                nvgFillColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(22 + 22 * prominence)));
                nvgFill(vg);
                strokeRoundedRectInside(
                    vg, selectorX, selectorY, selectorW, selectorH,
                    selectorRadius, 1.f,
                    nvgRGBA(255, 255, 255,
                        static_cast<unsigned char>(70 + 65 * prominence)));
            }
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 17.f + 5.f * prominence);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, beiklive::uiTextPrimary(alpha));
            nvgText(vg, labelX, centerY,
                    m_platformLabels[static_cast<size_t>(index)].c_str(), nullptr);
        }
    }

    _drawSwitchButton(vg, brls::BUTTON_Y, x + w - 145.f, y + 35.f,
                      23.f, beiklive::uiIconPrimary(0.94f));
    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 22.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, beiklive::uiTextPrimary(0.86f));
    nvgText(vg, x + w - 128.f, y + 35.f,
            m_viewMode == ViewMode::GRID ? L("切换为列表").c_str() : L("切换为网格").c_str(), nullptr);

    const float dividerX = x + 18.f;
    const float dividerY = y + _getContentTop() - 1.f;
    const float dividerW = w - 36.f;
    NVGpaint dividerShadow = nvgBoxGradient(
        vg, dividerX + 2.f, dividerY + 2.f, dividerW, 1.f,
        0.5f, 5.f, nvgRGBA(0, 0, 0, 62), nvgRGBA(0, 0, 0, 0));
    nvgBeginPath(vg);
    nvgRect(vg, dividerX - 3.f, dividerY - 3.f,
            dividerW + 10.f, 10.f);
    nvgRect(vg, dividerX, dividerY - 0.5f, dividerW, 1.f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, dividerShadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgMoveTo(vg, dividerX, dividerY);
    nvgLineTo(vg, dividerX + dividerW, dividerY);
    nvgStrokeColor(vg, beiklive::uiDivider());
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);
}

void GameGridView::_drawSwitchButton(NVGcontext* vg, brls::ControllerButton button,
                                     float x, float y, float size, NVGcolor color)
{
    if (m_switchIconFontId < 0)
        return;
    const std::string glyph = brls::Hint::getKeyIcon(button);
    nvgFontFaceId(vg, m_switchIconFontId);
    nvgFontSize(vg, size);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, color);
    nvgText(vg, x, y, glyph.c_str(), nullptr);
}

void GameGridView::_drawHint(NVGcontext* vg, float x, float y,
                             brls::ControllerButton button,
                             const std::string& label, int secondButton)
{
    _drawSwitchButton(vg, button, x + 17.f, y, 30.f,
                      beiklive::uiIconPrimary(0.96f));
    float labelX = x + 38.f;
    if (secondButton >= 0) {
        _drawSwitchButton(vg, static_cast<brls::ControllerButton>(secondButton),
                          x + 47.f, y, 30.f, beiklive::uiIconPrimary(0.96f));
        labelX = x + 69.f;
    }
    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 22.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, beiklive::uiTextPrimary(0.88f));
    nvgText(vg, labelX, y, label.c_str(), nullptr);
}

void GameGridView::_drawFooter(NVGcontext* vg, float x, float y, float w, float h)
{
    const float dividerX = x + 18.f;
    const float dividerY = y + 1.f;
    const float dividerW = w - 36.f;
    NVGpaint dividerShadow = nvgBoxGradient(
        vg, dividerX + 2.f, dividerY + 2.f, dividerW, 1.f,
        0.5f, 5.f, nvgRGBA(0, 0, 0, 62), nvgRGBA(0, 0, 0, 0));
    nvgBeginPath(vg);
    nvgRect(vg, dividerX - 3.f, dividerY - 3.f,
            dividerW + 10.f, 10.f);
    nvgRect(vg, dividerX, dividerY - 0.5f, dividerW, 1.f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, dividerShadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgMoveTo(vg, dividerX, dividerY);
    nvgLineTo(vg, dividerX + dividerW, dividerY);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 52));
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);

    const float cy = y + h * 0.5f;
    constexpr float gap = 44.f;
    float cursor = x + w - 22.f;
    auto drawRightAlignedHint = [&](brls::ControllerButton button,
                                    const std::string& label) {
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 22.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label.c_str(), nullptr, bounds);
        const float hintWidth = 38.f + bounds[2] - bounds[0];
        cursor -= hintWidth;
        _drawHint(vg, cursor, cy, button, label);
        cursor -= gap;
    };

    if (m_multiSelectMode) {
        drawRightAlignedHint(brls::BUTTON_A, L("勾选/取消勾选").c_str());
        drawRightAlignedHint(brls::BUTTON_B, L("退出多选").c_str());
        drawRightAlignedHint(brls::BUTTON_X, L("批量操作").c_str());
        drawRightAlignedHint(brls::BUTTON_START,
            isAllSelectedForDelete(m_items.size()) ? L("取消全选") : L("全选"));
        return;
    }

    drawRightAlignedHint(brls::BUTTON_A, L("选择").c_str());
    drawRightAlignedHint(brls::BUTTON_B, L("返回").c_str());
    drawRightAlignedHint(brls::BUTTON_X, L("多选").c_str());
    drawRightAlignedHint(brls::BUTTON_RT, L("搜索").c_str());
    drawRightAlignedHint(brls::BUTTON_LT, L("排序").c_str());
    if (m_viewMode == ViewMode::LIST) {
        drawRightAlignedHint(brls::BUTTON_LEFT, L("上翻一页").c_str());
        drawRightAlignedHint(brls::BUTTON_RIGHT, L("下翻一页").c_str());
    }
}

void GameGridView::_drawDetailsPanel(NVGcontext* vg, float x, float y,
                                     float w, float h)
{
    if (m_selectedIndex < 0 || static_cast<size_t>(m_selectedIndex) >= m_items.size())
        return;
    const auto& item = m_items[static_cast<size_t>(m_selectedIndex)];
    const float eased = 1.f - std::pow(1.f - m_detailTransition, 3.f);
    const float pageEased = 1.f - std::pow(1.f - m_pageEntrance, 3.f);

    nvgSave(vg);
    nvgGlobalAlpha(vg, (0.12f + pageEased * 0.88f) *
                           (0.45f + eased * 0.55f));
    x += (1.f - eased) * 22.f;

    const float panelY = y + 4.f;
    const float panelH = h - 8.f;
    NVGpaint panelShadow = nvgBoxGradient(
        vg, x + 3.f, panelY + 3.f, w, panelH,
        16.f, 5.f, nvgRGBA(0, 0, 0, 72), nvgRGBA(0, 0, 0, 0));
    nvgBeginPath(vg);
    nvgRect(vg, x - 2.f, panelY - 2.f, w + 10.f, panelH + 10.f);
    nvgRoundedRect(vg, x, panelY, w, panelH, 16.f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, panelShadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, panelY, w, panelH, 16.f);
    nvgFillColor(vg, item.favorite
        ? nvgRGBA(224, 166, 87, 30)
        : nvgRGBA(255, 255, 255, 16));
    nvgFill(vg);
    strokeRoundedRectInside(
        vg, x, panelY, w, panelH, 16.f, 1.f,
        item.favorite
            ? nvgRGBA(224, 166, 87, 145)
            : nvgRGBA(255, 255, 255, 65));

    const float pad = 22.f;
    const float dividerY = y + h * 0.34f;
    const float infoBlockHeight = 108.f;
    const float infoTop = y + 4.f +
        std::max(0.f, (dividerY - (y + 4.f) - infoBlockHeight) * 0.5f);
    nvgSave(vg);
    nvgIntersectScissor(vg, x + pad, infoTop, w - pad * 2.f, 42.f);
    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 25.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, beiklive::uiTextPrimary(0.98f));
    nvgText(vg, x + pad, infoTop + 21.f, item.title.c_str(), nullptr);
    nvgRestore(vg);

    const float badgeX = x + pad;
    const float badgeW = _drawBadge(vg, item, badgeX, infoTop + 49.f);
    const float playTimeX = badgeX + badgeW + 10.f;
    _drawPlayTime(vg, item.playTime, playTimeX, infoTop + 51.f,
                  std::max(1.f, x + w - pad - playTimeX));
    _drawSubText(vg, item.subText, x + pad, infoTop + 81.f, w - pad * 2.f);

    nvgBeginPath(vg);
    nvgMoveTo(vg, x + pad, dividerY);
    nvgLineTo(vg, x + w - pad, dividerY);
    nvgStrokeColor(vg, beiklive::uiDivider());
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);

    nvgFontFaceId(vg, m_fontId);
    nvgFontSize(vg, 16.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, beiklive::uiTextSecondary(0.88f));
    nvgText(vg, x + pad, dividerY + 24.f, L("封面预览").c_str(), nullptr);
    _drawImage(vg, item, x + pad, dividerY + 43.f,
               w - pad * 2.f, y + h - 18.f - (dividerY + 43.f));
    nvgRestore(vg);
}

void GameGridView::_drawScrollbar(NVGcontext* vg, float x, float y, float w, float h)
{
    if (m_maxScrollY <= 0.f) return;

    float totalH = _getRowCount() * _getRowHeight() + m_paddingTop;
    if (totalH <= 0.f) return;

    float barH = h * (h / totalH);
    barH = std::max(barH, 20.f);
    float barY = y + (m_scrollY / m_maxScrollY) * (h - barH);
    float barX = x + w - 5.f;
    float barW = 3.f;

    nvgBeginPath(vg);
    nvgRoundedRect(vg, barX, barY, barW, barH, 1.5f);
    nvgFillColor(vg, nvgRGBA(180, 180, 180, 120));
    nvgFill(vg);
}
