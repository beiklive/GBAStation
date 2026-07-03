#include "nds_stub/NdsMenuLayer.hpp"

#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub {

NdsMenuAction NdsMenuLayer::update(std::uint64_t buttonsDown)
{
    if (buttonsDown & HidNpadButton_ZR)
        m_visible = !m_visible;

    if (!m_visible)
        return NdsMenuAction::None;

    if (buttonsDown & HidNpadButton_B)
    {
        m_visible = false;
        return NdsMenuAction::None;
    }
    if (buttonsDown & HidNpadButton_X)
        return NdsMenuAction::ResetGame;
    if (buttonsDown & HidNpadButton_A)
        return NdsMenuAction::ExitGame;

    return NdsMenuAction::None;
}

void NdsMenuLayer::draw(double fps, long long runMs) const
{
    Gfx::DrawText(Gfx::SystemFontStandard,
                  {28.0f, 24.0f},
                  20.0f,
                  {0.78f, 0.90f, 1.0f, 1.0f},
                  "FPS %.1f  RUN %lldMS  DEKO", fps, runMs);

    if (!m_visible)
        return;

    Gfx::DrawRectangle({318.0f, 118.0f}, {644.0f, 484.0f}, {0.02f, 0.03f, 0.04f, 0.90f}, true);
    Gfx::DrawRectangle({318.0f, 118.0f}, {168.0f, 484.0f}, {0.08f, 0.12f, 0.16f, 0.96f}, true);
    Gfx::DrawRectangle({486.0f, 118.0f}, {476.0f, 484.0f}, {0.04f, 0.06f, 0.08f, 0.90f}, true);

    Gfx::DrawText(Gfx::SystemFontChinese, {350.0f, 165.0f}, 23.0f, {0.92f, 0.97f, 1.0f, 1.0f}, "菜单");
    Gfx::DrawText(Gfx::SystemFontChinese, {350.0f, 235.0f}, 18.0f, {0.72f, 0.82f, 0.92f, 1.0f}, "返回游戏");
    Gfx::DrawText(Gfx::SystemFontChinese, {350.0f, 286.0f}, 18.0f, {0.72f, 0.82f, 0.92f, 1.0f}, "保存状态");
    Gfx::DrawText(Gfx::SystemFontChinese, {350.0f, 337.0f}, 18.0f, {0.72f, 0.82f, 0.92f, 1.0f}, "读取状态");
    Gfx::DrawText(Gfx::SystemFontChinese, {350.0f, 388.0f}, 18.0f, {0.72f, 0.82f, 0.92f, 1.0f}, "画面设置");
    Gfx::DrawText(Gfx::SystemFontChinese, {350.0f, 439.0f}, 18.0f, {0.72f, 0.82f, 0.92f, 1.0f}, "退出游戏");

    Gfx::DrawText(Gfx::SystemFontChinese, {530.0f, 168.0f}, 28.0f, {0.95f, 0.98f, 1.0f, 1.0f}, "游戏菜单");
    Gfx::DrawText(Gfx::SystemFontChinese, {530.0f, 246.0f}, 22.0f, {0.80f, 0.90f, 0.98f, 1.0f}, "B 返回游戏");
    Gfx::DrawText(Gfx::SystemFontChinese, {530.0f, 302.0f}, 22.0f, {0.80f, 0.90f, 0.98f, 1.0f}, "X 重置游戏");
    Gfx::DrawText(Gfx::SystemFontChinese, {530.0f, 358.0f}, 22.0f, {1.00f, 0.78f, 0.42f, 1.0f}, "A 保存并退出");
}

} // namespace beiklive::nds_stub
