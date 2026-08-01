//
// Created by cpasjuste on 21/11/16.
//

#ifndef TINY3D_RENDERER_H
#define TINY3D_RENDERER_H

#include "cross2d/skeleton/renderer.h"
#include "cross2d/platforms/ps3/tiny3d/tiny3d.h"

namespace c2d {

    class TINY3DRenderer : public Renderer {

    public:

        explicit TINY3DRenderer(const Vector2f &size = Vector2f(0, 0));

        ~TINY3DRenderer() override;

        void draw(VertexArray *vertexArray, const Transform &transform,
                  Texture *texture, Sprite *sprite = nullptr) override;

        void flip(bool draw = true, bool inputs = true) override;

        void delay(unsigned int ms) override;
    };
}

#endif //TINY3D_RENDERER_H
