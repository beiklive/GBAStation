//
// Created by cpasjuste on 19/12/16.
//

#ifndef C2D_PSP2_IO_H
#define C2D_PSP2_IO_H

#include "cross2d/platforms/posix/posix_io.h"

namespace c2d {

    class PSP2Io : public POSIXIo {

    public:

        std::string getRomFsPath() override {
            return "app0:/";
        }

        std::string getDataPath() override {
            return "ux0:/data/";
        }
    };
}

#endif //C2D_PSP2_IO_H
