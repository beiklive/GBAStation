//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

std::string SDL2Device::getName() {
    return SDL_GetPlatform();
}

Vector2i SDL2Device::getDisplaySize() {
    SDL_DisplayMode dm;

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }

    SDL_GetDesktopDisplayMode(0, &dm);

    return {dm.w, dm.h};
}
