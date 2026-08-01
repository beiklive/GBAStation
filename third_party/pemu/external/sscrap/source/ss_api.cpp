//
// Created by cpasjuste on 05/04/19.
//

#include <cstdarg>
#include <zlib.h>
#include "ss_api.h"

using namespace ss_api;

std::string Api::ss_devid;
std::string Api::ss_devpassword;
std::string Api::ss_softname;
bool ss_debug = false;

std::string
Api::getXmlAttrStr(tinyxml2::XMLElement *element, const std::string &name, const std::string &defaultValue) {
    if (element == nullptr || element->Attribute(name.c_str()) == nullptr) {
        return defaultValue;
    }

    return element->Attribute(name.c_str());
}

int Api::getXmlAttrInt(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseInt(attr);
}

long Api::getXmlAttrLong(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseLong(attr);
}

unsigned long Api::getXmlAttrULong(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseULong(attr);
}

bool Api::getXmlAttrBool(tinyxml2::XMLElement *element, const std::string &name) {
    std::string attr = getXmlAttrStr(element, name);
    return parseBool(attr);
}

std::string Api::getXmlTextStr(tinyxml2::XMLElement *element, const std::string &defaultValue) {
    if (element == nullptr || element->GetText() == nullptr) {
        return defaultValue;
    }

    return element->GetText();
}

int Api::getXmlTextInt(tinyxml2::XMLElement *element) {
    std::string text = getXmlTextStr(element);
    return parseInt(text);
}

float Api::getXmlTextFloat(tinyxml2::XMLElement *element) {
    std::string text = getXmlTextStr(element);
    return parseFloat(text);
}

bool Api::getXmlTextBool(tinyxml2::XMLElement *element) {
    std::string text = getXmlTextStr(element);
    return parseBool(text);
}

tinyxml2::XMLElement *Api::addXmlElement(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *parent,
                                         const std::string &name, const std::string &value) {
    if (parent == nullptr) {
        return nullptr;
    }

    if (!value.empty()) {
        tinyxml2::XMLElement *element = doc->NewElement(name.c_str());
        element->SetText(value.c_str());
        parent->InsertEndChild(element);
        return element;
    }

    return nullptr;
}

bool Api::sortByName(const std::string &g1, const std::string &g2) {
#ifdef _MSC_VER
    return _stricmp(g1.c_str(), g2.c_str()) < 0;
#else
    return strcasecmp(g1.c_str(), g2.c_str()) < 0;
#endif
}

bool Api::sortInteger(int i1, int i2) {
    return i1 < i2;
}

bool Api::sortGameByName(const Game &g1, const Game &g2) {
    const std::string lhs = g1.name;
    const std::string rhs = g2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortGameByPath(const Game &g1, const Game &g2) {
    const std::string lhs = g1.path;
    const std::string rhs = g2.path;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortSystemByName(const System &s1, const System &s2) {
    const std::string lhs = s1.name;
    const std::string rhs = s2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortEditorByName(const Game::Editor &e1, const Game::Editor &e2) {
    const std::string lhs = e1.name;
    const std::string rhs = e2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortDeveloperByName(const Game::Developer &d1, const Game::Developer &d2) {
    const std::string lhs = d1.name;
    const std::string rhs = d2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

bool Api::sortGenreByName(const Game::Genre &g1, const Game::Genre &g2) {
    const std::string lhs = g1.name;
    const std::string rhs = g2.name;
    const auto result = mismatch(
            lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), [](const auto &lhs, const auto &rhs) {
                return tolower(lhs) == tolower(rhs);
            });
    return result.second != rhs.cend() &&
           (result.first == lhs.cend() || tolower(*result.first) < tolower(*result.second));
}

int Api::parseInt(const std::string &str, int defValue) {
    char *end = nullptr;
    long i = strtol(str.c_str(), &end, 10);
    if (end != nullptr && end != str.c_str()) {
        return (int) i;
    }
    return defValue;
}

long Api::parseLong(const std::string &str, long defValue) {
    char *end = nullptr;
    long i = strtol(str.c_str(), &end, 10);
    if (end != nullptr && end != str.c_str()) {
        return i;
    }
    return defValue;
}

unsigned long Api::parseULong(const std::string &str, unsigned long defValue) {
    char *end = nullptr;
    unsigned long i = strtoll(str.c_str(), &end, 10);
    if (end != nullptr && end != str.c_str()) {
        return i;
    }
    return defValue;
}

float Api::parseFloat(const std::string &str, float defValue) {
    char *end = nullptr;
    float f = strtof(str.c_str(), &end);
    if (end != nullptr && end != str.c_str()) {
        return f;
    }
    return defValue;
}

bool Api::parseBool(const std::string &str, bool defValue) {
    if (str.empty() || str != "true" || str != "false" || str != "0" || str != "1") {
        return defValue;
    }

    return str == "true" || str == "1";
}

std::string Api::getFileCrc(const std::string &zipPath) {
    unsigned char buffer[BUFSIZ];
    char hex[16];
    size_t size;
    FILE *pFile;

    memset(hex, 0, 16);

#ifdef _MSC_VER
    fopen_s(&pFile, zipPath.c_str(), "rb");
#else
    pFile = fopen(zipPath.c_str(), "rb");
#endif
    if (!pFile) {
        return hex;
    }

    uLong crc = crc32(0L, Z_NULL, 0);
    while ((size = fread(buffer, 1, BUFSIZ, pFile)) != 0) {
        crc = crc32(crc, buffer, size);
    }
    snprintf(hex, 16, "%08lx", crc);

    fclose(pFile);

    return hex;
}

#ifdef __WINDOWS__

void Api::printc(int color, const char *format, ...) {
#else

void Api::printc(const char *color, const char *format, ...) {
#endif

    char buffer[1024];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buffer, 1024, format, arg);
    va_end(arg);

#ifdef __WINDOWS__
    WORD consoleAttr = FOREGROUND_INTENSITY | FOREGROUND_RED
                       | FOREGROUND_GREEN | FOREGROUND_BLUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        consoleAttr = csbi.wAttributes;
    }
    SetConsoleTextAttribute(h, color);
    printf("%s", buffer);
    SetConsoleTextAttribute(h, consoleAttr);
#else
    printf("%s%s" "\033[0m", color, buffer);
#endif
}

void Api::printe(int code, int delay) {
    if (code == 429) {
        printc(COLOR_O, "NOK: maximum requests per minute reached... retrying in %i seconds\n", delay);
    } else if (code == 28) {
        printc(COLOR_O, "NOK: timeout reached... retrying in %i seconds\n", delay);
    }
}
