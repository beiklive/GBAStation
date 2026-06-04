#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include <functional>
#include "TabFrame.hpp"
#include "Box.hpp"

namespace beiklive::flash {

class FlashGameMenuView : public beiklive::Box {
public:
    FlashGameMenuView(beiklive::GameEntry gameData);
    ~FlashGameMenuView();

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void onShow();

    void setOnResume(std::function<void()> cb)   { m_onResume = std::move(cb); }
    void setOnReset(std::function<void()> cb)    { m_onReset  = std::move(cb); }
    void setOnExit(std::function<void()> cb)     { m_onExit   = std::move(cb); }
    void setKeyBindingCallback(std::function<void(const std::string&, const std::string&)> cb) {
        m_keyBindingCallback = std::move(cb);
    }

private:
    beiklive::GameEntry m_gameEntry;
    std::function<void()> m_onResume, m_onReset, m_onExit;
    std::function<void(const std::string&, const std::string&)> m_keyBindingCallback;

    beiklive::TabFrame* m_panel = nullptr;
    brls::Box* m_keymapPanel = nullptr;

    void _initLayout();
    brls::View* _createKeymapPanel();
    void _rebuildKeymapPanel();
};

} // namespace beiklive::flash
