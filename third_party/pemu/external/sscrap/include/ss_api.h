//
// Created by cpasjuste on 05/04/19.
//

#ifndef SS_API_H
#define SS_API_H

#include <tinyxml2.h>

#include "ss_curl.h"
#include "ss_io.h"
#include "ss_game.h"
#include "ss_user.h"
#include "ss_gameinfo.h"
#include "ss_gamesearch.h"
#include "ss_gamelist.h"
#include "ss_mediasgamelist.h"
#include "ss_systemlist.h"

#define SS_TIMEOUT 30

#ifdef __WINDOWS__
#define COLOR_R (FOREGROUND_INTENSITY | FOREGROUND_RED)
#define COLOR_G (FOREGROUND_INTENSITY | FOREGROUND_GREEN)
#define COLOR_Y (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
#define COLOR_O (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
#else
#define COLOR_R "\x1B[91m"
#define COLOR_G "\x1B[92m"
#define COLOR_Y "\x1B[93m"
#define COLOR_O "\x1B[33m"
#endif

namespace ss_api {

    class Api {

    public:

        static int parseInt(const std::string &str, int defValue = 0);

        static long parseLong(const std::string &str, long defValue = 0);

        static unsigned long parseULong(const std::string &str, unsigned long defValue = 0);

        static float parseFloat(const std::string &str, float defValue = 0);

        static bool parseBool(const std::string &str, bool defValue = false);

        // internal
        static bool sortInteger(int i1, int i2);

        static bool sortByName(const std::string &g1, const std::string &g2);

        static bool sortGameByName(const Game &g1, const Game &g2);

        static bool sortGameByPath(const Game &g1, const Game &g2);

        static bool sortSystemByName(const System &s1, const System &s2);

        static bool sortEditorByName(const Game::Editor &e1, const Game::Editor &e2);

        static bool sortDeveloperByName(const Game::Developer &d1, const Game::Developer &d2);

        static bool sortGenreByName(const Game::Genre &g1, const Game::Genre &g2);

        // get attributes
        static std::string getXmlAttrStr(tinyxml2::XMLElement *element, const std::string &name, const std::string &defaultValue = "");

        static int getXmlAttrInt(tinyxml2::XMLElement *element, const std::string &name);

        static long getXmlAttrLong(tinyxml2::XMLElement *element, const std::string &name);

        static unsigned long getXmlAttrULong(tinyxml2::XMLElement *element, const std::string &name);

        static bool getXmlAttrBool(tinyxml2::XMLElement *element, const std::string &name);

        // get text
        static std::string getXmlTextStr(tinyxml2::XMLElement *element, const std::string &defaultValue = "");

        static int getXmlTextInt(tinyxml2::XMLElement *element);

        static float getXmlTextFloat(tinyxml2::XMLElement *element);

        static bool getXmlTextBool(tinyxml2::XMLElement *element);

        // add
        static tinyxml2::XMLElement *addXmlElement(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *parent,
                                                   const std::string &name, const std::string &value);

        static std::string getFileCrc(const std::string &path);

#ifdef __WINDOWS__
        static void printc(int color, const char* format, ...);
#else
        static void printc(const char *color, const char *format, ...);
#endif

        static void printe(int code, int delay);

        static std::string ss_devid;
        static std::string ss_devpassword;
        static std::string ss_softname;
    };
}

// ss_debug print
extern bool ss_debug;
#define SS_PRINT(f_, ...) \
do \
{ \
    if(ss_debug) \
        printf((f_), ##__VA_ARGS__); \
} \
while(0)

#endif //SS_API_H
