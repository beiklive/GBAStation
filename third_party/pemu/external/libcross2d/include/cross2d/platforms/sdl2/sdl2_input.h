//
// Created by cpasjuste on 11/01/17.
//

#ifndef _SDL2_INPUT_H
#define _SDL2_INPUT_H

#include "cross2d/skeleton/input.h"

namespace c2d {

    class SDL2Input : public Input {

    public:
        SDL2Input();

        ~SDL2Input() override;

        Player *update() override;

        int waitButton(int player = 0) override;

    protected:
        Vector2f getAxisState(const Player &player, int xAxis, int yAxis) override;

        int getButtonState(const Player &player, int button) override;

        int getKeyState(int key) override;

        Vector2f getTouch() override;
    };
}

#endif //_SDL2_INPUT_H
