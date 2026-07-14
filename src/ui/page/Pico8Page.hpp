#pragma once

#include <chrono>

#include <borealis.hpp>

namespace beiklive
{
    class SwitchLayout;

    class Pico8Page : public brls::Box
    {
    public:
        explicit Pico8Page(SwitchLayout* homeLayout);
        ~Pico8Page() override;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;
        bool isTranslucent() override { return true; }
        brls::View* getDefaultFocus() override { return this; }

    private:
        void _beginClose();

        SwitchLayout* m_homeLayout = nullptr;
        int m_logoImageHandle = 0;
        bool m_closing = false;
        bool m_popScheduled = false;
        float m_closeProgress = 0.f;
        std::chrono::steady_clock::time_point m_lastFrameTime;
    };
}
