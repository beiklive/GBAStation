#pragma once
#include <borealis.hpp>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

#include "core/common.h"

namespace beiklive {

class FileListView : public brls::View {
public:
    FileListView();
    ~FileListView() override;

    void setItems(const std::vector<beiklive::ListItem>& items);
    void clearItems();
    bool focusItemByFilename(const std::string& filename);

    void setInteractionDisabled(bool disabled) { m_interactionDisabled = disabled; if (!disabled) _captureInputState(); }
    int getFocusedIndex() const { return m_focusedIndex; }

    void saveFocusState(const std::string& path);
    void restoreFocusState(const std::string& path);

    void applyFilter(const std::string& keyword);
    void removeFilter();
    bool hasActiveFilter() const { return m_filterActive; }
    int itemCount() const { return (int)m_items.size(); }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void frame(brls::FrameContext* ctx) override;

    std::function<void(const beiklive::ListItem&)> onItemClicked;
    std::function<void(const beiklive::ListItem&)> onItemFocused;
    std::function<void(const beiklive::ListItem&)> onItemFocusLost;

private:
    std::vector<beiklive::ListItem> m_items;
    std::vector<beiklive::ListItem> m_unfilteredItems;
    bool m_filterActive = false;
    int m_font = -1;
    int m_focusedIndex = -1;
    float m_scrollY = 0.f;
    float m_targetScrollY = 0.f;
    float m_viewHeight = 0.f;
    float m_lastLayoutHeight = 0.f;
    float m_itemHeight = 72.f;
    float m_iconSize = 48.f;
    float m_animTime = 0.f;
    float m_shakeTime = 0.f;
    int m_shakeDir = 0;

    bool m_interactionDisabled = false;

    // Input state
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
    float m_holdUpRepeat = 0.f;
    float m_holdDownRepeat = 0.f;
    float m_holdLeftTime = 0.f;
    float m_holdRightTime = 0.f;
    float m_holdLeftRepeat = 0.f;
    float m_holdRightRepeat = 0.f;

    static constexpr float HOLD_INITIAL_DELAY = 0.3f;
    static constexpr float HOLD_REPEAT = 0.08f;
    static constexpr float HOLD_REPEAT_FAST = 0.03f;
    static constexpr float HOLD_ACCEL_TIME = 1.5f;

    std::chrono::steady_clock::time_point m_lastFrameTime;

    // Focus state per directory
    std::unordered_map<std::string, int> m_dirFocusIndex;

    // Icon cache: path -> NVG image handle
    std::unordered_map<std::string, int> m_iconCache;
    std::unordered_set<std::string> m_failedIconPaths;

    void moveUp();
    void moveDown();
    void movePageUp();
    void movePageDown();
    void _captureInputState();

    void ensureFocusedVisible();
    void clampScroll();
    int visibleRows() const;
    void fireFocusCallbacks(int oldIndex);

    void loadVisibleIcons(NVGcontext* vg, int first, int last);
    int getCachedIcon(const std::string& path) const;
    void drawItem(NVGcontext* vg, int index, float itemY, float w, NVGcolor textColor);
    void drawScrollbar(NVGcontext* vg, float x, float y, float w, float h);
};

} // namespace beiklive
