//
// Created by cpasjuste on 01/12/16.
//

#ifndef GL_TEXTURE_H
#define GL_TEXTURE_H

#include "cross2d/skeleton/texture.h"

namespace c2d {
    class GLTexture : public Texture {
    public:
        explicit GLTexture(const std::string &path);

        explicit GLTexture(const unsigned char *buffer, int bufferSize);

        explicit GLTexture(const Vector2i &size = Vector2i(), Format format = Format::RGBA8);

        ~GLTexture() override;

        int resize(const Vector2i &size, bool keepPixels = false) override;

        void unlock(const uint8_t *pixels = nullptr) override;

        void setFilter(Filter filter) override;

        unsigned int m_texID = 0;

    private:
        struct PixelFormat {
            GLint internalFormat;
            GLenum format;
            GLenum type;
        };

        PixelFormat m_pixelFormat{};

        int createTexture();
    };
}

#endif //GL_TEXTURE_H
