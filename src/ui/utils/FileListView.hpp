#pragma once
#include <borealis.hpp>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <unordered_map>

#include "ListItem.hpp"
#include "core/common.h"

namespace beiklive {

class FileListView : public brls::View {
public:
    FileListView();

    void setItems(const std::vector<beiklive::ListItem>& items);
    void clearItems();

    void setInteractionDisabled(bool disabled) { m_interactionDisabled = disabled; }
    int getFocusedIndex() const { return m_focusedIndex; }

    void saveFocusState(const std::string& path);
    void restoreFocusState(const std::string& path);

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void frame(brls::FrameContext* ctx) override;

    std::function<void(const beiklive::ListItem&)> onItemClicked;
    std::function<void(const beiklive::ListItem&)> onItemFocused;
    std::function<void(const beiklive::ListItem&)> onItemFocusLost;

private:
    std::vector<beiklive::ListItem> m_items;

    int m_focusedIndex = -1;
    float m_scrollY = 0.f;
    float m_viewHeight = 0.f;
    float m_itemHeight = 72.f;
    float m_iconSize = 48.f;

    bool m_interactionDisabled = false;

    // Input state
    bool m_prevUp = false;
    bool m_prevDown = false;
    bool m_prevLeft = false;
    bool m_prevRight = false;
    bool m_prevA = false;
    float m_holdUpTime = 0.f;
    float m_holdDownTime = 0.f;
    float m_holdUpRepeat = 0.f;
    float m_holdDownRepeat = 0.f;

    static constexpr float HOLD_INITIAL_DELAY = 0.3f;
    static constexpr float HOLD_REPEAT = 0.08f;
    static constexpr float HOLD_REPEAT_FAST = 0.03f;
    static constexpr float HOLD_ACCEL_TIME = 1.5f;

    std::chrono::steady_clock::time_point m_lastFrameTime;

    // Focus state per directory
    std::unordered_map<std::string, int> m_dirFocusIndex;

    // Icon cache: path -> NVG image handle
    std::unordered_map<std::string, int> m_iconCache;

    void moveUp();
    void moveDown();
    void movePageUp();
    void movePageDown();

    void ensureFocusedVisible();
    int visibleRows() const;
    void fireFocusCallbacks(int oldIndex);

    int getOrLoadIcon(NVGcontext* vg, const std::string& path);
    void drawItem(NVGcontext* vg, int index, float itemY, float w, NVGcolor textColor);
    void drawScrollbar(NVGcontext* vg, float x, float y, float w, float h);
};

} // namespace beiklive
