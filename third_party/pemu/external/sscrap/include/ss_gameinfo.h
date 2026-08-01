//
// Created by cpasjuste on 11/12/2019.
//

#ifndef SSCRAP_SS_GAMEINFO_H
#define SSCRAP_SS_GAMEINFO_H

namespace ss_api {

    class GameInfo {
    public:
        GameInfo() = default;

        GameInfo(const std::string &crc, const std::string &md5, const std::string &sha1,
                 const std::string &systemeid, const std::string &romtype,
                 const std::string &romnom, const std::string &romtaille, const std::string &gameid,
                 const std::string &ssid = "", const std::string &sspassword = "", int retryDelay = 10);

        Game game;
        int http_error = 0;
    };
}

#endif //SSCRAP_SS_GAMEINFO_H
