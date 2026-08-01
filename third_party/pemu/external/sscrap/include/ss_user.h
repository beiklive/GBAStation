//
// Created by cpasjuste on 05/04/19.
//

#ifndef SS_USER_H
#define SS_USER_H

#include <string>

namespace ss_api {

    class User {

    public:

        User() = default;

        // if retryDelay == 0, disable retry or error
        User(const std::string &ssid, const std::string &sspassword, int retryDelay = 10);

        int getMaxThreads();

        static bool parseUser(User *user, tinyxml2::XMLNode *userNode);

        std::string id;
        std::string niveau;
        std::string contribution;
        std::string uploadsysteme;
        std::string uploadinfos;
        std::string romasso;
        std::string uploadmedia;
        std::string maxthreads;
        std::string maxdownloadspeed;
        std::string requeststoday;
        std::string maxrequestsperday;
        std::string visites;
        std::string datedernierevisite;
        std::string favregion;

        int http_error = 0;
    };
}

#endif //SS_USER_H
