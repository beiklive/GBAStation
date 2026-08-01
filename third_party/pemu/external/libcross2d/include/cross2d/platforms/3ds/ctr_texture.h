//
// Created by cpasjuste on 01/12/16.
//

#ifndef C2D_CTR_TEXTURE_H
#define C2D_CTR_TEXTURE_H

#include "cross2d/skeleton/texture.h"
#include "citro3d.h"

namespace c2d {

    class CTRTexture : public Texture {

    public:

        explicit CTRTexture(const std::string &path);

        explicit CTRTexture(const unsigned char *buffer, int bufferSize);

        explicit CTRTexture(const Vector2i &size = Vector2i(), Format format = Format::RGBA8);

        ~CTRTexture() override;

        void unlock(uint8_t *pixels = nullptr) override;

        int resize(const Vector2i &size, bool keepPixels = false) override;

        void setFilter(Filter filter) override;

        C3D_Tex *m_tex = nullptr;

    private:
        int createTexture();

        void upload();

        void uploadSoft();
    };
}

#endif //C2D_CTR_TEXTURE_H
