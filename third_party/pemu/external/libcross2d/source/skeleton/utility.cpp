//
// Created by cpasjuste on 22/06/18.
//

#include <algorithm>
#include "cross2d/c2d.h"

using namespace c2d;

typedef std::mt19937 RandomEngine;
static RandomEngine randomEngine = RandomEngine(static_cast<unsigned long>(std::time(nullptr)));

int Utility::random(int min, int max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(randomEngine);
}

float Utility::random(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(randomEngine);
}

int Utility::pow2(int i, int min) {
    if (i == 0) return 0;
    if ((i & (i - 1)) == 0) return i;
    int n = 2;
    while (i > n) n <<= 1;
    return n > min ? n : min;
}

std::string Utility::toLower(const std::string &str) {
    std::string ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ret;
}

std::string Utility::toUpper(const std::string &str) {
    std::string ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return ret;
}

std::vector<std::string> Utility::split(const std::string &str, const std::string &token) {

    std::vector<std::string> result;
    std::string s = str;

    while (!s.empty()) {
        size_t index = s.find(token);
        if (index != std::string::npos) {
            result.push_back(s.substr(0, index));
            s = s.substr(index + token.size());
            if (s.empty()) {
                result.push_back(s);
            }
        } else {
            result.push_back(s);
            s = "";
        }
    }

    return result;
}

bool Utility::startWith(const std::string &str, const std::string &value, bool sensitive) {
    if (sensitive) {
        return str.compare(0, value.length(), value) == 0;
    } else {
        std::string str_low = toLower(str);
        std::string val_low = toLower(value);
        return str_low.compare(0, val_low.length(), val_low) == 0;
    }
}

bool Utility::endsWith(const std::string &value, const std::string &ending, bool sensitive) {
    if (ending.size() > value.size()) return false;
    if (sensitive) {
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    } else {
        std::string val_low = toLower(value);
        std::string end_low = toLower(ending);
        return std::equal(end_low.rbegin(), end_low.rend(), val_low.rbegin());
    }
}

std::string Utility::remove(const std::string &str, const std::string &sub) {
    std::string ret = str;
    size_t pos;
    while ((pos = ret.find(sub)) != std::string::npos) {
        ret.erase(pos, sub.length());
    }
    return ret;
}

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::string Utility::trim(const std::string &str) {
    std::string ret = str;
    ltrim(ret);
    rtrim(ret);
    return ret;
}

bool Utility::contains(const std::string &str, const std::string &sub) {
    return str.find(sub) != std::string::npos;
}

std::string Utility::removeLastSlash(const std::string &string) {

    std::string str = string;
    size_t pos = str.find_last_of('/');
    if (pos == str.length() - 1) {
        str.erase(str.length() - 1);
    }

    return str;
}

std::string Utility::removeExt(const std::string &str) {
    size_t pos = str.find_last_of('.');
    if (pos != std::string::npos) {
        return str.substr(0, pos);
    } else {
        return str;
    }
}

int Utility::parseInt(const std::string &str, int defValue) {
    char *end = nullptr;
    long i = strtol(str.c_str(), &end, 10);
    if (end && end != str.c_str()) {
        return (int) i;
    }
    return defValue;
}

unsigned int Utility::parseHex(const std::string &str, int defValue) {
    size_t pos;
    auto i = std::stoul(str, &pos, 16);
    if (pos) {
        return (int) i;
    }
    return defValue;
}

float Utility::parseFloat(const std::string &str) {
    char *end = nullptr;
    double d = strtod(str.c_str(), &end);
    if (end && end != str.c_str()) {
        return (float) d;
    }
    return 0;
}

std::string Utility::toString(int value) {
    // dreamcast doesn't have std::to_string
    char str[16];
    snprintf(str, 16, "%i", value);
    return str;
}

std::string Utility::baseName(const std::string &path) {
    std::string name = path;
    if (path.size() > 1) {
        const size_t idx = path.find_last_of('/');
        if (idx != std::string::npos) {
            name.erase(0, idx + 1);
        }
    }
    return name;
}

std::string Utility::getSizeString(size_t size) {
    char output[200];
    const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
    char length = sizeof(suffix) / sizeof(suffix[0]);
    auto dblBytes = (double) size;
    int i = 0;

    if (size <= 0) {
        return "0 B";
    } else {
        if (size > 1024) {
            for (i = 0; (size / 1024) > 0 && i < length - 1; i++, size /= 1024)
                dblBytes = (double) size / 1024.0;
        }

        sprintf(output, "%.02lf %s", dblBytes, suffix[i]);
        return output;
    }
}
