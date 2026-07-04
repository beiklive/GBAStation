#include "nds_stub/NdsMenuLayer.hpp"

#include <algorithm>
#include <switch.h>

#include "nds_stub/ui/UiComponents.hpp"

namespace beiklive::nds_stub {

using namespace ui;

namespace {

constexpr float kPanelAnimationMs = 220.0f;

bool isDirectionUp(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyUp) != 0;
}

bool isDirectionDown(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyDown) != 0;
}

bool isDirectionLeft(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyLeft) != 0;
}

bool isDirectionRight(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyRight) != 0;
}

} // namespace

void NdsMenuLayer::beginSelectionAnimation(int oldSelected, int newSelected)
{
    if (oldSelected == newSelected)
        return;

    m_previousSelected = oldSelected;
    m_selected = newSelected;
    m_selectionAnimStartTick = armGetSystemTick();
    m_selectionAnimating = true;
    m_focusScope = FocusScope::Tabs;
    m_contentFocus = 0;
}

void NdsMenuLayer::beginPanelAnimation(bool opening)
{
    m_panelOpening = opening;
    m_panelAnimating = true;
    m_panelAnimStartTick = armGetSystemTick();
}

void NdsMenuLayer::open()
{
    if (m_visible && !m_panelAnimating)
        return;
    m_visible = true;
    m_focusScope = FocusScope::Tabs;
    m_contentFocus = 0;
    m_previousSelected = m_selected;
    m_selectionAnimStartTick = armGetSystemTick();
    m_selectionAnimating = true;
    beginPanelAnimation(true);
}

void NdsMenuLayer::close()
{
    if (!m_visible && !m_panelAnimating)
        return;
    m_visible = false;
    m_focusScope = FocusScope::Tabs;
    m_contentFocus = 0;
    beginPanelAnimation(false);
}

void NdsMenuLayer::toggle()
{
    if (m_visible)
        close();
    else
        open();
}

bool NdsMenuLayer::active() const
{
    if (m_visible)
        return true;
    return m_panelAnimating && animationProgress(m_panelAnimStartTick, kPanelAnimationMs) < 1.0f;
}

float NdsMenuLayer::panelProgress() const
{
    if (!m_panelAnimating)
        return m_visible ? 1.0f : 0.0f;

    const float progress = easeOutCubic(animationProgress(m_panelAnimStartTick, kPanelAnimationMs));
    return m_panelOpening ? progress : 1.0f - progress;
}

void NdsMenuLayer::setFastForwardMultiplier(int multiplier)
{
    m_fastForwardMultiplier = std::clamp(multiplier, 1, 10);
}

bool NdsMenuLayer::itemHasContent(Item item) const
{
    return item == Item::SaveState || item == Item::LoadState ||
           item == Item::Cheats || item == Item::Display;
}

int NdsMenuLayer::contentControlCount(Item item) const
{
    switch (item)
    {
    case Item::SaveState:
    case Item::LoadState:
        return 6;
    case Item::Display:
        return 2;
    case Item::Cheats:
        return 1;
    default:
        return 0;
    }
}

NdsMenuResult NdsMenuLayer::update(std::uint64_t buttonsDown)
{
    if (!active())
        return {};

    if (m_panelAnimating && animationProgress(m_panelAnimStartTick, kPanelAnimationMs) >= 1.0f)
        m_panelAnimating = false;

    if (!m_visible)
        return {};

    if (buttonsDown & HidNpadButton_B)
    {
        if (m_focusScope == FocusScope::Content)
        {
            m_focusScope = FocusScope::Tabs;
            return {};
        }

        close();
        return {};
    }

    const int itemCount = itemIndex(Item::Count);
    if (m_focusScope == FocusScope::Tabs && isDirectionUp(buttonsDown))
    {
        beginSelectionAnimation(m_selected, (m_selected + itemCount - 1) % itemCount);
        return {};
    }
    if (m_focusScope == FocusScope::Tabs && isDirectionDown(buttonsDown))
    {
        beginSelectionAnimation(m_selected, (m_selected + 1) % itemCount);
        return {};
    }

    const Item currentItem = static_cast<Item>(m_selected);
    if (m_focusScope == FocusScope::Content)
    {
        if (currentItem == Item::SaveState || currentItem == Item::LoadState)
        {
            const int col = m_contentFocus % 2;
            if (isDirectionLeft(buttonsDown) && col > 0)
                --m_contentFocus;
            else if (isDirectionRight(buttonsDown) && col < 1 && m_contentFocus + 1 < contentControlCount(currentItem))
                ++m_contentFocus;
            else if (isDirectionUp(buttonsDown) && m_contentFocus >= 2)
                m_contentFocus -= 2;
            else if (isDirectionDown(buttonsDown) && m_contentFocus + 2 < contentControlCount(currentItem))
                m_contentFocus += 2;

            if (buttonsDown & HidNpadButton_A)
            {
                return {currentItem == Item::SaveState ? NdsMenuAction::SaveState : NdsMenuAction::LoadState,
                        m_contentFocus};
            }
            return {};
        }

        if (currentItem == Item::Display)
        {
            if (isDirectionUp(buttonsDown) || isDirectionDown(buttonsDown))
                m_contentFocus = m_contentFocus == 0 ? 1 : 0;

            if (isDirectionLeft(buttonsDown) || isDirectionRight(buttonsDown))
                return cycleCurrentSetting(isDirectionRight(buttonsDown) ? 1 : -1)
                    ? NdsMenuResult{NdsMenuAction::DisplaySettingsChanged, -1}
                    : NdsMenuResult{};

            if ((buttonsDown & HidNpadButton_A) && m_contentFocus == 0)
            {
                m_linearFiltering = !m_linearFiltering;
                return {NdsMenuAction::DisplaySettingsChanged, -1};
            }
            return {};
        }

        return {};
    }

    if (buttonsDown & HidNpadButton_A)
    {
        switch (currentItem)
        {
        case Item::Resume:
            close();
            return {};
        case Item::SaveState:
        case Item::LoadState:
        case Item::Cheats:
        case Item::Display:
            if (itemHasContent(currentItem))
            {
                m_focusScope = FocusScope::Content;
                m_contentFocus = 0;
            }
            return {};
        case Item::Reset:
            return {NdsMenuAction::ResetGame, -1};
        case Item::Exit:
            return {NdsMenuAction::ExitGame, -1};
        default:
            return {};
        }
    }

    return {};
}

bool NdsMenuLayer::cycleCurrentSetting(int direction)
{
    if (static_cast<Item>(m_selected) != Item::Display)
        return false;

    m_fastForwardMultiplier = std::clamp(m_fastForwardMultiplier + direction, 1, 10);
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

    if (!active())
        return;

    const float panel = panelProgress();
    if (panel <= 0.0f)
        return;

    const float slideY = (1.0f - panel) * kScreenH;
    const float selectionProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;
    const float pageProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;

    const bool contentFocused = m_focusScope == FocusScope::Content;
    drawOverlay(panel);
    drawHeader(slideY);
    drawLeftMenu(m_selected, m_previousSelected, selectionProgress, !contentFocused, slideY);
    drawLine({kSeparatorX, 110.0f + slideY}, {1.0f, 500.0f}, {1.0f, 1.0f, 1.0f, 0.08f});
    drawTabFrame(static_cast<Item>(m_selected),
                 static_cast<Item>(m_previousSelected),
                 pageProgress,
                 m_linearFiltering,
                 m_fastForwardMultiplier,
                 m_contentFocus,
                 contentFocused,
                 slideY);
    drawFooter(contentFocused, slideY);
}

} // namespace beiklive::nds_stub
