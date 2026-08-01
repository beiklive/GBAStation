//
// Created by cpasjuste on 04/02/2021.
//

#ifndef C2D_SDL2_COND_H
#define C2D_SDL2_COND_H

namespace c2d {

    class SDL2Cond : public Cond {

    public:
        SDL2Cond();

        ~SDL2Cond() override;

        int wait(Mutex *mutex) override;

        int signal() override;

    private:
        SDL_cond *cond;
    };
}

#endif //C2D_SDL2_COND_H
