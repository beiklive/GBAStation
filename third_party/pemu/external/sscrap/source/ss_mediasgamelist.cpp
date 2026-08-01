//
// Created by cpasjuste on 11/12/2019.
//

#include "ss_api.h"

using namespace ss_api;

MediasGameList::MediasGameList(const std::string &ssid, const std::string &sspassword, int retryDelay) {

    long code = 0;
    Curl ss_curl;
    std::string soft = ss_curl.escape(Api::ss_softname);
    std::string url = "https://www.screenscraper.fr/api2/mediasJeuListe.php?devid="
                      + Api::ss_devid + "&devpassword=" + Api::ss_devpassword
                      + "&softname=" + soft + "&output=xml";

    url += ssid.empty() ? "" : "&ssid=" + ssid;
    url += sspassword.empty() ? "" : "&sspassword=" + sspassword;

    SS_PRINT("MediasGameList: %s\n", url.c_str());

    std::string xml = ss_curl.getString(url, SS_TIMEOUT, &code);
    if (retryDelay > 0) {
        while (code == 429 || code == 28) {
            Api::printe((int) code, retryDelay);
            Io::delay(retryDelay);
            xml = ss_curl.getString(url, SS_TIMEOUT, &code);
        }
    }

    if (code != 0 || xml.empty()) {
        SS_PRINT("MediasGameList: error %li\n", code);
        http_error = (int) code;
        return;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError e = doc.Parse(xml.c_str(), xml.size());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("MediasGameList: %s\n", doc.ErrorName());
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *pRoot = doc.FirstChildElement("Data");
    if (pRoot == nullptr) {
        SS_PRINT("MediasGameList: wrong xml format: \'Data\' tag not found\n");
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *mediasNode = pRoot->FirstChildElement("medias");
    if (mediasNode == nullptr) {
        SS_PRINT("MediasGameList: wrong xml format: \'medias\' tag not found\n");
    } else {
        tinyxml2::XMLNode *mediaNode = mediasNode->FirstChildElement("media");
        while (mediaNode != nullptr) {
            // add game to game list
            Media media;
            parseMedia(&media, mediaNode);
            medias.emplace_back(media);
            // move to next node (game)
            mediaNode = mediaNode->NextSibling();
        }
    }

    doc.Clear();
}

bool MediasGameList::parseMedia(Media *media, tinyxml2::XMLNode *mediaNode) {

    if (media == nullptr || mediaNode == nullptr) {
        return false;
    }

    media->id = Api::getXmlTextStr(mediaNode->FirstChildElement("id"));
    media->name = Api::getXmlTextStr(mediaNode->FirstChildElement("nom"));
    media->nameShort = Api::getXmlTextStr(mediaNode->FirstChildElement("nomcourt"));
    media->category = Api::getXmlTextStr(mediaNode->FirstChildElement("categorie"));
    media->platformtypes = Api::getXmlTextStr(mediaNode->FirstChildElement("plateformtypes"));
    media->plateforms = Api::getXmlTextStr(mediaNode->FirstChildElement("plateforms"));
    media->type = Api::getXmlTextStr(mediaNode->FirstChildElement("type"));
    media->fileformat = Api::getXmlTextStr(mediaNode->FirstChildElement("fileformat"));
    media->fileformat2 = Api::getXmlTextStr(mediaNode->FirstChildElement("fileformat2"));
    media->autogen = Api::getXmlTextStr(mediaNode->FirstChildElement("autogen"));
    media->multiregions = Api::getXmlTextStr(mediaNode->FirstChildElement("multiregions"));
    media->multisupports = Api::getXmlTextStr(mediaNode->FirstChildElement("multisupports"));
    media->multiversions = Api::getXmlTextStr(mediaNode->FirstChildElement("multiversions"));
    media->extrainfostxt = Api::getXmlTextStr(mediaNode->FirstChildElement("extrainfostxt"));

    return true;
}
