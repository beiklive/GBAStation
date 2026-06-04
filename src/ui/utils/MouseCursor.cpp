#include "MouseCursor.hpp"

namespace beiklive::flash {

void MouseCursor::draw(NVGcontext* vg, float cx, float cy, bool pressed)
{
    NVGcolor color = pressed ? nvgRGBA(255, 255, 255, 220)
                             : nvgRGBA(255, 60, 60, 220);
    nvgSave(vg);
    nvgBeginPath(vg);
    nvgStrokeWidth(vg, THICKNESS * 2.0f);
    nvgStrokeColor(vg, color);

    nvgMoveTo(vg, cx - HALF_SIZE, cy);
    nvgLineTo(vg, cx + HALF_SIZE, cy);
    nvgStroke(vg);

    nvgMoveTo(vg, cx, cy - HALF_SIZE);
    nvgLineTo(vg, cx, cy + HALF_SIZE);
    nvgStroke(vg);

    nvgRestore(vg);
}

} // namespace beiklive::flash
