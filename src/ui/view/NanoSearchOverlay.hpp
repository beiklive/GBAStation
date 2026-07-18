#pragma once

#include <borealis.hpp>

#include <chrono>
#include <functional>
#include <string>

namespace beiklive
{
    class NanoSearchOverlay final : public brls::View
    {
    public:
        NanoSearchOverlay();

        void open(std::string currentText,
                  std::function<void(const std::string&)> onApply);
        void close();
        bool isOpen() const { return m_open; }

        std::function<void()> onClosed;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;
        brls::View* getDefaultFocus() override { return this; }
        brls::View* getNextFocus(brls::FocusDirection, brls::View*) override
        { return this; }

    private:
        void _move(int direction);
        void _activate();
        void _finishClose();
        void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                       const char* text, float& cursor, float y, float alpha);

        bool m_open = false;
        bool m_closing = false;
        bool m_applyOnClose = false;
        float m_progress = 0.f;
        float m_time = 0.f;
        float m_press = 0.f;
        int m_selected = 0;
        int m_defaultFont = -1;
        int m_materialFont = -1;
        int m_switchFont = -1;
        std::string m_text;
        std::function<void(const std::string&)> m_onApply;
        std::chrono::steady_clock::time_point m_lastFrame;
    };
}
