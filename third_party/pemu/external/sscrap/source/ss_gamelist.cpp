//
// Created by cpasjuste on 03/12/2019.
//

#include <algorithm>
#include <cmath>
#include "ss_api.h"
#include "ss_gamelist.h"

using namespace ss_api;

bool GameList::append(const std::string &xmlPath, const std::string &rPath,
                      bool sort, const std::vector<std::string> &filters,
                      const System &system, bool availableOnly, const GameAddedCb &cb) {
    tinyxml2::XMLDocument doc;
    std::vector<Io::File> files;

    // add all files first
    if (!rPath.empty()) {
        romPaths.emplace_back(rPath);
        files = Io::getDirList(rPath, false, filters);
    }

    xml = xmlPath;
    tinyxml2::XMLError e = doc.LoadFile(xmlPath.c_str());
    if (e == tinyxml2::XML_SUCCESS) {
        tinyxml2::XMLNode *pRoot = doc.FirstChildElement("gameList");
        if (!pRoot) {
            pRoot = doc.FirstChildElement("datafile");
            if (!pRoot) {
                SS_PRINT("GameList: wrong xml format: \'Data\', \'gameList\' or \'datafile\' tag not found\n");
                format = Format::Unknown;
                doc.Clear();
                return false;
            }
            format = Format::FbNeoDat;
        } else {
            format = Format::EmulationStation;
        }

        tinyxml2::XMLNode *gameNode = pRoot->FirstChildElement("game");
        while (gameNode) {
            Game game;
            Game::parseGame(&game, gameNode, "", format);

            // set game "real path", minus filename (for pFBN)
            game.romsPath = rPath;

            // is rom available?
            const std::string n = game.path;
            auto it = std::find_if(files.begin(), files.end(), [n](const Io::File &f) {
                return f.name == n;
            });
            if (it != files.end()) {
                game.available = true;
                files.erase(it);
            } else if (availableOnly) {
                // move to next node (game)
                gameNode = gameNode->NextSibling();
                continue;
            }

            // add stuff for later filtering
            if (system.id) game.system = system;
            System sys1 = game.system;
            auto itSys = std::find_if(systemList.systems.begin(), systemList.systems.end(), [sys1](const System &sys2) {
                return sys1.id == sys2.id || sys1.name == sys2.name;
            });
            if (itSys == systemList.systems.end()) {
                systemList.systems.emplace_back(sys1);
            }

            Game::Editor ed = game.editor.name.empty() ? Game::Editor{0, "UNKNOWN"} : game.editor;
            auto itEd = std::find_if(editors.begin(), editors.end(), [ed](const Game::Editor &e) {
                return ed.id == e.id || ed.name == e.name;
            });
            if (itEd == editors.end()) {
                editors.emplace_back(ed);
            }

            Game::Developer dev = game.developer.name.empty() ? Game::Developer{0, "UNKNOWN"} : game.developer;
            auto itDev = std::find_if(developers.begin(), developers.end(), [dev](const Game::Developer &d) {
                return dev.id == d.id || dev.name == d.name;
            });
            if (itDev == developers.end()) {
                developers.emplace_back(dev);
            }

            Game::Genre genre = game.genre.name.empty() ? Game::Genre{0, "UNKNOWN"} : game.genre;
            auto itGenre = std::find_if(genres.begin(), genres.end(), [genre](const Game::Genre &g) {
                return genre.id == g.id || genre.name == g.name;
            });
            if (itGenre == genres.end()) {
                genres.emplace_back(genre);
            }

            auto itPlayers = std::find(players.begin(), players.end(), game.playersInt);
            if (itPlayers == players.end()) {
                players.emplace_back(game.playersInt);
            }

            auto itRat = std::find(ratings.begin(), ratings.end(), game.rating);
            if (itRat == ratings.end()) {
                ratings.emplace_back(game.rating);
            }

            auto itRot = std::find(rotations.begin(), rotations.end(), game.rotation);
            if (itRot == rotations.end()) {
                rotations.emplace_back(game.rotation);
            }

            std::string resolution = game.resolution.empty() ? "UNKNOWN" : game.resolution;
            auto it2 = std::find(resolutions.begin(), resolutions.end(), resolution);
            if (it2 == resolutions.end()) {
                resolutions.emplace_back(resolution);
            }

            it2 = std::find(dates.begin(), dates.end(), game.date);
            if (it2 == dates.end()) {
                dates.emplace_back(game.date);
            }

            // callback
            if (cb) cb(&game);

            // add game to game list
            games.emplace_back(game);

            // move to next node (game)
            gameNode = gameNode->NextSibling();
        }
        doc.Clear();
    } else {
        SS_PRINT("GameList: %s\n", doc.ErrorName());
    }

    // add "unknown" files (not in database)
    for (const auto &file: files) {
        Game game;
        game.id = std::hash<std::string>()(rPath + "/" + file.name);
        game.path = file.name;
        game.romsPath = rPath;
        game.name = file.name;
        game.available = true;
        game.system = system;
        System sys1 = game.system;
        auto itSys = std::find_if(systemList.systems.begin(), systemList.systems.end(), [sys1](const System &sys2) {
            return sys1.id == sys2.id || sys1.name == sys2.name;;
        });
        if (itSys == systemList.systems.end()) {
            systemList.systems.emplace_back(sys1);
        }
        // callback
        if (cb) cb(&game);
        games.emplace_back(game);
    }

    if (sort) {
        sortAlpha();
    }

    return true;
}

void GameList::sortAlpha(bool byZipName, bool gamesOnly) {
    // sort games
    if (byZipName) {
        std::sort(games.begin(), games.end(), Api::sortGameByPath);
    } else {
        std::sort(games.begin(), games.end(), Api::sortGameByName);
    }

    // sort lists
    if (!gamesOnly) {
        std::sort(systemList.systems.begin(), systemList.systems.end(), Api::sortSystemByName);
        std::sort(editors.begin(), editors.end(), Api::sortEditorByName);
        std::sort(developers.begin(), developers.end(), Api::sortDeveloperByName);
        std::sort(genres.begin(), genres.end(), Api::sortGenreByName);
        std::sort(players.begin(), players.end(), Api::sortInteger);
        std::sort(ratings.begin(), ratings.end(), Api::sortInteger);
        std::sort(rotations.begin(), rotations.end(), Api::sortInteger);
        std::sort(resolutions.begin(), resolutions.end(), Api::sortByName);
        std::sort(dates.begin(), dates.end(), Api::sortByName);
    }
}

bool GameList::save(const std::string &dstPath, const std::string &imageType,
                    const std::string &thumbnailType, const std::string &videoType) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *elem;

    tinyxml2::XMLDeclaration *dec = doc.NewDeclaration();
    doc.InsertFirstChild(dec);

    tinyxml2::XMLNode *pRoot = doc.NewElement("gameList");
    doc.InsertEndChild(pRoot);

    tinyxml2::XMLElement *pGames = pRoot->ToElement();

    // sort games
    std::sort(games.begin(), games.end(), Api::sortGameByName);

    for (const auto &game: games) {
        tinyxml2::XMLElement *gameElement = doc.NewElement("game");
        if (game.id > 0) {
            gameElement->SetAttribute("id", (int64_t) game.id);
        }
        Api::addXmlElement(&doc, gameElement, "path", game.path);
        Api::addXmlElement(&doc, gameElement, "name", game.name);
        Api::addXmlElement(&doc, gameElement, "desc", game.synopsis);
        if (game.rating > 0) {
            std::string rating = std::to_string((float) game.rating / 20.0f);
            Api::addXmlElement(&doc, gameElement, "rating", rating.substr(0, rating.find('.') + 3));
        }
        if (!game.date.empty() && game.date != "UNKNOWN") {
            Api::addXmlElement(&doc, gameElement, "releasedate", game.date + "0101T000000");
        }
        if (game.developer.id > 0) {
            elem = Api::addXmlElement(&doc, gameElement, "developer", game.developer.name);
            if (elem) elem->SetAttribute("id", game.developer.id);
        }
        if (game.editor.id > 0) {
            elem = Api::addXmlElement(&doc, gameElement, "publisher", game.editor.name);
            if (elem) elem->SetAttribute("id", game.editor.id);
        }
        if (game.genre.id > 0) {
            Api::addXmlElement(&doc, gameElement, "genre", game.genre.name);
            Api::addXmlElement(&doc, gameElement, "genreid", std::to_string(game.genre.id));
        }
        Api::addXmlElement(&doc, gameElement, "players", game.players);
        // pemu
        Api::addXmlElement(&doc, gameElement, "cloneof", game.cloneOf);
        elem = Api::addXmlElement(&doc, gameElement, "system", game.system.name);
        if (elem) {
            elem->SetAttribute("id", game.system.id);
            elem->SetAttribute("parentid", game.system.parentId);
        }
        if (game.rotation != 0) {
            Api::addXmlElement(&doc, gameElement, "rotation", std::to_string(game.rotation));
        }
        Api::addXmlElement(&doc, gameElement, "resolution", game.resolution);
        // pemu

        const std::vector<std::string> es_names = {"image", "thumbnail", "video"};
        const std::vector<std::string> ss_names = {imageType, thumbnailType, videoType};
        for (size_t i = 0; i < 3; i++) {
            Game::Media media = game.getMedia(ss_names.at(i));
            if (!media.url.empty()) {
                std::string mediaPath = media.url;
                if (mediaPath.rfind("http", 0) == 0) {
                    mediaPath = "media/" + ss_names.at(i) + "/"
                                + game.path.substr(0, game.path.find_last_of('.') + 1) + media.format;
                }
                elem = Api::addXmlElement(&doc, gameElement, es_names.at(i), mediaPath);
                if (elem) elem->SetAttribute("type", media.type.c_str());
            }
        }
        pGames->InsertEndChild(gameElement);
    }

    tinyxml2::XMLError e = doc.SaveFile(dstPath.c_str());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("GameList::save: %s\n", doc.ErrorName());
        doc.Clear();
        return false;
    }

    doc.Clear();

    return true;
}

GameList GameList::filter(bool available, bool clones, int system, int parent_system,
                          int editor, int developer, int player, int rating, int rotation, int genre,
                          const std::string &resolution, const std::string &date) {
    GameList gl;
    // update gamelist information
    gl.xml = xml;
    gl.romPaths = romPaths;
    gl.editors = editors;
    gl.developers = developers;
    gl.players = players;
    gl.ratings = ratings;
    gl.rotations = rotations;
    gl.resolutions = resolutions;
    gl.dates = dates;
    gl.genres = genres;
    gl.systemList = systemList;

    // update gamelist games
    std::copy_if(games.begin(), games.end(), std::back_inserter(gl.games),
                 [available, clones, system, parent_system, editor, developer, player, rating,
                         rotation, genre, resolution, date](const Game &game) {
                     return (!available || (available && game.available))
                            && (clones || !game.isClone())
                            && (system == -1 || game.system.id == system)
                            && (parent_system == -1 || game.system.parentId == parent_system)
                            && (editor == -1 || game.editor.id == editor)
                            && (developer == -1 || game.developer.id == developer)
                            && (player == -1 || game.playersInt == player)
                            && (rating == -1 || game.rating == rating)
                            && (rotation == -1 || game.rotation == rotation)
                            && (genre == -1 || game.genre.id == genre)
                            && (resolution == "ALL" || game.resolution == resolution
                                || game.resolution.empty() && resolution == "UNKNOWN")
                            && (date == "ALL" || game.date == date);
                 });
    return gl;
}

std::vector<Game> GameList::findGamesByName(const std::string &name) {
    std::vector<Game> matches;

    auto it = std::copy_if(games.begin(), games.end(), std::back_inserter(matches), [name](const Game &game) {
        return game.name == name;
    });

    return matches;
}

std::vector<Game> GameList::findGamesByName(const Game &game) {
    std::vector<Game> matches;

    auto it = std::copy_if(games.begin(), games.end(), std::back_inserter(matches), [game](const Game &g) {
        return game.name == g.name && game.path != g.path;
    });

    return matches;
}

std::vector<Game> GameList::findGamesBySystem(int systemId) {
    std::vector<Game> matches;

    auto it = std::copy_if(games.begin(), games.end(), std::back_inserter(matches), [systemId](const Game &game) {
        return game.system.id == systemId;
    });

    return matches;
}

Game GameList::findGameById(unsigned long id) {
    auto it = std::find_if(games.begin(), games.end(), [id](const Game &game) {
        return game.id == id;
    });

    if (it != games.end()) {
        return *it;
    }

    return {};
}

Game GameList::findGameByPath(const std::string &path) {
    auto it = std::find_if(games.begin(), games.end(), [path](const Game &game) {
        return game.path == path;
    });

    if (it != games.end()) {
        return *it;
    }

    return {};
}

Game GameList::findGameByPathAndSystem(const std::string &path, int systemId) {
    auto it = std::find_if(games.begin(), games.end(), [path, systemId](const Game &game) {
        return game.system.id == systemId && game.path == path;
    });

    if (it != games.end()) {
        return *it;
    }

    return {};
}

Game::Editor GameList::findEditorByName(const std::string &name) {
    auto it = std::find_if(editors.begin(), editors.end(), [name](const Game::Editor &ed) {
        return ed.name == name;
    });

    if (it != editors.end()) {
        return *it;
    }

    return {};
}

std::vector<std::string> GameList::getEditorNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &ed: editors) {
        list.emplace_back(ed.name);
    }
    return list;
}

Game::Developer GameList::findDeveloperByName(const std::string &name) {
    auto it = std::find_if(developers.begin(), developers.end(), [name](const Game::Developer &dev) {
        return dev.name == name;
    });

    if (it != developers.end()) {
        return *it;
    }

    return {};
}

Game::Genre GameList::findGenreByName(const std::string &name) {
    auto it = std::find_if(genres.begin(), genres.end(), [name](const Game::Genre &genre) {
        return genre.name == name;
    });

    if (it != genres.end()) {
        return *it;
    }

    return {};
}

std::vector<std::string> GameList::getDeveloperNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &dev: developers) {
        list.emplace_back(dev.name);
    }
    return list;
}

std::vector<std::string> GameList::getGenreNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &genre: genres) {
        list.emplace_back(genre.name);
    }
    return list;
}

std::vector<std::string> GameList::getRatingNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: ratings) {
        list.emplace_back(std::to_string(i));
    }
    return list;
}

std::vector<std::string> GameList::getRotationNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: rotations) {
        list.emplace_back(std::to_string(i));
    }
    return list;
}

std::vector<std::string> GameList::getPlayersNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: players) {
        list.emplace_back(std::to_string(i));
    }
    return list;
}

std::vector<std::string> GameList::getDates() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: dates) {
        list.emplace_back(i);
    }
    return list;
}

std::vector<std::string> GameList::getResolutions() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: resolutions) {
        list.emplace_back(i);
    }
    return list;
}

bool GameList::exist(unsigned long id) {
    auto it = std::find_if(games.begin(), games.end(), [id](const Game &game) {
        return game.id == id;
    });

    return it != games.end();
}

bool GameList::remove(unsigned long id) {
    auto it = std::find_if(games.begin(), games.end(), [id](const Game &game) {
        return game.id == id;
    });

    if (it != games.end()) {
        games.erase(it);
        return true;
    }

    return false;
}

size_t GameList::getAvailableCount(int systemId) {
    if (systemId < 0) {
        return std::count_if(games.begin(), games.end(), [](const Game &game) {
            return game.available;
        });
    } else {
        return std::count_if(games.begin(), games.end(), [systemId](const Game &game) {
            return game.available && game.system.id == systemId;
        });
    }
}

size_t GameList::getCount(int systemId) {
    if (systemId < 0) return games.size();
    return std::count_if(games.begin(), games.end(), [systemId](const Game &game) {
        return game.system.id == systemId;
    });
}