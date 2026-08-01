//
// Created by cpasjuste on 23/02/2021.
//

#ifndef SSCRAP_SS_DREAMCAST_H
#define SSCRAP_SS_DREAMCAST_H

#include <string>
#include <vector>

namespace ss_api {

    class Dreamcast {

    public:
        class HeaderFix {
        public:
            std::string headerName;
            std::string searchName;
        };

        static HeaderFix getHeaderFix(const std::string &headerName);
    };
}

#endif //SSCRAP_SS_DREAMCAST_H
