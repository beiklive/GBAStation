//
// Created by cpasjuste on 03/12/2019.
//

#ifndef SSCRAP_SS_GAMELIST_H
#define SSCRAP_SS_GAMELIST_H

#include <string>
#include <functional>
#include "ss_systemlist.h"

namespace ss_api {

    class GameList {
    public:

        enum Format {
            ScreenScraper,
            EmulationStation,
            FbNeoDat,
            Unknown
        };

        typedef std::function<void(Game *)> GameAddedCb;

        GameList() = default;

        bool append(const std::string &xmlPath, const std::string &romPath = "", bool sort = true,
                    const std::vector<std::string> &filters = {".zip"}, const System &system = {0, 0, "UNKNOWN"},
                    bool availableOnly = false, const GameAddedCb &cb = nullptr);

        void sortAlpha(bool byZipName = false, bool gamesOnly = true);

        GameList filter(bool available = false, bool clones = false,
                        int system = -1, int parent_system = -1, int editor = -1, int developer = -1,
                        int players = -1, int rating = -1, int rotation = -1, int genre = -1,
                        const std::string &resolution = "ALL", const std::string &date = "ALL");

        bool save(const std::string &dstPath, const std::string &imageType,
                  const std::string &thumbnailType, const std::string &videoType);

        std::vector<Game> findGamesByName(const std::string &name);

        std::vector<Game> findGamesByName(const Game &game);

        std::vector<Game> findGamesBySystem(int systemId);

        Game findGameById(unsigned long romId);

        Game findGameByPath(const std::string &path);

        Game findGameByPathAndSystem(const std::string &path, int systemId);

        Game::Editor findEditorByName(const std::string &name);

        std::vector<std::string> getEditorNames();

        Game::Developer findDeveloperByName(const std::string &name);

        Game::Genre findGenreByName(const std::string &name);

        std::vector<std::string> getDeveloperNames();

        std::vector<std::string> getGenreNames();

        std::vector<std::string> getRatingNames();

        std::vector<std::string> getPlayersNames();

        std::vector<std::string> getRotationNames();

        std::vector<std::string> getDates();

        std::vector<std::string> getResolutions();

        bool exist(unsigned long romId);

        bool remove(unsigned long romId);

        size_t getAvailableCount(int systemId = -1);

        size_t getCount(int systemId);

        std::string xml;
        Format format = EmulationStation;
        SystemList systemList;
        std::vector<std::string> romPaths;
        std::vector<Game> games;
        std::vector<Game::Editor> editors;
        std::vector<Game::Developer> developers;
        std::vector<Game::Genre> genres;
        std::vector<int> players;
        std::vector<int> ratings;
        std::vector<int> rotations;
        std::vector<std::string> resolutions;
        std::vector<std::string> dates;
    };
}

#endif //SSCRAP_SS_GAMELIST_H
