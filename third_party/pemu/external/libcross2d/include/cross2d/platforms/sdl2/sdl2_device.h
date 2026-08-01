//
// Created by cpasjuste on 04/02/2021.
//

#ifndef C2D_SDL2_DEVICE_H
#define C2D_SDL2_DEVICE_H

namespace c2d {
    class SDL2Device {
    public:
        static std::string getName();

        static Vector2i getDisplaySize();
    };
}

#endif //C2D_SDL2_DEVICE_H
