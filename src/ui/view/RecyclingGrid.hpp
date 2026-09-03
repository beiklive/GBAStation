#pragma once

#include <borealis.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <chrono>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

#include "RecyclingGridItem.hpp"
#include "RecyclingGridDataSource.hpp"

class GameGridView : public brls::View {
public:
    enum class ViewMode : int {
        GRID = 0,
        LIST = 1,
    };

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
    void setInteractionDisabled(bool disabled) {
        if (!disabled && isDeleteAnimationRunning())
            return;
        m_interactionDisabled = disabled;
    }
    void setTitleFontSize(int opt);
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }
    void toggleViewMode();
    void setLibraryContext(std::string category, std::string detail);
    void setPlatformCarousel(std::vector<std::string> labels, int selected, int direction);
    void startContentTransition(int direction = 0, bool wholePageSlide = false);
    void restartEntranceAnimation();
    void showLoadingSkeleton();
    void playLaunchAnimation(size_t index, std::function<void()> completion,
                             bool startCentered = false);
    void resetLaunchAnimation();
    void playExitAnimation(std::function<void()> completion);

    void setMultiSelectMode(bool on);
    bool isMultiSelectMode() const { return m_multiSelectMode; }
    bool isDeleteAnimationRunning() const {
        return m_deleteWaiting || m_deleteCollapsing || m_deleteBackendFinished ||
            m_reflowPending || m_reflowTransition < 1.f;
    }
    void toggleDeleteSelection(size_t index);
    void selectAllForDelete(size_t count);
    void deselectAllForDelete();
    bool isAllSelectedForDelete(size_t count) const;
    const std::unordered_set<int>& getDeleteSelection() const { return m_selectedForDelete; }
    void clearDeleteSelection();
    void setItemFavourite(size_t index, bool fav);
    void setItemTitle(size_t index, const std::string& title);
    void setItemImagePath(size_t index, const std::string& path);
    void beginDeleteAnimation(const std::vector<int>& indices);
    void completeDeleteAnimation(std::function<void()> completion);
    void cancelDeleteAnimation();

    void setPadding(float top, float right, float bottom, float left);

    int spanCount = 3;
    float estimatedRowHeight = 120.f;
    float estimatedRowSpace = 8.f;

private:
    GameGridDataSource* m_dataSource = nullptr;

    std::vector<GridDrawItem> m_items;

    int m_selectedIndex = 0;
    int m_preferredColumn = 0;
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
    bool m_hasPresentedData = false;
    bool m_loadingSkeleton = false;
    bool m_launchAnimationRunning = false;
    bool m_launchStartsCentered = false;
    bool m_deleteWaiting = false;
    bool m_deleteCollapsing = false;
    bool m_deleteBackendFinished = false;
    bool m_exitAnimationRunning = false;
    ViewMode m_viewMode = ViewMode::GRID;

    float m_shakeTime = 0.f;
    float m_shakeDir = 0.f;
    float m_focusBorderAnimTime = 0.f;
    float m_platformTransition = 1.f;
    float m_contentTransition = 1.f;
    float m_detailTransition = 1.f;
    float m_pageEntrance = 0.f;
    float m_launchAnimationTime = 0.f;
    float m_deleteAnimationTime = 0.f;
    float m_deleteCollapseProgress = 0.f;
    float m_reflowTransition = 1.f;
    int m_launchItemIndex = -1;
    int m_platformSlideDirection = 0;
    int m_platformCarouselDirection = 0;
    int m_contentSlideDirection = 0;
    bool m_contentWholePageSlide = false;

    std::function<void()> m_nextPageCallback;
    std::function<void(int)> m_focusChangeCallback;
    std::function<void()> m_exitCompletion;
    std::function<void()> m_launchCompletion;
    std::function<void()> m_deleteCompletion;

    std::unordered_map<std::string, int> m_textureCache;
    std::unordered_map<std::string, uint64_t> m_textureLastUsed;
    std::unordered_map<std::string, size_t> m_textureMemoryBytes;
    size_t m_textureCacheBytes = 0;
    uint64_t m_textureUseTick = 0;
    std::unordered_set<int> m_selectedForDelete;
    std::unordered_set<int> m_deletingIndices;
    std::vector<int> m_reflowOrigins;
    int m_reflowFirstMovedIndex = 0;
    bool m_reflowPending = false;

    int m_fontId = -1;
    int m_materialFontId = -1;
    int m_switchIconFontId = -1;
    int m_titleFontSize = 16;
    int m_favIconHandle = -1;
    std::string m_categoryLabel = "所有";
    std::string m_detailLabel;
    std::vector<std::string> m_platformLabels{"所有"};
    int m_platformIndex = 0;

    struct DecodedTexture {
        std::string path;
        uint64_t generation = 0;
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
        bool failed = false;
    };

    struct TextureLoaderState {
        std::atomic<bool> alive{true};
        std::mutex mutex;
        std::deque<DecodedTexture> ready;
        uint64_t generation = 0;
        std::unordered_set<std::string> wanted;
        std::unordered_map<std::string, uint64_t> pending;
    };

    std::shared_ptr<TextureLoaderState> m_textureLoader;
    std::unordered_set<std::string> m_failedTextures;
    int m_requestedStartRow = -1;
    int m_requestedEndRow = -1;
    int m_requestedScrollDirection = 1;
    ViewMode m_requestedViewMode = ViewMode::GRID;
    uint64_t m_requestedGameId = 0;

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
    int m_scrollDirection = 1;

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
    void _movePageUp();
    void _movePageDown();
    bool _tryMoveUp();
    bool _tryMoveDown();
    bool _tryMoveLeft();
    bool _tryMoveRight();
    bool _tryMovePageUp();
    bool _tryMovePageDown();
    void _captureInputState();

    void _drawItem(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, bool focused, int idx);
    void _drawSkeletonItem(NVGcontext* vg, float x, float y, float w, float h, int idx);
    void _drawLaunchOverlay(NVGcontext* vg, float x, float y, float w, float h);
    void _drawImage(NVGcontext* vg, const GridDrawItem& item, float x, float y,
                    float boxW, float boxH, bool platformDefault = false);
    float _drawBadge(NVGcontext* vg, const GridDrawItem& item, float x, float y);
    void _drawTitle(NVGcontext* vg, const GridDrawItem& item, float x, float y, float maxWidth, bool focused);
    void _drawSubText(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth);
    void _drawPlayTime(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth);
    void _drawEmptyItem(NVGcontext* vg, float x, float y, float w, float h);
    void _drawScrollbar(NVGcontext* vg, float x, float y, float w, float h);
    void _drawFavourite(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, float sx, float sy);
    void _drawToolbar(NVGcontext* vg, float x, float y, float w);
    void _drawFooter(NVGcontext* vg, float x, float y, float w, float h);
    void _drawDetailsPanel(NVGcontext* vg, float x, float y, float w, float h);
    void _drawSwitchButton(NVGcontext* vg, brls::ControllerButton button,
                           float x, float y, float size, NVGcolor color);
    void _drawHint(NVGcontext* vg, float x, float y, brls::ControllerButton button,
                   const std::string& label, int secondButton = -1);
    void _drawMaterialIcon(NVGcontext* vg, char32_t icon, float x, float y,
                           float size, NVGcolor color, int align);
    void _requestTexture(const std::string& path,
                         const std::string& sourcePath = "",
                         const std::string& fallbackPath = "");
    void _uploadDecodedTextures(NVGcontext* vg);
    void _populateItem(size_t index);
    void _populateVisibleItems();

    float _getItemX(int col);
    float _getItemY(int row);
    float _getItemWidth();
    float _getRowHeight();
    int _getRowCount();
    float _getContentTop() const;
    float _getFooterHeight() const;
    float _getViewportHeight();
    float _getListPaneWidth();
    void _recalculateScrollBounds();

    NVGcolor _getBadgeColor(PlatformBadgeColor color) const;
};
