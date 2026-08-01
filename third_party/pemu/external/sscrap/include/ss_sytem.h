//
// Created by cpasjuste on 16/03/2022.
//

#ifndef SSCRAP_SS_SYTEM_H
#define SSCRAP_SS_SYTEM_H

#include <string>

namespace ss_api {

    class System {
    public:
        System() = default;

        System(int id, int parentId, const std::string &name) {
            this->id = id;
            this->parentId = parentId;
            this->name = name;
        }

        std::string name = "UNKNOWN";
        int id = 0;
        int parentId = 0;
    };
}

#endif //SSCRAP_SS_SYTEM_H
