//
// Created by cpasjuste on 01/12/16.
//

#include "cross2d/c2d.h"
#include <rsx/mm.h>

using namespace c2d;

bool TINY3DTexture::loadTexture(const std::string &path, const unsigned char *buffer, int bufferSize) {
    pngData png;

    if (buffer == nullptr) {
        if (pngLoadFromFile(path.c_str(), &png)) {
            printf("TINY3DTexture::loadTexture: could not load png (pngLoadFromFile)\n");
            return false;
        }
    } else {
        if (pngLoadFromBuffer(buffer, bufferSize, &png)) {
            printf("TINY3DTexture::loadTexture: could not load png (pngLoadFromBuffer)\n");
            return false;
        }
    }

    fmt = bpp == 2 ? TINY3D_TEX_FORMAT_R5G6B5 : TINY3D_TEX_FORMAT_A8R8G8B8;
    pitch = (int) png.pitch;
    pixels = (u32 *) tiny3d_AllocTexture(pitch * png.height);
    if (!pixels) {
        printf("TINY3DTexture::loadTexture: could not create texture (tiny3d_AllocTexture)\n");
        free(png.bmp_out);
        return false;
    }

    RSX_MEMCPY(pixels, png.bmp_out, pitch * png.height);
    free(png.bmp_out);

    offset = tiny3d_TextureOffset(pixels);
    tex_size = {(int) png.width, (int) png.height};
    TINY3DTexture::setSize((float) tex_size.x, (float) tex_size.y);
    TINY3DTexture::setTexture(this, true);
    TINY3DTexture::setFilter(filter);
    TINY3DTexture::setShader(0);

    return true;
}

TINY3DTexture::TINY3DTexture(const std::string &path) : Texture(path) {
    available = loadTexture(path);
}

TINY3DTexture::TINY3DTexture(const unsigned char *buffer, int bufferSize) : Texture(buffer, bufferSize) {
    available = loadTexture("", buffer, bufferSize);
}

TINY3DTexture::TINY3DTexture(const Vector2f &size, Format format) : Texture(size, format) {

    pixels = (u32 *) tiny3d_AllocTexture((u32) size.x * (u32) size.y * bpp);
    if (!pixels) {
        printf("TINY3DTexture: could not create texture (tiny3d_AllocTexture)\n");
        return;
    }

    fmt = bpp == 2 ? TINY3D_TEX_FORMAT_R5G6B5 : TINY3D_TEX_FORMAT_A8R8G8B8;
    offset = tiny3d_TextureOffset(pixels);
    tex_size = {(int) size.x, (int) size.y};
    TINY3DTexture::setSize(size.x, size.y);
    TINY3DTexture::setTextureRect(IntRect(0, 0, (int) size.x, (int) size.y));
    TINY3DTexture::setTexture(this, true);
    TINY3DTexture::setFilter(filter);
    TINY3DTexture::setShader(0);

    available = true;
}

int TINY3DTexture::lock(FloatRect *rect, void **pix, int *p) {

    if (!rect) {
        *pix = pixels;
    } else {
        *pix = (void *) ((uint8_t *) pixels + (int) rect->top * pitch + (int) rect->left * bpp);
    }

    if (p) {
        *p = pitch;
    }

    return 0;
}

void TINY3DTexture::unlock(void *data) {
    if (data) {
        RSX_MEMCPY(pixels, data, pitch * (int) TINY3DTexture::getSize().y);
    }
}

TINY3DTexture::~TINY3DTexture() {
    if (pixels) {
        rsxFree(pixels);
    }
}
