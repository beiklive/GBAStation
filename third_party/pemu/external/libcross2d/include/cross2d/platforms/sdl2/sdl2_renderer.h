//
// Created by cpasjuste on 21/11/16.
//

#ifndef C2D_SDL2_RENDERER_H
#define C2D_SDL2_RENDERER_H

namespace c2d {

    class SDL2Renderer : public GLRenderer {

    public:

        explicit SDL2Renderer(const Vector2f &size = Vector2f(0, 0));

        void flip(bool draw = true, bool inputs = true) override;

        void delay(unsigned int ms) override;

        SDL_Window *getWindow();

        SDL_GLContext getContext();

        void setFullscreen(bool value) override;

    private:

        static void exitCallback();
    };
}

#endif // C2D_SDL2_RENDERER_H
