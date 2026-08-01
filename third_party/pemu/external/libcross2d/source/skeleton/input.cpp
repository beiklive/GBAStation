//
// Created by cpasjuste on 13/01/17.
//

#include "cross2d/c2d.h"

using namespace c2d;

Input::Input() {
    // set default joystick mapping
    std::vector<c2d::Input::ButtonMapping> joy_mapping = {
            {Button::Up,     KEY_JOY_UP_DEFAULT},
            {Button::Down,   KEY_JOY_DOWN_DEFAULT},
            {Button::Left,   KEY_JOY_LEFT_DEFAULT},
            {Button::Right,  KEY_JOY_RIGHT_DEFAULT},
            {Button::Select, KEY_JOY_SELECT_DEFAULT},
            {Button::Start,  KEY_JOY_START_DEFAULT},
            {Button::A,      KEY_JOY_A_DEFAULT},
            {Button::B,      KEY_JOY_B_DEFAULT},
            {Button::X,      KEY_JOY_X_DEFAULT},
            {Button::Y,      KEY_JOY_Y_DEFAULT},
            {Button::LT,     KEY_JOY_LT_DEFAULT},
            {Button::RT,     KEY_JOY_RT_DEFAULT},
            {Button::LB,     KEY_JOY_LB_DEFAULT},
            {Button::RB,     KEY_JOY_RB_DEFAULT},
            {Button::LS,     KEY_JOY_LS_DEFAULT},
            {Button::RS,     KEY_JOY_RS_DEFAULT},
            {Button::Menu1,  KEY_JOY_MENU1_DEFAULT},
            {Button::Menu2,  KEY_JOY_MENU2_DEFAULT}
    };
    for (int i = 0; i < PLAYER_MAX; i++) {
        Input::setJoystickMapping(
                i, joy_mapping,
                {KEY_JOY_AXIS_LX, KEY_JOY_AXIS_LY},
                {KEY_JOY_AXIS_RX, KEY_JOY_AXIS_RY}, 8000);
        m_players[i].mapping_default = joy_mapping;
    }

#ifndef NO_KEYBOARD
    // set default keyboard mapping
    std::vector<c2d::Input::ButtonMapping> kb_mapping = {
            {Button::Up,     KEY_KB_UP_DEFAULT},
            {Button::Down,   KEY_KB_DOWN_DEFAULT},
            {Button::Left,   KEY_KB_LEFT_DEFAULT},
            {Button::Right,  KEY_KB_RIGHT_DEFAULT},
            {Button::Select, KEY_KB_SELECT_DEFAULT},
            {Button::Start,  KEY_KB_START_DEFAULT},
            {Button::A,      KEY_KB_A_DEFAULT},
            {Button::B,      KEY_KB_B_DEFAULT},
            {Button::X,      KEY_KB_X_DEFAULT},
            {Button::Y,      KEY_KB_Y_DEFAULT},
            {Button::LT,     KEY_KB_LT_DEFAULT},
            {Button::RT,     KEY_KB_RT_DEFAULT},
            {Button::LB,     KEY_KB_LB_DEFAULT},
            {Button::RB,     KEY_KB_RB_DEFAULT},
            {Button::LS,     KEY_KB_LS_DEFAULT},
            {Button::RS,     KEY_KB_RS_DEFAULT},
            {Button::Menu1,  KEY_KB_MENU1_DEFAULT},
            {Button::Menu2,  KEY_KB_MENU2_DEFAULT}
    };
    Input::setKeyboardMapping(kb_mapping);
    m_keyboard.mapping_default = kb_mapping;
#endif

    // auto repeat timer
    m_repeatClock = new C2DClock();
}

Input::Player *Input::update() {
    for (auto &player: m_players) {
        if (!player.enabled) {
            continue;
        }

        // reset buttons state
        player.buttons = 0;

        /// process axis (@ rsn8887)
        Vector2f analogState;
        auto deadZone = (float) player.dz;
        float scalingFactor, magnitude;
        Axis *currentStickXAxis;
        Axis *currentStickYAxis;
        float slope = 0.414214f; // tangent of 22.5 degrees for size of angular zones

        // process directional axis
        for (int i = 0; i < 2; i++) {
            if (i == 0) {
                // left stick
                currentStickXAxis = &(player.lx);
                currentStickYAxis = &(player.ly);
            } else {
                // right stick
                currentStickXAxis = &(player.rx);
                currentStickYAxis = &(player.ry);
            }

            analogState = getAxisState(player, currentStickXAxis->id, currentStickYAxis->id);

            // radial and scaled deadzone
            // http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html

            if ((magnitude = std::sqrt(analogState.x * analogState.x + analogState.y * analogState.y)) >= deadZone) {
                // analog control
                scalingFactor = 32767.0f / magnitude * (magnitude - deadZone) / (32769.0f - deadZone);
                currentStickXAxis->value = (short) (analogState.x * scalingFactor);
                currentStickYAxis->value = (short) (analogState.y * scalingFactor);

                // symmetric angular zones for all eight digital directions
                analogState.y = -analogState.y;
                if (analogState.y > 0 && analogState.x > 0) {
                    // upper right quadrant
                    if (analogState.y > slope * analogState.x)
                        player.buttons |= Input::Button::Up;
                    if (analogState.x > slope * analogState.y)
                        player.buttons |= Input::Button::Right;
                } else if (analogState.y > 0 && analogState.x <= 0) {
                    // upper left quadrant
                    if (analogState.y > slope * (-analogState.x))
                        player.buttons |= Input::Button::Up;
                    if ((-analogState.x) > slope * analogState.y)
                        player.buttons |= Input::Button::Left;
                } else if (analogState.y <= 0 && analogState.x > 0) {
                    // lower right quadrant
                    if ((-analogState.y) > slope * analogState.x)
                        player.buttons |= Input::Button::Down;
                    if (analogState.x > slope * (-analogState.y))
                        player.buttons |= Input::Button::Right;
                } else if (analogState.y <= 0 && analogState.x <= 0) {
                    // lower left quadrant
                    if ((-analogState.y) > slope * (-analogState.x))
                        player.buttons |= Input::Button::Down;
                    if ((-analogState.x) > slope * (-analogState.y))
                        player.buttons |= Input::Button::Left;
                }
            } else {
                currentStickXAxis->value = 0;
                currentStickYAxis->value = 0;
            }
        }

        /// process buttons
        for (const auto &buttonMap: player.mapping) {
            if (getButtonState(player, buttonMap.value)) {
                player.buttons |= buttonMap.button;
            }
        }

        /// apply rotation if needed
        applyRotation(&player);
    }

    /// process keyboard
    for (const auto &keyMap: m_keyboard.mapping) {
        if (getKeyState(keyMap.value)) {
            m_players[0].buttons |= keyMap.button;
        }
    }

    /// process touch
    m_players[0].touch = getTouch();
    if (m_players[0].touch != Vector2f()) {
        m_players[0].buttons |= Input::Button::Touch;
    }

    /// process auto repeat
    if (!m_repeat || m_players[0].buttons & Input::Button::Quit) {
        m_stateOld = m_players[0].buttons;
        return m_players;
    }

    if (m_repeatClock->getElapsedTime().asMilliseconds() >= m_repeatDelay) {
        m_repeatClock->restart();
        m_stateOld = m_players[0].buttons;
        return m_players;
    } else {
        unsigned int diff = m_stateOld ^ m_players[0].buttons;
        m_stateOld = m_players[0].buttons;
        if (diff > 0) {
            m_repeatClock->restart();
        } else {
            m_players[0].buttons = Button::Delay;
        }
    }

    return m_players;
}

void Input::setRepeatDelay(int ms) {
    m_repeatDelay = ms;
    m_repeat = ms > 0;
}

int Input::getRepeatDelay() {
    return m_repeatDelay;
}

int Input::clear(int player) {
    while (true) {
        Player p = update()[player];
        if (!p.enabled || p.buttons == 0 || p.buttons & Input::Button::Quit) {
            break;
        }
    }

    return 0;
}

void Input::setJoystickMapping(int player, const std::vector<ButtonMapping> &mapping,
                               const Vector2i &leftAxis, const Vector2i &rightAxis, int dz) {
    if (player < PLAYER_MAX) {
        m_players[player].mapping = mapping;
        m_players[player].lx.id = leftAxis.x;
        m_players[player].ly.id = leftAxis.y;
        m_players[player].rx.id = rightAxis.x;
        m_players[player].ry.id = rightAxis.y;
        m_players[player].dz = dz;
    }
}

void Input::setKeyboardMapping(const std::vector<ButtonMapping> &mapping) {
    m_keyboard.mapping = mapping;
}

std::vector<Input::ButtonMapping> Input::getKeyboardMapping() {
    return m_keyboard.mapping;
}

std::vector<Input::ButtonMapping> Input::getKeyboardMappingDefault() {
    return m_keyboard.mapping_default;
}

void Input::setRotation(const Rotation &dirRotation, const Rotation &buttonRotation) {
    m_dir_rotation = dirRotation;
    m_button_rotation = buttonRotation;
}

Input::Rotation Input::getDirRotation() {
    return m_dir_rotation;
}

Input::Rotation Input::getButtonRotation() {
    return m_button_rotation;
}

unsigned int Input::getButtons(int player) {
    if (player < PLAYER_MAX) {
        return m_players[player].buttons;
    }
    return 0;
}

Input::Player *Input::getPlayer(int player) {
    if (player < PLAYER_MAX) {
        return &m_players[player];
    }
    return nullptr;
}

Input::Player *Input::getPlayers() {
    return m_players;
}

void Input::applyRotation(Player *player) {
    unsigned int buttons = player->buttons;

    // handle directional rotation
    if (m_dir_rotation != Input::Rotation::R0) {
        player->buttons &= ~Input::Button::Up;
        player->buttons &= ~Input::Button::Right;
        player->buttons &= ~Input::Button::Down;
        player->buttons &= ~Input::Button::Left;
        if (m_dir_rotation == R90) {
            if (buttons & Button::Up)
                player->buttons |= Button::Left;
            if (buttons & Button::Right)
                player->buttons |= Button::Up;
            if (buttons & Button::Down)
                player->buttons |= Button::Right;
            if (buttons & Button::Left)
                player->buttons |= Button::Down;
        } else if (m_dir_rotation == R180) {
            if (buttons & Button::Up)
                player->buttons |= Button::Down;
            if (buttons & Button::Right)
                player->buttons |= Button::Left;
            if (buttons & Button::Down)
                player->buttons |= Button::Up;
            if (buttons & Button::Left)
                player->buttons |= Button::Right;
        } else if (m_dir_rotation == R270) {
            if (buttons & Button::Up)
                player->buttons |= Button::Right;
            if (buttons & Button::Right)
                player->buttons |= Button::Down;
            if (buttons & Button::Down)
                player->buttons |= Button::Left;
            if (buttons & Button::Left)
                player->buttons |= Button::Up;
        }
    }

    // handle buttons rotation
    if (m_button_rotation != Input::Rotation::R0) {
        player->buttons &= ~Input::Button::A;
        player->buttons &= ~Input::Button::B;
        player->buttons &= ~Input::Button::X;
        player->buttons &= ~Input::Button::Y;
        if (m_dir_rotation == R90) {
            if (buttons & Button::Y)
                player->buttons |= Button::X;
            if (buttons & Button::B)
                player->buttons |= Button::Y;
            if (buttons & Button::A)
                player->buttons |= Button::B;
            if (buttons & Button::X)
                player->buttons |= Button::A;
        } else if (m_dir_rotation == R180) {
            if (buttons & Button::Y)
                player->buttons |= Button::A;
            if (buttons & Button::B)
                player->buttons |= Button::X;
            if (buttons & Button::A)
                player->buttons |= Button::Y;
            if (buttons & Button::X)
                player->buttons |= Button::B;
        } else if (m_dir_rotation == R270) {
            if (buttons & Button::Y)
                player->buttons |= Button::B;
            if (buttons & Button::B)
                player->buttons |= Button::A;
            if (buttons & Button::A)
                player->buttons |= Button::X;
            if (buttons & Button::X)
                player->buttons |= Button::Y;
        }
    }
}

Input::~Input() {
    delete (m_repeatClock);
}
