//
// Created by cpasjuste on 26/11/18.
//

#ifndef CROSS2D_SWITCH_RENDERER_H
#define CROSS2D_SWITCH_RENDERER_H

#include "cross2d/platforms/sdl2/sdl2_renderer.h"

namespace c2d {

    class SWITCHRenderer : public SDL2Renderer {

    public:

        explicit SWITCHRenderer(const Vector2f &size = Vector2f(0, 0));

        ~SWITCHRenderer() override;

    };
}

#endif //CROSS2D_SWITCH_RENDERER_H
