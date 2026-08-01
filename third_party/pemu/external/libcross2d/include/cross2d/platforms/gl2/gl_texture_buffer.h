//
// Created by cpasjuste on 01/12/16.
//

#ifndef GL_TEXTURE_BUFFER_H
#define GL_TEXTURE_BUFFER_H

#include "cross2d/skeleton/texture.h"

namespace c2d {

    class GLTextureBuffer : public Texture {

    public:

        explicit GLTextureBuffer(const Vector2i &size = Vector2i(0, 0), Format format = Format::RGBA8);

        ~GLTextureBuffer() override;

        int resize(const Vector2i &size, bool keepPixels = false) override;

        void setFilter(Filter filter) override;

        unsigned int texID = 0;
        unsigned int fbo = 0;

    private:
        int createTexture(Format format = Format::RGBA8);

        void deleteTexture();
    };
}

#endif //GL_TEXTURE_BUFFER_H
