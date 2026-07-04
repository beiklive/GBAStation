#include "nds_stub/NdsMenuLayer.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <switch.h>

#include "nds_stub/ui/UiComponents.hpp"

namespace beiklive::nds_stub {

using namespace ui;

namespace {

constexpr float kPanelAnimationMs = 220.0f;
constexpr float kSelectorInitialDelayMs = 320.0f;
constexpr float kSaveCardH = 94.0f;
constexpr float kSaveCardGapY = 14.0f;
constexpr float kSettingRowH = 42.0f;
constexpr float kSettingStepY = 48.0f;

constexpr float kFastForwardValues[] = {
    0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f,
};

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

float focusedScroll(float focusedTop, float focusedH, float contentH)
{
    const float bodyH = contentBodyHeight();
    const float maxScroll = std::max(0.0f, contentH - bodyH);
    float scroll = 0.0f;
    if (focusedTop + focusedH > bodyH)
        scroll = focusedTop + focusedH - bodyH;
    if (focusedTop < scroll)
        scroll = focusedTop;
    return std::clamp(scroll, 0.0f, maxScroll);
}

float displayRowY(int row)
{
    switch (row)
    {
    case 0: return 0.0f;
    case 1: return kSettingStepY;
    case 2: return kSettingStepY * 2.0f;
    case 3: return kSettingStepY * 3.0f;
    case 4: return kSettingStepY * 4.0f;
    case 5: return kSettingStepY * 5.0f;
    case 6: return kSettingStepY * 6.0f + 36.0f;
    case 7: return kSettingStepY * 7.0f + 36.0f;
    case 8: return kSettingStepY * 8.0f + 72.0f;
    case 9: return kSettingStepY * 9.0f + 72.0f;
    case 10: return kSettingStepY * 10.0f + 72.0f;
    default: return 0.0f;
    }
}

bool pushMenuOrientationTransform(int orientation)
{
    orientation = std::clamp(orientation, 0, 3);
    if (orientation == 0)
        return false;

    if (orientation == 1)
    {
        Gfx::PushDrawTransform(0.0f, -1.0f, 1280.0f, 1.0f, 0.0f, 0.0f);
        return true;
    }
    if (orientation == 2)
    {
        Gfx::PushDrawTransform(-1.0f, 0.0f, 1280.0f, 0.0f, -1.0f, 720.0f);
        return true;
    }

    Gfx::PushDrawTransform(0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 720.0f);
    return true;
}

} // namespace

void NdsMenuLayer::setStateSlots(const std::array<NdsStateSlotInfo, 10>& slots)
{
    m_slots = slots;
}

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
    resetContentScroll();
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
    resetContentScroll();
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
    resetContentScroll();
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

void NdsMenuLayer::resetContentScroll()
{
    m_contentScrollY = 0.0f;
    m_contentScrollLastTick = 0;
}

float NdsMenuLayer::targetContentScrollY() const
{
    setMenuMetricsOrientation(m_display.orientation);
    const Item item = static_cast<Item>(m_selected);
    switch (item)
    {
    case Item::SaveState:
    case Item::LoadState:
    {
        const int columns = saveSlotColumns();
        const int rows = (10 + columns - 1) / columns;
        const int row = std::clamp(m_contentFocus, 0, 9) / columns;
        const float focusedTop = row * (saveCardHeight() + saveCardGapY());
        const float contentH = static_cast<float>(rows) * saveCardHeight() +
            static_cast<float>(std::max(0, rows - 1)) * saveCardGapY();
        return focusedScroll(focusedTop, saveCardHeight(), contentH);
    }
    case Item::Display:
    {
        const int row = std::clamp(m_contentFocus, 0, 10);
        const float contentH = displayRowY(10) + kSettingRowH;
        return focusedScroll(displayRowY(row), kSettingRowH, contentH);
    }
    default:
        return 0.0f;
    }
}

float NdsMenuLayer::smoothedContentScrollY() const
{
    const float target = targetContentScrollY();
    const std::uint64_t now = armGetSystemTick();
    if (m_contentScrollLastTick == 0)
    {
        m_contentScrollLastTick = now;
        m_contentScrollY = target;
        return m_contentScrollY;
    }

    const float dtMs = static_cast<float>(armTicksToNs(now - m_contentScrollLastTick)) / 1000000.0f;
    m_contentScrollLastTick = now;
    const float t = 1.0f - std::exp(-dtMs / 72.0f);
    m_contentScrollY += (target - m_contentScrollY) * std::clamp(t, 0.0f, 1.0f);
    if (std::fabs(target - m_contentScrollY) < 0.5f)
        m_contentScrollY = target;
    return m_contentScrollY;
}

void NdsMenuLayer::setFastForwardMultiplier(float multiplier)
{
    m_display.fastForwardMultiplier = std::clamp(multiplier, 0.1f, 5.0f);
}

void NdsMenuLayer::setDisplaySettings(const NdsDisplaySettings& settings)
{
    m_display = settings;
    m_display.fastForwardMultiplier = std::clamp(m_display.fastForwardMultiplier, 0.1f, 5.0f);
    m_display.layout = std::clamp(m_display.layout, 0, 7);
    m_display.orientation = std::clamp(m_display.orientation, 0, 3);
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
        return 10;
    case Item::Display:
        return 11;
    case Item::Cheats:
        return 1;
    default:
        return 0;
    }
}

int NdsMenuLayer::nextFocusableDisplayRow(int from, int direction) const
{
    int row = from;
    for (int i = 0; i < contentControlCount(Item::Display); ++i)
    {
        row = (row + direction + contentControlCount(Item::Display)) % contentControlCount(Item::Display);
        if (row == 4 && m_display.layout != 7)
            continue;
        return row;
    }
    return from;
}

bool NdsMenuLayer::activateDisplayControl()
{
    switch (m_contentFocus)
    {
    case 2:
        m_display.integerScale = !m_display.integerScale;
        return true;
    case 4:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return false;
    default:
        return false;
    }
}

bool NdsMenuLayer::cycleCurrentSetting(int direction)
{
    if (static_cast<Item>(m_selected) != Item::Display || direction == 0)
        return false;

    auto cycleIndex = [direction](int value, int count) {
        return (value + direction + count) % count;
    };

    switch (m_contentFocus)
    {
    case 0:
    {
        int idx = 2;
        for (int i = 0; i < static_cast<int>(std::size(kFastForwardValues)); ++i)
        {
            if (std::fabs(kFastForwardValues[i] - m_display.fastForwardMultiplier) < 0.01f)
            {
                idx = i;
                break;
            }
        }
        idx = cycleIndex(idx, static_cast<int>(std::size(kFastForwardValues)));
        m_display.fastForwardMultiplier = kFastForwardValues[idx];
        return true;
    }
    case 1:
        m_display.linearFiltering = !m_display.linearFiltering;
        return true;
    case 3:
        m_display.layout = cycleIndex(m_display.layout, 8);
        if (m_contentFocus == 4 && m_display.layout != 7)
            m_contentFocus = nextFocusableDisplayRow(m_contentFocus, direction);
        return true;
    case 5:
        m_display.orientation = cycleIndex(m_display.orientation, 4);
        return true;
    default:
        return false;
    }
}

bool NdsMenuLayer::updateHeldSelector(std::uint64_t buttonsHeld)
{
    if (m_focusScope != FocusScope::Content ||
        static_cast<Item>(m_selected) != Item::Display ||
        (buttonsHeld & (HidNpadButton_L | HidNpadButton_R)) == 0)
    {
        m_selectorDirection = 0;
        return false;
    }

    const int direction = (buttonsHeld & HidNpadButton_R) ? 1 : -1;
    const std::uint64_t now = armGetSystemTick();
    if (m_selectorDirection != direction)
    {
        m_selectorDirection = direction;
        m_selectorRepeatStartTick = now;
        m_selectorLastStepTick = now;
        return false;
    }

    const float heldMs = static_cast<float>(armTicksToNs(now - m_selectorRepeatStartTick)) / 1000000.0f;
    if (heldMs < kSelectorInitialDelayMs)
        return false;

    const float intervalMs = std::max(52.0f, 180.0f - (heldMs - kSelectorInitialDelayMs) * 0.25f);
    const float sinceLastMs = static_cast<float>(armTicksToNs(now - m_selectorLastStepTick)) / 1000000.0f;
    if (sinceLastMs < intervalMs)
        return false;

    m_selectorLastStepTick = now;
    return cycleCurrentSetting(direction);
}

void NdsMenuLayer::openDeleteDialog()
{
    if (m_focusScope != FocusScope::Content)
        return;
    const Item item = static_cast<Item>(m_selected);
    if (item != Item::SaveState && item != Item::LoadState)
        return;
    if (m_contentFocus < 0 || m_contentFocus >= static_cast<int>(m_slots.size()) || !m_slots[m_contentFocus].exists)
        return;

    m_deleteSlot = m_contentFocus;
    m_deleteDialogVisible = true;
}

void NdsMenuLayer::closeDeleteDialog()
{
    m_deleteDialogVisible = false;
    m_deleteSlot = -1;
}

NdsMenuResult NdsMenuLayer::update(std::uint64_t buttonsDown, std::uint64_t buttonsHeld)
{
    if (!active())
        return {};

    if (m_panelAnimating && animationProgress(m_panelAnimStartTick, kPanelAnimationMs) >= 1.0f)
        m_panelAnimating = false;

    if (!m_visible)
        return {};

    if (m_deleteDialogVisible)
    {
        if (buttonsDown & HidNpadButton_B)
        {
            closeDeleteDialog();
            return {};
        }
        if (buttonsDown & HidNpadButton_A)
        {
            const int slot = m_deleteSlot;
            closeDeleteDialog();
            return {NdsMenuAction::DeleteState, slot};
        }
        return {};
    }

    if (buttonsDown & HidNpadButton_B)
    {
        if (m_focusScope == FocusScope::Content)
        {
            m_focusScope = FocusScope::Tabs;
            resetContentScroll();
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
            const int columns = (m_display.orientation == 1 || m_display.orientation == 3) ? 1 : 2;
            const int col = m_contentFocus % columns;
            if (columns > 1 && isDirectionLeft(buttonsDown) && col > 0)
                --m_contentFocus;
            else if (columns > 1 && isDirectionRight(buttonsDown) && col < columns - 1 && m_contentFocus + 1 < contentControlCount(currentItem))
                ++m_contentFocus;
            else if (isDirectionUp(buttonsDown) && m_contentFocus >= columns)
                m_contentFocus -= columns;
            else if (isDirectionDown(buttonsDown) && m_contentFocus + columns < contentControlCount(currentItem))
                m_contentFocus += columns;

            if (buttonsDown & HidNpadButton_A)
            {
                return {currentItem == Item::SaveState ? NdsMenuAction::SaveState : NdsMenuAction::LoadState,
                        m_contentFocus};
            }
            if (buttonsDown & HidNpadButton_X)
                openDeleteDialog();
            return {};
        }

        if (currentItem == Item::Display)
        {
            if (isDirectionUp(buttonsDown))
                m_contentFocus = nextFocusableDisplayRow(m_contentFocus, -1);
            if (isDirectionDown(buttonsDown))
                m_contentFocus = nextFocusableDisplayRow(m_contentFocus, 1);

            if (buttonsDown & HidNpadButton_L)
                return cycleCurrentSetting(-1) ? NdsMenuResult{NdsMenuAction::DisplaySettingsChanged, -1}
                                               : NdsMenuResult{};
            if (buttonsDown & HidNpadButton_R)
                return cycleCurrentSetting(1) ? NdsMenuResult{NdsMenuAction::DisplaySettingsChanged, -1}
                                              : NdsMenuResult{};
            if (updateHeldSelector(buttonsHeld))
                return {NdsMenuAction::DisplaySettingsChanged, -1};
            if (isDirectionLeft(buttonsDown) || isDirectionRight(buttonsDown))
                return cycleCurrentSetting(isDirectionRight(buttonsDown) ? 1 : -1)
                    ? NdsMenuResult{NdsMenuAction::DisplaySettingsChanged, -1}
                    : NdsMenuResult{};

            if ((buttonsDown & HidNpadButton_A) && activateDisplayControl())
                return {NdsMenuAction::DisplaySettingsChanged, -1};
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
                resetContentScroll();
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

void NdsMenuLayer::draw() const
{
    if (!active())
        return;

    setMenuMetricsOrientation(m_display.orientation);
    const float panel = panelProgress();
    if (panel <= 0.0f)
    {
        setMenuMetricsOrientation(0);
        return;
    }

    const float slideY = (1.0f - panel) * kScreenH;
    const float selectionProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;
    const float pageProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;

    const bool contentFocused = m_focusScope == FocusScope::Content;
    const Item currentItem = static_cast<Item>(m_selected);
    const bool canDelete = contentFocused &&
        (currentItem == Item::SaveState || currentItem == Item::LoadState) &&
        m_contentFocus >= 0 && m_contentFocus < static_cast<int>(m_slots.size()) &&
        m_slots[m_contentFocus].exists;
    const bool transformed = pushMenuOrientationTransform(m_display.orientation);
    drawOverlay(panel);
    drawHeader(slideY);
    drawLeftMenu(m_selected, m_previousSelected, selectionProgress, !contentFocused, slideY);
    drawLine({kSeparatorX, menuMetrics().separatorY + slideY},
             {1.0f, menuMetrics().separatorH},
             {1.0f, 1.0f, 1.0f, 0.08f});
    drawTabFrame(static_cast<Item>(m_selected),
                 static_cast<Item>(m_previousSelected),
                 pageProgress,
                 m_display,
                 m_slots,
                 m_contentFocus,
                 contentFocused,
                 smoothedContentScrollY(),
                 slideY);
    drawFooter(contentFocused, canDelete, slideY);
    if (m_deleteDialogVisible)
        drawDeleteDialog(m_deleteSlot, panel);
    if (transformed)
        Gfx::PopDrawTransform();
    setMenuMetricsOrientation(0);
}

} // namespace beiklive::nds_stub
