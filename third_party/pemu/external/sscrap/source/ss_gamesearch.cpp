//
// Created by cpasjuste on 11/12/2019.
//

#include "ss_api.h"

using namespace ss_api;

GameSearch::GameSearch(const std::string &recherche, const std::string &systemeid,
                       const std::string &ssid, const std::string &sspassword, int retryDelay) {

    long code = 0;
    Curl ss_curl;
    std::string search = ss_curl.escape(recherche);
    std::string soft = ss_curl.escape(Api::ss_softname);
    std::string url = "https://www.screenscraper.fr/api2/jeuRecherche.php?devid="
                      + Api::ss_devid + "&devpassword=" + Api::ss_devpassword
                      + "&softname=" + soft + "&output=xml" + "&recherche=" + search;

    url += ssid.empty() ? "" : "&ssid=" + ssid;
    url += sspassword.empty() ? "" : "&sspassword=" + sspassword;
    url += systemeid.empty() ? "" : "&systemeid=" + systemeid;

    SS_PRINT("GameSearch: %s\n", url.c_str());

    std::string xml = ss_curl.getString(url, SS_TIMEOUT, &code);
    if (retryDelay > 0) {
        while (code == 429 || code == 28) {
            Api::printe((int) code, retryDelay);
            Io::delay(retryDelay);
            xml = ss_curl.getString(url, SS_TIMEOUT, &code);
        }
    }

    if (code != 0 || xml.empty()) {
        SS_PRINT("GameSearch: error %li\n", code);
        http_error = (int) code;
        return;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError e = doc.Parse(xml.c_str(), xml.size());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("GameSearch: %s\n", doc.ErrorName());
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *pRoot = doc.FirstChildElement("Data");
    if (pRoot == nullptr) {
        SS_PRINT("GameSearch: wrong xml format: \'Data\' tag not found\n");
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *userNode = pRoot->FirstChildElement("ssuser");
    if (userNode == nullptr) {
        SS_PRINT("GameSearch: wrong xml format: \'ssuser\' tag not found\n");
    } else {
        User::parseUser(&user, userNode);
    }

    tinyxml2::XMLNode *gamesNode = pRoot->FirstChildElement("jeux");
    if (gamesNode == nullptr) {
        SS_PRINT("GameSearch: wrong xml format: \'jeux\' tag not found\n");
    } else {
        tinyxml2::XMLNode *gameNode = gamesNode->FirstChildElement("jeu");
        while (gameNode != nullptr) {
            // add game to game list
            Game game;
            Game::parseGame(&game, gameNode, "", GameList::Format::ScreenScraper);
            games.emplace_back(game);
            // move to next node (game)
            gameNode = gameNode->NextSibling();
        }
    }

    doc.Clear();
}
