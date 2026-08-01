//
// Created by cpasjuste on 06/01/18.
//

#ifndef CROSS2D_GL1RENDER_H
#define CROSS2D_GL1RENDER_H

#include "cross2d/skeleton/renderer.h"

namespace c2d {

    class GLRenderer : public Renderer {

    public:

        explicit GLRenderer(const Vector2f &size = Vector2f(0, 0));

        ~GLRenderer() override;

        virtual void initGL();

        void draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) override;

        void clear() override;

        void flip(bool draw = true, bool inputs = true) override;

    private:

        const GLenum modes[7] = {GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES,
                                 GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUADS};
    };
}

#endif //CROSS2D_GL1RENDER_H
