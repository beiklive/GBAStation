//
// Created by cpasjuste on 01/12/16.
//

#ifndef C2D_TEXTURE_H_
#define C2D_TEXTURE_H_

#include "object.h"
#include "shader_list.h"
#include "cross2d/skeleton/sfml/RectangleShape.hpp"

namespace c2d {
    class Texture : public RectangleShape {
    public:
        enum class Format : int {
            RGB565, RGBA8,
#if defined(__GL2__) && !defined(__GLES2__) // GL2 / GLES3 only
            BGRA8, ARGB8, ABGR8, XRGB8, XBGR8,
#endif
        };

        enum class Filter : int {
            Point, Linear
        };

        explicit Texture(const std::string &path);

        explicit Texture(const unsigned char *buffer, int bufferSize);

        explicit Texture(const Vector2i &size = Vector2i(), Format format = Format::RGBA8);

        virtual ~Texture();

        virtual int lock(uint8_t **pixels, int *pitch = nullptr, IntRect rect = IntRect());

        virtual void unlock(const uint8_t *pixels = nullptr) {
        };

        virtual int resize(const Vector2i &size, bool keepPixels = false) { return -1; };

        virtual int save(const std::string &path);

        virtual void setFilter(Filter filter) { m_filter = filter; };

        virtual void applyShader() {
        };

        virtual void setShader(int shaderIndex);

        virtual void setShader(const std::string &shaderName);

        // internally used for fonts
        virtual void setUnpackRowLength(int rowLength) { m_unpack_row_length = rowLength; };

        virtual Vector2i getTextureSize() {
            return m_tex_size;
        }

        virtual Vector2i getTextureSizePot() {
            return m_tex_size_pot;
        }

        virtual bool isPot() {
            return m_pot;
        }

        std::string m_path;
        int m_bpp = 4;
        int m_pitch = 0;
        Format m_format = Format::RGBA8;
        Filter m_filter = Filter::Linear;
        ShaderList::Shader *m_shader = nullptr;
        uint8_t *m_pixels = nullptr;

    protected:
        void toPot(int w, int h);

        Vector2i m_tex_size;
        Vector2i m_tex_size_pot;
        IntRect m_unlock_rect = IntRect();
        int m_unpack_row_length = 0;
#ifdef __3DS__
        bool m_pot = true;
#else
        bool m_pot = false;
#endif
    };
}

#endif //C2D_TEXTURE_H_
