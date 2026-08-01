//
// Created by cpasjuste on 21/11/16.
//

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <cstdio>

#include "texture.h"
#include "shader_list.h"
#include "cross2d/skeleton/sfml/Sprite.hpp"
#include "cross2d/skeleton/sfml/Rectangle.hpp"
#include "cross2d/skeleton/sfml/Clock.hpp"
#include "cross2d/skeleton/sfml/Font.hpp"
#include "cross2d/skeleton/input.h"
#include "cross2d/skeleton/io.h"

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

namespace c2d {

    class Renderer : public Rectangle {

    public:

        explicit Renderer(const Vector2f &size = Vector2f(0, 0));

        ~Renderer() override;

        virtual void draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) {};

        virtual void clear() {};

        virtual void flip(bool draw = true, bool process_inputs = true);

        virtual void delay(unsigned int millis) {};

        virtual void delayUs(unsigned int micros) {
            if (micros < 1000) micros = 1000;
            delay(micros / 1000);
        };

        virtual ShaderList *getShaderList();

        virtual void setShaderList(ShaderList *list);

        virtual void setClearColor(const Color &color);

        // SDL2 only
        virtual void setFullscreen(bool value) {}

        Color getClearColor() const;

        Time getDeltaTime() const;

        Time getElapsedTime() const;

        float getFps() const;

        virtual Io *getIo() { return m_io; };

        virtual void setIo(Io *io) {
            if (m_io) { delete (m_io); }
            m_io = io;
        };

        virtual Font *getFont() { return m_font; };

        virtual void setFont(Font *font) {
            if (m_font) { delete (m_font); }
            m_font = font;
        };

        virtual Input *getInput() { return m_input; };

        virtual void setInput(Input *input) {
            if (m_input) { delete (m_input); }
            m_input = input;
        };

        virtual void setPrintStats(bool enable) {
            m_stats_print = enable;
        }

    protected:

        void onUpdate() override;

        Color m_clearColor = Color::Black;
        bool m_process_inputs = true;
        Input *m_input = nullptr;
        Io *m_io = nullptr;
        Font *m_font = nullptr;
        ShaderList *m_shaderList = nullptr;
        Clock *m_elapsedClock, *m_deltaClock, *m_fpsClock;
        Time m_deltaTime;
        float m_fps = 0;
        float m_frames = 0;
        bool m_stats_print = false;
    };
}

#endif //_RENDERER_H_
