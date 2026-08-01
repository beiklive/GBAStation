//
// Created by cpasjuste on 01/12/16.
//

#ifndef _TINY3D_TEXTURE_H_
#define _TINY3D_TEXTURE_H_

#include <pngdec/pngdec.h>
#include <ppu-lv2.h>

#include "cross2d/skeleton/renderer.h"
#include "cross2d/platforms/ps3/tiny3d/tiny3d.h"

namespace c2d {

    class TINY3DTexture : public Texture {

    public:

        explicit TINY3DTexture(const std::string &path);

        explicit TINY3DTexture(const unsigned char *buffer, int bufferSize);

        explicit TINY3DTexture(const Vector2f &size = Vector2f(0, 0), Format format = Format::RGBA8);

        ~TINY3DTexture() override;

        int lock(FloatRect *rect, void **pixels, int *pitch) override;

        void unlock(void *data = nullptr) override;

        //private:
        u32 *pixels;
        u32 offset;
        text_format fmt;

    private:
        bool loadTexture(const std::string &path, const unsigned char *buffer = nullptr, int bufferSize = 0);
    };
}

#endif //_TINY3D_TEXTURE_H_
