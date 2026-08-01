//
// Created by cpasjuste on 21/11/16.
//

#ifndef SDL_RENDERER_H
#define SDL_RENDERER_H

namespace c2d {

    class SDL1Renderer : public GLRenderer {

    public:

        explicit SDL1Renderer(const Vector2f &size = Vector2f(0, 0));

        void flip(bool draw = true, bool inputs = true) override;

        void delay(unsigned int ms) override;

    private:

        static void exitCallback();

    };
}

#endif // SDL_RENDERER_H
