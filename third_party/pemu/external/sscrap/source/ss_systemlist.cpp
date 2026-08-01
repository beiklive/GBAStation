//
// Created by cpasjuste on 13/12/2019.
//

#include <algorithm>
#include "ss_api.h"
#include "ss_systemlist.h"

using namespace ss_api;

SystemList::SystemList(const std::string &ssid, const std::string &sspassword, int retryDelay) {
    long code = 0;
    Curl ss_curl;
    std::string soft = ss_curl.escape(Api::ss_softname);
    std::string url = "https://www.screenscraper.fr/api2/systemesListe.php?devid="
                      + Api::ss_devid + "&devpassword=" + Api::ss_devpassword
                      + "&softname=" + soft + "&output=xml";

    url += ssid.empty() ? "" : "&ssid=" + ssid;
    url += sspassword.empty() ? "" : "&sspassword=" + sspassword;

    SS_PRINT("SystemList: %s\n", url.c_str());

    std::string xml = ss_curl.getString(url, SS_TIMEOUT, &code);
    if (retryDelay > 0) {
        while (code == 429 || code == 28) {
            Api::printe((int) code, retryDelay);
            Io::delay(retryDelay);
            xml = ss_curl.getString(url, SS_TIMEOUT, &code);
        }
    }

    if (code != 0 || xml.empty()) {
        SS_PRINT("SystemList: error %li\n", code);
        http_error = (int) code;
        return;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError e = doc.Parse(xml.c_str(), xml.size());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("SystemList: %s\n", doc.ErrorName());
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *pRoot = doc.FirstChildElement("Data");
    if (pRoot == nullptr) {
        SS_PRINT("SystemList: wrong xml format: \'Data\' tag not found\n");
        doc.Clear();
        return;
    }

    tinyxml2::XMLNode *systemNode = pRoot->FirstChildElement("systeme");
    while (systemNode != nullptr) {
        System system;
        parseSystem(&system, systemNode);
        systems.emplace_back(system);
        systemNode = systemNode->NextSibling();
    }

    doc.Clear();
}

bool SystemList::parseSystem(struct ss_api::System *system, tinyxml2::XMLNode *systemNode) {
    if (!system || !systemNode) {
        return false;
    }

    system->id = Api::getXmlTextInt(systemNode->FirstChildElement("id"));
    system->parentId = Api::getXmlTextInt(systemNode->FirstChildElement("parentid"));
    tinyxml2::XMLNode *element = systemNode->FirstChildElement("noms");
    if (element != nullptr) {
        system->name = Api::getXmlTextStr(element->FirstChildElement("nom_eu"));
    }

    return true;
}

System *SystemList::find(const System &system) {
    for (auto &sys: systems) {
        if (sys.name == system.name && sys.id == system.id) return &sys;
    }

    return nullptr;
}

System *SystemList::find(const std::string &name) {
    for (auto &sys: systems) {
        if (sys.name == name) return &sys;
    }

    return nullptr;
}

System *SystemList::find(int id) {
    for (auto &sys: systems) {
        if (sys.id == id) return &sys;
    }

    return nullptr;
}

System SystemList::findById(int id) {
    auto it = std::find_if(systems.begin(), systems.end(), [id](const System &system) {
        return system.id == id;
    });

    if (it != systems.end()) {
        return *it;
    }

    return {};
}

System SystemList::findByName(const std::string &name) {
    auto it = std::find_if(systems.begin(), systems.end(), [name](const System &system) {
        return system.name == name;
    });

    if (it != systems.end()) {
        return *it;
    }

    return {};
}

bool SystemList::remove(int id) {
    auto it = std::remove_if(systems.begin(), systems.end(), [id](const System &sys) { return sys.id == id; });
    if (it != systems.end()) {
        systems.erase(it, systems.end());
        return true;
    }

    return false;
}

std::vector<std::string> SystemList::getNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");

    for (const auto &sys: systems) {
        list.emplace_back(sys.name);
    }

    return list;
}
