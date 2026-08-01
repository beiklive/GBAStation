//
// Created by cpasjuste on 04/02/2021.
//

#ifndef C2D_PSP2_DEVICE_H
#define C2D_PSP2_DEVICE_H

namespace c2d {
    class PSP2Device {
    public:
        static std::string getName() {
            return "Sony PS Vita";
        }

        static Vector2i getDisplaySize() {
            return {960, 544};
        }
    };
}

#endif //C2D_PSP2_DEVICE_H
