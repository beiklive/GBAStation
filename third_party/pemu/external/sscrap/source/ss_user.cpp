//
// Created by cpasjuste on 11/12/2019.
//

#include "ss_api.h"

using namespace ss_api;

User::User(const std::string &ssid, const std::string &sspassword, int retryDelay) {

    long code = 0;
    Curl ss_curl;
    std::string soft = ss_curl.escape(Api::ss_softname);
    std::string url = "https://www.screenscraper.fr/api2/ssuserInfos.php?devid="
                      + Api::ss_devid + "&devpassword=" + Api::ss_devpassword
                      + "&softname=" + soft + "&output=xml";

    url += ssid.empty() ? "" : "&ssid=" + ssid;
    url += sspassword.empty() ? "" : "&sspassword=" + sspassword;

    SS_PRINT("User: %s\n", url.c_str());

    std::string xml = ss_curl.getString(url, SS_TIMEOUT, &code);
    if (retryDelay > 0) {
        while (code == 429 || code == 28) {
            Api::printe((int) code, retryDelay);
            Io::delay(retryDelay);
            xml = ss_curl.getString(url, SS_TIMEOUT, &code);
        }
    }

    if (code != 0 || xml.empty()) {
        SS_PRINT("User: error %li\n", code);
        http_error = (int) code;
        return;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError e = doc.Parse(xml.c_str(), xml.size());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("User: %s\n", doc.ErrorName());
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *pRoot = doc.FirstChildElement("Data");
    if (pRoot == nullptr) {
        SS_PRINT("User: wrong xml format: \'Data\' tag not found\n");
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *userNode = pRoot->FirstChildElement("ssuser");
    if (userNode == nullptr) {
        SS_PRINT("User: wrong xml format: \'ssuser\' tag not found\n");
    } else {
        parseUser(this, userNode);
    }

    doc.Clear();
}

bool User::parseUser(User *user, tinyxml2::XMLNode *userNode) {

    if (user == nullptr || userNode == nullptr) {
        return false;
    }

    user->id = Api::getXmlTextStr(userNode->FirstChildElement("id"));
    user->niveau = Api::getXmlTextStr(userNode->FirstChildElement("niveau"));
    user->contribution = Api::getXmlTextStr(userNode->FirstChildElement("contribution"));
    user->uploadsysteme = Api::getXmlTextStr(userNode->FirstChildElement("uploadsysteme"));
    user->uploadinfos = Api::getXmlTextStr(userNode->FirstChildElement("uploadinfos"));
    user->romasso = Api::getXmlTextStr(userNode->FirstChildElement("romasso"));
    user->uploadmedia = Api::getXmlTextStr(userNode->FirstChildElement("uploadmedia"));
    user->maxthreads = Api::getXmlTextStr(userNode->FirstChildElement("maxthreads"));
    user->maxdownloadspeed = Api::getXmlTextStr(userNode->FirstChildElement("maxdownloadspeed"));
    user->requeststoday = Api::getXmlTextStr(userNode->FirstChildElement("requeststoday"));
    user->maxrequestsperday = Api::getXmlTextStr(userNode->FirstChildElement("maxrequestsperday"));
    user->visites = Api::getXmlTextStr(userNode->FirstChildElement("visites"));
    user->datedernierevisite = Api::getXmlTextStr(userNode->FirstChildElement("datedernierevisite"));
    user->favregion = Api::getXmlTextStr(userNode->FirstChildElement("favregion"));

    return true;
}

int User::getMaxThreads() {
    return Api::parseInt(maxthreads, 1);
}
