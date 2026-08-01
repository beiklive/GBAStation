//
// Created by cpasjuste on 11/01/17.
//

#include "cross2d/c2d.h"

using namespace c2d;

DCInput::DCInput() : Input() {
    for (int i = 0; i < PLAYER_MAX; i++) {
        maple_device_t *cont = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            m_players[i].id = i;
            m_players[i].data = new InputData({.cont = cont});
            m_players[i].enabled = true;
            printf("Joystick[%i] enabled...\n", i);
        }
    }
}

Input::Player *DCInput::update() {
    return Input::update();
}

Vector2f DCInput::getAxisState(const Player &player, int xAxis, int yAxis) {
    auto data = (InputData *) player.data;

    data->state = (cont_state_t *) maple_dev_status(data->cont);
    if (!data->state) return {};

    return {(float) (xAxis == KEY_JOY_AXIS_LX ? data->state->joyx : data->state->joy2x),
            (float) (yAxis == KEY_JOY_AXIS_LY ? data->state->joyy : data->state->joy2y)};
}

int DCInput::getButtonState(const Player &player, int button) {
    auto data = (InputData *) player.data;

    data->state = (cont_state_t *) maple_dev_status(data->cont);
    if (!data->state) return -1;

    return (data->state->buttons & button) == 1;
}

int DCInput::getKeyState(int key) {
    return 0;
}

Vector2f DCInput::getTouch() {
    return {};
}

int DCInput::waitButton(int player) {
    // TODO
    return -1;
}

DCInput::~DCInput() {
    for (auto &p: m_players) {
        if (p.data) delete ((InputData *) p.data);
    }
}
