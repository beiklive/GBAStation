#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace beiklive::nds_stub {

enum class NdsMenuAction {
    None,
    SaveState,
    LoadState,
    DeleteState,
    DisplaySettingsChanged,
    ResetGame,
    ExitGame,
};

struct NdsMenuResult {
    NdsMenuAction action = NdsMenuAction::None;
    int slot = -1;
};

struct NdsStateSlotInfo {
    bool exists = false;
    std::string statePath;
    std::string thumbnailPath;
    std::string modifiedTime;
};

struct NdsDisplaySettings {
    float fastForwardMultiplier = 1.0f;
    bool linearFiltering = false;
    bool integerScale = false;
    int layout = 0;
    int orientation = 0;
};

class NdsMenuLayer {
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

    NdsMenuResult update(std::uint64_t buttonsDown, std::uint64_t buttonsHeld);
    void draw(double fps, long long runMs, bool fastForwardActive) const;
    void setStateSlots(const std::array<NdsStateSlotInfo, 10>& slots);

    void open();
    void close();
    void toggle();
    bool visible() const { return m_visible; }
    bool active() const;
    bool linearFiltering() const { return m_display.linearFiltering; }
    float fastForwardMultiplier() const { return m_display.fastForwardMultiplier; }
    void setFastForwardMultiplier(float multiplier);

private:
    enum class FocusScope {
        Tabs,
        Content,
    };

    bool cycleCurrentSetting(int direction);
    bool activateDisplayControl();
    void beginSelectionAnimation(int oldSelected, int newSelected);
    void beginPanelAnimation(bool opening);
    float panelProgress() const;
    bool itemHasContent(Item item) const;
    int contentControlCount(Item item) const;
    int nextFocusableDisplayRow(int from, int direction) const;
    bool updateHeldSelector(std::uint64_t buttonsHeld);
    void openDeleteDialog();
    void closeDeleteDialog();

    bool m_visible = false;
    int m_selected = 0;
    FocusScope m_focusScope = FocusScope::Tabs;
    int m_contentFocus = 0;
    NdsDisplaySettings m_display {};
    std::array<NdsStateSlotInfo, 10> m_slots {};
    int m_previousSelected = 0;
    std::uint64_t m_selectionAnimStartTick = 0;
    bool m_selectionAnimating = false;
    std::uint64_t m_panelAnimStartTick = 0;
    bool m_panelAnimating = false;
    bool m_panelOpening = false;
    bool m_deleteDialogVisible = false;
    int m_deleteSlot = -1;
    std::uint64_t m_selectorRepeatStartTick = 0;
    std::uint64_t m_selectorLastStepTick = 0;
    int m_selectorDirection = 0;
};

} // namespace beiklive::nds_stub
