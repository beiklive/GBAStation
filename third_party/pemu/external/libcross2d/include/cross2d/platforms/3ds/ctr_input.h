//
// Created by cpasjuste on 11/01/17.
//

#ifndef C2D_CTR_INPUT_H
#define C2D_CTR_INPUT_H

#include "cross2d/skeleton/renderer.h"
#include "cross2d/skeleton/input.h"

#define CTR_BUTTON_A 0
#define CTR_BUTTON_B 1
#define CTR_BUTTON_X 2
#define CTR_BUTTON_Y 3
#define CTR_BUTTON_SELECT 4
#define CTR_BUTTON_START 5
#define CTR_BUTTON_L 6
#define CTR_BUTTON_R 7
#define CTR_BUTTON_ZL 8
#define CTR_BUTTON_ZR 9
#define CTR_BUTTON_UP 10
#define CTR_BUTTON_DOWN 11
#define CTR_BUTTON_LEFT 12
#define CTR_BUTTON_RIGHT 13

namespace c2d {

    class CTRInput : public Input {

    public:

        explicit CTRInput();

        Player *update() override;

        int waitButton(int player = 0) override;

    private:
        int getButtonState(const Player &player, int button) override;

        Vector2f getTouch() override;

        int ctr2c2d(u32 ctrButton);

        u32 c2d2ctr(int c2dButton);
    };
}

#endif //C2D_CTR_INPUT_H
