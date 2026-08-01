//
// Created by cpasjuste on 11/12/2019.
//

#include "ss_api.h"

using namespace ss_api;

ss_api::GameInfo::GameInfo(const std::string &crc, const std::string &md5, const std::string &sha1,
                           const std::string &systemeid, const std::string &romtype, const std::string &romnom,
                           const std::string &romtaille, const std::string &gameid, const std::string &ssid,
                           const std::string &sspassword, int retryDelay) {

    long code = 0;
    Curl ss_curl;
    std::string search = ss_curl.escape(romnom);
    std::string soft = ss_curl.escape(Api::ss_softname);
    std::string url = "https://www.screenscraper.fr/api2/jeuInfos.php?devid="
                      + Api::ss_devid + "&devpassword=" + Api::ss_devpassword + "&softname=" + soft + "&output=xml";

    url += ssid.empty() ? "" : "&ssid=" + ssid;
    url += sspassword.empty() ? "" : "&sspassword=" + sspassword;
    url += crc.empty() ? "" : "&crc=" + crc;
    url += md5.empty() ? "" : "&md5=" + md5;
    url += sha1.empty() ? "" : "&sha1=" + sha1;
    url += systemeid.empty() ? "" : "&systemeid=" + systemeid;
    url += romtype.empty() ? "" : "&romtype=" + romtype;
    url += romtaille.empty() ? "" : "&romtaille=" + romtaille;
    url += gameid.empty() ? "" : "&gameid=" + gameid;
    url += search.empty() ? "" : "&romnom=" + search;

    SS_PRINT("GameInfo: %s\n", url.c_str());

    std::string xml = ss_curl.getString(url, SS_TIMEOUT, &code);
    if (retryDelay > 0) {
        while (code == 429 || code == 28) {
            Api::printe((int) code, retryDelay);
            Io::delay(retryDelay);
            xml = ss_curl.getString(url, SS_TIMEOUT, &code);
        }
    }

    if (code != 0 || xml.empty()) {
        SS_PRINT("GameInfo: error %li\n", code);
        http_error = (int) code;
        return;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError e = doc.Parse(xml.c_str(), xml.size());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("GameInfo: %s\n", doc.ErrorName());
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *pRoot = doc.FirstChildElement("Data");
    if (pRoot == nullptr) {
        SS_PRINT("GameInfo: wrong xml format: \'Data\' tag not found\n");
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *gameNode = pRoot->FirstChildElement("jeu");
    if (gameNode == nullptr) {
        SS_PRINT("GameInfo: wrong xml format: \'jeu\' tag not found\n");
    } else {
        Game::parseGame(&game, gameNode, romnom, GameList::Format::ScreenScraper);
    }

    doc.Clear();
}
