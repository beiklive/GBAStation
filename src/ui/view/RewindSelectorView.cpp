#include "RewindSelectorView.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace beiklive
{
    namespace
    {
        constexpr float kPanelH = 240.0f;
        constexpr float kInfoH = 30.0f;
        constexpr float kItemAreaH = 180.0f;
        constexpr float kViewportPad = 32.0f;
        constexpr float kGap = 14.0f;
        constexpr float kItemW = 202.0f;
        constexpr float kItemH = 164.0f;
        constexpr float kThumbH = 124.0f;
        constexpr float kHoldInitialDelay = 0.26f;
        constexpr float kHoldMinInterval = 0.048f;
        constexpr float kHoldStartInterval = 0.128f;

        std::vector<std::uint8_t> rgb565ToRgba8888(const std::vector<std::uint16_t>& src,
                                                   unsigned width,
                                                   unsigned height)
        {
            if (width == 0 || height == 0 ||
                src.size() < static_cast<std::size_t>(width) * height)
                return {};

            std::vector<std::uint8_t> dst(static_cast<std::size_t>(width) * height * 4, 255);
            for (std::size_t i = 0; i < static_cast<std::size_t>(width) * height; ++i)
            {
                const std::uint16_t px = src[i];
                const std::uint8_t r5 = static_cast<std::uint8_t>((px >> 11) & 0x1F);
                const std::uint8_t g6 = static_cast<std::uint8_t>((px >> 5) & 0x3F);
                const std::uint8_t b5 = static_cast<std::uint8_t>(px & 0x1F);
                dst[i * 4 + 0] = static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2));
                dst[i * 4 + 1] = static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4));
                dst[i * 4 + 2] = static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2));
                dst[i * 4 + 3] = 255;
            }
            return dst;
        }

        float alphaScale(float channel, float alpha)
        {
            return std::clamp(channel * alpha, 0.0f, 255.0f);
        }

        NVGcolor rgba(int r, int g, int b, int a, float alpha = 1.0f)
        {
            return nvgRGBA(static_cast<unsigned char>(r),
                           static_cast<unsigned char>(g),
                           static_cast<unsigned char>(b),
                           static_cast<unsigned char>(alphaScale(static_cast<float>(a), alpha)));
        }
    }

    RewindSelectorView::RewindSelectorView()
    {
        setFocusable(true);
        setHideHighlightBackground(true);
        setHideHighlightBorder(true);
        setHideClickAnimation(true);
        setBackground(brls::ViewBackground::NONE);
        setClipsToBounds(false);
        setFocusSound(brls::SOUND_NONE);
        setCustomNavigationRoute(brls::FocusDirection::UP, this);
        setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        m_font = brls::Application::getDefaultFont();
        m_lastFrameTime = std::chrono::steady_clock::now();

        registerAction("读取", brls::BUTTON_A, [this](brls::View*) -> bool {
            _selectCurrent();
            return true;
        }, false, false, brls::SOUND_CLICK);

        registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            _close();
            return true;
        }, false, false, brls::SOUND_BACK);

        auto consumeNavigation = [](brls::View*) -> bool { return true; };
        registerAction("", brls::BUTTON_NAV_UP, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_UP, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LEFT, consumeNavigation, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, consumeNavigation, true, true, brls::SOUND_NONE);
    }

    RewindSelectorView::~RewindSelectorView()
    {
        _clearItems(brls::Application::getNVGContext());
    }

    brls::View* RewindSelectorView::getDefaultFocus()
    {
        return this;
    }

    brls::View* RewindSelectorView::getNextFocus(brls::FocusDirection direction, brls::View* currentView)
    {
        (void)direction;
        (void)currentView;
        return this;
    }

    void RewindSelectorView::_clearItems(NVGcontext* vg)
    {
        if (vg)
        {
            for (auto& item : m_items)
            {
                if (item.nvgImage > 0)
                    nvgDeleteImage(vg, item.nvgImage);
                item.nvgImage = 0;
            }
        }
        m_items.clear();
        m_selected = 0;
    }

    void RewindSelectorView::openWithFrames(std::vector<RewindThumbSnapshot> frames)
    {
        _clearItems(brls::Application::getNVGContext());

        if (frames.empty())
        {
            brls::Application::notify("暂无倒带记录");
            if (m_onClose)
                m_onClose();
            return;
        }

        m_items.reserve(frames.size());
        for (const auto& snap : frames)
        {
            ThumbItem item;
            item.frameIndex = snap.bufferIdx;
            item.secondsAgo = snap.secondsAgo;
            item.width = snap.thumbW;
            item.height = snap.thumbH;
            item.rgba = rgb565ToRgba8888(snap.thumb, item.width, item.height);
            item.imageCreated = item.rgba.empty();
            m_items.push_back(std::move(item));
        }

        m_selected = static_cast<int>(m_items.size()) - 1;
        _captureInputState();
        _notifyFrameFocused();
        invalidate();
    }

    void RewindSelectorView::_captureInputState()
    {
        const auto& state = brls::Application::getControllerState();
        m_prevLeft = state.buttons[brls::BUTTON_LEFT] || state.buttons[brls::BUTTON_NAV_LEFT];
        m_prevRight = state.buttons[brls::BUTTON_RIGHT] || state.buttons[brls::BUTTON_NAV_RIGHT];
        m_holdLeftTime = 0.0f;
        m_holdRightTime = 0.0f;
        m_holdLeftRepeat = 0.0f;
        m_holdRightRepeat = 0.0f;
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    void RewindSelectorView::_moveSelection(int delta)
    {
        if (m_items.empty() || delta == 0)
            return;

        const int previous = m_selected;
        m_selected = std::clamp(m_selected + delta, 0, static_cast<int>(m_items.size()) - 1);
        if (m_selected != previous)
        {
            _notifyFrameFocused();
            if (auto* player = brls::Application::getAudioPlayer())
                player->play(brls::SOUND_FOCUS_CHANGE);
            invalidate();
        }
        else if (auto* player = brls::Application::getAudioPlayer())
        {
            player->play(brls::SOUND_FOCUS_ERROR);
        }
    }

    void RewindSelectorView::_notifyFrameFocused()
    {
        if (m_items.empty() || !m_onFrameFocused)
            return;

        m_selected = std::clamp(m_selected, 0, static_cast<int>(m_items.size()) - 1);
        m_onFrameFocused(m_items[static_cast<std::size_t>(m_selected)].frameIndex);
    }

    void RewindSelectorView::_selectCurrent()
    {
        if (m_items.empty())
        {
            _close();
            return;
        }
        m_selected = std::clamp(m_selected, 0, static_cast<int>(m_items.size()) - 1);
        if (m_onFrameSelected)
            m_onFrameSelected(m_items[static_cast<std::size_t>(m_selected)].frameIndex);
    }

    void RewindSelectorView::_close()
    {
        if (m_onClose)
            m_onClose();
    }

    void RewindSelectorView::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);

        if (!isFocused() || m_items.empty() || isHidden())
            return;

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.0f || dt > 0.5f)
            dt = 0.016f;

        const auto& state = brls::Application::getControllerState();
        const bool leftNow = state.buttons[brls::BUTTON_LEFT] || state.buttons[brls::BUTTON_NAV_LEFT];
        const bool rightNow = state.buttons[brls::BUTTON_RIGHT] || state.buttons[brls::BUTTON_NAV_RIGHT];

        auto processHold = [&](bool nowDown, bool& previous, float& holdTime, float& repeatTime, int delta) {
            if (nowDown && !previous)
            {
                holdTime = 0.0f;
                repeatTime = 0.0f;
                _moveSelection(delta);
            }

            if (nowDown)
            {
                holdTime += dt;
                if (holdTime > kHoldInitialDelay)
                {
                    repeatTime += dt;
                    const float interval = std::max(kHoldMinInterval,
                                                    kHoldStartInterval - (holdTime - kHoldInitialDelay) * 0.12f);
                    while (repeatTime >= interval)
                    {
                        repeatTime -= interval;
                        _moveSelection(delta);
                    }
                }
            }
            else
            {
                holdTime = 0.0f;
                repeatTime = 0.0f;
            }
            previous = nowDown;
        };

        if (leftNow && rightNow)
        {
            m_prevLeft = true;
            m_prevRight = true;
            m_holdLeftTime = m_holdRightTime = 0.0f;
            m_holdLeftRepeat = m_holdRightRepeat = 0.0f;
            return;
        }

        processHold(leftNow, m_prevLeft, m_holdLeftTime, m_holdLeftRepeat, -1);
        processHold(rightNow, m_prevRight, m_holdRightTime, m_holdRightRepeat, 1);
    }

    void RewindSelectorView::_createImage(NVGcontext* vg, ThumbItem& item)
    {
        if (item.imageCreated)
            return;

        if (!item.rgba.empty())
        {
            item.nvgImage = nvgCreateImageRGBA(vg,
                                               static_cast<int>(item.width),
                                               static_cast<int>(item.height),
                                               NVG_IMAGE_NEAREST,
                                               item.rgba.data());
            item.rgba.clear();
            item.rgba.shrink_to_fit();
        }
        item.imageCreated = true;
    }

    void RewindSelectorView::_drawText(NVGcontext* vg, float x, float y, float size,
                                       NVGcolor color, int align, const char* text) const
    {
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, size);
        nvgTextAlign(vg, align);
        nvgFillColor(vg, color);
        nvgText(vg, x, y, text, nullptr);
    }

    void RewindSelectorView::_drawFocusBorder(NVGcontext* vg, float x, float y,
                                              float w, float h, float alpha) const
    {
        const float border = 3.0f;
        const float radius = 8.0f;
        NVGpaint paint = nvgLinearGradient(vg, x, y, x + w, y + h,
                                           rgba(96, 206, 255, 245, alpha),
                                           rgba(176, 116, 255, 235, alpha));

        nvgSave(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x - border, y - border, w + border * 2.0f, h + border * 2.0f, radius + border);
        nvgRoundedRect(vg, x, y, w, h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
        nvgRestore(vg);
    }

    void RewindSelectorView::draw(NVGcontext* vg, float x, float y, float w, float h,
                                  brls::Style style, brls::FrameContext* ctx)
    {
        (void)style;
        (void)ctx;

        if (m_font < 0)
            m_font = brls::Application::getDefaultFont();

        const float panelH = std::min(kPanelH, std::max(210.0f, h * 0.34f));
        const float panelY = y + h - panelH;
        const float panelW = w;
        const float alpha = 1.0f;

        nvgSave(vg);

        nvgBeginPath(vg);
        nvgRect(vg, x, panelY, panelW, panelH);
        nvgFillColor(vg, rgba(5, 7, 11, 230, alpha));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRect(vg, x, panelY, panelW, 1.5f);
        nvgFillColor(vg, rgba(87, 184, 255, 140, alpha));
        nvgFill(vg);

        _drawText(vg, x + 32.0f, panelY + 20.0f, 18.0f,
                  rgba(219, 237, 255, 235, alpha), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, "倒带");
        _drawText(vg, x + panelW - 324.0f, panelY + 21.0f, 15.0f,
                  rgba(184, 209, 235, 210, alpha), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, "← → 选择");
        _drawText(vg, x + panelW - 194.0f, panelY + 21.0f, 15.0f,
                  rgba(184, 209, 235, 210, alpha), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, "A 读取");
        _drawText(vg, x + panelW - 96.0f, panelY + 21.0f, 15.0f,
                  rgba(184, 209, 235, 210, alpha), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, "B 返回");

        if (m_items.empty())
        {
            _drawText(vg, x + panelW * 0.5f, panelY + panelH * 0.58f, 24.0f,
                      rgba(204, 224, 245, 158, alpha), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, "暂无倒带缓存");
            nvgRestore(vg);
            return;
        }

        const int count = static_cast<int>(m_items.size());
        m_selected = std::clamp(m_selected, 0, count - 1);

        const float viewportX = x + kViewportPad;
        const float viewportW = std::max(1.0f, panelW - kViewportPad * 2.0f);
        const float totalW = kItemW * static_cast<float>(count) + kGap * static_cast<float>(std::max(0, count - 1));
        const float selectedCenter = static_cast<float>(m_selected) * (kItemW + kGap) + kItemW * 0.5f;
        const float scrollX = std::clamp(selectedCenter - viewportW * 0.5f,
                                         0.0f,
                                         std::max(0.0f, totalW - viewportW));
        const float startX = viewportX - scrollX;
        const float itemAreaY = panelY + kInfoH;
        const float itemY = itemAreaY + 8.0f;

        nvgSave(vg);
        nvgIntersectScissor(vg, viewportX - 8.0f, itemAreaY, viewportW + 16.0f, kItemAreaH);

        for (int i = 0; i < count; ++i)
        {
            ThumbItem& item = m_items[static_cast<std::size_t>(i)];
            _createImage(vg, item);

            const float itemX = startX + static_cast<float>(i) * (kItemW + kGap);
            if (itemX > viewportX + viewportW || itemX + kItemW < viewportX)
                continue;

            const bool focused = (i == m_selected);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, itemX, itemY, kItemW, kItemH, 7.0f);
            nvgFillColor(vg, focused ? rgba(31, 54, 79, 235, alpha) : rgba(20, 25, 36, 200, alpha));
            nvgFill(vg);

            if (focused)
                _drawFocusBorder(vg, itemX, itemY, kItemW, kItemH, alpha);
            else
            {
                nvgBeginPath(vg);
                nvgRoundedRect(vg, itemX + 0.5f, itemY + 0.5f, kItemW - 1.0f, kItemH - 1.0f, 7.0f);
                nvgStrokeWidth(vg, 1.0f);
                nvgStrokeColor(vg, rgba(89, 109, 132, 72, alpha));
                nvgStroke(vg);
            }

            constexpr float innerPad = 8.0f;
            const float aspect = item.height > 0
                ? static_cast<float>(item.width) / static_cast<float>(item.height)
                : static_cast<float>(RewindFrame::DEFAULT_THUMB_W) /
                      static_cast<float>(RewindFrame::DEFAULT_THUMB_H);
            const float thumbW = std::min(kItemW - innerPad * 2.0f,
                                          kThumbH * aspect);
            const float thumbX = itemX + (kItemW - thumbW) * 0.5f;
            const float thumbY = itemY + innerPad;

            if (item.nvgImage > 0)
            {
                NVGpaint img = nvgImagePattern(vg, thumbX, thumbY, thumbW, kThumbH,
                                               0.0f, item.nvgImage, focused ? 1.0f : 0.92f);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, thumbX, thumbY, thumbW, kThumbH, 3.0f);
                nvgFillPaint(vg, img);
                nvgFill(vg);
            }
            else
            {
                nvgBeginPath(vg);
                nvgRoundedRect(vg, thumbX, thumbY, thumbW, kThumbH, 3.0f);
                nvgFillColor(vg, rgba(12, 16, 24, 210, alpha));
                nvgFill(vg);
                _drawText(vg, thumbX + thumbW * 0.5f, thumbY + kThumbH * 0.5f, 14.0f,
                          rgba(166, 199, 235, 128, alpha), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, "NO THUMB");
            }

            char timeText[32] = {};
            std::snprintf(timeText, sizeof(timeText), "-%d秒", std::max(1, item.secondsAgo));
            _drawText(vg, itemX + kItemW * 0.5f, itemY + 146.0f, 16.0f,
                      rgba(209, 230, 255, focused ? 230 : 190, alpha),
                      NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, timeText);
        }

        nvgRestore(vg);

        const float progressW = std::min(360.0f, panelW - 64.0f);
        const float progressX = x + (panelW - progressW) * 0.5f;
        const float progressY = panelY + kInfoH + kItemAreaH + 9.0f;
        const float ratio = count > 1 ? static_cast<float>(m_selected) / static_cast<float>(count - 1) : 1.0f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, progressX, progressY, progressW, 4.0f, 2.0f);
        nvgFillColor(vg, rgba(74, 91, 111, 120, alpha));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, progressX, progressY, std::max(4.0f, progressW * ratio), 4.0f, 2.0f);
        nvgFillColor(vg, rgba(87, 184, 255, 190, alpha));
        nvgFill(vg);

        nvgRestore(vg);
    }

} // namespace beiklive
