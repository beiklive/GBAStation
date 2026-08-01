//
// Created by cpasjuste on 01/12/16.
//

#if defined(__GL2__) || defined(__GL1__)

#include "cross2d/c2d.h"

using namespace c2d;

GLTexture::GLTexture(const std::string &path) : Texture(path) {
    available = createTexture() == 0;
}

GLTexture::GLTexture(const unsigned char *buffer, int bufferSize) : Texture(buffer, bufferSize) {
    available = createTexture() == 0;
}

GLTexture::GLTexture(const Vector2i &size, Format format) : Texture(size, format) {
    available = createTexture() == 0;
}

int GLTexture::createTexture() {
    if (!m_pixels) {
        return -1;
    }

    if (m_format == Format::RGB565) {
        m_pixelFormat = {
            .internalFormat = GL_RGB565,
            .format = GL_RGB,
            .type = GL_UNSIGNED_SHORT_5_6_5
        };
    } else {
        m_pixelFormat = {
            .internalFormat = GL_RGBA8,
            .format = GL_RGBA,
            .type = GL_UNSIGNED_BYTE
        };
    }

    glGenTextures(1, &m_texID);
    glBindTexture(GL_TEXTURE_2D, m_texID);
#ifdef GL_UNPACK_ROW_LENGTH
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, m_pixelFormat.internalFormat, m_tex_size_pot.x, m_tex_size_pot.y,
                 0, m_pixelFormat.format, m_pixelFormat.type, m_pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if defined(__GL2__) && !defined(__GLES2__) // GL2 / GLES3 only
    if (m_format == Format::XBGR8 || m_format == Format::ABGR8) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        if (m_format == Format::XBGR8) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        }
    } else if (m_format == Format::XRGB8 || m_format == Format::ARGB8) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_GREEN);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ALPHA);
        if (m_format == Format::XRGB8) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        }
    } else if (m_format == Format::BGRA8) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    }

#endif
    glBindTexture(GL_TEXTURE_2D, 0);

    return 0;
}

// TODO: handle pot textures
int GLTexture::resize(const Vector2i &size, bool keepPixels) {
    printf("GLTexture::resize: %ix%i > %ix%i\n",
           m_tex_size.x, m_tex_size.y, (int) size.x, (int) size.y);

    if (size == m_tex_size) {
        printf("GLTexture::resize: size not changed, skipping...\n");
        return -1;
    }

    //auto dst_rect = IntRect(0, 0, size.x, size.y);
    auto dst_pitch = size.x * m_bpp;
    auto dst_pixels = (uint8_t *) malloc(dst_pitch * size.y);
    memset(dst_pixels, 0, dst_pitch * size.y);

    // copy pixels if requested
    if (keepPixels) {
        auto dst = dst_pixels;
        auto src = m_pixels;
        for (int i = 0; i < m_tex_size.y; i++) {
            memcpy(dst, src, m_pitch);
            src += m_pitch;
            dst += dst_pitch;
        }
    }

    // replace pixels buffer
    free(m_pixels);
    m_pixels = dst_pixels;

    // update texture
    glBindTexture(GL_TEXTURE_2D, m_texID);
#if defined(GL_UNPACK_ROW_LENGTH)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, m_pixelFormat.internalFormat, 0, 0, 0,
                 m_pixelFormat.format, m_pixelFormat.type, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, m_pixelFormat.internalFormat, size.x, size.y,
                 0, m_pixelFormat.format, m_pixelFormat.type, m_pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    // update texture information
    m_pitch = dst_pitch;
    m_tex_size = {size.x, size.y};
    m_unlock_rect = {0, 0, size.x, size.y};
    setSize({(float) size.x, (float) size.y});
    setTextureRect(m_unlock_rect);
    setFilter(m_filter);

    return 0;
}

void GLTexture::unlock(const uint8_t *pixels) {
    const uint8_t *data = (pixels ? pixels : m_pixels) + m_unlock_rect.top * m_pitch + m_unlock_rect.left * m_bpp;

    //printf("GLTexture::unlock(%p): rect: {%i, %i, %i, %i}, pixels: %p\n",
    //     this, m_unlock_rect.left, m_unlock_rect.top, m_unlock_rect.width, m_unlock_rect.height, data);

    glBindTexture(GL_TEXTURE_2D, m_texID);

#if defined(GL_UNPACK_ROW_LENGTH)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_unpack_row_length);
#else
    // "fix" missing GL_UNPACK_ROW_LENGTH support on ps4, used for fonts
    const IntRect rect = m_unlock_rect;
    if (m_unpack_row_length > 0) {
        // fix missing GL_UNPACK_ROW_LENGTH support on ps4, used for fonts
        data = pixels ? pixels : m_pixels;
        m_unlock_rect = {0, 0, m_tex_size.x, m_tex_size.y};
    }
#endif

    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    m_unlock_rect.left, m_unlock_rect.top, m_unlock_rect.width, m_unlock_rect.height,
                    m_pixelFormat.format, m_pixelFormat.type, data);

    glBindTexture(GL_TEXTURE_2D, 0);

#if !defined(GL_UNPACK_ROW_LENGTH)
    m_unlock_rect = rect;
#endif
}

void GLTexture::setFilter(Filter f) {
    Texture::setFilter(f);
    glBindTexture(GL_TEXTURE_2D, m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_filter == Filter::Linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_filter == Filter::Linear ? GL_LINEAR : GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLTexture::~GLTexture() {
    if (glIsTexture(m_texID)) {
        //printf("glDeleteTextures(%i)\n", texID);
        glDeleteTextures(1, &m_texID);
    }
}

#endif // __GL2__
