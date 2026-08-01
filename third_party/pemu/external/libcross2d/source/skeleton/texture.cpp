//
// Created by cpasjuste on 29/11/17.
//

#include "cross2d/c2d.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef __3DS__
//#define STBI_MALLOC linearAlloc
//#define STBI_REALLOC linearRealloc
//#define STBI_FREE linearFree
#define MALLOC linearAlloc
#define FREE linearFree
#else
#define MALLOC malloc
#define FREE free
#endif

#include "cross2d/skeleton/stb_image.h"
#include "cross2d/skeleton/stb_image_write.h"

using namespace c2d;

Texture::Texture(const std::string &p) : RectangleShape({0, 0}) {
    int w, h, n;
    type = Type::Texture;
    m_format = Format::RGBA8;
    m_bpp = 4;
    m_path = p;

    // load pixels buffer
    m_pixels = stbi_load(m_path.c_str(), &w, &h, &n, m_bpp);
    if (!m_pixels) {
        printf("Texture(%p): stbi_load failed (%s): %s\n", this, m_path.c_str(), stbi_failure_reason());
        return;
    }

    // handle pot textures
    if (m_pot) {
        toPot(w, h);
    } else {
        m_tex_size_pot = {w, h};
    }

    // set texture parameters
    m_pitch = m_tex_size_pot.x * m_bpp;
    m_tex_size = {w, h};
    m_unlock_rect = {0, 0, w, h};
    Texture::setSize((float) w, (float) h);
    Texture::setTexture(this);
    Texture::setFillColor(Color::White);
    Texture::setShader(0);

    printf("Texture(%p): tex: %ix%i (pot: %ix%i, rect: %ix%i), bpp: %i, pitch: %i, path: %s\n",
           this, m_tex_size.x, m_tex_size.y, m_tex_size_pot.x, m_tex_size_pot.y,
           m_unlock_rect.width, m_unlock_rect.height, m_bpp, m_pitch, m_path.c_str());
}

Texture::Texture(const unsigned char *buffer, int bufferSize) : RectangleShape({0, 0}) {
    int w, h, n;
    type = Type::Texture;
    m_format = Format::RGBA8;
    m_bpp = 4;

    // load pixels buffer
    m_pixels = stbi_load_from_memory(buffer, bufferSize, &w, &h, &n, m_bpp);
    if (!m_pixels) {
        return;
    }

    // handle pot textures
    if (m_pot) {
        toPot(w, h);
    } else {
        m_tex_size_pot = {w, h};
    }

    // set texture parameters
    m_pitch = m_tex_size_pot.x * m_bpp;
    m_tex_size = {w, h};
    m_unlock_rect = {0, 0, w, h};
    Texture::setSize((float) w, (float) h);
    Texture::setTexture(this);
    Texture::setFillColor(Color::White);
    Texture::setShader(0);

    printf("Texture(%p): tex: %ix%i (pot: %ix%i, rect: %ix%i), bpp: %i, pitch: %i\n",
           this, m_tex_size.x, m_tex_size.y, m_tex_size_pot.x, m_tex_size_pot.y,
           m_unlock_rect.width, m_unlock_rect.height, m_bpp, m_pitch);
}

Texture::Texture(const Vector2i &size, Format format) : RectangleShape(Vector2f{0, 0}) {
    type = Type::Texture;
    m_format = format;
    m_bpp = format == Format::RGB565 ? 2 : 4;

    // handle pot textures
    if (m_pot) {
        m_tex_size_pot = {Utility::pow2(size.x), Utility::pow2(size.y)};
    } else {
        m_tex_size_pot = {size.x, size.y};
    }

    m_pixels = (uint8_t *) MALLOC(m_tex_size_pot.x * m_tex_size_pot.y * m_bpp);
    if (!m_pixels) {
        return;
    }
    memset(m_pixels, 0, m_tex_size_pot.x * m_tex_size_pot.y * m_bpp);

    // set texture parameters
    m_pitch = m_tex_size_pot.x * m_bpp;
    m_tex_size = {size.x, size.y};
    m_unlock_rect = {0, 0, size.x, size.y};
    Texture::setSize((float) size.x, (float) size.y);
    Texture::setTexture(this);
    Texture::setFillColor(Color::White);
    Texture::setShader(0);

    printf("Texture(%p): tex: %ix%i (pot: %ix%i, rect: %ix%i), bpp: %i, pitch: %i\n",
           this, m_tex_size.x, m_tex_size.y, m_tex_size_pot.x, m_tex_size_pot.y,
           m_unlock_rect.width, m_unlock_rect.height, m_bpp, m_pitch);
}

int Texture::lock(uint8_t **pixels, int *pitch, IntRect rect) {
    if (rect != IntRect()) {
        m_unlock_rect = rect;
    }

    if (pitch) {
        *pitch = m_pitch;
    }

    *pixels = m_pixels + m_unlock_rect.top * m_pitch + m_unlock_rect.left * m_bpp;

    //printf("Texture::lock(%p): rect: {%i, %i, %i, %i}, pixels: %p\n",
    //     this, m_unlock_rect.left, m_unlock_rect.top, m_unlock_rect.width, m_unlock_rect.height, *pixels);

    return 0;
}

int Texture::save(const std::string &path) {
    int res;
    int width = m_unlock_rect.width;
    int height = m_unlock_rect.height;

    printf("Texture::save(%p): %ix%i, bpp: %i, pitch: %i\n",
           this, width, height, m_bpp, m_pitch);

    if (!m_pixels) {
        return -1;
    }

    if (m_bpp == 2) {
        // convert RGB565 to RGB888
        auto *dst = new std::byte[(width * height * 3) / 2 * 3];
        for (int i = 0, j = 0; i < width * height * 3; i += 2) {
            int c = m_pixels[i] + (m_pixels[i + 1] << 8);
            dst[j++] = std::byte(((c & 0xF800) >> 11) << 3);
            dst[j++] = std::byte(((c & 0x7E0) >> 5) << 2);
            dst[j++] = std::byte(((c & 0x1F)) << 3);
        }
        res = stbi_write_png(path.c_str(), width, height, 3, dst, width * 3);
        delete[] (dst);
    } else {
        res = stbi_write_png(path.c_str(), width, height, m_bpp, m_pixels, m_pitch);
    }

    return res;
}

void Texture::toPot(int w, int h) {
    m_tex_size_pot = {Utility::pow2(w), Utility::pow2(h)};

    auto *pixels = (uint8_t *) MALLOC(m_tex_size_pot.x * m_tex_size_pot.y * m_bpp);
    if (!pixels) {
        FREE(m_pixels);
        m_pixels = nullptr;
        return;
    }

    memset(pixels, 0, m_tex_size_pot.x * m_tex_size_pot.y * m_bpp);
    uint8_t *dst = pixels;
    int dst_pitch = m_tex_size_pot.x * m_bpp;
    uint8_t *src = m_pixels;
    int src_pitch = w * m_bpp;
    for (int i = 0; i < h; i++) {
        memcpy(dst, src, src_pitch);
        dst += dst_pitch;
        src += src_pitch;
    }

    FREE(m_pixels);
    m_pixels = pixels;
}

void Texture::setShader(int shaderIndex) {
    ShaderList *shaderList = c2d_renderer->getShaderList();
    if (shaderList) {
        m_shader = shaderList->get(shaderIndex);
        if (!m_shader) {
            //printf("Texture::setShader: shader not found (%i)\n", shaderIndex);
            m_shader = shaderList->get(0);
        }
    }
}

void Texture::setShader(const std::string &shaderName) {
    ShaderList *shaderList = c2d_renderer->getShaderList();
    if (shaderList) {
        m_shader = shaderList->get(shaderName);
        if (!m_shader) {
            //printf("Texture::setShader: shader not found (%s)\n", shaderName.c_str());
            m_shader = shaderList->get(0);
        }
    }
}

Texture::~Texture() {
    printf("~Texture(%p)\n", this);
    if (m_pixels) {
        FREE(m_pixels);
    }
}
