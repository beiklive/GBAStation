#include "nds_stub/ui/UiComponents.hpp"

namespace beiklive::nds_stub::ui {

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

} // namespace beiklive::nds_stub::ui
