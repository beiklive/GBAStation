#include "Pico8Video.hpp"

#include <borealis.hpp>

#define NANOVG_GL3 1
#include <borealis/extern/nanovg/nanovg_gl.h>

namespace
{
    constexpr int PICO8_WIDTH = 128;
    constexpr int PICO8_HEIGHT = 128;
}

namespace beiklive::pico8
{
    Video::~Video()
    {
        shutdown(brls::Application::getNVGContext());
    }

    bool Video::initialize(NVGcontext* vg)
    {
        if (isReady())
            return true;
        if (!vg)
            return false;

        shutdown(vg);
        glGenTextures(1, &m_texture);
        if (!m_texture)
            return false;

        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PICO8_WIDTH, PICO8_HEIGHT,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_image = nvglCreateImageFromHandleGL3(
            vg, m_texture, PICO8_WIDTH, PICO8_HEIGHT,
            NVG_IMAGE_NODELETE | NVG_IMAGE_NEAREST);
        if (m_image <= 0) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
            return false;
        }
        return true;
    }

    bool Video::upload(const uint8_t* rgba)
    {
        if (!rgba || !m_texture)
            return false;
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        PICO8_WIDTH, PICO8_HEIGHT,
                        GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void Video::shutdown(NVGcontext* vg)
    {
        if (m_image > 0 && vg)
            nvgDeleteImage(vg, m_image);
        m_image = 0;
        if (m_texture) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
    }
}
