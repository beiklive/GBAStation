//
// Created by cpasjuste on 27/06/19.
//

#ifndef PEMU_PFBNEO_IO_H
#define PEMU_PFBNEO_IO_H

#include <string>

#include "burner.h"

extern void BurnPathsInit(c2d::Io *io);
extern void BurnPathsSetGame(c2d::Io *io, const std::string &gameName);

namespace c2d {
    class PFBAIo : public c2d::C2DIo {
    public:
        PFBAIo() : C2DIo() {
            C2DIo::create(PFBAIo::getDataPath());
            C2DIo::create(PFBAIo::getDataPath() + "configs");
            C2DIo::create("sdmc:/GBAStation/saves/FBNeo/");
            BurnPathsInit(this);
            BurnLibInit();
        }

        ~PFBAIo() override {
            printf("~PFBAIo()\n");
            BurnLibExit();
        }

#ifdef __PSP2__
        std::string getDataPath() override {
            return "ux0:/data/pfba/";
        }
#elif __PS4__
        std::string getDataPath() override {
            return "/data/pfba/";
        }
#ifndef NDEBUG
        std::string getRomFsPath() override {
            return "/data/pfba/";
        }

#endif
#elif __3DS__
#ifndef NDEBUG
        std::string getDataPath() override {
            return "/3ds/pfbn/";
        }
#endif
#elif __SWITCH__
        std::string getDataPath() override {
            return "sdmc:/GBAStation/FBNeo/";
        }
#endif
    };
}

#endif //PEMU_PFBNEO_IO_H
