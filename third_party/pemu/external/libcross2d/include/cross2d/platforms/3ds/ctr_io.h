//
// Created by cpasjuste on 19/12/16.
//

#ifndef C2D_CTR_IO_H
#define C2D_CTR_IO_H

#include <3ds.h>
#include "cross2d/platforms/posix/posix_io.h"

namespace c2d {

    class CTRIo : public POSIXIo {

    public:

        CTRIo() {
            romfsExit();
            romfsInit();
        };

        std::string getRomFsPath() override {
            return "romfs:/";
        }
    };
}

#endif //C2D_CTR_IO_H
