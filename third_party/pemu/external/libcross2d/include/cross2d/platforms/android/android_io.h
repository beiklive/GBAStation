//
// Created by cpasjuste on 07/10/2022.
//

#ifndef C2D_ANDROID_IO_H
#define C2D_ANDROID_IO_H

#include "../posix/posix_io.h"

namespace c2d {

    class AndroidIo : public POSIXIo {

    public:
        AndroidIo();

        std::string getDataPath() override;

        std::string getRomFsPath() override;

    private:
        std::string m_dataPath;
        std::string m_romFsPath;
    };
}

#endif //C2D_ANDROID_IO_H
