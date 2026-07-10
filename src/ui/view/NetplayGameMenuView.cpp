#include "ui/view/NetplayGameMenuView.hpp"

#include <array>
#include <utility>

namespace beiklive
{
namespace
{
NVGcolor menuPanel() { return nvgRGBA(14, 18, 22, 232); }
NVGcolor menuSoft() { return nvgRGBA(32, 40, 48, 205); }
NVGcolor menuStroke() { return nvgRGBA(255, 255, 255, 52); }
NVGcolor menuAccent() { return nvgRGBA(86, 200, 172, 238); }
NVGcolor menuAccentSoft() { return nvgRGBA(86, 200, 172, 62); }
NVGcolor menuText() { return nvgRGBA(246, 248, 250, 246); }
NVGcolor menuMuted() { return nvgRGBA(186, 196, 206, 210); }
} // namespace

NetplayGameMenuView::NetplayGameMenuView()
{
    showHeader(false);
    showFooter(false);
    hideFooterLine();
    setFocusable(true);
    setHideHighlight(true);
    setVisibility(brls::Visibility::GONE);

    registerAction("选择", brls::BUTTON_A, [this](brls::View*) {
        activateCurrent();
        return true;
    });
    registerAction("返回", brls::BUTTON_B, [this](brls::View*) {
        if (m_onResume)
            m_onResume();
        return true;
    });
    registerAction("", brls::BUTTON_UP, [this](brls::View*) {
        moveSelection(-1);
        return true;
    }, true, true, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_DOWN, [this](brls::View*) {
        moveSelection(1);
        return true;
    }, true, true, brls::SOUND_NONE);
}

void NetplayGameMenuView::open()
{
    m_selectedIndex = 0;
    setVisibility(brls::Visibility::VISIBLE);
    setFocusable(true);
    brls::Application::giveFocus(this);
}

void NetplayGameMenuView::close()
{
    setVisibility(brls::Visibility::GONE);
    setFocusable(false);
}

void NetplayGameMenuView::activateCurrent()
{
    if (m_selectedIndex == 0)
    {
        if (m_onResume)
            m_onResume();
        return;
    }

    if (m_onCloseNetplay)
        m_onCloseNetplay();
}

void NetplayGameMenuView::moveSelection(int delta)
{
    m_selectedIndex = (m_selectedIndex + delta + 2) % 2;
}

void NetplayGameMenuView::draw(NVGcontext* vg, float x, float y, float w, float h,
                               brls::Style style, brls::FrameContext* ctx)
{
    beiklive::Box::draw(vg, x, y, w, h, style, ctx);
    if (m_font < 0)
        m_font = brls::Application::getDefaultFont();

    // 绘制联机菜单半透明遮罩：覆盖游戏画面但不暂停底层游戏线程。
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 96));
    nvgFill(vg);

    const float panelW = 420.f;
    const float panelH = 260.f;
    const float panelX = x + (w - panelW) * 0.5f;
    const float panelY = y + (h - panelH) * 0.5f;

    // 绘制联机菜单主体面板：位于屏幕中央，包含标题和两个操作按钮。
    drawRoundedRect(vg, panelX, panelY, panelW, panelH, 8.f, menuPanel(), menuStroke(), 1.f);
    drawText(vg, "联机菜单", panelX + 30.f, panelY + 42.f, 25.f, menuText(),
             NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, "游戏仍在运行", panelX + panelW - 30.f, panelY + 42.f, 14.f, menuMuted(),
             NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);

    const std::array<const char*, 2> labels = {"返回游戏", "关闭联机"};
    const std::array<const char*, 2> hints = {"关闭菜单并继续当前联机", "断开联机并退出游戏"};
    for (int i = 0; i < 2; ++i)
    {
        const bool focused = i == m_selectedIndex;
        const float itemX = panelX + 34.f;
        const float itemY = panelY + 88.f + i * 74.f;
        const float itemW = panelW - 68.f;
        const float itemH = 56.f;

        // 绘制菜单按钮：第一项返回游戏，第二项关闭联机，方向键切换，A 确认。
        drawRoundedRect(vg, itemX, itemY, itemW, itemH, 7.f,
                        focused ? menuAccentSoft() : menuSoft(),
                        focused ? menuAccent() : menuStroke(),
                        focused ? 2.2f : 1.f);
        drawText(vg, labels[i], itemX + 24.f, itemY + 19.f, 17.f,
                 focused ? menuText() : menuMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, hints[i], itemX + 24.f, itemY + 40.f, 12.f, menuMuted(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    }

    // 绘制底部按键提示：说明 A/B/方向键的当前含义。
    drawText(vg, "A 确认     B 返回游戏     方向键选择",
             panelX + panelW * 0.5f, panelY + panelH - 28.f, 13.f, menuMuted(),
             NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
}

void NetplayGameMenuView::drawText(NVGcontext* vg, const std::string& text, float x, float y,
                                   float size, NVGcolor color, int align) const
{
    nvgFontFaceId(vg, m_font);
    nvgFontSize(vg, size);
    nvgFillColor(vg, color);
    nvgTextAlign(vg, align);
    nvgText(vg, x, y, text.c_str(), nullptr);
}

void NetplayGameMenuView::drawRoundedRect(NVGcontext* vg, float x, float y, float w, float h,
                                          float radius, NVGcolor fill, NVGcolor stroke,
                                          float strokeWidth) const
{
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, radius);
    nvgFillColor(vg, fill);
    nvgFill(vg);
    if (strokeWidth > 0.f)
    {
        nvgStrokeColor(vg, stroke);
        nvgStrokeWidth(vg, strokeWidth);
        nvgStroke(vg);
    }
}

} // namespace beiklive
