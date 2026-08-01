//
// Created by cpasjuste on 11/01/17.
//

#ifndef DREAMCAST_INPUT_H
#define DREAMCAST_INPUT_H

#include "cross2d/skeleton/input.h"

namespace c2d {

    class DCInput : public Input {

    public:
        DCInput();

        ~DCInput() override;

        Player *update() override;

        int waitButton(int player = 0) override;

    protected:
        Vector2f getAxisState(const Player &player, int xAxis, int yAxis) override;

        int getButtonState(const Player &player, int button) override;

        int getKeyState(int key) override;

        Vector2f getTouch() override;

    private:
        struct InputData {
            maple_device_t *cont;
            cont_state_t *state;
        };
    };
}

#endif //DREAMCAST_INPUT_H
