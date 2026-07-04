#include "nds_stub/NdsMenuLayer.hpp"

#include <algorithm>
#include <switch.h>

#include "nds_stub/ui/UiComponents.hpp"

namespace beiklive::nds_stub {

using namespace ui;

void NdsMenuLayer::beginSelectionAnimation(int oldSelected, int newSelected)
{
    if (oldSelected == newSelected)
        return;

    m_previousSelected = oldSelected;
    m_selected = newSelected;
    m_selectionAnimStartTick = armGetSystemTick();
    m_selectionAnimating = true;
}

NdsMenuAction NdsMenuLayer::update(std::uint64_t buttonsDown)
{
    if (buttonsDown & HidNpadButton_ZR)
    {
        m_visible = !m_visible;
        m_previousSelected = m_selected;
        m_selectionAnimStartTick = armGetSystemTick();
        m_selectionAnimating = true;
        return NdsMenuAction::None;
    }

    if (!m_visible)
        return NdsMenuAction::None;

    if (buttonsDown & HidNpadButton_B)
    {
        m_visible = false;
        return NdsMenuAction::None;
    }

    const int itemCount = itemIndex(Item::Count);
    if (buttonsDown & HidNpadButton_AnyUp)
    {
        beginSelectionAnimation(m_selected, (m_selected + itemCount - 1) % itemCount);
        return NdsMenuAction::None;
    }
    if (buttonsDown & HidNpadButton_AnyDown)
    {
        beginSelectionAnimation(m_selected, (m_selected + 1) % itemCount);
        return NdsMenuAction::None;
    }

    if (buttonsDown & HidNpadButton_AnyLeft)
        return cycleCurrentSetting(-1) ? NdsMenuAction::DisplaySettingsChanged : NdsMenuAction::None;
    if (buttonsDown & HidNpadButton_AnyRight)
        return cycleCurrentSetting(1) ? NdsMenuAction::DisplaySettingsChanged : NdsMenuAction::None;

    if (buttonsDown & HidNpadButton_A)
    {
        switch (static_cast<Item>(m_selected))
        {
        case Item::Resume:
            m_visible = false;
            return NdsMenuAction::None;
        case Item::Display:
            m_linearFiltering = !m_linearFiltering;
            return NdsMenuAction::DisplaySettingsChanged;
        case Item::Reset:
            return NdsMenuAction::ResetGame;
        case Item::Exit:
            return NdsMenuAction::ExitGame;
        default:
            return NdsMenuAction::None;
        }
    }

    return NdsMenuAction::None;
}

bool NdsMenuLayer::cycleCurrentSetting(int direction)
{
    if (static_cast<Item>(m_selected) != Item::Display)
        return false;

    m_fastForwardMultiplier = std::clamp(m_fastForwardMultiplier + direction, 1, 4);
    return true;
}

void NdsMenuLayer::draw(double fps, long long runMs, bool fastForwardActive) const
{
    Gfx::DrawText(Gfx::SystemFontStandard,
                  {28.0f, 24.0f},
                  20.0f,
                  {0.78f, 0.90f, 1.0f, 1.0f},
                  "FPS %.1f  RUN %lldMS  FF x%d%s  %s",
                  fps,
                  runMs,
                  m_fastForwardMultiplier,
                  fastForwardActive ? "*" : "",
                  filterLabel(m_linearFiltering));

    if (!m_visible)
        return;

    const float selectionProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;
    const float pageProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;

    drawOverlay();
    drawHeader();
    drawLeftMenu(m_selected, m_previousSelected, selectionProgress);
    drawLine({kSeparatorX, 110.0f}, {1.0f, 500.0f}, {1.0f, 1.0f, 1.0f, 0.08f});
    drawTabFrame(static_cast<Item>(m_selected),
                 static_cast<Item>(m_previousSelected),
                 pageProgress,
                 m_linearFiltering,
                 m_fastForwardMultiplier);
    drawFooter();
}

} // namespace beiklive::nds_stub
