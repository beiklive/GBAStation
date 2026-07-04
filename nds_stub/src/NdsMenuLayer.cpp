#include "nds_stub/NdsMenuLayer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub {

namespace {

using Gfx::Color;
using Gfx::Vector2f;

constexpr float kScreenW = 1280.0f;
constexpr float kScreenH = 720.0f;
constexpr float kLeftX = 56.0f;
constexpr float kLeftY = 120.0f;
constexpr float kMenuW = 280.0f;
constexpr float kItemH = 58.0f;
constexpr float kItemGap = 18.0f;
constexpr float kSeparatorX = 340.0f;
constexpr float kContentX = 380.0f;
constexpr float kContentY = 110.0f;
constexpr float kContentW = 840.0f;
constexpr float kContentH = 520.0f;
constexpr float kGradientFocusFlowCycleMs = 1800.0f;
constexpr float kGradientFocusBrightness = 1.0f;

constexpr int itemIndex(NdsMenuLayer::Item item)
{
    return static_cast<int>(item);
}

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

float easeOutQuart(float t)
{
    t = clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv * inv;
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

float gradientFocusAnimationOffset()
{
    const double ms = static_cast<double>(armTicksToNs(armGetSystemTick())) / 1000000.0;
    return std::fmod(static_cast<float>(ms) / kGradientFocusFlowCycleMs, 1.0f);
}

Color mixColor(Color a, Color b, float t)
{
    t = clamp01(t);
    return {
        lerp(a.R, b.R, t),
        lerp(a.G, b.G, t),
        lerp(a.B, b.B, t),
        lerp(a.A, b.A, t),
    };
}

Color gradientFocusColor(float offset, float alpha)
{
    offset = offset - std::floor(offset);

    struct Stop {
        float pos;
        Color color;
    };

    constexpr int stopCount = 6;
    const Stop stops[stopCount] = {
        {0.00f, {0.31f, 0.76f, 1.00f, 1.0f}},
        {0.18f, {0.25f, 0.95f, 0.86f, 1.0f}},
        {0.38f, {0.72f, 0.46f, 1.00f, 1.0f}},
        {0.58f, {1.00f, 0.42f, 0.82f, 1.0f}},
        {0.78f, {0.38f, 0.63f, 1.00f, 1.0f}},
        {1.00f, {0.31f, 0.76f, 1.00f, 1.0f}},
    };

    Color color = stops[0].color;
    for (int i = 0; i < stopCount - 1; ++i)
    {
        if (offset >= stops[i].pos && offset <= stops[i + 1].pos)
        {
            const float localT = (offset - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            color = mixColor(stops[i].color, stops[i + 1].color, localT);
            break;
        }
    }

    color.R *= kGradientFocusBrightness;
    color.G *= kGradientFocusBrightness;
    color.B *= kGradientFocusBrightness;
    color.A = alpha;
    return color;
}

const char* filterLabel(bool linear)
{
    return linear ? "Linear" : "Nearest";
}

const char* itemLabel(NdsMenuLayer::Item item)
{
    switch (item)
    {
    case NdsMenuLayer::Item::Resume: return "返回游戏";
    case NdsMenuLayer::Item::SaveState: return "保存状态";
    case NdsMenuLayer::Item::LoadState: return "读取状态";
    case NdsMenuLayer::Item::Cheats: return "金手指设置";
    case NdsMenuLayer::Item::Display: return "画面设置";
    case NdsMenuLayer::Item::Reset: return "重置游戏";
    case NdsMenuLayer::Item::Exit: return "退出游戏";
    default: return "";
    }
}

const char* itemIcon(NdsMenuLayer::Item item)
{
    switch (item)
    {
    case NdsMenuLayer::Item::Resume: return ">";
    case NdsMenuLayer::Item::SaveState: return "S";
    case NdsMenuLayer::Item::LoadState: return "L";
    case NdsMenuLayer::Item::Cheats: return "C";
    case NdsMenuLayer::Item::Display: return "D";
    case NdsMenuLayer::Item::Reset: return "R";
    case NdsMenuLayer::Item::Exit: return "X";
    default: return "";
    }
}

float menuItemY(int index)
{
    float y = kLeftY + index * (kItemH + kItemGap);
    if (index >= itemIndex(NdsMenuLayer::Item::Reset))
        y += 18.0f;
    return y;
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

void drawGradientBorder(Vector2f pos, Vector2f size, float width)
{
    const float animationOffset = gradientFocusAnimationOffset();
    const float alpha = 1.0f;
    const float radius = 0.0f;
    const float perimeter = size.X * 2.0f + size.Y * 2.0f;

    drawRect({pos.X - 7.0f, pos.Y - 7.0f}, {size.X + 14.0f, size.Y + 14.0f},
             {0.18f, 0.42f, 0.78f, 0.11f}, true);
    drawRect({pos.X - 4.0f, pos.Y - 4.0f}, {size.X + 8.0f, size.Y + 8.0f},
             {0.26f, 0.55f, 0.92f, 0.12f}, true);
    drawRect(pos, size, {1.0f, 1.0f, 1.0f, 0.05f}, true);

    auto drawFlowSegment = [&](Vector2f segmentPos, Vector2f segmentSize, float pathCenter) {
        const float lutPos = (pathCenter / perimeter) + animationOffset;
        drawRect(segmentPos, segmentSize, gradientFocusColor(lutPos, alpha));
    };

    constexpr int horizontalSegments = 28;
    constexpr int verticalSegments = 6;
    const float topW = (size.X - radius * 2.0f) / horizontalSegments;
    const float sideH = (size.Y - radius * 2.0f) / verticalSegments;

    for (int i = 0; i < horizontalSegments; ++i)
    {
        const float x = radius + i * topW;
        drawFlowSegment({pos.X + x, pos.Y}, {topW + 0.75f, width}, x + topW * 0.5f);

        const float bottomX = size.X - radius - (i + 1) * topW;
        drawFlowSegment({pos.X + bottomX, pos.Y + size.Y - width}, {topW + 0.75f, width},
                        size.X + size.Y + (radius + i * topW + topW * 0.5f));
    }

    for (int i = 0; i < verticalSegments; ++i)
    {
        const float y = radius + i * sideH;
        drawFlowSegment({pos.X + size.X - width, pos.Y + y}, {width, sideH + 0.75f},
                        size.X + y + sideH * 0.5f);

        const float leftY = size.Y - radius - (i + 1) * sideH;
        drawFlowSegment({pos.X, pos.Y + leftY}, {width, sideH + 0.75f},
                        size.X * 2.0f + size.Y + (radius + i * sideH + sideH * 0.5f));
    }

    const Color c0 = gradientFocusColor(animationOffset + 0.00f, alpha);
    const Color c1 = gradientFocusColor(animationOffset + 0.25f, alpha);
    const Color c2 = gradientFocusColor(animationOffset + 0.50f, alpha);
    const Color c3 = gradientFocusColor(animationOffset + 0.75f, alpha);
    drawRect({pos.X + 2.0f, pos.Y + 2.0f}, {radius, width}, c0);
    drawRect({pos.X + 2.0f, pos.Y + 2.0f}, {width, radius}, c0);
    drawRect({pos.X + size.X - radius - 2.0f, pos.Y + 2.0f}, {radius, width}, c1);
    drawRect({pos.X + size.X - width - 2.0f, pos.Y + 2.0f}, {width, radius}, c1);
    drawRect({pos.X + size.X - radius - 2.0f, pos.Y + size.Y - width - 2.0f}, {radius, width}, c2);
    drawRect({pos.X + size.X - width - 2.0f, pos.Y + size.Y - radius - 2.0f}, {width, radius}, c2);
    drawRect({pos.X + 2.0f, pos.Y + size.Y - width - 2.0f}, {radius, width}, c3);
    drawRect({pos.X + 2.0f, pos.Y + size.Y - radius - 2.0f}, {width, radius}, c3);

    drawRect({pos.X + 10.0f, pos.Y + (size.Y - 40.0f) * 0.5f}, {5.0f, 40.0f},
             gradientFocusColor(animationOffset, 1.0f));
}

void drawOverlay()
{
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.54f}, true);

    constexpr int bands = 8;
    for (int i = 0; i < bands; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(bands - 1);
        const float y = kScreenH * static_cast<float>(i) / static_cast<float>(bands);
        const float h = kScreenH / static_cast<float>(bands) + 1.0f;
        drawRect({0.0f, y}, {kScreenW, h},
                 {lerp(0.16f, 0.0f, t),
                  lerp(0.16f, 0.0f, t),
                  lerp(0.16f, 0.0f, t),
                  lerp(0.38f, 0.68f, t)},
                 true);
    }
}

void drawHeader()
{
    Gfx::DrawText(Gfx::SystemFontChinese, {64.0f, 30.0f}, 26.0f,
                  {1.0f, 1.0f, 1.0f, 1.0f}, "游戏菜单");
    drawLine({56.0f, 92.0f}, {1168.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.18f});
}

void drawMenuSeparator()
{
    const float y = menuItemY(itemIndex(NdsMenuLayer::Item::Reset)) - 14.0f;
    drawLine({kLeftX + 18.0f, y}, {kMenuW - 36.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.14f});
}

void drawLeftMenu(int selected, int previousSelected, float selectionProgress)
{
    const float eased = easeOutCubic(selectionProgress);
    const float previousY = menuItemY(previousSelected);
    const float selectedY = menuItemY(selected);
    const float highlightY = lerp(previousY, selectedY, eased);

    drawGradientBorder({kLeftX, highlightY}, {kMenuW, kItemH}, 4.0f);

    for (int i = 0; i < itemIndex(NdsMenuLayer::Item::Count); ++i)
    {
        const auto item = static_cast<NdsMenuLayer::Item>(i);
        const float y = menuItemY(i);
        const bool isSelected = i == selected;
        const Color iconColor = isSelected ? Color{0.82f, 0.94f, 1.0f, 1.0f}
                                           : Color{1.0f, 1.0f, 1.0f, 0.74f};
        const Color textColor = isSelected ? Color{1.0f, 1.0f, 1.0f, 1.0f}
                                           : Color{1.0f, 1.0f, 1.0f, 0.78f};

        Gfx::DrawText(Gfx::SystemFontStandard,
                      {kLeftX + 28.0f, y + kItemH * 0.5f},
                      20.0f,
                      iconColor,
                      Gfx::align_Center,
                      Gfx::align_Center,
                      itemIcon(item));
        Gfx::DrawText(Gfx::SystemFontChinese,
                      {kLeftX + 56.0f, y + 19.0f},
                      18.0f,
                      textColor,
                      "%s", itemLabel(item));
    }

    drawMenuSeparator();
}

void drawFooter()
{
    drawRect({0.0f, 648.0f}, {kScreenW, 72.0f}, {0.0f, 0.0f, 0.0f, 0.40f}, true);
    drawLine({0.0f, 648.0f}, {kScreenW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.14f});

    const float y = 682.0f;
    const float right = 1194.0f;
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {right - 190.0f, y}, 30.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f}, Gfx::align_Center, Gfx::align_Center,
                  GFX_NINTENDOFONT_B_BUTTON);
    Gfx::DrawText(Gfx::SystemFontChinese, {right - 166.0f, y - 10.0f}, 20.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f}, "返回");
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {right - 72.0f, y}, 30.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f}, Gfx::align_Center, Gfx::align_Center,
                  GFX_NINTENDOFONT_A_BUTTON);
    Gfx::DrawText(Gfx::SystemFontChinese, {right - 48.0f, y - 10.0f}, 20.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f}, "确定");
}

void drawSaveSlotCard(int slot, Vector2f pos, bool focused, bool existing)
{
    const Vector2f size{390.0f, 100.0f};
    const Vector2f drawPos = focused ? pos - Vector2f{4.0f, 3.0f} : pos;
    const Vector2f drawSize = focused ? size + Vector2f{8.0f, 6.0f} : size;

    if (focused)
        drawRect(drawPos - Vector2f{4.0f, 4.0f}, drawSize + Vector2f{8.0f, 8.0f},
                 {0.12f, 0.40f, 0.70f, 0.18f}, true);

    drawRect(drawPos, drawSize, {1.0f, 1.0f, 1.0f, 0.04f}, true);
    drawBorder(drawPos, drawSize, focused ? 2.0f : 1.0f,
               focused ? Color{0.31f, 0.70f, 1.0f, 0.96f}
                       : Color{1.0f, 1.0f, 1.0f, 0.10f});

    const Vector2f thumbPos = drawPos + Vector2f{14.0f, 10.0f};
    drawRect(thumbPos, {110.0f, 80.0f}, existing ? Color{0.16f, 0.20f, 0.24f, 0.95f}
                                                 : Color{1.0f, 1.0f, 1.0f, 0.03f});
    drawBorder(thumbPos, {110.0f, 80.0f}, 1.0f, {1.0f, 1.0f, 1.0f, existing ? 0.08f : 0.22f});

    char title[32];
    std::snprintf(title, sizeof(title), "槽位%d", slot + 1);
    if (existing)
    {
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{142.0f, 24.0f}, 22.0f,
                      {1.0f, 1.0f, 1.0f, 0.96f}, "%s", title);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{142.0f, 58.0f}, 15.0f,
                      {1.0f, 1.0f, 1.0f, 0.55f}, "已有状态  00:00");
        Gfx::DrawText(Gfx::SystemFontStandard, thumbPos + Vector2f{55.0f, 40.0f}, 18.0f,
                      {0.75f, 0.88f, 1.0f, 0.42f}, Gfx::align_Center, Gfx::align_Center, "NDS");
    }
    else
    {
        Gfx::DrawText(Gfx::SystemFontStandard, thumbPos + Vector2f{55.0f, 36.0f}, 42.0f,
                      {1.0f, 1.0f, 1.0f, 0.45f}, Gfx::align_Center, Gfx::align_Center, "+");
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{142.0f, 24.0f}, 22.0f,
                      {1.0f, 1.0f, 1.0f, 0.88f}, "%s", title);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{142.0f, 58.0f}, 15.0f,
                      {1.0f, 1.0f, 1.0f, 0.48f}, "空存档槽");
    }
}

void drawSaveSlotGrid(bool loadMode)
{
    constexpr float cardW = 390.0f;
    constexpr float cardH = 100.0f;
    constexpr float gapX = 18.0f;
    constexpr float gapY = 18.0f;
    const Vector2f start{kContentX, kContentY + 70.0f};

    for (int i = 0; i < 6; ++i)
    {
        const int col = i % 2;
        const int row = i / 2;
        const Vector2f pos = start + Vector2f{col * (cardW + gapX), row * (cardH + gapY)};
        drawSaveSlotCard(i, pos, i == 0, loadMode && i < 2);
    }
}

void drawInfoPage(const char* title, const char* body, float offsetX, float opacity)
{
    const Vector2f base{kContentX + offsetX, kContentY};
    const Color titleColor{1.0f, 1.0f, 1.0f, opacity};
    const Color bodyColor{0.80f, 0.90f, 0.98f, opacity * 0.82f};

    Gfx::DrawText(Gfx::SystemFontChinese, base, 20.0f, titleColor, "%s", title);
    drawLine({base.X, base.Y + 44.0f}, {kContentW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, base + Vector2f{0.0f, 96.0f}, 22.0f, bodyColor, "%s", body);
}

void drawDisplayPage(bool linearFiltering, int fastForwardMultiplier, float offsetX, float opacity)
{
    const Vector2f base{kContentX + offsetX, kContentY};
    Gfx::DrawText(Gfx::SystemFontChinese, base, 20.0f, {1.0f, 1.0f, 1.0f, opacity}, "画面设置");
    drawLine({base.X, base.Y + 44.0f}, {kContentW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});

    struct SettingRow {
        const char* label;
        char value[24];
    };

    SettingRow rows[2] = {
        {"画面过滤", ""},
        {"快进倍率", ""},
    };
    std::snprintf(rows[0].value, sizeof(rows[0].value), "%s", filterLabel(linearFiltering));
    std::snprintf(rows[1].value, sizeof(rows[1].value), "x%d", fastForwardMultiplier);

    for (int i = 0; i < 2; ++i)
    {
        const Vector2f pos = base + Vector2f{0.0f, 86.0f + i * 72.0f};
        drawRect(pos, {520.0f, 54.0f}, {1.0f, 1.0f, 1.0f, 0.045f * opacity}, true);
        drawBorder(pos, {520.0f, 54.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 0.11f * opacity});
        Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{22.0f, 17.0f}, 18.0f,
                      {1.0f, 1.0f, 1.0f, 0.88f * opacity}, "%s", rows[i].label);
        Gfx::DrawText(Gfx::SystemFontStandard, pos + Vector2f{455.0f, 17.0f}, 18.0f,
                      {0.78f, 0.92f, 1.0f, 0.94f * opacity}, Gfx::align_Right, Gfx::align_Left,
                      rows[i].value);
    }

    Gfx::DrawText(Gfx::SystemFontChinese, base + Vector2f{0.0f, 272.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, 0.56f * opacity}, "A 切换过滤，左右键调整快进倍率");
}

void drawTabFrame(NdsMenuLayer::Item item,
                  NdsMenuLayer::Item previousItem,
                  float pageProgress,
                  bool linearFiltering,
                  int fastForwardMultiplier)
{
    drawRect({kContentX - 22.0f, kContentY - 24.0f}, {kContentW + 44.0f, kContentH + 34.0f},
             {0.02f, 0.03f, 0.04f, 0.18f}, true);

    auto drawPage = [&](NdsMenuLayer::Item page, float offsetX, float opacity) {
        switch (page)
        {
        case NdsMenuLayer::Item::SaveState:
            Gfx::DrawText(Gfx::SystemFontChinese, {kContentX + offsetX, kContentY}, 20.0f,
                          {1.0f, 1.0f, 1.0f, opacity}, "保存状态");
            drawLine({kContentX + offsetX, kContentY + 44.0f}, {kContentW, 1.0f},
                     {1.0f, 1.0f, 1.0f, 0.10f * opacity});
            if (opacity > 0.5f)
                drawSaveSlotGrid(false);
            break;
        case NdsMenuLayer::Item::LoadState:
            Gfx::DrawText(Gfx::SystemFontChinese, {kContentX + offsetX, kContentY}, 20.0f,
                          {1.0f, 1.0f, 1.0f, opacity}, "读取状态");
            drawLine({kContentX + offsetX, kContentY + 44.0f}, {kContentW, 1.0f},
                     {1.0f, 1.0f, 1.0f, 0.10f * opacity});
            if (opacity > 0.5f)
                drawSaveSlotGrid(true);
            break;
        case NdsMenuLayer::Item::Display:
            drawDisplayPage(linearFiltering, fastForwardMultiplier, offsetX, opacity);
            break;
        case NdsMenuLayer::Item::Cheats:
            drawInfoPage("金手指设置", "金手指列表将在后续阶段接入。", offsetX, opacity);
            break;
        case NdsMenuLayer::Item::Reset:
            drawInfoPage("重置游戏", "按 A 将重新加载当前游戏。", offsetX, opacity);
            break;
        case NdsMenuLayer::Item::Exit:
            drawInfoPage("退出游戏", "按 A 退出 NDS Stub 并返回主程序。", offsetX, opacity);
            break;
        case NdsMenuLayer::Item::Resume:
        default:
            drawInfoPage("返回游戏", "按 A / B / ZR 返回游戏画面。", offsetX, opacity);
            break;
        }
    };

    const float outT = clamp01(pageProgress / 0.68f);
    const float inT = easeOutQuart(pageProgress);
    if (previousItem != item && pageProgress < 1.0f)
        drawPage(previousItem, lerp(0.0f, -50.0f, outT), 1.0f - outT);
    drawPage(item, lerp(120.0f, 0.0f, inT), inT);
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
