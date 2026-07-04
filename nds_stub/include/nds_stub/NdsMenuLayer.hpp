#pragma once

#include <cstdint>

namespace beiklive::nds_stub {

enum class NdsMenuAction {
    None,
    SaveState,
    LoadState,
    DisplaySettingsChanged,
    ResetGame,
    ExitGame,
};

struct NdsMenuResult {
    NdsMenuAction action = NdsMenuAction::None;
    int slot = -1;
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

    NdsMenuResult update(std::uint64_t buttonsDown);
    void draw(double fps, long long runMs, bool fastForwardActive) const;

    void open();
    void close();
    void toggle();
    bool visible() const { return m_visible; }
    bool active() const;
    bool linearFiltering() const { return m_linearFiltering; }
    int fastForwardMultiplier() const { return m_fastForwardMultiplier; }
    void setFastForwardMultiplier(int multiplier);

private:
    enum class FocusScope {
        Tabs,
        Content,
    };

    bool cycleCurrentSetting(int direction);
    void beginSelectionAnimation(int oldSelected, int newSelected);
    void beginPanelAnimation(bool opening);
    float panelProgress() const;
    bool itemHasContent(Item item) const;
    int contentControlCount(Item item) const;

    bool m_visible = false;
    int m_selected = 0;
    FocusScope m_focusScope = FocusScope::Tabs;
    int m_contentFocus = 0;
    bool m_linearFiltering = false;
    int m_fastForwardMultiplier = 1;
    int m_previousSelected = 0;
    std::uint64_t m_selectionAnimStartTick = 0;
    bool m_selectionAnimating = false;
    std::uint64_t m_panelAnimStartTick = 0;
    bool m_panelAnimating = false;
    bool m_panelOpening = false;
};

} // namespace beiklive::nds_stub
