//
// Created by cpasjuste on 11/12/2019.
//

#ifndef SSCRAP_SS_MEDIASGAMELIST_H
#define SSCRAP_SS_MEDIASGAMELIST_H

#include <string>
#include <vector>

namespace ss_api {

    class MediasGameList {

    public:

        struct Media {
            Media() = default;

            std::string id;
            std::string name;
            std::string nameShort;
            std::string category;
            std::string platformtypes;
            std::string plateforms;
            std::string type;
            std::string fileformat;
            std::string fileformat2;
            std::string autogen;
            std::string multiregions;
            std::string multisupports;
            std::string multiversions;
            std::string extrainfostxt;
        };

        MediasGameList() = default;

        MediasGameList(const std::string &ssid, const std::string &sspassword, int retryDelay = 10);

        static bool parseMedia(Media *media, tinyxml2::XMLNode *mediaNode);

        std::vector<Media> medias;
        int http_error = 0;
    };
}

#endif //SSCRAP_SS_MEDIASGAMELIST_H
