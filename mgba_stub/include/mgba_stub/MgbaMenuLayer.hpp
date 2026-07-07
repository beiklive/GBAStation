#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace beiklive::mgba_stub {

enum class MgbaMenuAction {
    None,
    SaveState,
    LoadState,
    DeleteState,
    ResetGame,
    ExitGame,
};

struct MgbaMenuResult {
    MgbaMenuAction action = MgbaMenuAction::None;
    int slot = -1;
};

struct MgbaStateSlotInfo {
    bool exists = false;
    bool loadable = false;
    std::string statePath;
    std::string modifiedTime;
};

class MgbaMenuLayer {
public:
    enum class Item {
        Resume,
        SaveState,
        LoadState,
        Cheats,
        Display,
        Reset,
        Exit,
        Count,
    };

    MgbaMenuResult update(std::uint64_t buttonsDown, std::uint64_t buttonsHeld);
    void draw() const;
    void setStateSlots(const std::array<MgbaStateSlotInfo, 10>& slots);

    void open();
    void close();
    void toggle();
    bool visible() const { return m_visible; }
    bool active() const { return m_visible || m_panelAnimating || m_deleteDialogVisible; }

    void showToast(const std::string& message);
    void clearToast();

private:
    enum class FocusScope {
        Tabs,
        Content,
    };

    int contentControlCount(Item item) const;
    bool itemHasContent(Item item) const;
    void beginSelectionAnimation(int oldSelected, int newSelected);
    void beginPanelAnimation(bool opening);
    float panelProgress() const;
    void openDeleteDialog();
    void closeDeleteDialog();
    void resetContentScroll();
    float smoothedContentScrollY() const;

    bool m_visible = false;
    int m_selected = 0;
    int m_previousSelected = 0;
    FocusScope m_focusScope = FocusScope::Tabs;
    int m_contentFocus = 0;
    std::array<MgbaStateSlotInfo, 10> m_slots {};
    bool m_deleteDialogVisible = false;
    int m_deleteSlot = -1;
    std::uint64_t m_selectionAnimStartTick = 0;
    mutable bool m_selectionAnimating = false;
    std::uint64_t m_panelAnimStartTick = 0;
    mutable bool m_panelAnimating = false;
    bool m_panelOpening = false;
    mutable float m_contentScrollY = 0.0f;
    mutable std::uint64_t m_contentScrollLastTick = 0;
    mutable std::string m_toastMessage;
    mutable std::uint64_t m_toastStartTick = 0;
};

void drawMgbaGameStatusBadges(double fps,
                              bool showFps,
                              bool fastForwardActive,
                              bool showFastForward,
                              bool paused);

} // namespace beiklive::mgba_stub
