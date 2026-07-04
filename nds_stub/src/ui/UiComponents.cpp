#include "nds_stub/ui/UiComponents.hpp"

namespace beiklive::nds_stub::ui {

const char* filterLabel(bool linear)
{
    return linear ? "Linear" : "Nearest";
}

namespace {

const char* layoutLabel(int index)
{
    static const char* labels[] = {"对称", "大屏优先", "小屏优先", "混合", "自定义"};
    return labels[std::clamp(index, 0, 4)];
}

const char* orientationLabel(int index)
{
    static const char* labels[] = {"0度", "90度", "180度", "270度"};
    return labels[std::clamp(index, 0, 3)];
}

void drawLrSelectorRow(Vector2f pos,
                       const char* label,
                       const char* value,
                       bool focused,
                       bool enabled,
                       float opacity)
{
    const Color rowBg = enabled ? Color{1.0f, 1.0f, 1.0f, 0.045f * opacity}
                                : Color{1.0f, 1.0f, 1.0f, 0.020f * opacity};
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {796.0f, 48.0f}, 3.0f);
    drawRect(pos, {790.0f, 42.0f}, rowBg, true);
    drawBorder(pos, {790.0f, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, enabled ? 0.10f * opacity : 0.04f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, enabled ? 0.88f * opacity : 0.34f * opacity}, "%s", label);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{610.0f, 21.0f}, 24.0f,
                  {0.80f, 0.92f, 1.0f, enabled ? 0.90f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center, "L");
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{692.0f, 12.0f}, 17.0f,
                  {0.78f, 0.92f, 1.0f, enabled ? 0.96f * opacity : 0.28f * opacity},
                  "%s", value);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, pos + Vector2f{770.0f, 21.0f}, 24.0f,
                  {0.80f, 0.92f, 1.0f, enabled ? 0.90f * opacity : 0.28f * opacity},
                  Gfx::align_Center, Gfx::align_Center, "R");
}

void drawSwitchRow(Vector2f pos, const char* label, bool value, bool focused, float opacity)
{
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {796.0f, 48.0f}, 3.0f);
    drawRect(pos, {790.0f, 42.0f}, {1.0f, 1.0f, 1.0f, 0.045f * opacity}, true);
    drawBorder(pos, {790.0f, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, 0.88f * opacity}, "%s", label);
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{746.0f, 12.0f}, 17.0f,
                  value ? Color{0.34f, 0.78f, 1.0f, 0.96f * opacity}
                        : Color{0.60f, 0.64f, 0.68f, 0.80f * opacity},
                  Gfx::align_Right, Gfx::align_Left, value ? "开" : "关");
}

void drawSubPageRow(Vector2f pos, const char* label, bool focused, bool enabled, float opacity)
{
    if (focused && enabled)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {796.0f, 48.0f}, 3.0f);
    drawRect(pos, {790.0f, 42.0f}, {1.0f, 1.0f, 1.0f, enabled ? 0.045f * opacity : 0.020f * opacity}, true);
    drawBorder(pos, {790.0f, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, enabled ? 0.10f * opacity : 0.04f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, enabled ? 0.88f * opacity : 0.34f * opacity}, "%s", label);
    Gfx::DrawText(Gfx::SystemFontStandard, pos + Vector2f{752.0f, 7.0f}, 28.0f,
                  {0.32f, 0.75f, 1.0f, enabled ? 0.96f * opacity : 0.25f * opacity}, ">");
}

void drawButtonRow(Vector2f pos, const char* label, bool focused, float opacity)
{
    if (focused)
        drawGradientBorder(pos - Vector2f{3.0f, 3.0f}, {796.0f, 48.0f}, 3.0f);
    drawRect(pos, {790.0f, 42.0f}, {1.0f, 1.0f, 1.0f, 0.045f * opacity}, true);
    drawBorder(pos, {790.0f, 42.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{18.0f, 12.0f}, 17.0f,
                  {1.0f, 1.0f, 1.0f, 0.88f * opacity}, "%s", label);
}

void drawSectionLabel(Vector2f pos, const char* label, float opacity)
{
    drawLine(pos + Vector2f{0.0f, 10.0f}, {250.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{268.0f, 0.0f}, 16.0f,
                  {0.72f, 0.82f, 0.92f, 0.70f * opacity}, "%s", label);
    drawLine(pos + Vector2f{390.0f, 10.0f}, {400.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.10f * opacity});
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

const char* itemIconPath(NdsMenuLayer::Item item)
{
    switch (item)
    {
    case NdsMenuLayer::Item::Resume: return "romfs:/ui/menu/resume.png";
    case NdsMenuLayer::Item::SaveState: return "romfs:/ui/menu/save_state.png";
    case NdsMenuLayer::Item::LoadState: return "romfs:/ui/menu/load_state.png";
    case NdsMenuLayer::Item::Cheats: return "romfs:/ui/menu/cheats.png";
    case NdsMenuLayer::Item::Display: return "romfs:/ui/menu/display.png";
    case NdsMenuLayer::Item::Reset: return "romfs:/ui/menu/reset.png";
    case NdsMenuLayer::Item::Exit: return "romfs:/ui/menu/exit.png";
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
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.54f * alphaScale}, true);

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
                  lerp(0.38f, 0.68f, t) * alphaScale},
                 true);
    }
}

void drawHeader(float offsetY)
{
    Gfx::DrawText(Gfx::SystemFontChinese, {64.0f, 30.0f + offsetY}, 26.0f,
                  {1.0f, 1.0f, 1.0f, 1.0f}, "游戏菜单");
    drawLine({56.0f, 92.0f + offsetY}, {1168.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.18f});
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

        const Color iconColor = isSelected ? Color{0.66f, 0.88f, 1.0f, 1.0f}
                                           : Color{1.0f, 1.0f, 1.0f, 0.74f};
        const Color textColor = isSelected ? Color{1.0f, 1.0f, 1.0f, 1.0f}
                                           : Color{1.0f, 1.0f, 1.0f, 0.78f};

        const Vector2f iconPos{kLeftX + 17.0f, y + offsetY + 17.0f};
        drawRect(iconPos, {24.0f, 24.0f}, isSelected ? Color{0.30f, 0.62f, 0.92f, 0.30f}
                                                      : Color{1.0f, 1.0f, 1.0f, 0.07f}, true);
        drawBorder(iconPos, {24.0f, 24.0f}, 1.0f, isSelected ? Color{0.70f, 0.88f, 1.0f, 0.42f}
                                                             : Color{1.0f, 1.0f, 1.0f, 0.10f});
        (void)itemIconPath(item);
        Gfx::DrawText(Gfx::SystemFontStandard,
                      {kLeftX + 29.0f, y + offsetY + kItemH * 0.5f},
                      15.0f,
                      iconColor,
                      Gfx::align_Center,
                      Gfx::align_Center,
                      itemIcon(item));
        Gfx::DrawText(Gfx::SystemFontChinese,
                      {kLeftX + 56.0f, y + offsetY + 19.0f},
                      18.0f,
                      textColor,
                      "%s", itemLabel(item));
    }

    drawMenuSeparator(offsetY);
}

void drawFooter(bool contentFocused, bool canDelete, float offsetY)
{
    drawRect({0.0f, 648.0f + offsetY}, {kScreenW, 72.0f}, {0.0f, 0.0f, 0.0f, 0.40f}, true);
    drawLine({0.0f, 648.0f + offsetY}, {kScreenW, 1.0f}, {1.0f, 1.0f, 1.0f, 0.14f});

    const float y = 682.0f + offsetY;
    const float right = 1194.0f;
    float x = right - (canDelete ? 314.0f : 190.0f);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {right - 190.0f, y}, 30.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f}, Gfx::align_Center, Gfx::align_Center,
                  GFX_NINTENDOFONT_B_BUTTON);
    Gfx::DrawText(Gfx::SystemFontChinese, {right - 166.0f, y - 10.0f}, 20.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f}, contentFocused ? "返回列表" : "返回");
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {right - 72.0f, y}, 30.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f}, Gfx::align_Center, Gfx::align_Center,
                  GFX_NINTENDOFONT_A_BUTTON);
    Gfx::DrawText(Gfx::SystemFontChinese, {right - 48.0f, y - 10.0f}, 20.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f}, "确定");
    if (canDelete)
    {
        Gfx::DrawText(Gfx::SystemFontNintendoExt, {x, y}, 30.0f,
                      {1.0f, 1.0f, 1.0f, 0.92f}, Gfx::align_Center, Gfx::align_Center,
                      GFX_NINTENDOFONT_X_BUTTON);
        Gfx::DrawText(Gfx::SystemFontChinese, {x + 24.0f, y - 10.0f}, 20.0f,
                      {1.0f, 1.0f, 1.0f, 0.76f}, "删除");
    }
}

void drawSaveSlotCard(int slot, Vector2f pos, bool focused, const NdsStateSlotInfo& info, float offsetY)
{
    pos.Y += offsetY;
    const Vector2f size{790.0f, 44.0f};
    const Vector2f drawPos = focused ? pos - Vector2f{3.0f, 2.0f} : pos;
    const Vector2f drawSize = focused ? size + Vector2f{6.0f, 4.0f} : size;
    if (focused)
        drawGradientBorder(drawPos - Vector2f{3.0f, 3.0f}, drawSize + Vector2f{6.0f, 6.0f}, 3.0f);

    drawRect(drawPos, drawSize, {1.0f, 1.0f, 1.0f, info.exists ? 0.045f : 0.026f}, true);
    drawBorder(drawPos, drawSize, focused ? 2.0f : 1.0f,
               focused ? Color{0.31f, 0.70f, 1.0f, 0.96f}
                       : Color{1.0f, 1.0f, 1.0f, info.exists ? 0.10f : 0.06f});

    const Vector2f thumbPos = drawPos + Vector2f{10.0f, 6.0f};
    drawRect(thumbPos, {58.0f, 32.0f}, info.exists ? Color{0.13f, 0.18f, 0.23f, 0.95f}
                                                   : Color{1.0f, 1.0f, 1.0f, 0.025f});
    drawBorder(thumbPos, {58.0f, 32.0f}, 1.0f, {1.0f, 1.0f, 1.0f, info.exists ? 0.10f : 0.16f});

    char title[32];
    std::snprintf(title, sizeof(title), "槽位 %d", slot);
    if (info.exists)
    {
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{84.0f, 7.0f}, 17.0f,
                      {1.0f, 1.0f, 1.0f, 0.96f}, "%s", title);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{220.0f, 7.0f}, 15.0f,
                      {1.0f, 1.0f, 1.0f, 0.55f}, "%s", info.modifiedTime.empty() ? "已有状态" : info.modifiedTime.c_str());
        Gfx::DrawText(Gfx::SystemFontStandard, thumbPos + Vector2f{29.0f, 16.0f}, 13.0f,
                      {0.75f, 0.88f, 1.0f, 0.46f}, Gfx::align_Center, Gfx::align_Center,
                      info.thumbnailPath.empty() ? "NDS" : "PNG");
    }
    else
    {
        Gfx::DrawText(Gfx::SystemFontStandard, thumbPos + Vector2f{29.0f, 14.0f}, 24.0f,
                      {1.0f, 1.0f, 1.0f, 0.45f}, Gfx::align_Center, Gfx::align_Center, "+");
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{84.0f, 7.0f}, 17.0f,
                      {1.0f, 1.0f, 1.0f, 0.88f}, "%s", title);
        Gfx::DrawText(Gfx::SystemFontChinese, drawPos + Vector2f{220.0f, 7.0f}, 15.0f,
                      {1.0f, 1.0f, 1.0f, 0.48f}, "空存档槽");
    }
}

void drawSaveSlotGrid(const std::array<NdsStateSlotInfo, 10>& slots,
                      int focusedSlot,
                      bool contentFocused,
                      float offsetY)
{
    constexpr float cardH = 44.0f;
    constexpr float gapY = 6.0f;
    const Vector2f start{kContentX, kContentY + 58.0f};

    for (int i = 0; i < 10; ++i)
    {
        const Vector2f pos = start + Vector2f{0.0f, i * (cardH + gapY)};
        drawSaveSlotCard(i, pos, contentFocused && i == focusedSlot, slots[i], offsetY);
    }
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
                     int focusedRow,
                     bool contentFocused,
                     float offsetX,
                     float offsetY,
                     float opacity)
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

    float y = 62.0f;
    auto rowPos = [&](float rowY) { return base + Vector2f{0.0f, rowY}; };
    drawLrSelectorRow(rowPos(y), "快进倍率", ffValue, contentFocused && focusedRow == 0, true, opacity); y += 48.0f;
    drawLrSelectorRow(rowPos(y), "画面过滤", filterLabel(linearFiltering), contentFocused && focusedRow == 1, true, opacity); y += 48.0f;
    drawSwitchRow(rowPos(y), "整数倍缩放", integerScale, contentFocused && focusedRow == 2, opacity); y += 48.0f;
    drawLrSelectorRow(rowPos(y), "画面布局", layoutLabel(layout), contentFocused && focusedRow == 3, true, opacity); y += 48.0f;
    drawSubPageRow(rowPos(y), "自定义画面布局", contentFocused && focusedRow == 4, layout == 4, opacity); y += 48.0f;
    drawLrSelectorRow(rowPos(y), "画面方向", orientationLabel(orientation), contentFocused && focusedRow == 5, true, opacity); y += 54.0f;
    drawSectionLabel(rowPos(y + 2.0f), "个性化设置", opacity); y += 30.0f;
    drawSubPageRow(rowPos(y), "遮罩选择", contentFocused && focusedRow == 6, true, opacity); y += 48.0f;
    drawSubPageRow(rowPos(y), "滤镜选择", contentFocused && focusedRow == 7, true, opacity); y += 54.0f;
    drawSectionLabel(rowPos(y + 2.0f), "同步设置", opacity); y += 30.0f;
    drawButtonRow(rowPos(y), "同步画面设置", contentFocused && focusedRow == 8, opacity); y += 48.0f;
    drawButtonRow(rowPos(y), "同步遮罩设置", contentFocused && focusedRow == 9, opacity); y += 48.0f;
    drawButtonRow(rowPos(y), "同步滤镜设置", contentFocused && focusedRow == 10, opacity);
}

void drawDeleteDialog(int slot, float opacity)
{
    opacity = clamp01(opacity);
    drawRect({0.0f, 0.0f}, {kScreenW, kScreenH}, {0.0f, 0.0f, 0.0f, 0.54f * opacity}, true);
    const Vector2f pos{390.0f, 248.0f};
    const Vector2f size{500.0f, 210.0f};
    drawRect(pos, size, {0.04f, 0.055f, 0.075f, 0.96f * opacity}, true);
    drawBorder(pos, size, 1.0f, {1.0f, 1.0f, 1.0f, 0.16f * opacity});
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{34.0f, 30.0f}, 24.0f,
                  {1.0f, 1.0f, 1.0f, 0.96f * opacity}, "删除即时存档");
    Gfx::DrawText(Gfx::SystemFontChinese, pos + Vector2f{34.0f, 88.0f}, 18.0f,
                  {1.0f, 1.0f, 1.0f, 0.72f * opacity}, "确认删除 ss%d 及对应截图？", slot);
    const float y = pos.Y + 162.0f;
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {pos.X + 328.0f, y}, 28.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f * opacity}, Gfx::align_Center, Gfx::align_Center,
                  GFX_NINTENDOFONT_B_BUTTON);
    Gfx::DrawText(Gfx::SystemFontChinese, {pos.X + 350.0f, y - 9.0f}, 18.0f,
                  {1.0f, 1.0f, 1.0f, 0.76f * opacity}, "取消");
    Gfx::DrawText(Gfx::SystemFontNintendoExt, {pos.X + 424.0f, y}, 28.0f,
                  {1.0f, 1.0f, 1.0f, 0.92f * opacity}, Gfx::align_Center, Gfx::align_Center,
                  GFX_NINTENDOFONT_A_BUTTON);
    Gfx::DrawText(Gfx::SystemFontChinese, {pos.X + 446.0f, y - 9.0f}, 18.0f,
                  {0.38f, 0.78f, 1.0f, 0.92f * opacity}, "删除");
}

void drawTabFrame(NdsMenuLayer::Item item,
                  NdsMenuLayer::Item previousItem,
                  float pageProgress,
                  const NdsDisplaySettings& display,
                  const std::array<NdsStateSlotInfo, 10>& slots,
                  int contentFocus,
                  bool contentFocused,
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
                drawSaveSlotGrid(slots, contentFocus, contentFocused, offsetY);
            break;
        case NdsMenuLayer::Item::LoadState:
            Gfx::DrawText(Gfx::SystemFontChinese, {kContentX + offsetX, kContentY + offsetY}, 20.0f,
                          {1.0f, 1.0f, 1.0f, opacity}, "读取状态");
            drawLine({kContentX + offsetX, kContentY + offsetY + 44.0f}, {kContentW, 1.0f},
                     {1.0f, 1.0f, 1.0f, 0.10f * opacity});
            if (opacity > 0.5f)
                drawSaveSlotGrid(slots, contentFocus, contentFocused, offsetY);
            break;
        case NdsMenuLayer::Item::Display:
            drawDisplayPage(display.linearFiltering,
                            display.fastForwardMultiplier,
                            display.integerScale,
                            display.layout,
                            display.orientation,
                            contentFocus,
                            contentFocused,
                            offsetX,
                            offsetY,
                            opacity);
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
