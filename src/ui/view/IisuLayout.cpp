#include "IisuLayout.hpp"

#include "core/Translation.hpp"

namespace beiklive
{
    IisuLayout::IisuLayout() : Layout()
    {
        setFocusable(true);
        setHideHighlightBackground(true);
        setHideHighlightBorder(true);
        setHideClickAnimation(true);
        setBackground(brls::ViewBackground::NONE);
        setClipsToBounds(true);
        setFocusSound(brls::SOUND_NONE);
        setCustomNavigationRoute(brls::FocusDirection::UP, this);
        setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        m_fontId = brls::Application::getDefaultFont();

        // 占位阶段：吞掉所有方向键/确认键，避免焦点泄漏到其他视图
        auto consume = [](brls::View*) -> bool { return true; };
        registerAction("", brls::BUTTON_A, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LB, consume, true, false, brls::SOUND_NONE);
    }

    void IisuLayout::refreshGameList(beiklive::GameList gameList)
    {
        m_games = std::move(gameList);
        invalidate();
    }

    void IisuLayout::restoreCardFocus(bool /*animated*/)
    {
        brls::Application::giveFocus(this);
    }

    void IisuLayout::resetCardFocusToFirst()
    {
        restoreCardFocus(false);
    }

    void IisuLayout::removeGameByPath(const std::string& /*path*/)
    {
        // TODO: iisu 布局删除动画
    }

    void IisuLayout::completeGameRemoval(std::function<void()> completion)
    {
        if (completion)
            completion();
    }

    void IisuLayout::cancelGameRemoval()
    {
        // TODO: iisu 布局删除动画取消
    }

    int IisuLayout::acquireSelectedCoverTexture()
    {
        return 0;
    }

    void IisuLayout::releaseSelectedCoverTexture()
    {
    }

    void IisuLayout::playEntranceAnimation()
    {
        // TODO: iisu 布局入场动画
    }

    void IisuLayout::playExitAnimation(std::function<void()> completion)
    {
        if (completion)
            completion();
    }

    void IisuLayout::playPico8ExitAnimation(std::function<void()> completion)
    {
        if (completion)
            completion();
    }

    void IisuLayout::beginPico8ReturnAnimation()
    {
        // TODO: iisu 布局 PICO-8 返回动画
    }

    void IisuLayout::setPico8ReturnProgress(float /*progress*/)
    {
    }

    void IisuLayout::finishPico8ReturnAnimation()
    {
    }

    void IisuLayout::setPico8ShortcutVisible(bool visible)
    {
        m_pico8ShortcutVisible = visible;
        invalidate();
    }

    void IisuLayout::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
    }

    void IisuLayout::draw(NVGcontext* vg, float x, float y, float w, float h,
                          brls::Style style, brls::FrameContext* ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        // ── 占位绘制：后续替换为 iisu 完整布局 ──────────────────────────
        if (m_fontId < 0)
            m_fontId = brls::Application::getDefaultFont();

        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, w, h);

        const float cx = x + w * 0.5f;
        const float cy = y + h * 0.5f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, cx - 300.f, cy - 120.f, 600.f, 240.f, 18.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 10));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 70));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 40.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(250, 251, 255, 235));
        nvgText(vg, cx, cy - 50.f, "IISU Layout", nullptr);

        nvgFontSize(vg, 22.f);
        nvgFillColor(vg, nvgRGBA(211, 219, 233, 200));
        const std::string gamesInfo =
            L("最近游戏: ") + std::to_string(m_games.size());
        nvgText(vg, cx, cy - 8.f, gamesInfo.c_str(), nullptr);

        nvgFontSize(vg, 18.f);
        nvgFillColor(vg, nvgRGBA(190, 200, 218, 160));
        nvgText(vg, cx, cy + 36.f,
                L("iisu 布局占位页面，等待实现").c_str(), nullptr);

        nvgResetScissor(vg);
        nvgRestore(vg);
    }
} // namespace beiklive
