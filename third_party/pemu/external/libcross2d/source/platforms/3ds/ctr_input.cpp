//
// Created by cpasjuste on 11/01/17.
//

#include <3ds.h>
#include "cross2d/platforms/3ds/ctr_input.h"

using namespace c2d;

CTRInput::CTRInput() : Input() {
    m_players[0].enabled = true;
}

Input::Player *CTRInput::update() {
    hidScanInput();
    Player *players = Input::update();

    if (!aptMainLoop()) {
        players[0].buttons |= Input::Button::Quit;
        return players;
    }

    return players;
}

int CTRInput::getButtonState(const Player &player, int button) {
    u32 ctrBtn = c2d2ctr(button);
    return (hidKeysHeld() & ctrBtn) > 0;
}

int CTRInput::waitButton(int player) {
    aptMainLoop();
    hidScanInput();
    if (hidKeysHeld() > 0) {
        return ctr2c2d(hidKeysHeld());
    }

    return -1;
}

Vector2f CTRInput::getTouch() {
    touchPosition touch;
    hidTouchRead(&touch);
    return {touch.px, touch.py};
}

int CTRInput::ctr2c2d(u32 ctrButton) {
    switch (ctrButton) {
        case KEY_DUP:
        case KEY_CPAD_UP:
        case KEY_CSTICK_UP:
            return CTR_BUTTON_UP;
        case KEY_DDOWN:
        case KEY_CPAD_DOWN:
        case KEY_CSTICK_DOWN:
            return CTR_BUTTON_DOWN;
        case KEY_DLEFT:
        case KEY_CPAD_LEFT:
        case KEY_CSTICK_LEFT:
            return CTR_BUTTON_LEFT;
        case KEY_DRIGHT:
        case KEY_CPAD_RIGHT:
        case KEY_CSTICK_RIGHT:
            return CTR_BUTTON_RIGHT;
        case KEY_A:
            return CTR_BUTTON_A;
        case KEY_B:
            return CTR_BUTTON_B;
        case KEY_X:
            return CTR_BUTTON_X;
        case KEY_Y:
            return CTR_BUTTON_Y;
        case KEY_SELECT:
            return CTR_BUTTON_SELECT;
        case KEY_START:
            return CTR_BUTTON_START;
        case KEY_L:
            return CTR_BUTTON_L;
        case KEY_R:
            return CTR_BUTTON_R;
        case KEY_ZL:
            return CTR_BUTTON_ZL;
        case KEY_ZR:
            return CTR_BUTTON_ZR;
        default:
            break;
    }

    return -1;
}

u32 CTRInput::c2d2ctr(int c2dButton) {
    switch (c2dButton) {
        case CTR_BUTTON_UP:
            return KEY_UP;
        case CTR_BUTTON_DOWN:
            return KEY_DOWN;
        case CTR_BUTTON_LEFT:
            return KEY_LEFT;
        case CTR_BUTTON_RIGHT:
            return KEY_RIGHT;
        case CTR_BUTTON_A:
            return KEY_A;
        case CTR_BUTTON_B:
            return KEY_B;
        case CTR_BUTTON_X:
            return KEY_X;
        case CTR_BUTTON_Y:
            return KEY_Y;
        case CTR_BUTTON_SELECT:
            return KEY_SELECT;
        case CTR_BUTTON_START:
            return KEY_START;
        case CTR_BUTTON_L:
            return KEY_L;
        case CTR_BUTTON_R:
            return KEY_R;
        case CTR_BUTTON_ZL:
            return KEY_ZL;
        case CTR_BUTTON_ZR:
            return KEY_ZR;
        default:
            break;
    }

    return -1;
}
