#pragma once

#include "ui/widget/Box.hpp"

#include <functional>
#include <string>

namespace beiklive
{

class NetplayGameMenuView : public beiklive::Box
{
public:
    NetplayGameMenuView();
    ~NetplayGameMenuView() override = default;

    void open();
    void close();

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

    void setOnResume(std::function<void()> cb) { m_onResume = std::move(cb); }
    void setOnCloseNetplay(std::function<void()> cb) { m_onCloseNetplay = std::move(cb); }

private:
    void activateCurrent();
    void moveSelection(int delta);
    void drawText(NVGcontext* vg, const std::string& text, float x, float y,
                  float size, NVGcolor color, int align) const;
    void drawRoundedRect(NVGcontext* vg, float x, float y, float w, float h,
                         float radius, NVGcolor fill, NVGcolor stroke,
                         float strokeWidth) const;

    std::function<void()> m_onResume;
    std::function<void()> m_onCloseNetplay;
    int m_selectedIndex = 0;
    int m_font = -1;
};

} // namespace beiklive
