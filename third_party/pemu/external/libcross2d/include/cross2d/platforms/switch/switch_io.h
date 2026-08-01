//
// Created by cpasjuste on 19/12/16.
//

#ifndef C2D_SWITCH_IO_H
#define C2D_SWITCH_IO_H

#include <switch.h>
#include "cross2d/platforms/posix/posix_io.h"

namespace c2d {

    class NXIo : public POSIXIo {

    public:

        NXIo() : POSIXIo() {
            romfsInit();
        }

        std::string getRomFsPath() override {
            return "romfs:/";
        }
    };
}

#endif //C2D_SWITCH_IO_H
