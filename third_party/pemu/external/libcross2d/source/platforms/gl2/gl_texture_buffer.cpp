//
// Created by cpasjuste on 01/12/16.
//

#ifdef __GL2__

#include "cross2d/c2d.h"
#include "cross2d/platforms/gl2/gl_texture_buffer.h"

#define GL_ABGR_EXT 0x8000

using namespace c2d;

// TODO: fix npot textures (vita)

GLTextureBuffer::GLTextureBuffer(const Vector2i &size, Format format) : Texture(size, format) {
    createTexture(format);
}

int GLTextureBuffer::createTexture(Texture::Format format) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &texID);

    if (texID) {
        glBindTexture(GL_TEXTURE_2D, texID);
        if (format == Format::RGBA8) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei) m_tex_size_pot.x, (GLsizei) m_tex_size_pot.y, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, (GLsizei) m_tex_size_pot.x, (GLsizei) m_tex_size_pot.y, 0,
                         GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texID, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("GLTextureBuffer(%p): couldn't create texture buffer", this);
        } else {
            available = true;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        printf("GLTextureBuffer(%p): couldn't create texture buffer", this);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //printf("GLTextureBuffer(%p): %ix%i\n", this, (int) size.x, (int) size.y);

    return 0;
}

void GLTextureBuffer::deleteTexture() {
    //printf("~GLTextureBuffer(%p)\n", this);
    if (glIsFramebuffer(fbo) == GL_TRUE) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }

    if (glIsTexture(texID) == GL_TRUE) {
        //printf("glDeleteTextures(%i)\n", texID);
        glDeleteTextures(1, &texID);
        texID = 0;
    }
}

int GLTextureBuffer::resize(const Vector2i &size, bool keepPixels) {
    printf("GLTextureBuffer::resize: %i x %i\n", (int) size.x, (int) size.y);
    if (size.x == m_tex_size.x && size.y == m_tex_size.y) {
        return -1;
    }

    deleteTexture();
    m_tex_size = size;
    m_tex_size_pot = isPot() ? Vector2i{Utility::pow2(size.x), Utility::pow2(size.y)} : size;
    createTexture(m_format);

    return 0;
}

void GLTextureBuffer::setFilter(Filter f) {
    this->m_filter = f;

    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    m_filter == Filter::Linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    m_filter == Filter::Linear ? GL_LINEAR : GL_NEAREST);
}

GLTextureBuffer::~GLTextureBuffer() {
    deleteTexture();
}

#endif // __GL2__
