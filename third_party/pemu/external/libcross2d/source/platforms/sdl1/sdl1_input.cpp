//
// Created by cpasjuste on 11/01/17.
//

#include "cross2d/c2d.h"

#if SDL_VERSION_ATLEAST(1, 3, 0)
#include <SDL/SDL_keyboard.h>
#endif

using namespace c2d;

SDL1Input::SDL1Input() : Input() {
    if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0) {
        printf("SDL1Input: SDL_INIT_JOYSTICK\n");
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    }

    int joystick_count = SDL_NumJoysticks();
    if (joystick_count > PLAYER_MAX) {
        joystick_count = PLAYER_MAX;
    }
    printf("%d Joystick(s) Found\n", joystick_count);

    if (joystick_count > 0) {
        for (int i = 0; i < joystick_count; i++) {
            printf("Joystick: %i\n", i);
            m_players[i].data = SDL_JoystickOpen(i);
            m_players[i].id = i;
            m_players[i].enabled = true;
            printf("Name: %s\n", SDL_JoystickName(i));
            printf("Hats %d\n", SDL_JoystickNumHats((SDL_Joystick *) m_players[i].data));
            printf("Buttons %d\n", SDL_JoystickNumButtons((SDL_Joystick *) m_players[i].data));
            printf("Axis %d\n", SDL_JoystickNumAxes((SDL_Joystick *) m_players[i].data));
        }
    } else {
        // allow keyboard mapping to player1
        m_players[0].enabled = true;
    }
}

Input::Player *SDL1Input::update() {
    Player *players = Input::update();

    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            players[0].buttons |= Input::Button::Quit;
            return players;
        }
    }

    return players;
}

Vector2f SDL1Input::getAxisState(const Player &player, int xAxis, int yAxis) {
    auto joystick = (SDL_Joystick *) player.data;

    return {
            (float) SDL_JoystickGetAxis(joystick, xAxis),
            (float) SDL_JoystickGetAxis(joystick, yAxis)
    };
}

int SDL1Input::getButtonState(const Player &player, int button) {
    // TODO: handle hat
    return SDL_JoystickGetButton((SDL_Joystick *) player.data, button) > 0;
}

int SDL1Input::getKeyState(int key) {
#ifndef NO_KEYBOARD
#if SDL_VERSION_ATLEAST(1, 3, 0)
    return SDL_GetKeyboardState(nullptr)[key] > 0;
#else
    return SDL_GetKeyState(nullptr)[key] > 0;
#endif
#else
    return 0;
#endif
}

Vector2f SDL1Input::getTouch() {
    int x, y;

    if (SDL_GetMouseState(&x, &y) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        return {(float) x, (float) y};
    }

    return {};
}

int SDL1Input::waitButton(int player) {
    SDL_Event event = {};

    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_JOYBUTTONDOWN) {
            return event.jbutton.button;
        } else if (event.type == SDL_JOYAXISMOTION) {
            // TODO
#if 0
            if ((event.jaxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
                 event.jaxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                && (event.jaxis.value < -16000 || event.jaxis.value > 16000)) {
                return event.jaxis.axis + 100;
            }
#endif
        } else if (event.type == SDL_KEYDOWN) {
            return event.key.keysym.scancode;
        }
    }

    return -1;
}

SDL1Input::~SDL1Input() {
    if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }
}
