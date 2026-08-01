//
// Created by cpasjuste on 22/06/18.
//

#ifndef C2D_UTILITY_H
#define C2D_UTILITY_H

#include <string>
#include <vector>
#include <random>
#include <ctime>

namespace c2d {

    class Utility {

    public:
        static int random(int min, int max);

        static float random(float min, float max);

        static int pow2(int i, int min = 64);

        static int parseInt(const std::string &str, int defValue = 0);

        static unsigned int parseHex(const std::string &str, int defValue = 0);

        static float parseFloat(const std::string &str);

        static std::string toString(int value);

        static std::string toLower(const std::string &str);

        static std::string toUpper(const std::string &str);

        static std::vector<std::string> split(const std::string &str, const std::string &token);

        static std::string remove(const std::string &str, const std::string &sub);

        static std::string trim(const std::string &str);

        static bool contains(const std::string &str, const std::string &sub);

        static bool startWith(const std::string &str, const std::string &value, bool sensitive = true);

        static bool endsWith(const std::string &value, const std::string &ending, bool sensitive = true);

        static std::string removeLastSlash(const std::string &string);

        static std::string removeExt(const std::string &str);

        static std::string baseName(const std::string &path);

        static std::string getSizeString(size_t size);
    };
}

#endif //C2D_UTILITY_H
