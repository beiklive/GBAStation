//
// Created by cpasjuste on 13/12/2019.
//

#ifndef SSCRAP_SS_SYSTEM_H
#define SSCRAP_SS_SYSTEM_H

#include <string>
#include <vector>
#include <tinyxml2.h>

#include "ss_sytem.h"

#define SYSTEM_ID_COLECO 48
#define SYSTEM_ID_GAMEGEAR 21
#define SYSTEM_ID_SMS 2
#define SYSTEM_ID_NES 3
#define SYSTEM_ID_MEGADRIVE 1
#define SYSTEM_ID_MEGADRIVE_HACK 203
#define SYSTEM_ID_MSX 113
#define SYSTEM_ID_MSX2 116
#define SYSTEM_ID_PCE 31
#define SYSTEM_ID_SG1000 109
#define SYSTEM_ID_SGX 105
#define SYSTEM_ID_TG16 758 // pFBN custom id
#define SYSTEM_ID_ZX3 76
#define SYSTEM_ID_NES 3
#define SYSTEM_ID_NES_FDS 106
#define SYSTEM_ID_NGP 25
#define SYSTEM_ID_NGPC 82
#define SYSTEM_ID_CHANNELF 80
#define SYSTEM_ID_DREAMCAST 23
#define SYSTEM_ID_ATOMISWAVE 53

namespace ss_api {

    class SystemList {
    public:
        SystemList() = default;

        SystemList(const std::string &ssid, const std::string &sspassword, int retryDelay = 10);

        System *find(const System &system);

        System *find(const std::string &name);

        System *find(int id);

        System findById(int id);

        System findByName(const std::string &name);

        bool remove(int id);

        std::vector<std::string> getNames();

        std::vector<System> systems;

        static bool parseSystem(System *system, tinyxml2::XMLNode *systemNode);

        int http_error = 0;
    };
}

#endif //SSCRAP_SS_SYSTEM_H
