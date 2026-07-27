#include "ui/view/NanoSearchOverlay.hpp"

#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"

#include <algorithm>
#include <cmath>

namespace beiklive
{
namespace
{
    float clamp01(float value) { return std::max(0.f, std::min(1.f, value)); }
    float smooth(float value)
    {
        value = clamp01(value);
        return value * value * (3.f - 2.f * value);
    }
    float back(float value)
    {
        value = clamp01(value);
        constexpr float c1 = 1.35f;
        constexpr float c3 = c1 + 1.f;
        const float shifted = value - 1.f;
        return 1.f + c3 * shifted * shifted * shifted + c1 * shifted * shifted;
    }
    std::string utf8(char32_t codepoint)
    {
        std::string out;
        if (codepoint <= 0x7F) out.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return out;
    }
}

NanoSearchOverlay::NanoSearchOverlay()
{
    setFocusable(true);
    setVisibility(brls::Visibility::GONE);
    setPositionType(brls::PositionType::ABSOLUTE);
    setPositionTop(0.f);
    setPositionLeft(0.f);
    setWidth(1280.f);
    setHeight(720.f);
    HIDE_BRLS_HIGHLIGHT(this);
    setCustomNavigationRoute(brls::FocusDirection::UP, this);
    setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
    setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
    setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);
    auto up = [this](brls::View*) {
        if (!m_open) return false;
        if (m_closing) return true;
        _move(-1);
        return true;
    };
    auto down = [this](brls::View*) {
        if (!m_open) return false;
        if (m_closing) return true;
        _move(1);
        return true;
    };
    auto consume = [this](brls::View*) {
        return m_open;
    };
    registerAction("", brls::BUTTON_UP, up, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_DOWN, down, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_LEFT, consume, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_RIGHT, consume, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_NAV_UP, up, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_NAV_DOWN, down, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_NAV_LEFT, consume, true, false, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_NAV_RIGHT, consume, true, false, brls::SOUND_NONE);
    registerAction("选择", brls::BUTTON_A,
        [this](brls::View*) { _activate(); return true; },
        false, false, brls::SOUND_NONE);
    registerAction("返回", brls::BUTTON_B,
        [this](brls::View*) { close(); return true; },
        false, false, brls::SOUND_NONE);
}

void NanoSearchOverlay::open(
    std::string currentText,
    std::function<void(const std::string&)> onApply)
{
    m_text = std::move(currentText);
    m_onApply = std::move(onApply);
    m_selected = 0;
    m_open = true;
    m_closing = false;
    m_applyOnClose = false;
    m_progress = 0.f;
    m_press = 0.f;
    m_lastFrame = std::chrono::steady_clock::now();
    setVisibility(brls::Visibility::VISIBLE);
    brls::Application::giveFocus(this);
}

void NanoSearchOverlay::close()
{
    if (!m_open || m_closing) return;
    m_closing = true;
    brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
}

void NanoSearchOverlay::_finishClose()
{
    auto apply = std::move(m_onApply);
    const bool shouldApply = m_applyOnClose;
    const std::string text = m_text;
    m_open = false;
    m_closing = false;
    m_applyOnClose = false;
    setVisibility(brls::Visibility::GONE);
    if (shouldApply && apply) apply(text);
    if (onClosed) onClosed();
}

void NanoSearchOverlay::frame(brls::FrameContext* ctx)
{
    brls::View::frame(ctx);
    if (!m_open) return;
    const auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrame).count();
    m_lastFrame = now;
    if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
    m_time += dt;
    m_press = std::max(0.f, m_press - dt * 6.f);
    m_progress += (m_closing ? -1.f : 1.f) * dt *
        (m_closing ? 5.2f : 4.5f);
    m_progress = clamp01(m_progress);
    if (m_closing && m_progress <= 0.f) _finishClose();
    invalidate();
}

void NanoSearchOverlay::_move(int direction)
{
    if (!m_open || m_closing) return;
    m_selected = (m_selected + (direction < 0 ? -1 : 1) + 3) % 3;
    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
}

void NanoSearchOverlay::_activate()
{
    if (!m_open || m_closing) return;
    m_press = 1.f;
    brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    if (m_selected == 0) {
        auto* ime = brls::Application::getPlatform()->getImeManager();
        if (!ime) return;
        ime->openForText([this](std::string value) {
            if (!m_open) return;
            m_text = std::move(value);
            invalidate();
        }, "搜索游戏", "输入标题或文件名", 128, m_text,
            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
    } else {
        if (m_selected == 2) m_text.clear();
        m_applyOnClose = true;
        m_closing = true;
    }
}

void NanoSearchOverlay::draw(NVGcontext* vg, float x, float y, float w, float h,
                             brls::Style, brls::FrameContext*)
{
    if (!m_open || !vg) return;
    if (m_defaultFont < 0) m_defaultFont = brls::Application::getDefaultFont();
    if (m_materialFont < 0) m_materialFont =
        brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
    if (m_switchFont < 0) m_switchFont =
        brls::Application::getFont(brls::FONT_SWITCH_ICONS);
    const float alpha = smooth(m_progress);
    const float eased = back(m_progress);
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, nvgRGBA(0, 0, 0,
        static_cast<unsigned char>(212.f * alpha)));
    nvgFill(vg);
    const float panelW = 720.f;
    const float panelH = 420.f;
    const float panelX = x + (w - panelW) * 0.5f;
    const float panelY = y + (h - panelH) * 0.5f + (1.f - eased) * 44.f;
    const NVGpaint shadow = nvgBoxGradient(
        vg, panelX + 5.f, panelY + 7.f, panelW, panelH, 20.f, 8.f,
        nvgRGBA(0, 0, 0, 115), nvgRGBA(0, 0, 0, 0));
    nvgBeginPath(vg);
    nvgRect(vg, panelX - 4.f, panelY - 4.f, panelW + 18.f, panelH + 20.f);
    nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 20.f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadow);
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 20.f);
    nvgFillColor(vg, nvgRGBA(25, 29, 39, 248));
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 74));
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);
    nvgFontFaceId(vg, m_materialFont);
    nvgFontSize(vg, 38.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(91, 193, 255, 245));
    const std::string searchIcon = utf8(0xE8B6);
    nvgText(vg, panelX + 34.f, panelY + 49.f, searchIcon.c_str(), nullptr);
    nvgFontFaceId(vg, m_defaultFont);
    nvgFontSize(vg, 28.f);
    nvgFillColor(vg, nvgRGBA(246, 248, 252, 248));
    nvgText(vg, panelX + 84.f, panelY + 49.f, "搜索游戏库", nullptr);
    nvgFontSize(vg, 15.f);
    nvgFillColor(vg, nvgRGBA(192, 201, 215, 190));
    nvgText(vg, panelX + 35.f, panelY + 82.f,
            "支持游戏标题和 ROM 文件名模糊匹配", nullptr);

    static const char* labels[] = {"输入关键词", "应用搜索", "清除搜索"};
    static const char32_t icons[] = {0xE8B6, 0xE876, 0xE14C};
    for (int i = 0; i < 3; ++i) {
        const float rowY = panelY + 116.f + i * 82.f;
        const bool focused = i == m_selected;
        const float scale = focused ? 1.f - m_press * 0.025f : 1.f;
        nvgSave(vg);
        nvgTranslate(vg, panelX + panelW * 0.5f, rowY + 32.f);
        nvgScale(vg, scale, scale);
        nvgTranslate(vg, -(panelX + panelW * 0.5f), -(rowY + 32.f));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panelX + 28.f, rowY, panelW - 56.f, 64.f, 12.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, focused ? 38 : 9));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, focused ? 112 : 42));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        if (focused) beiklive::ui::drawGradientFocusBorder(
            vg, panelX + 28.f, rowY, panelW - 56.f, 64.f, 12.f, 3.f, 1.f,
            beiklive::ui::gradientFocusAnimationOffset(m_time));
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 29.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(96, 195, 255, 245));
        const std::string icon = utf8(icons[i]);
        nvgText(vg, panelX + 52.f, rowY + 32.f, icon.c_str(), nullptr);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 22.f);
        nvgFillColor(vg, nvgRGBA(242, 245, 250, 242));
        nvgText(vg, panelX + 101.f, rowY + 32.f, labels[i], nullptr);
        if (i == 0) {
            nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgFontSize(vg, 18.f);
            nvgFillColor(vg, nvgRGBA(205, 214, 228, 220));
            const std::string value = m_text.empty() ? "未输入  ›" : m_text + "  ›";
            nvgSave(vg);
            nvgIntersectScissor(vg, panelX + 320.f, rowY,
                                panelW - 375.f, 64.f);
            nvgText(vg, panelX + panelW - 52.f, rowY + 32.f,
                    value.c_str(), nullptr);
            nvgRestore(vg);
        }
        nvgRestore(vg);
    }
    float cursor = panelX + panelW - 30.f;
    _drawHint(vg, brls::BUTTON_B, "返回", cursor, panelY + panelH - 25.f, alpha);
    _drawHint(vg, brls::BUTTON_A, "选择", cursor, panelY + panelH - 25.f, alpha);
}

void NanoSearchOverlay::_drawHint(NVGcontext* vg, brls::ControllerButton button,
                                  const char* text, float& cursor, float y,
                                  float alpha)
{
    nvgFontFaceId(vg, m_defaultFont);
    nvgFontSize(vg, 16.f);
    float bounds[4]{};
    nvgTextBounds(vg, 0, 0, text, nullptr, bounds);
    cursor -= bounds[2] - bounds[0] + 41.f;
    nvgFontFaceId(vg, m_switchFont);
    nvgFontSize(vg, 26.f);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(255, 255, 255,
        static_cast<unsigned char>(245.f * alpha)));
    const std::string glyph = brls::Hint::getKeyIcon(button);
    nvgText(vg, cursor + 12.f, y, glyph.c_str(), nullptr);
    nvgFontFaceId(vg, m_defaultFont);
    nvgFontSize(vg, 16.f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgText(vg, cursor + 28.f, y, text, nullptr);
    cursor -= 12.f;
}
}
