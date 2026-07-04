#include "nds_stub/NdsMenuLayer.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub {

namespace {

constexpr int itemIndex(NdsMenuLayer::Item item)
{
    return static_cast<int>(item);
}

const char* filterLabel(bool linear)
{
    return linear ? "Linear" : "Nearest";
}

} // namespace

NdsMenuAction NdsMenuLayer::update(std::uint64_t buttonsDown)
{
    if (buttonsDown & HidNpadButton_ZR)
    {
        m_visible = !m_visible;
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
        m_selected = (m_selected + itemCount - 1) % itemCount;
        return NdsMenuAction::None;
    }
    if (buttonsDown & HidNpadButton_AnyDown)
    {
        m_selected = (m_selected + 1) % itemCount;
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
        case Item::Filtering:
        case Item::FastForward:
            return cycleCurrentSetting(1) ? NdsMenuAction::DisplaySettingsChanged : NdsMenuAction::None;
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
    switch (static_cast<Item>(m_selected))
    {
    case Item::Filtering:
        m_linearFiltering = !m_linearFiltering;
        return true;
    case Item::FastForward:
        m_fastForwardMultiplier = std::clamp(m_fastForwardMultiplier + direction, 1, 4);
        return true;
    default:
        return false;
    }
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

    Gfx::DrawRectangle({270.0f, 86.0f}, {740.0f, 548.0f}, {0.02f, 0.03f, 0.04f, 0.92f}, true);
    Gfx::DrawRectangle({270.0f, 86.0f}, {230.0f, 548.0f}, {0.08f, 0.12f, 0.16f, 0.96f}, true);
    Gfx::DrawRectangle({500.0f, 86.0f}, {510.0f, 548.0f}, {0.04f, 0.06f, 0.08f, 0.92f}, true);

    struct Row {
        const char* label;
        const char* value;
    };

    char ffValue[16];
    std::snprintf(ffValue, sizeof(ffValue), "x%d", m_fastForwardMultiplier);

    const std::array<Row, itemIndex(Item::Count)> rows {{
        {"返回游戏", ""},
        {"保存状态", "待实现"},
        {"读取状态", "待实现"},
        {"金手指", "待实现"},
        {"画面设置", ""},
        {"画面过滤", filterLabel(m_linearFiltering)},
        {"快进倍率", ffValue},
        {"重置游戏", ""},
        {"退出游戏", ""},
    }};

    Gfx::DrawText(Gfx::SystemFontChinese, {315.0f, 128.0f}, 24.0f,
                  {0.92f, 0.97f, 1.0f, 1.0f}, "菜单");

    float rowY = 182.0f;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i)
    {
        const bool selected = i == m_selected;
        if (selected)
            Gfx::DrawRectangle({292.0f, rowY - 11.0f}, {186.0f, 36.0f},
                               {0.18f, 0.35f, 0.52f, 0.95f}, true);
        Gfx::DrawText(Gfx::SystemFontChinese,
                      {315.0f, rowY},
                      17.0f,
                      selected ? Gfx::Color{1.0f, 1.0f, 1.0f, 1.0f}
                               : Gfx::Color{0.72f, 0.82f, 0.92f, 1.0f},
                      "%s", rows[i].label);
        if (rows[i].value[0])
            Gfx::DrawText(Gfx::SystemFontStandard,
                          {432.0f, rowY},
                          15.0f,
                          selected ? Gfx::Color{0.95f, 1.0f, 0.78f, 1.0f}
                                   : Gfx::Color{0.62f, 0.72f, 0.82f, 1.0f},
                          "%s", rows[i].value);
        rowY += 42.0f;
    }

    Gfx::DrawText(Gfx::SystemFontChinese, {540.0f, 134.0f}, 28.0f,
                  {0.95f, 0.98f, 1.0f, 1.0f}, "NDS 模拟器菜单");
    Gfx::DrawText(Gfx::SystemFontChinese, {540.0f, 212.0f}, 21.0f,
                  {0.80f, 0.90f, 0.98f, 1.0f}, "方向键选择，A 确认");
    Gfx::DrawText(Gfx::SystemFontChinese, {540.0f, 260.0f}, 21.0f,
                  {0.80f, 0.90f, 0.98f, 1.0f}, "左右键调整设置项");
    Gfx::DrawText(Gfx::SystemFontChinese, {540.0f, 308.0f}, 21.0f,
                  {0.80f, 0.90f, 0.98f, 1.0f}, "B 或 ZR 返回游戏");
    Gfx::DrawText(Gfx::SystemFontChinese, {540.0f, 376.0f}, 19.0f,
                  {1.00f, 0.86f, 0.58f, 1.0f}, "快进倍率高于 x1 后立即生效");
}

} // namespace beiklive::nds_stub
