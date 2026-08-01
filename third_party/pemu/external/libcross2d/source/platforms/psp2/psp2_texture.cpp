//
// Created by cpasjuste on 01/12/16.
//

#include <cstring>
#include <cstdio>

#include "cross2d/c2d.h"
#include "cross2d/platforms/psp2/psp2_shaders.h"

using namespace c2d;

PSP2Texture::PSP2Texture(const std::string &p) : Texture(p) {
    available = createTexture() == 0;
}

PSP2Texture::PSP2Texture(const unsigned char *buffer, int bufferSize) : Texture(buffer, bufferSize) {
    available = createTexture() == 0;
}

PSP2Texture::PSP2Texture(const Vector2i &size, Format fmt) : Texture(size, fmt) {
    available = createTexture() == 0;
}

int PSP2Texture::createTexture() {
    if (!m_pixels) {
        printf("PSP2Texture::createTexture: m_pixels is null\n");
        return -1;
    }

    m_tex = vita2d_create_empty_texture_format(
            m_tex_size.x, m_tex_size.y,
            m_format == Format::RGBA8 ? SCE_GXM_TEXTURE_FORMAT_A8B8G8R8 : SCE_GXM_TEXTURE_FORMAT_R5G6B5);
    if (!m_tex) {
        printf("PSP2Texture::createTexture: vita2d_create_empty_texture_format failed\n");
        return -1;
    }

    auto src = (uint8_t *) m_pixels;
    auto dst = (uint8_t *) vita2d_texture_get_datap(m_tex);
    int dst_pitch = (int) vita2d_texture_get_stride(m_tex);
    for (int i = 0; i < m_tex_size.y; i++) {
        memcpy(dst, src, m_pitch);
        src += m_pitch;
        dst += dst_pitch;
    }

    free(m_pixels);
    m_pixels = (uint8_t *) vita2d_texture_get_datap(m_tex);
    m_pitch = (int) vita2d_texture_get_stride(m_tex);

    return 0;
}

int PSP2Texture::resize(const Vector2i &size, bool copyPixels) {
    printf("PSP2Texture::resize: %ix%i > %ix%i\n",
           m_tex_size.x, m_tex_size.y, (int) size.x, (int) size.y);

    if (size == m_tex_size || size.x < m_tex_size.x || size.y < m_tex_size.y) {
        printf("Texture::resize: invalid parameters\n");
        return -1;
    }

    vita2d_texture *tex_new = vita2d_create_empty_texture_format(
            (unsigned int) size.x, (unsigned int) size.y,
            m_format == Format::RGBA8 ? SCE_GXM_TEXTURE_FORMAT_A8B8G8R8
                                      : SCE_GXM_TEXTURE_FORMAT_R5G6B5);

    if (!tex_new) {
        printf("PSP2Texture: vita2d_create_empty_texture_format failed\n");
        return -1;
    }

    if (copyPixels) {
        auto *src = (unsigned char *) vita2d_texture_get_datap(m_tex);
        auto *dst = (unsigned char *) vita2d_texture_get_datap(tex_new);
        int dst_pitch = (int) vita2d_texture_get_stride(tex_new);
        for (int i = 0; i < m_tex_size.y; i++) {
            memcpy(dst, src, m_pitch);
            src += m_pitch;
            dst += dst_pitch;
        }
    }

    vita2d_wait_rendering_done();
    vita2d_free_texture(m_tex);
    m_tex = tex_new;
    m_pitch = (int) vita2d_texture_get_stride(m_tex);
    m_pixels = (uint8_t *) vita2d_texture_get_datap(m_tex);
    m_tex_size = size;
    setSize({(float) size.x, (float) size.y});
    setTextureRect(IntRect(0, 0, (int) size.x, (int) size.y));
    setFilter(m_filter);

    return 0;
}

void PSP2Texture::setFilter(Filter filter) {
    Texture::setFilter(filter);
    SceGxmTextureFilter sceFilter = filter == Filter::Point ?
                                    SCE_GXM_TEXTURE_FILTER_POINT : SCE_GXM_TEXTURE_FILTER_LINEAR;
    vita2d_texture_set_filters(m_tex, sceFilter, sceFilter);
}

void PSP2Texture::applyShader() {
    auto *s = (PSP2Shader *) m_shader;

    sceGxmSetVertexProgram(vita2d_get_context(), s->vertexProgram);
    sceGxmSetFragmentProgram(vita2d_get_context(), s->fragmentProgram);

    // set shader params
    auto *tex_size = (float *) vita2d_pool_memalign(2 * sizeof(float), sizeof(float));
    tex_size[0] = (float) getSize().x;
    tex_size[1] = (float) getSize().y;
    //printf("tex_size: %f %f\n", tex_size[0], tex_size[1]);

    auto *out_size = (float *) vita2d_pool_memalign(2 * sizeof(float), sizeof(float));
    out_size[0] = getSize().x * getScale().x;
    out_size[1] = getSize().y * getScale().y;
    //printf("out_size: %f %f\n", out_size[0], out_size[1]);

    if (s->vertexInput.texture_size) {
        s->setVertexUniform((SceGxmProgramParameter *) s->vertexInput.texture_size, tex_size, 2);
    }

    if (s->vertexInput.output_size) {
        s->setVertexUniform((SceGxmProgramParameter *) s->vertexInput.output_size, out_size, 2);
    }

    if (s->vertexInput.video_size) {
        s->setVertexUniform((SceGxmProgramParameter *) s->vertexInput.video_size, tex_size, 2);
    }

    if (s->fragmentInput.texture_size) {
        s->setFragmentUniform((SceGxmProgramParameter *) s->fragmentInput.texture_size, tex_size, 2);
    }

    if (s->fragmentInput.output_size) {
        s->setFragmentUniform((SceGxmProgramParameter *) s->fragmentInput.output_size, out_size, 2);
    }

    if (s->fragmentInput.video_size) {
        s->setFragmentUniform((SceGxmProgramParameter *) s->fragmentInput.video_size, tex_size, 2);
    }
}

PSP2Texture::~PSP2Texture() {
    if (m_tex) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(m_tex);
        m_pixels = nullptr;
    }
}
