#include "Pico8Page.hpp"

#include "core/common.h"
#include "ui/utils/Pico8Transition.hpp"
#include "ui/view/SwitchLayout.hpp"

#include <algorithm>
#include <cmath>

namespace beiklive
{
    Pico8Page::Pico8Page(SwitchLayout* homeLayout)
        : brls::Box(brls::Axis::COLUMN), m_homeLayout(homeLayout)
    {
        setFocusable(true);
        setGrow(1.f);
        setBackground(brls::ViewBackground::NONE);
        setHideHighlightBackground(true);
        setHideHighlightBorder(true);
        setHideClickAnimation(true);
        m_lastFrameTime = std::chrono::steady_clock::now();

        registerAction("", brls::BUTTON_B,
            [this](brls::View*) {
                _beginClose();
                return true;
            }, true, false, brls::SOUND_BACK);
    }

    Pico8Page::~Pico8Page()
    {
        if (m_logoImageHandle > 0) {
            if (auto* vg = brls::Application::getNVGContext())
                nvgDeleteImage(vg, m_logoImageHandle);
        }
    }

    void Pico8Page::_beginClose()
    {
        if (m_closing || m_popScheduled)
            return;
        m_closing = true;
        m_closeProgress = 0.f;
        m_lastFrameTime = std::chrono::steady_clock::now();
        if (m_homeLayout)
            m_homeLayout->beginPico8ReturnAnimation();
        brls::Application::blockInputs();
        invalidate();
    }

    void Pico8Page::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;

        if (!m_closing || m_popScheduled)
            return;

        m_closeProgress = std::min(
            1.f, m_closeProgress +
                dt / beiklive::pico8_transition::TRANSITION_DURATION);
        if (m_homeLayout)
            m_homeLayout->setPico8ReturnProgress(m_closeProgress);

        if (m_closeProgress >= 1.f) {
            m_popScheduled = true;
            if (m_homeLayout)
                m_homeLayout->finishPico8ReturnAnimation();
            brls::sync([]() {
                brls::Application::popActivity(
                    brls::TransitionAnimation::NONE,
                    []() { brls::Application::unblockInputs(); });
            });
        }
        invalidate();
    }

    void Pico8Page::draw(NVGcontext* vg, float x, float y,
                         float width, float height, brls::Style,
                         brls::FrameContext*)
    {
        if (!vg)
            return;
        if (m_logoImageHandle == 0)
            m_logoImageHandle = nvgCreateImage(
                vg, BK_RES("img/pico8_logo_vector.png").c_str(), 0);
        if (m_logoImageHandle <= 0)
            return;

        const auto geometry = beiklive::pico8_transition::geometry(
            0.f, 0.f, brls::Application::contentWidth,
            brls::Application::contentHeight);
        const float transition = m_closing
            ? 1.f - m_closeProgress
            : 1.f;
        const auto pose = beiklive::pico8_transition::logoPose(
            geometry, transition);

        const float centerX = pose.x + pose.width * 0.5f;
        const float centerY = pose.y + pose.height * 0.5f;
        nvgSave(vg);
        nvgTranslate(vg, centerX, centerY);
        nvgRotate(vg, pose.rotation);
        nvgTranslate(vg, -centerX, -centerY);
        nvgBeginPath(vg);
        nvgRect(vg, pose.x, pose.y, pose.width, pose.height);
        const NVGpaint paint = nvgImagePattern(
            vg, pose.x, pose.y, pose.width, pose.height, 0.f,
            m_logoImageHandle, 1.f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
        nvgRestore(vg);
    }
}
