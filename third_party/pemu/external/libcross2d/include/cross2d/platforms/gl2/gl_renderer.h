//
// Created by cpasjuste on 06/01/18.
//

#ifndef C2D_RENDERERGL_H
#define C2D_RENDERERGL_H

#include "cross2d/skeleton/renderer.h"

namespace c2d {

    class GLRenderer : public Renderer {

    public:

        explicit GLRenderer(const Vector2f &size = Vector2f(0, 0));

        ~GLRenderer() override;

        void initGL();

        void draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) override;

        void clear() override;

        void flip(bool draw = true, bool inputs = true) override;

        unsigned int vao = 0;

    private:

        const GLenum modes[7] = {GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES,
                                 GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUADS};
    };

    void CheckOpenGLError(const char *stmt, const char *fname, int line);

#ifndef NDEBUG
#define GL_CHECK(stmt) do { \
            stmt; \
            CheckOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

}

#endif //C2D_RENDERERGL_H
