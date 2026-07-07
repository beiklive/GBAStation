#include "mgba_stub/MgbaMenuLayer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::mgba_stub {

using Gfx::Color;
using Gfx::Vector2f;

namespace {

constexpr float kScreenW = 1280.0f;
constexpr float kScreenH = 720.0f;
constexpr float kLeftX = 48.0f;
constexpr float kLeftY = 116.0f;
constexpr float kMenuW = 336.0f;
constexpr float kItemH = 70.0f;
constexpr float kItemGap = 10.0f;
constexpr float kSeparatorX = 404.0f;
constexpr float kContentX = 432.0f;
constexpr float kContentY = 110.0f;
constexpr float kContentW = 790.0f;
constexpr float kContentH = 520.0f;
constexpr float kContentBodyTop = 64.0f;
constexpr float kContentBodyH = 444.0f;
constexpr float kSaveCardW = 386.0f;
constexpr float kSaveCardH = 112.0f;
constexpr float kSaveCardGapX = 18.0f;
constexpr float kSaveCardGapY = 16.0f;
constexpr float kPanelAnimationMs = 220.0f;
constexpr float kSaveSlotStepY = kSaveCardH + kSaveCardGapY;

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float easeOutCubic(float t)
{
    t = clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float animationProgress(std::uint64_t startTick, float durationMs)
{
    if (startTick == 0)
        return 1.0f;
    const std::uint64_t elapsedTicks = armGetSystemTick() - startTick;
    const double elapsedMs = static_cast<double>(armTicksToNs(elapsedTicks)) / 1000000.0;
    return clamp01(static_cast<float>(elapsedMs / durationMs));
}

void drawRect(Vector2f pos, Vector2f size, Color color, bool cool = false)
{
    Gfx::DrawRectangle(pos, size, color, cool);
}

void drawLine(Vector2f pos, Vector2f size, Color color)
{
    Gfx::DrawRectangle(pos, size, color);
}

void drawBorder(Vector2f pos, Vector2f size, float width, Color color)
{
    drawRect(pos, {size.X, width}, color);
    drawRect({pos.X, pos.Y + size.Y - width}, {size.X, width}, color);
    drawRect(pos, {width, size.Y}, color);
    drawRect({pos.X + size.X - width, pos.Y}, {width, size.Y}, color);
}

void drawFocusBorder(Vector2f pos, Vector2f size)
{
    const double ms = static_cast<double>(armTicksToNs(armGetSystemTick())) / 1000000.0;
    const float pulse = 0.55f + 0.45f * std::sin(static_cast<float>(ms) * 0.0048f);
    const Color c{0.32f + 0.18f * pulse, 0.76f + 0.14f * pulse, 1.0f, 0.95f};
    drawBorder(pos - Vector2f{4.0f, 4.0f}, size + Vector2f{8.0f, 8.0f}, 4.0f, c);
}

const char* itemLabel(MgbaMenuLayer::Item item)
{
    switch (item)
    {
    case MgbaMenuLayer::Item::Resume: return "返回游戏";
    case MgbaMenuLayer::Item::SaveState: return "保存状态";
    case MgbaMenuLayer::Item::LoadState: return "读取状态";
    case MgbaMenuLayer::Item::Cheats: return "金手指";
    case MgbaMenuLayer::Item::Display: return "画面设置";
    case MgbaMenuLayer::Item::Reset: return "重置游戏";
    case MgbaMenuLayer::Item::Exit: return "退出游戏";
    default: return "";
    }
}

const char* itemIcon(MgbaMenuLayer::Item item)
{
    switch (item)
    {
    case MgbaMenuLayer::Item::Resume: return "<";
    case MgbaMenuLayer::Item::SaveState: return "S";
    case MgbaMenuLayer::Item::LoadState: return "L";
    case MgbaMenuLayer::Item::Cheats: return "*";
    case MgbaMenuLayer::Item::Display: return "D";
    case MgbaMenuLayer::Item::Reset: return "R";
    case MgbaMenuLayer::Item::Exit: return "X";
    default: return "";
    }
}

float menuItemY(int index)
{
    return kLeftY + static_cast<float>(index) * (kItemH + kItemGap);
}

bool isDirectionUp(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyUp) != 0;
}

bool isDirectionDown(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyDown) != 0;
}

void drawOverlay(float alpha)
{
    alpha = clamp01(alpha);
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.62f * alpha}, true);
    for (int i = 0; i < 8; ++i)
    {
        const float y = static_cast<float>(i) * kScreenH / 8.0f;
        const float a = (0.08f + static_cast<float>(i) * 0.018f) * alpha;
        drawRect({0.0f, y}, {kScreenW, kScreenH / 8.0f + 1.0f}, {0.02f, 0.05f, 0.08f, a}, true);
    }
}

void drawHeader(float offsetY)
{
    Gfx::DrawText(Gfx::SystemFontChinese, {kLeftX, 48.0f + offsetY}, 32.0f,
                  {1.0f, 1.0f, 1.0f, 0.96f}, "游戏菜单");
    Gfx::DrawText(Gfx::SystemFontStandard, {kLeftX + 146.0f, 58.0f + offsetY}, 18.0f,
                  {0.62f, 0.78f, 0.94f, 0.72f}, "mGBA");
    drawLine({kLeftX, 92.0f + offsetY}, {310.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.12f});
}

void drawLeftMenu(int selected, int previousSelected, float selectionProgress, bool tabsFocused, float offsetY)
{
    const int count = static_cast<int>(MgbaMenuLayer::Item::Count);
    for (int i = 0; i < count; ++i)
    {
        const auto item = static_cast<MgbaMenuLayer::Item>(i);
        const float y = menuItemY(i) + offsetY;
        const bool active = i == selected;
        const bool previous = i == previousSelected && previousSelected != selected && selectionProgress < 1.0f;
        float activeAlpha = active ? selectionProgress : (previous ? 1.0f - selectionProgress : 0.0f);
        activeAlpha = clamp01(activeAlpha);

        if (activeAlpha > 0.0f)
        {
            drawRect({kLeftX - 4.0f, y - 4.0f},
                     {kMenuW + 8.0f, kItemH + 8.0f},
                     {0.16f, 0.44f, 0.70f, 0.20f * activeAlpha},
                     true);
            if (tabsFocused && active)
                drawFocusBorder({kLeftX, y}, {kMenuW, kItemH});
        }

        drawRect({kLeftX, y}, {kMenuW, kItemH},
                 active ? Color{1.0f, 1.0f, 1.0f, 0.070f}
                        : Color{1.0f, 1.0f, 1.0f, 0.026f},
                 true);
        Gfx::DrawText(Gfx::SystemFontStandard,
                      {kLeftX + 38.0f, y + 34.0f},
                      23.0f,
                      active ? Color{0.58f, 0.88f, 1.0f, 0.95f}
                             : Color{1.0f, 1.0f, 1.0f, 0.58f},
                      Gfx::align_Center,
                      Gfx::align_Center,
                      itemIcon(item));
        Gfx::DrawText(Gfx::SystemFontChinese,
                      {kLeftX + 78.0f, y + 21.0f},
                      23.0f,
                      active ? Color{1.0f, 1.0f, 1.0f, 0.96f}
                             : Color{1.0f, 1.0f, 1.0f, 0.70f},
                      "%s",
                      itemLabel(item));
    }

    const float sepY = menuItemY(static_cast<int>(MgbaMenuLayer::Item::Reset)) - 13.0f + offsetY;
    drawLine({kLeftX + 12.0f, sepY}, {kMenuW - 24.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f});
}

float focusedScroll(int focusedSlot)
{
    const int row = std::max(0, focusedSlot / 2);
    const float focusedTop = static_cast<float>(row) * kSaveSlotStepY;
    const float contentH = 5.0f * kSaveSlotStepY;
    const float maxScroll = std::max(0.0f, contentH - kContentBodyH);
    float scroll = 0.0f;
    if (focusedTop + kSaveCardH > kContentBodyH)
        scroll = focusedTop + kSaveCardH - kContentBodyH;
    if (focusedTop < scroll)
        scroll = focusedTop;
    return std::clamp(scroll, 0.0f, maxScroll);
}

void drawSaveSlotCard(int slot, Vector2f pos, bool focused, const MgbaStateSlotInfo& info, float opacity)
{
    if (focused)
        drawFocusBorder(pos, {kSaveCardW, kSaveCardH});
    drawRect(pos,
             {kSaveCardW, kSaveCardH},
             info.exists ? Color{0.08f, 0.18f, 0.26f, 0.58f * opacity}
                         : Color{1.0f, 1.0f, 1.0f, 0.045f * opacity},
             true);
    drawBorder(pos,
               {kSaveCardW, kSaveCardH},
               1.0f,
               info.exists ? Color{0.42f, 0.82f, 1.0f, 0.22f * opacity}
                           : Color{1.0f, 1.0f, 1.0f, 0.10f * opacity});

    drawRect(pos + Vector2f{18.0f, 18.0f},
             {78.0f, 76.0f},
             info.exists ? Color{0.08f, 0.30f, 0.44f, 0.42f * opacity}
                         : Color{1.0f, 1.0f, 1.0f, 0.035f * opacity},
             true);
    Gfx::DrawText(Gfx::SystemFontStandard,
                  pos + Vector2f{57.0f, 56.0f},
                  info.exists ? 22.0f : 34.0f,
                  info.exists ? Color{0.52f, 0.86f, 1.0f, 0.90f * opacity}
                              : Color{1.0f, 1.0f, 1.0f, 0.42f * opacity},
                  Gfx::align_Center,
                  Gfx::align_Center,
                  info.exists ? "GBA" : "+");

    char title[32] = {};
    std::snprintf(title, sizeof(title), "Slot %d", slot);
    Gfx::DrawText(Gfx::SystemFontStandard, pos + Vector2f{116.0f, 27.0f}, 24.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f * opacity}, "%s", title);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{116.0f, 60.0f}, 18.0f,
                  {0.78f, 0.88f, 0.96f, 0.70f * opacity},
                  "%s",
                  info.exists ? "已有状态" : "空存档槽");
    if (!info.modifiedTime.empty())
        Gfx::DrawText(Gfx::SystemFontStandard, pos + Vector2f{116.0f, 84.0f}, 14.0f,
                      {0.68f, 0.78f, 0.88f, 0.52f * opacity},
                      "%s",
                      info.modifiedTime.c_str());
}

void drawStatePage(const char* title,
                   const std::array<MgbaStateSlotInfo, 10>& slots,
                   int focusedSlot,
                   bool contentFocused,
                   float scrollY,
                   float opacity,
                   float offsetY)
{
    Gfx::DrawText(Gfx::SystemFontChinese, {kContentX, kContentY + 6.0f + offsetY}, 27.0f,
                  {1.0f, 1.0f, 1.0f, 0.94f * opacity}, "%s", title);
    drawLine({kContentX, kContentY + 52.0f + offsetY}, {kContentW, 1.0f},
             {1.0f, 1.0f, 1.0f, 0.10f * opacity});

    Gfx::PushScissor(static_cast<u32>(kContentX - 18.0f),
                     static_cast<u32>(kContentY + kContentBodyTop + offsetY - 12.0f),
                     static_cast<u32>(kContentW + 36.0f),
                     static_cast<u32>(kContentBodyH + 24.0f));
    for (int i = 0; i < static_cast<int>(slots.size()); ++i)
    {
        const int col = i % 2;
        const int row = i / 2;
        const Vector2f pos{kContentX + static_cast<float>(col) * (kSaveCardW + kSaveCardGapX),
                           kContentY + kContentBodyTop + static_cast<float>(row) * kSaveSlotStepY - scrollY + offsetY};
        if (pos.Y + kSaveCardH < kContentY + kContentBodyTop + offsetY - 20.0f ||
            pos.Y > kContentY + kContentBodyTop + kContentBodyH + offsetY + 20.0f)
            continue;
        drawSaveSlotCard(i, pos, contentFocused && i == focusedSlot, slots[i], opacity);
    }
    Gfx::PopScissor();
}

void drawInfoPage(const char* title, const char* body, float opacity, float offsetY)
{
    Gfx::DrawText(Gfx::SystemFontChinese, {kContentX, kContentY + 6.0f + offsetY}, 27.0f,
                  {1.0f, 1.0f, 1.0f, 0.94f * opacity}, "%s", title);
    drawLine({kContentX, kContentY + 52.0f + offsetY}, {kContentW, 1.0f},
             {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    drawRect({kContentX + 4.0f, kContentY + 100.0f + offsetY},
             {kContentW - 8.0f, 128.0f},
             {1.0f, 1.0f, 1.0f, 0.040f * opacity},
             true);
    Gfx::DrawText(Gfx::SystemFontChinese, {kContentX + 34.0f, kContentY + 145.0f + offsetY}, 22.0f,
                  {0.78f, 0.88f, 0.96f, 0.78f * opacity}, "%s", body);
}

void drawFooter(bool contentFocused, bool canDelete, float offsetY)
{
    const float y = kScreenH - 70.0f + offsetY;
    drawRect({0.0f, y}, {kScreenW, 70.0f}, {0.0f, 0.0f, 0.0f, 0.35f}, true);
    drawLine({0.0f, y}, {kScreenW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f});
    Gfx::DrawText(Gfx::SystemFontChinese, {kScreenW - 336.0f, y + 24.0f}, 21.0f,
                  {0.38f, 0.78f, 1.0f, 0.92f}, "A 确定");
    Gfx::DrawText(Gfx::SystemFontChinese, {kScreenW - 218.0f, y + 24.0f}, 21.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f}, contentFocused ? "B 返回" : "B 关闭");
    if (canDelete)
        Gfx::DrawText(Gfx::SystemFontChinese, {kScreenW - 100.0f, y + 24.0f}, 21.0f,
                      {1.0f, 0.70f, 0.62f, 0.86f}, "X 删除");
}

void drawDeleteDialog(int slot, float opacity)
{
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.48f * opacity}, true);
    const Vector2f pos{390.0f, 246.0f};
    const Vector2f size{500.0f, 220.0f};
    drawRect(pos, size, {0.018f, 0.024f, 0.034f, 0.98f * opacity}, true);
    drawBorder(pos, size, 1.0f, {1.0f, 1.0f, 1.0f, 0.16f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{34.0f, 34.0f}, 27.0f,
                  {1.0f, 1.0f, 1.0f, 0.95f * opacity}, "删除状态");
    char body[96] = {};
    std::snprintf(body, sizeof(body), "确定删除 Slot %d 的即时状态？", slot);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{34.0f, 92.0f}, 22.0f,
                  {0.82f, 0.90f, 0.96f, 0.78f * opacity}, "%s", body);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{size.X - 34.0f, size.Y - 48.0f}, 21.0f,
                  {0.90f, 0.96f, 1.0f, 0.82f * opacity},
                  Gfx::align_Right,
                  Gfx::align_Left,
                  "A 删除   B 取消");
}

void drawToast(const std::string& message, float progress)
{
    progress = easeOutCubic(clamp01(progress));
    const float w = std::min(760.0f, std::max(250.0f, Gfx::MeasureText(Gfx::SystemFontChinese, 21.0f, message.c_str()).X + 72.0f));
    const float h = 54.0f;
    const float x = (kScreenW - w) * 0.5f;
    const float y = 34.0f - (1.0f - progress) * 42.0f;
    drawRect({x, y}, {w, h}, {0.02f, 0.04f, 0.06f, 0.92f * progress}, true);
    drawBorder({x, y}, {w, h}, 1.0f, {1.0f, 1.0f, 1.0f, 0.14f * progress});
    Gfx::DrawText(Gfx::SystemFontChinese, {kScreenW * 0.5f, y + h * 0.5f}, 21.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f * progress},
                  Gfx::align_Center,
                  Gfx::align_Center,
                  message.c_str());
}

} // namespace

void MgbaMenuLayer::setStateSlots(const std::array<MgbaStateSlotInfo, 10>& slots)
{
    m_slots = slots;
}

void MgbaMenuLayer::open()
{
    if (m_visible)
        return;
    m_visible = true;
    m_focusScope = FocusScope::Tabs;
    beginPanelAnimation(true);
}

void MgbaMenuLayer::close()
{
    if (!m_visible)
        return;
    m_visible = false;
    m_focusScope = FocusScope::Tabs;
    closeDeleteDialog();
    beginPanelAnimation(false);
}

void MgbaMenuLayer::toggle()
{
    if (m_visible)
        close();
    else
        open();
}

void MgbaMenuLayer::showToast(const std::string& message)
{
    m_toastMessage = message;
    m_toastStartTick = armGetSystemTick();
}

void MgbaMenuLayer::clearToast()
{
    m_toastMessage.clear();
    m_toastStartTick = 0;
}

int MgbaMenuLayer::contentControlCount(Item item) const
{
    switch (item)
    {
    case Item::SaveState:
    case Item::LoadState:
        return 10;
    case Item::Cheats:
    case Item::Display:
        return 0;
    default:
        return 0;
    }
}

bool MgbaMenuLayer::itemHasContent(Item item) const
{
    return item == Item::SaveState ||
           item == Item::LoadState ||
           item == Item::Cheats ||
           item == Item::Display;
}

void MgbaMenuLayer::beginSelectionAnimation(int oldSelected, int newSelected)
{
    m_previousSelected = oldSelected;
    m_selected = newSelected;
    m_selectionAnimStartTick = armGetSystemTick();
    m_selectionAnimating = true;
    resetContentScroll();
}

void MgbaMenuLayer::beginPanelAnimation(bool opening)
{
    m_panelAnimStartTick = armGetSystemTick();
    m_panelAnimating = true;
    m_panelOpening = opening;
}

float MgbaMenuLayer::panelProgress() const
{
    if (!m_panelAnimating)
        return m_visible ? 1.0f : 0.0f;
    const float t = easeOutCubic(animationProgress(m_panelAnimStartTick, kPanelAnimationMs));
    if (t >= 1.0f)
        m_panelAnimating = false;
    return m_panelOpening ? t : 1.0f - t;
}

void MgbaMenuLayer::openDeleteDialog()
{
    const Item item = static_cast<Item>(m_selected);
    if ((item != Item::SaveState && item != Item::LoadState) ||
        m_focusScope != FocusScope::Content ||
        m_contentFocus < 0 ||
        m_contentFocus >= static_cast<int>(m_slots.size()) ||
        !m_slots[m_contentFocus].exists)
        return;
    m_deleteSlot = m_contentFocus;
    m_deleteDialogVisible = true;
}

void MgbaMenuLayer::closeDeleteDialog()
{
    m_deleteDialogVisible = false;
    m_deleteSlot = -1;
}

void MgbaMenuLayer::resetContentScroll()
{
    m_contentScrollY = 0.0f;
    m_contentScrollLastTick = 0;
}

float MgbaMenuLayer::smoothedContentScrollY() const
{
    const Item item = static_cast<Item>(m_selected);
    if (item != Item::SaveState && item != Item::LoadState)
        return 0.0f;

    const float target = focusedScroll(m_contentFocus);
    const std::uint64_t now = armGetSystemTick();
    if (m_contentScrollLastTick == 0)
    {
        m_contentScrollLastTick = now;
        m_contentScrollY = target;
        return m_contentScrollY;
    }

    const float dtMs = static_cast<float>(armTicksToNs(now - m_contentScrollLastTick)) / 1000000.0f;
    m_contentScrollLastTick = now;
    const float t = 1.0f - std::exp(-dtMs / 68.0f);
    m_contentScrollY += (target - m_contentScrollY) * std::clamp(t, 0.0f, 1.0f);
    if (std::fabs(target - m_contentScrollY) < 0.5f)
        m_contentScrollY = target;
    return m_contentScrollY;
}

MgbaMenuResult MgbaMenuLayer::update(std::uint64_t buttonsDown, std::uint64_t)
{
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
            return {MgbaMenuAction::DeleteState, slot};
        }
        return {};
    }

    if (!m_visible)
        return {};

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

    const Item currentItem = static_cast<Item>(m_selected);
    if (m_focusScope == FocusScope::Content)
    {
        const int count = contentControlCount(currentItem);
        if (count > 0)
        {
            const int oldFocus = m_contentFocus;
            if (isDirectionUp(buttonsDown))
                m_contentFocus = std::max(0, m_contentFocus - 2);
            if (isDirectionDown(buttonsDown))
                m_contentFocus = std::min(count - 1, m_contentFocus + 2);
            if (buttonsDown & HidNpadButton_AnyLeft)
                m_contentFocus = std::max(0, m_contentFocus - 1);
            if (buttonsDown & HidNpadButton_AnyRight)
                m_contentFocus = std::min(count - 1, m_contentFocus + 1);
            if (m_contentFocus != oldFocus)
                m_contentScrollLastTick = 0;

            if (buttonsDown & HidNpadButton_A)
            {
                if (currentItem == Item::LoadState && !m_slots[m_contentFocus].loadable)
                    return {};
                return {currentItem == Item::SaveState ? MgbaMenuAction::SaveState : MgbaMenuAction::LoadState,
                        m_contentFocus};
            }
            if (buttonsDown & HidNpadButton_X)
                openDeleteDialog();
        }
        return {};
    }

    const int itemCount = static_cast<int>(Item::Count);
    if (isDirectionUp(buttonsDown))
    {
        beginSelectionAnimation(m_selected, (m_selected + itemCount - 1) % itemCount);
        return {};
    }
    if (isDirectionDown(buttonsDown))
    {
        beginSelectionAnimation(m_selected, (m_selected + 1) % itemCount);
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
            m_focusScope = FocusScope::Content;
            m_contentFocus = 0;
            resetContentScroll();
            return {};
        case Item::Cheats:
        case Item::Display:
            m_focusScope = FocusScope::Content;
            m_contentFocus = 0;
            return {};
        case Item::Reset:
            return {MgbaMenuAction::ResetGame, -1};
        case Item::Exit:
            return {MgbaMenuAction::ExitGame, -1};
        default:
            return {};
        }
    }

    return {};
}

void MgbaMenuLayer::draw() const
{
    auto drawToastIfNeeded = [&]() {
        if (m_toastMessage.empty() || m_toastStartTick == 0)
            return;
        constexpr float kToastInMs = 180.0f;
        constexpr float kToastHoldMs = 2000.0f;
        constexpr float kToastOutMs = 180.0f;
        const std::uint64_t now = armGetSystemTick();
        const float elapsedMs = static_cast<float>(armTicksToNs(now - m_toastStartTick)) / 1000000.0f;
        const float totalMs = kToastInMs + kToastHoldMs + kToastOutMs;
        if (elapsedMs >= totalMs)
        {
            m_toastMessage.clear();
            m_toastStartTick = 0;
            return;
        }
        float progress = 1.0f;
        if (elapsedMs < kToastInMs)
            progress = elapsedMs / kToastInMs;
        else if (elapsedMs > kToastInMs + kToastHoldMs)
            progress = 1.0f - (elapsedMs - kToastInMs - kToastHoldMs) / kToastOutMs;
        drawToast(m_toastMessage, progress);
    };

    if (!active())
    {
        drawToastIfNeeded();
        return;
    }

    const float panel = panelProgress();
    if (panel <= 0.0f)
    {
        drawToastIfNeeded();
        return;
    }

    const float slideY = (1.0f - panel) * kScreenH;
    const float selectionProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;
    if (selectionProgress >= 1.0f)
        m_selectionAnimating = false;

    const bool contentFocused = m_focusScope == FocusScope::Content;
    const Item currentItem = static_cast<Item>(m_selected);
    const bool canDelete = contentFocused &&
        (currentItem == Item::SaveState || currentItem == Item::LoadState) &&
        m_contentFocus >= 0 && m_contentFocus < static_cast<int>(m_slots.size()) &&
        m_slots[m_contentFocus].exists;

    drawOverlay(panel);
    drawHeader(slideY);
    drawLeftMenu(m_selected, m_previousSelected, selectionProgress, !contentFocused, slideY);
    drawLine({kSeparatorX, 110.0f + slideY}, {1.0f, 500.0f}, {1.0f, 1.0f, 1.0f, 0.08f});

    if (currentItem != Item::Resume && currentItem != Item::Reset && currentItem != Item::Exit)
    {
        drawRect({kContentX - 22.0f, kContentY - 24.0f + slideY},
                 {kContentW + 44.0f, kContentH + 34.0f},
                 {0.117f, 0.117f, 0.117f, 1.0f},
                 true);
        if (currentItem == Item::SaveState)
            drawStatePage("保存状态", m_slots, m_contentFocus, contentFocused, smoothedContentScrollY(), panel, slideY);
        else if (currentItem == Item::LoadState)
            drawStatePage("读取状态", m_slots, m_contentFocus, contentFocused, smoothedContentScrollY(), panel, slideY);
        else if (currentItem == Item::Cheats)
            drawInfoPage("金手指设置", "此页面暂未开放。", panel, slideY);
        else if (currentItem == Item::Display)
            drawInfoPage("画面设置", "此页面暂未开放。", panel, slideY);
    }

    drawFooter(contentFocused, canDelete, slideY);
    if (m_deleteDialogVisible)
        drawDeleteDialog(m_deleteSlot, panel);
    drawToastIfNeeded();
}

void drawMgbaGameStatusBadges(double fps,
                              bool showFps,
                              bool fastForwardActive,
                              bool showFastForward,
                              bool paused)
{
    if (showFps && fps > 0.0)
    {
        char text[32] = {};
        std::snprintf(text, sizeof(text), "%.1f FPS", fps);
        drawRect({22.0f, 18.0f}, {118.0f, 34.0f}, {0.0f, 0.0f, 0.0f, 0.42f}, true);
        Gfx::DrawText(Gfx::SystemFontStandard, {36.0f, 27.0f}, 16.0f,
                      {0.92f, 0.96f, 1.0f, 0.92f}, "%s", text);
    }
    if (fastForwardActive && showFastForward)
    {
        drawRect({kScreenW - 150.0f, 18.0f}, {128.0f, 34.0f}, {0.0f, 0.0f, 0.0f, 0.42f}, true);
        Gfx::DrawText(Gfx::SystemFontChinese, {kScreenW - 132.0f, 27.0f}, 16.0f,
                      {0.62f, 0.86f, 1.0f, 0.92f}, "快进");
    }
    if (paused)
    {
        drawRect({kScreenW * 0.5f - 62.0f, 18.0f}, {124.0f, 34.0f}, {0.0f, 0.0f, 0.0f, 0.42f}, true);
        Gfx::DrawText(Gfx::SystemFontChinese, {kScreenW * 0.5f, 35.0f}, 17.0f,
                      {0.92f, 0.96f, 1.0f, 0.92f},
                      Gfx::align_Center,
                      Gfx::align_Center,
                      "暂停");
    }
}

} // namespace beiklive::mgba_stub
