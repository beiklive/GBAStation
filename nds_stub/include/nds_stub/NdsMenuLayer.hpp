#pragma once

#include <cstdint>

namespace beiklive::nds_stub {

enum class NdsMenuAction {
    None,
    DisplaySettingsChanged,
    ResetGame,
    ExitGame,
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

    NdsMenuAction update(std::uint64_t buttonsDown);
    void draw(double fps, long long runMs, bool fastForwardActive) const;

    bool visible() const { return m_visible; }
    void close() { m_visible = false; }
    bool linearFiltering() const { return m_linearFiltering; }
    int fastForwardMultiplier() const { return m_fastForwardMultiplier; }

private:
    bool cycleCurrentSetting(int direction);
    void beginSelectionAnimation(int oldSelected, int newSelected);

    bool m_visible = false;
    int m_selected = 0;
    bool m_linearFiltering = false;
    int m_fastForwardMultiplier = 1;
    int m_previousSelected = 0;
    std::uint64_t m_selectionAnimStartTick = 0;
    bool m_selectionAnimating = false;
};

} // namespace beiklive::nds_stub
