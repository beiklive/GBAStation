#include "nds_stub/ui/UiComponents.hpp"

namespace beiklive::nds_stub::ui {

namespace {

constexpr UiMetrics kLandscapeMetrics {
    1280.0f, 720.0f,
    56.0f, 120.0f,
    280.0f, 58.0f, 18.0f,
    340.0f, 110.0f, 500.0f,
    380.0f, 110.0f, 840.0f, 520.0f,
    58.0f, 450.0f,
    386.0f, 94.0f, 18.0f, 14.0f,
    48.0f, 14.0f,
    2,
};

constexpr UiMetrics kPortraitMetrics {
    720.0f, 1280.0f,
    36.0f, 120.0f,
    210.0f, 58.0f, 18.0f,
    268.0f, 110.0f, 1040.0f,
    300.0f, 110.0f, 384.0f, 1040.0f,
    58.0f, 970.0f,
    384.0f, 94.0f, 0.0f, 14.0f,
    48.0f, 14.0f,
    1,
};

int gMenuOrientation = 0;

} // namespace

const UiMetrics& menuMetrics()
{
    return (gMenuOrientation == 1 || gMenuOrientation == 3) ? kPortraitMetrics : kLandscapeMetrics;
}

void setMenuMetricsOrientation(int orientation)
{
    gMenuOrientation = std::clamp(orientation, 0, 3);
}

int saveSlotColumns()
{
    return menuMetrics().saveColumns;
}

float contentBodyHeight()
{
    return menuMetrics().contentBodyH;
}

float saveCardHeight()
{
    return menuMetrics().saveCardH;
}

float saveCardGapY()
{
    return menuMetrics().saveCardGapY;
}

float settingStepY()
{
    return menuMetrics().settingStepY;
}

const char* filterLabel(bool linear)
{
    return linear ? "Linear" : "Nearest";
}

namespace {

const char* layoutLabel(int index)
{
    static const char* labels[] = {"纵向对称", "横向对称", "上屏优先", "下屏优先", "混合横向", "单上屏", "单下屏", "自定义"};
    return labels[std::clamp(index, 0, 7)];
}

const char* orientationLabel(int index)
{
    static const char* labels[] = {"0度", "90度", "180度", "270度"};
    return labels[std::clamp(index, 0, 3)];
}

#define kContentBodyTop (::beiklive::nds_stub::ui::menuMetrics().contentBodyTop)
#define kContentBodyH (::beiklive::nds_stub::ui::menuMetrics().contentBodyH)
#define kSaveCardW (::beiklive::nds_stub::ui::menuMetrics().saveCardW)
#define kSaveCardH (::beiklive::nds_stub::ui::menuMetrics().saveCardH)
#define kSaveCardGapX (::beiklive::nds_stub::ui::menuMetrics().saveCardGapX)
#define kSaveCardGapY (::beiklive::nds_stub::ui::menuMetrics().saveCardGapY)
#define kSettingStepY (::beiklive::nds_stub::ui::menuMetrics().settingStepY)
#define kContentScissorPad (::beiklive::nds_stub::ui::menuMetrics().contentScissorPad)

float settingRowW()
{
    return std::max(320.0f, kContentW - 50.0f);
}

void pushContentBodyScissor(float offsetY)
{
    constexpr float minH = 1.0f;
    float x = kContentX - kContentScissorPad;
    float y = kContentY + kContentBodyTop + offsetY - kContentScissorPad;
    float w = kContentW + kContentScissorPad * 2.0f;
    float h = kContentBodyH + kContentScissorPad * 2.0f;

    if (x < 0.0f)
    {
        w += x;
        x = 0.0f;
    }
    if (x + w > kScreenW)
        w = kScreenW - x;

    if (y < 0.0f)
    {
        h += y;
        y = 0.0f;
    }
    if (y >= kScreenH)
    {
        y = kScreenH - minH;
        h = minH;
    }
    else if (y + h > kScreenH)
    {
        h = kScreenH - y;
    }

    Gfx::PushScissor(static_cast<u32>(x),
                     static_cast<u32>(y),
                     static_cast<u32>(std::max(minH, w)),
                     static_cast<u32>(std::max(minH, h)));
}

void drawBadge(Vector2f pos, Vector2f minSize, const char* text, Color bgColor, Color textColor)
{
    const float fontSize = 14.0f;
    const Vector2f textSize = Gfx::MeasureText(Gfx::SystemFontStandard, fontSize, text);
    const Vector2f size{std::max(minSize.X, textSize.X + 22.0f), minSize.Y};
    drawRect(pos, size, bgColor, true);
    Gfx::DrawText(Gfx::SystemFontStandard,
                  pos + size * 0.5f,
                  fontSize,
                  textColor,
                  Gfx::align_Center,
                  Gfx::align_Center,
                  text);
}

void drawLrSelectorRow(Vector2f pos,
                       const char* label,
                       const char* value,
                       bool focused,
                       bool enabled,
                       float opacity)
{
    const float rowW = settingRowW();
    const Color rowBg = enabled ? Color{1.0f, 1.0f, 1.0f, 0.045f * opacity}
                                : Color{1.0f, 1.0f, 1.0f, 0.020f * opacity};
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {rowW + 6.0f, 48.0f}, 3.0f);
    drawRect(pos, {rowW, 42.0f}, rowBg, true);
    drawBorder(pos, {rowW, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, enabled ? 0.10f * opacity : 0.04f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, enabled ? 0.88f * opacity : 0.34f * opacity}, "%s", label);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{rowW - 180.0f, 21.0f}, 24.0f,
                  {0.80f, 0.92f, 1.0f, enabled ? 0.90f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center, NDS_STUB_KEYICON_LB);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{rowW - 100.0f, 21.0f}, 17.0f,
                  {0.78f, 0.92f, 1.0f, enabled ? 0.96f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center,
                  value);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{rowW - 20.0f, 21.0f}, 24.0f,
                  {0.80f, 0.92f, 1.0f, enabled ? 0.90f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center, NDS_STUB_KEYICON_RB);
}

void drawNumberAdjusterRow(Vector2f pos,
                           const char* label,
                           int value,
                           const char* unit,
                           int defaultValue,
                           int step,
                           bool focused,
                           bool enabled,
                           float opacity)
{
    const float rowW = settingRowW();
    const Color rowBg = enabled ? Color{1.0f, 1.0f, 1.0f, 0.045f * opacity}
                                : Color{1.0f, 1.0f, 1.0f, 0.020f * opacity};
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {rowW + 6.0f, 48.0f}, 3.0f);

    drawRect(pos, {rowW, 42.0f}, rowBg, true);
    drawBorder(pos, {rowW, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, enabled ? 0.10f * opacity : 0.04f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, enabled ? 0.88f * opacity : 0.34f * opacity}, "%s", label);

    char valueText[32];
    std::snprintf(valueText, sizeof(valueText), "%d%s", value, unit ? unit : "");

    char metaText[48];
    std::snprintf(metaText,
                  sizeof(metaText),
                  value == defaultValue ? "默认 / 步长 %d" : "默认 %d / 步长 %d",
                  value == defaultValue ? step : defaultValue,
                  step);

    const float valueCenterX = rowW - 100.0f;
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{rowW - 180.0f, 21.0f}, 24.0f,
                  {0.80f, 0.92f, 1.0f, enabled ? 0.90f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center, NDS_STUB_KEYICON_LB);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{valueCenterX, 12.0f}, 17.0f,
                  {0.78f, 0.92f, 1.0f, enabled ? 0.96f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Left,
                  valueText);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{valueCenterX, 29.0f}, 10.0f,
                  {0.74f, 0.82f, 0.90f, enabled ? 0.52f * opacity : 0.18f * opacity},
                  Gfx::align_Center, Gfx::align_Left,
                  metaText);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{rowW - 20.0f, 21.0f}, 24.0f,
                  {0.80f, 0.92f, 1.0f, enabled ? 0.90f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center, NDS_STUB_KEYICON_RB);
}

void drawFloatAdjusterRow(Vector2f pos,
                          float rowW,
                          const char* label,
                          float value,
                          const char* unit,
                          float defaultValue,
                          float step,
                          bool focused,
                          float opacity,
                          int decimals)
{
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {rowW + 6.0f, 48.0f}, 3.0f);

    drawRect(pos, {rowW, 42.0f}, {1.0f, 1.0f, 1.0f, 0.055f * opacity}, true);
    drawBorder(pos, {rowW, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 0.11f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{16.0f, 12.0f}, 16.0f,
                  {1.0f, 1.0f, 1.0f, 0.90f * opacity}, "%s", label);

    char valueText[40];
    if (decimals <= 0)
        std::snprintf(valueText, sizeof(valueText), "%.0f%s", value, unit ? unit : "");
    else
        std::snprintf(valueText, sizeof(valueText), "%.*f%s", decimals, value, unit ? unit : "");

    char metaText[56];
    if (decimals <= 0)
        std::snprintf(metaText, sizeof(metaText), "默认 %.0f / 步长 %.0f", defaultValue, step);
    else
        std::snprintf(metaText, sizeof(metaText), "默认 %.*f / 步长 %.*f", decimals, defaultValue, decimals, step);

    const float valueCenterX = rowW - 92.0f;
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{rowW - 168.0f, 21.0f}, 23.0f,
                  {0.80f, 0.92f, 1.0f, 0.90f * opacity},
                  Gfx::align_Center, Gfx::align_Center, NDS_STUB_KEYICON_LB);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{valueCenterX, 11.0f}, 16.0f,
                  {0.78f, 0.92f, 1.0f, 0.96f * opacity},
                  Gfx::align_Center, Gfx::align_Left,
                  valueText);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{valueCenterX, 29.0f}, 9.0f,
                  {0.74f, 0.82f, 0.90f, 0.52f * opacity},
                  Gfx::align_Center, Gfx::align_Left,
                  metaText);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{rowW - 18.0f, 21.0f}, 23.0f,
                  {0.80f, 0.92f, 1.0f, 0.90f * opacity},
                  Gfx::align_Center, Gfx::align_Center, NDS_STUB_KEYICON_RB);
}

void drawSwitchRow(Vector2f pos, const char* label, bool value, bool focused, float opacity)
{
    const float rowW = settingRowW();
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {rowW + 6.0f, 48.0f}, 3.0f);
    drawRect(pos, {rowW, 42.0f}, {1.0f, 1.0f, 1.0f, 0.045f * opacity}, true);
    drawBorder(pos, {rowW, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, 0.88f * opacity}, "%s", label);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{rowW - 44.0f, 12.0f}, 17.0f,
                  value ? Color{0.34f, 0.78f, 1.0f, 0.96f * opacity}
                        : Color{0.60f, 0.64f, 0.68f, 0.80f * opacity},
                  Gfx::align_Right, Gfx::align_Left, value ? "开" : "关");
}

void drawSubPageRow(Vector2f pos, const char* label, bool focused, bool enabled, float opacity)
{
    const float rowW = settingRowW();
    if (focused && enabled)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {rowW + 6.0f, 48.0f}, 3.0f);
    drawRect(pos, {rowW, 42.0f}, {1.0f, 1.0f, 1.0f, enabled ? 0.045f * opacity : 0.020f * opacity}, true);
    drawBorder(pos, {rowW, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, enabled ? 0.10f * opacity : 0.04f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, enabled ? 0.88f * opacity : 0.34f * opacity}, "%s", label);
    Gfx::DrawText(Gfx::SystemFontStandard, pos + Vector2f{rowW - 38.0f, 7.0f}, 28.0f,
                  {0.32f, 0.75f, 1.0f, enabled ? 0.96f * opacity : 0.25f * opacity}, ">");
}

void drawButtonRow(Vector2f pos, const char* label, bool focused, float opacity)
{
    const float rowW = settingRowW();
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {rowW + 6.0f, 48.0f}, 3.0f);
    drawRect(pos, {rowW, 42.0f}, {1.0f, 1.0f, 1.0f, 0.045f * opacity}, true);
    drawBorder(pos, {rowW, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, 0.88f * opacity}, "%s", label);
}

void drawSectionLabel(Vector2f pos, const char* label, float opacity)
{
    const float rowW = settingRowW();
    const float leftW = std::max(72.0f, rowW * 0.30f);
    drawLine(pos + Vector2f{0.0f, 10.0f}, {leftW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{leftW + 18.0f, 0.0f}, 16.0f,
                  {0.72f, 0.82f, 0.92f, 0.70f * opacity}, "%s", label);
    drawLine(pos + Vector2f{leftW + 140.0f, 10.0f},
             {std::max(24.0f, rowW - leftW - 140.0f), 1.0f},
             {1.0f, 1.0f, 1.0f, 0.10f * opacity});
}

} // namespace

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

float menuItemY(int index)
{
    float y = kLeftY + index * (kItemH + kItemGap);
    if (index >= itemIndex(NdsMenuLayer::Item::Reset))
        y += 18.0f;
    return y;
}

void drawOverlay(float alphaScale)
{
    alphaScale = clamp01(alphaScale);
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.70f * alphaScale}, true);

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
                  lerp(0.48f, 0.82f, t) * alphaScale},
                 true);
    }
}

void drawHeader(float offsetY)
{
    const float padX = (kScreenW <= 720.0f) ? 36.0f : 64.0f;
    Gfx::DrawText(Gfx::SystemFontChinese, {padX, 30.0f + offsetY}, 26.0f,
                  {1.0f, 1.0f, 1.0f, 1.0f}, "游戏菜单");
    drawLine({padX - 8.0f, 92.0f + offsetY}, {kScreenW - (padX - 8.0f) * 2.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.18f});
}

void drawGameStatusBadges(double fps,
                          bool showFps,
                          bool fastForwardActive,
                          bool showFastForward,
                          bool paused)
{
    if (showFps && fps > 0.0)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
        drawBadge({4.0f, 4.0f}, {90.0f, 22.0f}, buf,
                  {0.0f, 0.0f, 0.0f, 0.63f},
                  {0.0f, 1.0f, 0.31f, 0.90f});
    }

    if (fastForwardActive && showFastForward)
    {
        drawBadge({kScreenW - 94.0f, 4.0f}, {90.0f, 22.0f}, ">>>",
                  {0.0f, 0.0f, 0.0f, 0.63f},
                  {0.39f, 0.86f, 1.0f, 0.90f});
    }

    if (paused && !fastForwardActive)
    {
        drawBadge({(kScreenW - 90.0f) * 0.5f, 4.0f}, {90.0f, 22.0f}, "Paused",
                  {0.0f, 0.0f, 0.0f, 0.70f},
                  {1.0f, 0.86f, 0.24f, 0.90f});
    }
}

void drawMenuSeparator(float offsetY)
{
    const float y = menuItemY(itemIndex(NdsMenuLayer::Item::Reset)) - 14.0f;
    drawLine({kLeftX + 18.0f, y + offsetY}, {kMenuW - 36.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.14f});
}

void drawLeftMenu(int selected,
                  int previousSelected,
                  float selectionProgress,
                  bool tabsFocused,
                  float offsetY)
{
    const float eased = easeOutCubic(selectionProgress);
    const float previousY = menuItemY(previousSelected);
    const float selectedY = menuItemY(selected);
    const float highlightY = lerp(previousY, selectedY, eased);

    if (tabsFocused)
        drawGradientBorder({kLeftX, highlightY + offsetY}, {kMenuW, kItemH}, 4.0f);
    else
        drawRect({kLeftX, highlightY + offsetY}, {kMenuW, kItemH}, {0.13f, 0.42f, 0.70f, 0.20f}, true);

    for (int i = 0; i < itemIndex(NdsMenuLayer::Item::Count); ++i)
    {
        const auto item = static_cast<NdsMenuLayer::Item>(i);
        const float y = menuItemY(i);
        const bool isSelected = i == selected;
        if (isSelected)
            drawRect({kLeftX + 4.0f, y + offsetY + 4.0f}, {kMenuW - 8.0f, kItemH - 8.0f},
                     {0.12f, 0.38f, 0.66f, tabsFocused ? 0.16f : 0.26f}, true);

        const Color textColor = isSelected ? Color{1.0f, 1.0f, 1.0f, 1.0f}
                                           : Color{1.0f, 1.0f, 1.0f, 0.78f};

        Gfx::DrawText(Gfx::SystemFontChinese,
                      {kLeftX + 28.0f, y + offsetY + 19.0f},
                      18.0f,
                      textColor,
                      "%s", itemLabel(item));
    }

    drawMenuSeparator(offsetY);
}

void drawFooter(bool contentFocused, bool canDelete, float offsetY)
{
    const float footerY = kScreenH - 72.0f + offsetY;
    drawRect({0.0f, footerY}, {kScreenW, 72.0f}, {0.0f, 0.0f, 0.0f, 0.40f}, true);
    drawLine({0.0f, footerY}, {kScreenW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.14f});

    const float y = kScreenH - 38.0f + offsetY;
    float right = kScreenW - 86.0f;
    auto drawHint = [&](const char* icon, const char* text) {
        constexpr float iconSize = 30.0f;
        constexpr float textSize = 20.0f;
        constexpr float iconTextGap = 10.0f;
        constexpr float groupGap = 28.0f;

        const Vector2f textSizePx = Gfx::MeasureText(Gfx::SystemFontChinese, textSize, text);
        const float textW = std::max(28.0f, textSizePx.X);
        const float groupW = iconSize + iconTextGap + textW;
        const float x = right - groupW;

        Gfx::DrawText(Gfx::SystemFontNintendoExt, {x + iconSize * 0.5f, y}, iconSize,
                      {1.0f, 1.0f, 1.0f, 0.92f}, Gfx::align_Center, Gfx::align_Center, icon);
        Gfx::DrawText(Gfx::SystemFontChinese, {x + iconSize + iconTextGap, y - 10.0f}, textSize,
                      {1.0f, 1.0f, 1.0f, 0.76f}, "%s", text);
        right = x - groupGap;
    };

    drawHint(NDS_STUB_KEYICON_A, "确定");
    drawHint(NDS_STUB_KEYICON_B, contentFocused ? "返回列表" : "返回");
    if (canDelete)
        drawHint(NDS_STUB_KEYICON_X, "删除");
}

void drawSaveSlotCard(int slot, Vector2f pos, bool focused, const NdsStateSlotInfo& info, float offsetY)
{
    pos.Y += offsetY;
    const Vector2f size{kSaveCardW, kSaveCardH};
    const Vector2f drawPos = focused ? pos - Vector2f{3.0f, 2.0f} : pos;
    const Vector2f drawSize = focused ? size + Vector2f{6.0f, 4.0f} : size;
    if (focused)
        drawGradientBorder(drawPos - Vector2f{3.0f, 3.0f}, drawSize + Vector2f{6.0f, 6.0f}, 3.0f);

    drawRect(drawPos, drawSize, {1.0f, 1.0f, 1.0f, info.exists ? 0.045f : 0.026f}, true);
    drawBorder(drawPos, drawSize, focused ? 2.0f : 1.0f,
               focused ? Color{0.31f, 0.70f, 1.0f, 0.96f}
                       : Color{1.0f, 1.0f, 1.0f, info.exists ? 0.10f : 0.06f});

    const Vector2f thumbSize{112.0f, 72.0f};
    const Vector2f thumbPos = drawPos + Vector2f{12.0f, 11.0f};
    drawRect(thumbPos, thumbSize, info.exists ? Color{0.12f, 0.17f, 0.22f, 0.96f}
                                              : Color{1.0f, 1.0f, 1.0f, 0.025f});
    if (info.exists && info.thumbnailTexture != 0 && info.thumbnailWidth > 0 && info.thumbnailHeight > 0)
    {
        const float texAspect = static_cast<float>(info.thumbnailWidth) / static_cast<float>(info.thumbnailHeight);
        const float boxAspect = thumbSize.X / thumbSize.Y;
        Vector2f fittedSize = thumbSize;
        if (texAspect > boxAspect)
        {
            fittedSize.Y = thumbSize.X / texAspect;
        }
        else
        {
            fittedSize.X = thumbSize.Y * texAspect;
        }
        const Vector2f fittedPos = thumbPos + (thumbSize - fittedSize) * 0.5f;
        Gfx::SetSampler(Gfx::sampler_Linear | Gfx::sampler_ClampToEdge);
        Gfx::DrawRectangle(info.thumbnailTexture,
                           fittedPos,
                           fittedSize,
                           {0.0f, 0.0f},
                           {static_cast<float>(info.thumbnailWidth),
                            static_cast<float>(info.thumbnailHeight)},
                           {1.0f, 1.0f, 1.0f, 1.0f});
        Gfx::SetSampler(Gfx::sampler_Nearest | Gfx::sampler_ClampToEdge);
    }
    drawBorder(thumbPos, thumbSize, 1.0f, {1.0f, 1.0f, 1.0f, info.exists ? 0.12f : 0.16f});

    char title[32];
    std::snprintf(title, sizeof(title), "槽位 %d", slot);
    if (info.exists)
    {
        const float textX = std::min(142.0f, drawSize.X * 0.38f);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{textX, 22.0f}, 20.0f,
                      {1.0f, 1.0f, 1.0f, 0.96f}, "%s", title);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{textX, 54.0f}, 14.0f,
                      {1.0f, 1.0f, 1.0f, 0.55f}, "%s", info.modifiedTime.empty() ? "已有状态" : info.modifiedTime.c_str());
        if (info.thumbnailTexture == 0)
            Gfx::DrawText(Gfx::SystemFontStandard, thumbPos + thumbSize * 0.5f, 13.0f,
                          {0.75f, 0.88f, 1.0f, 0.46f}, Gfx::align_Center, Gfx::align_Center,
                          !info.thumbnailCacheAvailable ? "NO THUMB" :
                          (info.thumbnailLoadAttempted ? "LOAD FAIL" : "SCREEN"));
    }
    else
    {
        Gfx::DrawText(Gfx::SystemFontStandard, thumbPos + thumbSize * 0.5f, 34.0f,
                      {1.0f, 1.0f, 1.0f, 0.45f}, Gfx::align_Center, Gfx::align_Center, "+");
        const float textX = std::min(142.0f, drawSize.X * 0.38f);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{textX, 22.0f}, 20.0f,
                      {1.0f, 1.0f, 1.0f, 0.88f}, "%s", title);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{textX, 54.0f}, 14.0f,
                      {1.0f, 1.0f, 1.0f, 0.48f}, "空存档槽");
    }
}

void drawSaveSlotGrid(const std::array<NdsStateSlotInfo, 10>& slots,
                      int focusedSlot,
                      bool contentFocused,
                      float offsetX,
                      float scrollY,
                      float opacity,
                      float offsetY)
{
    const Vector2f start{kContentX + offsetX, kContentY + kContentBodyTop - scrollY};
    const int columns = saveSlotColumns();

    for (int i = 0; i < 10; ++i)
    {
        const int col = i % columns;
        const int row = i / columns;
        const Vector2f pos = start + Vector2f{col * (kSaveCardW + kSaveCardGapX),
                                              row * (kSaveCardH + kSaveCardGapY)};
        if (pos.Y + offsetY > kContentY + kContentH || pos.Y + offsetY + kSaveCardH < kContentY)
            continue;
        drawSaveSlotCard(i, pos, contentFocused && i == focusedSlot, slots[i], offsetY);
    }

    if (opacity > 0.5f && scrollY > 1.0f)
        drawRect({kContentX + kContentW - 4.0f, kContentY + kContentBodyTop + offsetY},
                 {3.0f, kContentBodyH}, {1.0f, 1.0f, 1.0f, 0.08f});
}

void drawInfoPage(const char* title, const char* body, float offsetX, float offsetY, float opacity)
{
    const Vector2f base{kContentX + offsetX, kContentY + offsetY};
    const Color titleColor{1.0f, 1.0f, 1.0f, opacity};
    const Color bodyColor{0.80f, 0.90f, 0.98f, opacity * 0.82f};

    Gfx::DrawText(Gfx::SystemFontChinese, base, 20.0f, titleColor, "%s", title);
    drawLine({base.X, base.Y + 44.0f}, {kContentW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, base + Vector2f{0.0f, 96.0f}, 22.0f, bodyColor, "%s", body);
}

void drawDisplayPage(bool linearFiltering,
                     float fastForwardMultiplier,
                     bool integerScale,
                     int layout,
                     int orientation,
                     int screenGap,
                     int focusedRow,
                     bool contentFocused,
                     float offsetX,
                     float offsetY,
                     float opacity,
                     float scrollY)
{
    const Vector2f base{kContentX + offsetX, kContentY + offsetY};
    Gfx::DrawText(Gfx::SystemFontChinese, base, 20.0f, {1.0f, 1.0f, 1.0f, opacity}, "画面设置");
    drawLine({base.X, base.Y + 44.0f}, {kContentW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});

    char ffValue[24];
    if (fastForwardMultiplier < 1.0f)
        std::snprintf(ffValue, sizeof(ffValue), "%.1fx", fastForwardMultiplier);
    else if (std::fabs(fastForwardMultiplier - std::round(fastForwardMultiplier)) < 0.01f)
        std::snprintf(ffValue, sizeof(ffValue), "%.0fx", fastForwardMultiplier);
    else
        std::snprintf(ffValue, sizeof(ffValue), "%.2fx", fastForwardMultiplier);
    pushContentBodyScissor(offsetY);

    float y = kContentBodyTop - scrollY;
    auto rowPos = [&](float rowY) { return base + Vector2f{0.0f, rowY}; };
    drawLrSelectorRow(rowPos(y), "快进倍率", ffValue, contentFocused && focusedRow == 0, true, opacity); y += kSettingStepY;
    drawLrSelectorRow(rowPos(y), "画面过滤", filterLabel(linearFiltering), contentFocused && focusedRow == 1, true, opacity); y += kSettingStepY;
    drawSwitchRow(rowPos(y), "整数倍缩放", integerScale, contentFocused && focusedRow == 2, opacity); y += kSettingStepY;
    drawLrSelectorRow(rowPos(y), "画面布局", layoutLabel(layout), contentFocused && focusedRow == 3, true, opacity); y += kSettingStepY;
    drawSubPageRow(rowPos(y), "自定义画面布局", contentFocused && focusedRow == 4, layout == 7, opacity); y += kSettingStepY;
    drawLrSelectorRow(rowPos(y), "画面方向", orientationLabel(orientation), contentFocused && focusedRow == 5, true, opacity); y += kSettingStepY;
    drawNumberAdjusterRow(rowPos(y), "屏幕间距", screenGap, "px", 0, 1, contentFocused && focusedRow == 6, true, opacity); y += 54.0f;
    drawSectionLabel(rowPos(y + 2.0f), "个性化设置", opacity); y += 30.0f;
    drawSubPageRow(rowPos(y), "遮罩选择", contentFocused && focusedRow == 7, true, opacity); y += kSettingStepY;
    drawSubPageRow(rowPos(y), "滤镜选择", contentFocused && focusedRow == 8, true, opacity); y += 54.0f;
    drawSectionLabel(rowPos(y + 2.0f), "同步设置", opacity); y += 30.0f;
    drawButtonRow(rowPos(y), "同步画面设置", contentFocused && focusedRow == 9, opacity); y += kSettingStepY;
    drawButtonRow(rowPos(y), "同步遮罩设置", contentFocused && focusedRow == 10, opacity); y += kSettingStepY;
    drawButtonRow(rowPos(y), "同步滤镜设置", contentFocused && focusedRow == 11, opacity);

    if (opacity > 0.5f && scrollY > 1.0f)
        drawRect({kContentX + kContentW - 4.0f, kContentY + kContentBodyTop + offsetY},
                 {3.0f, kContentBodyH}, {1.0f, 1.0f, 1.0f, 0.08f});

    Gfx::PopScissor();
}

void drawDeleteDialog(int slot, float opacity)
{
    opacity = clamp01(opacity);
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.54f * opacity}, true);
    const Vector2f size{std::min(500.0f, kScreenW - 72.0f), 210.0f};
    const Vector2f pos{(kScreenW - size.X) * 0.5f, (kScreenH - size.Y) * 0.5f};
    drawRect(pos, size, {0.04f, 0.055f, 0.075f, 0.96f * opacity}, true);
    drawBorder(pos, size, 1.0f, {1.0f, 1.0f, 1.0f, 0.16f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{34.0f, 30.0f}, 24.0f,
                  {1.0f, 1.0f, 1.0f, 0.96f * opacity}, "删除即时存档");
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{34.0f, 88.0f}, 18.0f,
                  {1.0f, 1.0f, 1.0f, 0.72f * opacity}, "确认删除 ss%d 及对应截图？", slot);
    const float y = pos.Y + 162.0f;
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {pos.X + 328.0f, y}, 28.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f * opacity}, Gfx::align_Center, Gfx::align_Center,
                  NDS_STUB_KEYICON_B);
    Gfx::DrawText(Gfx::SystemFontChinese, {pos.X + 350.0f, y - 9.0f}, 18.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f * opacity}, "取消");
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {pos.X + 424.0f, y}, 28.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f * opacity}, Gfx::align_Center, Gfx::align_Center,
                  NDS_STUB_KEYICON_A);
    Gfx::DrawText(Gfx::SystemFontChinese, {pos.X + 446.0f, y - 9.0f}, 18.0f,
                  {0.38f, 0.78f, 1.0f, 0.92f * opacity}, "删除");
}

void drawCustomLayoutSidebar(const NdsCustomLayoutSettings& settings,
                             int focusedRow,
                             float progress,
                             float opacity)
{
    opacity = clamp01(opacity);
    progress = easeOutQuart(clamp01(progress));
    const bool portrait = kScreenH > kScreenW;
    const float panelW = portrait ? 320.0f : 360.0f;
    const float panelX = kScreenW - panelW + (1.0f - progress) * panelW;
    const float rowW = panelW - 48.0f;
    const Vector2f panelPos{panelX, 0.0f};
    const float headerY = portrait ? 38.0f : 30.0f;
    const float hintY = headerY + 32.0f;
    const float topSectionY = portrait ? 144.0f : 108.0f;
    const float topRowY = topSectionY + 34.0f;
    const float rowGap = portrait ? 62.0f : 54.0f;
    const float bottomSectionY = topRowY + rowGap * 3.0f + (portrait ? 50.0f : 22.0f);
    const float bottomRowY = bottomSectionY + 34.0f;

    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.22f * opacity}, true);
    drawRect(panelPos, {panelW, kScreenH}, {0.015f, 0.020f, 0.030f, 0.94f * opacity}, true);
    drawLine({panelX, 0.0f}, {1.0f, kScreenH}, {1.0f, 1.0f, 1.0f, 0.14f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, {panelX + 28.0f, headerY}, 23.0f,
                  {1.0f, 1.0f, 1.0f, 0.96f * opacity}, "自定义画面布局");
    Gfx::DrawText(Gfx::SystemFontChinese, {panelX + 28.0f, hintY}, 13.0f,
                  {0.78f, 0.86f, 0.94f, 0.62f * opacity}, "B 返回   A 重置当前项");

    auto section = [&](float y, const char* text) {
        drawLine({panelX + 24.0f, y + 10.0f}, {82.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.13f * opacity});
        Gfx::DrawText(Gfx::SystemFontChinese, {panelX + 118.0f, y}, 15.0f,
                      {0.72f, 0.84f, 0.96f, 0.76f * opacity}, "%s", text);
        drawLine({panelX + 208.0f, y + 10.0f}, {panelW - 232.0f, 1.0f},
                 {1.0f, 1.0f, 1.0f, 0.13f * opacity});
    };

    section(topSectionY, "上屏布局");
    drawFloatAdjusterRow({panelX + 24.0f, topRowY}, rowW, "缩放", settings.topScale, "", 1.0f, 0.1f, focusedRow == 0, opacity, 1);
    drawFloatAdjusterRow({panelX + 24.0f, topRowY + rowGap}, rowW, "X偏移", settings.topOffsetX, "px", 0.0f, 1.0f, focusedRow == 1, opacity, 0);
    drawFloatAdjusterRow({panelX + 24.0f, topRowY + rowGap * 2.0f}, rowW, "Y偏移", settings.topOffsetY, "px", 0.0f, 1.0f, focusedRow == 2, opacity, 0);

    section(bottomSectionY, "下屏布局");
    drawFloatAdjusterRow({panelX + 24.0f, bottomRowY}, rowW, "缩放", settings.bottomScale, "", 1.0f, 0.1f, focusedRow == 3, opacity, 1);
    drawFloatAdjusterRow({panelX + 24.0f, bottomRowY + rowGap}, rowW, "X偏移", settings.bottomOffsetX, "px", 0.0f, 1.0f, focusedRow == 4, opacity, 0);
    drawFloatAdjusterRow({panelX + 24.0f, bottomRowY + rowGap * 2.0f}, rowW, "Y偏移", settings.bottomOffsetY, "px", 0.0f, 1.0f, focusedRow == 5, opacity, 0);
}

void drawTabFrame(NdsMenuLayer::Item item,
                  NdsMenuLayer::Item previousItem,
                  float pageProgress,
                  const NdsDisplaySettings& display,
                  const std::array<NdsStateSlotInfo, 10>& slots,
                  int contentFocus,
                  bool contentFocused,
                  float contentScrollY,
                  float offsetY)
{
    if (item == NdsMenuLayer::Item::Resume ||
        item == NdsMenuLayer::Item::Reset ||
        item == NdsMenuLayer::Item::Exit)
        return;

    drawRect({kContentX - 22.0f, kContentY - 24.0f + offsetY}, {kContentW + 44.0f, kContentH + 34.0f},
             {0.02f, 0.03f, 0.04f, 0.18f}, true);

    auto drawPage = [&](NdsMenuLayer::Item page, float offsetX, float opacity) {
        switch (page)
        {
        case NdsMenuLayer::Item::SaveState:
            Gfx::DrawText(Gfx::SystemFontChinese, {kContentX + offsetX, kContentY + offsetY}, 20.0f,
                          {1.0f, 1.0f, 1.0f, opacity}, "保存状态");
            drawLine({kContentX + offsetX, kContentY + offsetY + 44.0f}, {kContentW, 1.0f},
                     {1.0f, 1.0f, 1.0f, 0.10f * opacity});
            if (opacity > 0.5f)
            {
                pushContentBodyScissor(offsetY);
                drawSaveSlotGrid(slots,
                                 contentFocus,
                                 contentFocused,
                                 offsetX,
                                 contentScrollY,
                                 opacity,
                                 offsetY);
                Gfx::PopScissor();
            }
            break;
        case NdsMenuLayer::Item::LoadState:
            Gfx::DrawText(Gfx::SystemFontChinese, {kContentX + offsetX, kContentY + offsetY}, 20.0f,
                          {1.0f, 1.0f, 1.0f, opacity}, "读取状态");
            drawLine({kContentX + offsetX, kContentY + offsetY + 44.0f}, {kContentW, 1.0f},
                     {1.0f, 1.0f, 1.0f, 0.10f * opacity});
            if (opacity > 0.5f)
            {
                pushContentBodyScissor(offsetY);
                drawSaveSlotGrid(slots,
                                 contentFocus,
                                 contentFocused,
                                 offsetX,
                                 contentScrollY,
                                 opacity,
                                 offsetY);
                Gfx::PopScissor();
            }
            break;
        case NdsMenuLayer::Item::Display:
            drawDisplayPage(display.linearFiltering,
                            display.fastForwardMultiplier,
                            display.integerScale,
                            display.layout,
                            display.orientation,
                            display.screenGap,
                            contentFocus,
                            contentFocused,
                            offsetX,
                            offsetY,
                            opacity,
                            contentScrollY);
            break;
        case NdsMenuLayer::Item::Cheats:
            drawInfoPage("金手指设置", "金手指列表将在后续阶段接入。", offsetX, offsetY, opacity);
            break;
        case NdsMenuLayer::Item::Reset:
            drawInfoPage("重置游戏", "按 A 将重新加载当前游戏。", offsetX, offsetY, opacity);
            break;
        case NdsMenuLayer::Item::Exit:
            drawInfoPage("退出游戏", "按 A 退出 NDS Stub 并返回主程序。", offsetX, offsetY, opacity);
            break;
        case NdsMenuLayer::Item::Resume:
        default:
            drawInfoPage("返回游戏", "按 A / B 返回游戏画面。", offsetX, offsetY, opacity);
            break;
        }
    };

    const float outT = clamp01(pageProgress / 0.68f);
    const float inT = easeOutQuart(pageProgress);
    const bool previousHasPage = previousItem == NdsMenuLayer::Item::SaveState ||
                                 previousItem == NdsMenuLayer::Item::LoadState ||
                                 previousItem == NdsMenuLayer::Item::Cheats ||
                                 previousItem == NdsMenuLayer::Item::Display;
    if (previousItem != item && previousHasPage && pageProgress < 1.0f)
        drawPage(previousItem, lerp(0.0f, -50.0f, outT), 1.0f - outT);
    drawPage(item, lerp(120.0f, 0.0f, inT), inT);
}

} // namespace beiklive::nds_stub::ui
