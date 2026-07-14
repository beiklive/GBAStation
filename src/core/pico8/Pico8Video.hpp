#pragma once

#include <cstdint>

#include <glad/glad.h>

struct NVGcontext;

namespace beiklive::pico8
{
    class Video
    {
    public:
        Video() = default;
        ~Video();

        Video(const Video&) = delete;
        Video& operator=(const Video&) = delete;

        bool initialize(NVGcontext* vg);
        bool upload(const uint8_t* rgba);
        void shutdown(NVGcontext* vg);

        bool isReady() const { return m_texture != 0 && m_image > 0; }
        int imageHandle() const { return m_image; }

    private:
        GLuint m_texture = 0;
        int m_image = 0;
    };
}
