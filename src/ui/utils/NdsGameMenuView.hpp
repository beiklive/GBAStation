#pragma once

#include "core/common.h"
#include "TabFrame.hpp"
#include "Box.hpp"
#include <functional>

namespace beiklive
{
    class NdsGameMenuView : public beiklive::Box
    {
    public:
        NdsGameMenuView(beiklive::GameEntry gameData);
        ~NdsGameMenuView();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;
        void onShow();

        void setOnResume(std::function<void()> cb) { m_onResume = std::move(cb); }
        void setOnExit(std::function<void()> cb)   { m_onExit = std::move(cb); }

    private:
        beiklive::GameEntry m_gameEntry;
        std::function<void()> m_onResume, m_onExit;
        beiklive::TabFrame* m_panel = nullptr;

        void _initLayout();
    };

} // namespace beiklive
