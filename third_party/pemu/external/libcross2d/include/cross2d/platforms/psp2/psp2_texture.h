//
// Created by cpasjuste on 01/12/16.
//

#ifndef _PSP2_TEXTURE_H_
#define _PSP2_TEXTURE_H_

#include "cross2d/skeleton/texture.h"
#include "vita2d.h"

namespace c2d {

    class PSP2Texture : public Texture {

    public:

        explicit PSP2Texture(const std::string &path);

        explicit PSP2Texture(const unsigned char *buffer, int bufferSize);

        explicit PSP2Texture(const Vector2i &size = Vector2i(), Format format = Format::RGBA8);

        ~PSP2Texture() override;

        int resize(const Vector2i &size, bool keepPixels = false) override;

        void setFilter(Filter filter) override;

        void applyShader() override;

        vita2d_texture *m_tex = nullptr;

    private:
        int createTexture();
    };
}

#endif //_PSP2_TEXTURE_H_
