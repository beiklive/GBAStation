//
// Created by cpasjuste on 11/12/2019.
//

#ifndef SSCRAP_SS_GAMESEARCH_H
#define SSCRAP_SS_GAMESEARCH_H

#include <vector>
#include "ss_game.h"
#include "ss_user.h"

namespace ss_api {

    class GameSearch {
    public:
        GameSearch() = default;

        GameSearch(const std::string &recherche, const std::string &systemeid,
                   const std::string &ssid = "", const std::string &sspassword = "", int retryDelay = 10);

        User user;
        std::vector<Game> games;

        int http_error = 0;
    };

}

#endif //SSCRAP_SS_GAMESEARCH_H
