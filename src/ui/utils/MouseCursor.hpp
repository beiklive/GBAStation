#pragma once

#include <borealis.hpp>

namespace beiklive::flash {

class MouseCursor {
public:
    void draw(NVGcontext* vg, float cx, float cy, bool pressed);

private:
    static constexpr float HALF_SIZE = 5.5f;
    static constexpr float THICKNESS = 1.5f;
};

} // namespace beiklive::flash
