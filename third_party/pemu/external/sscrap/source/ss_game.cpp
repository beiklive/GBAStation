//
// Created by cpasjuste on 11/09/2019.
//

#include <algorithm>
#include "ss_api.h"

using namespace ss_api;

static int parsePlayerString(const std::string &players) {
    if (players == "1") {
        return 1;
    } else if (players == "1-10") {
        return 10;
    } else if (players == "1-16") {
        return 16;
    } else if (players == "1-2") {
        return 2;
    } else if (players == "1-3") {
        return 3;
    } else if (players == "1-4") {
        return 4;
    } else if (players == "1-5") {
        return 5;
    } else if (players == "1-6") {
        return 6;
    } else if (players == "1-8") {
        return 8;
    } else if (players == "2") {
        return 2;
    } else if (players == "2-4") {
        return 4;
    } else if (players == "4") {
        return 4;
    } else if (players == "4+") {
        return 4;
    } else if (players == "6") {
        return 6;
    } else if (players == "8") {
        return 8;
    } else if (players == "8+") {
        return 8;
    }

    // UNKNOWN
    return 1;
}

Game::Media Game::getMedia(const std::string &type) const {
    std::vector<Game::Media> mediaList;

    if (medias.empty()) {
        return {};
    }

    remove_copy_if(medias.begin(), medias.end(), back_inserter(mediaList), [type](const Game::Media &media) {
        return media.type != type;
    });

    return mediaList.empty() ? Media{} : mediaList.at(0);
}

bool Game::isClone() const {
    return !cloneOf.empty() && cloneOf != "0";
}

int Game::Media::download(const std::string &dstPath, int retryDelay) {
    if (dstPath.empty()) {
        return -1;
    }

    SS_PRINT("Game::Media::download: %s\n", url.c_str());
    long code = 0;
    Curl ss_curl;
    int res = ss_curl.getData(url, dstPath, SS_TIMEOUT, &code);
    if (retryDelay > 0) {
        while (code == 429 || code == 28) {
            Api::printe((int) code, retryDelay);
            Io::delay(retryDelay);
            res = ss_curl.getData(url, dstPath, SS_TIMEOUT, &code);
        }
    }

    if (res != 0) {
        SS_PRINT("Game::Media::download: error: curl failed: %s, http_code: %li\n",
                 curl_easy_strerror((CURLcode) res), code);
        return (int) code;
    }

    return 0;
}

bool Game::parseGame(Game *game, tinyxml2::XMLNode *gameNode, const std::string &romName, int format) {
    tinyxml2::XMLElement *element;

    if (!game || !gameNode) {
        return false;
    }

    if (format == GameList::Format::FbNeoDat) {
        // fbneo dat
        game->name = Api::getXmlTextStr(gameNode->FirstChildElement("description"));
        game->path = Api::getXmlAttrStr(gameNode->ToElement(), "name");
        if (!game->path.empty()) {
            game->path += ".zip";
        }
        game->date = Api::getXmlTextStr(gameNode->FirstChildElement("year"));
        if (game->date.empty()) {
            game->date = "UNKNOWN";
        }
        game->developer.name = Api::getXmlTextStr(gameNode->FirstChildElement("manufacturer"));
        game->editor.name = game->developer.name;
        game->cloneOf = Api::getXmlAttrStr(gameNode->ToElement(), "cloneof");
        return true;
    }

    // game id (rom id)
    game->id = Api::getXmlAttrULong(gameNode->ToElement(), "id");

    // game path
    game->path = Api::getXmlTextStr(gameNode->FirstChildElement("path"));
    if (game->path.empty()) {
        game->path = romName;
    }
    // recalbox only ?
    if (game->path.length() > 1 && game->path[0] == '.' && game->path[1] == '/') {
        game->path = game->path.replace(0, 2, "");
    }

    // game name
    if (format == GameList::Format::ScreenScraper) {
        element = gameNode->FirstChildElement("noms");
        if (element) {
            tinyxml2::XMLNode *node = element->FirstChildElement("nom");
            while (node) {
                if (Api::getXmlAttrStr(node->ToElement(), "region") == "wor") {
                    game->name = Api::getXmlTextStr(node->ToElement());
                    break;
                }
                node = node->NextSibling();
            }
            // if "wor" name not found, take first one
            if (game->name.empty()) {
                node = element->FirstChildElement("nom");
                if (node) {
                    game->name = Api::getXmlTextStr(node->ToElement());
                }
            }
        }
    } else {
        game->name = Api::getXmlTextStr(gameNode->FirstChildElement("name"));
    }

    // game clone name
    game->cloneOf = Api::getXmlTextStr(gameNode->FirstChildElement("cloneof"));

    // game system
    std::string system = format == GameList::Format::ScreenScraper ? "systeme" : "system";
    game->system.id = Api::getXmlAttrInt(gameNode->FirstChildElement(system.c_str()), "id");
    game->system.parentId = Api::getXmlAttrInt(gameNode->FirstChildElement(system.c_str()), "parentid");
    game->system.name = Api::getXmlTextStr(gameNode->FirstChildElement(system.c_str()));

    // game synopsis
    if (format == GameList::Format::ScreenScraper) {
        element = gameNode->FirstChildElement("synopsis");
        if (element) {
            tinyxml2::XMLNode *node = element->FirstChildElement("synopsis");
            while (node) {
                if (Api::getXmlAttrStr(node->ToElement(), "langue") == "en") {
                    game->synopsis = Api::getXmlTextStr(node->ToElement());
                    break;
                }
                node = node->NextSibling();
            }
            // if "en" synopsis not found, take first one
            if (game->synopsis.empty()) {
                node = element->FirstChildElement("synopsis");
                if (node) {
                    game->synopsis = Api::getXmlTextStr(node->ToElement());
                }
            }
        }
    } else {
        game->synopsis = Api::getXmlTextStr(gameNode->FirstChildElement("desc"));
    }

    // game medias
    if (format == GameList::Format::ScreenScraper) {
        element = gameNode->FirstChildElement("medias");
        if (element) {
            tinyxml2::XMLNode *node = element->FirstChildElement("media");
            while (node) {
                std::string region = Api::getXmlAttrStr(node->ToElement(), "region");
                std::string type = Api::getXmlAttrStr(node->ToElement(), "type");
                if (region == "wor" || type == "video") {
                    Game::Media media = {
                            Api::getXmlTextStr(node->ToElement()), type,
                            Api::getXmlAttrStr(node->ToElement(), "format")
                    };
                    game->medias.emplace_back(media);
                }
                node = node->NextSibling();
            }
        }
    } else {
        element = gameNode->FirstChildElement("image");
        game->medias.push_back({Api::getXmlTextStr(element), Api::getXmlAttrStr(element, "type", "mixrbv2"), "png"});
        element = gameNode->FirstChildElement("thumbnail");
        game->medias.push_back({Api::getXmlTextStr(element), Api::getXmlAttrStr(element, "type", "mixrbv2"), "png"});
        element = gameNode->FirstChildElement("video");
        game->medias.push_back({Api::getXmlTextStr(element), Api::getXmlAttrStr(element, "type", "video"), "mp4"});
    }

    // game rating
    if (format == GameList::Format::ScreenScraper) {
        game->rating = Api::getXmlTextInt(gameNode->FirstChildElement("note"));
    } else {
        float rating = Api::getXmlTextFloat(gameNode->FirstChildElement("rating"));
        game->rating = (int) (rating * 20);
    }

    // game release date
    if (format == GameList::Format::ScreenScraper) {
        element = gameNode->FirstChildElement("dates");
        if (element != nullptr) {
            tinyxml2::XMLNode *node = element->FirstChildElement("date");
            while (node != nullptr) {
                if (Api::getXmlAttrStr(node->ToElement(), "region") == "wor") {
                    game->date = Api::getXmlTextStr(node->ToElement());
                    break;
                }
                node = node->NextSibling();
            }
            // if "wor" date not found, take first one
            if (game->date.empty()) {
                node = element->FirstChildElement("date");
                if (node) {
                    game->date = Api::getXmlTextStr(node->ToElement());
                }
            }
        }
    } else {
        game->date = Api::getXmlTextStr(gameNode->FirstChildElement("releasedate"));
    }
    if (game->date.size() >= 4) {
        game->date = game->date.substr(0, 4);
    } else if (game->date.empty()) {
        game->date = "UNKNOWN";
    }

    // game developer
    if (format == GameList::Format::ScreenScraper) {
        game->developer.id = Api::getXmlAttrInt(gameNode->FirstChildElement("developpeur"), "id");
        game->developer.name = Api::getXmlTextStr(gameNode->FirstChildElement("developpeur"), "UNKNOWN");
    } else {
        game->developer.id = Api::getXmlAttrInt(gameNode->FirstChildElement("developer"), "id");
        game->developer.name = Api::getXmlTextStr(gameNode->FirstChildElement("developer"), "UNKNOWN");
    }

    // game editor
    if (format == GameList::Format::ScreenScraper) {
        game->editor.id = Api::getXmlAttrInt(gameNode->FirstChildElement("editeur"), "id");
        game->editor.name = Api::getXmlTextStr(gameNode->FirstChildElement("editeur"), "UNKNOWN");
    } else {
        game->editor.id = Api::getXmlAttrInt(gameNode->FirstChildElement("publisher"), "id");
        game->editor.name = Api::getXmlTextStr(gameNode->FirstChildElement("publisher"), "UNKNOWN");
    }

    // game genre
    if (format == GameList::Format::ScreenScraper) {
        element = gameNode->FirstChildElement("genres");
        if (element != nullptr) {
            tinyxml2::XMLNode *node = element->FirstChildElement("genre");
            while (node != nullptr) {
                if (Api::getXmlAttrStr(node->ToElement(), "langue") == "en") {
                    game->genre = {Api::getXmlAttrInt(node->ToElement(), "id"),
                                   Api::getXmlTextStr(node->ToElement(), "UNKNOWN")};
                    break;
                }
                node = node->NextSibling();
            }
            // if "en" genre not found, take first one
            if (game->genre.name.empty()) {
                node = element->FirstChildElement("genre");
                if (node) {
                    game->genre = {Api::getXmlAttrInt(node->ToElement(), "id"),
                                   Api::getXmlTextStr(node->ToElement(), "UNKNOWN")};
                }
            }
        }
    } else {
        game->genre = {Api::getXmlTextInt(gameNode->FirstChildElement("genreid")),
                       Api::getXmlTextStr(gameNode->FirstChildElement("genre"), "UNKNOWN")};
    }

    // game players
    if (format == GameList::Format::ScreenScraper) {
        game->players = Api::getXmlTextStr(gameNode->FirstChildElement("joueurs"));
    } else {
        game->players = Api::getXmlTextStr(gameNode->FirstChildElement("players"), "UNKNOWN");
    }
    game->playersInt = parsePlayerString(game->players);

    // game rotation
    game->rotation = Api::getXmlTextInt(gameNode->FirstChildElement("rotation"));

    // game resolution
    game->resolution = Api::getXmlTextStr(gameNode->FirstChildElement("resolution"));

    return true;
}
