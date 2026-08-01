//
// Created by cpasjuste on 05/04/19.
//

#ifndef SS_GAME_H
#define SS_GAME_H

#include <string>
#include <vector>
#include <tinyxml2.h>

#include "ss_sytem.h"

namespace ss_api {

    class Game {

    public:

        Game() = default;

        struct Editor {
            Editor() = default;

            Editor(int id, const std::string &name) {
                this->id = id;
                this->name = name;
            }

            int id = 0;
            std::string name = "UNKNOWN";
        };

        struct Developer {
            Developer() = default;

            Developer(int id, const std::string &name) {
                this->id = id;
                this->name = name;
            }

            int id = 0;
            std::string name = "UNKNOWN";
        };

        struct Genre {
            Genre() = default;

            Genre(int id, const std::string &name) {
                this->id = id;
                this->name = name;
            }

            int id = 0;
            std::string name = "UNKNOWN";
        };

        struct Media {
            std::string url;
            std::string type;
            std::string format;

            int download(const std::string &dstPath, int retryDelay = 10);
        };

        Game::Media getMedia(const std::string &type) const;

        bool isClone() const;

        static bool parseGame(Game *game, tinyxml2::XMLNode *gameNode,
                              const std::string &romName, int format);

        unsigned long id = 0;
        int rating = 0;
        int rotation = 0;
        bool available = false;
        std::string cloneOf;
        std::string players;
        int playersInt = 0;
        std::string resolution;
        System system;
        Editor editor;
        Developer developer;
        std::string name;
        std::string synopsis;
        Genre genre;
        std::string date;
        std::vector<Media> medias;
        std::string path;
        std::string romsPath;
    };
}

#endif //SS_GAME_H
