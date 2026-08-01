//
// Created by cpasjuste on 11/01/17.
//

#include "cross2d/c2d.h"

using namespace c2d;

SDL2Input::SDL2Input() : Input() {
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    int joystick_count = SDL_NumJoysticks();
    if (joystick_count > PLAYER_MAX) {
        joystick_count = PLAYER_MAX;
    }

    printf("SDL2Input: %i Joystick(s) Found\n", joystick_count);

    if (joystick_count > 0) {
        for (int i = 0; i < joystick_count; i++) {
            if (!SDL_IsGameController(i)) {
                printf("SDL2Input: joystick %i is not a game controller, skipping!", i);
                continue;
            }

            SDL_GameController *pad = SDL_GameControllerOpen(i);
            char *mapping = SDL_GameControllerMapping(pad);
            printf("SDL2Input: GameController detected: %s, mapping: %s\n",
                   SDL_GameControllerName(pad), mapping);
            SDL_free(mapping);

            m_players[i].data = pad;
            m_players[i].id = i;
            m_players[i].enabled = true;
        }
    } else {
        // allow keyboard mapping to player1
        m_players[0].enabled = true;
    }
}

Input::Player *SDL2Input::update() {
    bool quit = false;

    // be sure sdl events are "pumped/cleared" before calling Input::update
    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit = true;
            break;
        }
    }

    Player *players = Input::update();
    if (quit) {
        players[0].buttons |= Input::Button::Quit;
    }

    return players;
}

Vector2f SDL2Input::getAxisState(const Player &player, int xAxis, int yAxis) {
    auto pad = (SDL_GameController *) player.data;

    return {
            (float) SDL_GameControllerGetAxis(pad, (SDL_GameControllerAxis) xAxis),
            (float) SDL_GameControllerGetAxis(pad, (SDL_GameControllerAxis) yAxis)
    };
}

int SDL2Input::getButtonState(const Player &player, int button) {
    auto pad = (SDL_GameController *) player.data;

    if (button == ((int) SDL_CONTROLLER_AXIS_TRIGGERLEFT + 100)
        || button == ((int) SDL_CONTROLLER_AXIS_TRIGGERRIGHT + 100)) {
        return SDL_GameControllerGetAxis(pad, (SDL_GameControllerAxis) (button - 100)) > player.dz;
    }

    return SDL_GameControllerGetButton(pad, (SDL_GameControllerButton) button) > 0;
}

int SDL2Input::getKeyState(int key) {
#ifndef NO_KEYBOARD
    return SDL_GetKeyboardState(nullptr)[key] > 0;
#else
    return 0;
#endif
}

Vector2f SDL2Input::getTouch() {
    int x, y;

    if (SDL_GetMouseState(&x, &y) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        return {(float) x, (float) y};
    }

    return {};
}

int SDL2Input::waitButton(int player) {
    SDL_Event event = {};

    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            return event.cbutton.button;
        } else if (event.type == SDL_CONTROLLERAXISMOTION) {
            if ((event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
                 event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                && (event.caxis.value < -16000 || event.caxis.value > 16000)) {
                return event.caxis.axis + 100;
            }
        } else if (event.type == SDL_KEYDOWN) {
            return event.key.keysym.scancode;
        }
    }

    return -1;
}

SDL2Input::~SDL2Input() {
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}
