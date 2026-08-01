//
// Created by cpasjuste on 12/09/2019.
//

#ifndef ARGS_PARSER_H
#define ARGS_PARSER_H

#include <algorithm>
#include "utility.h"

class ArgumentParser {
public:
    ArgumentParser() = default;

    ArgumentParser(int argc, char *argv[]) {
        for (int i = 1; i < argc; ++i) {
            tokens.emplace_back(std::string(argv[i]));
        }
    }

    const std::string &get(const std::string &option) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != tokens.end() && ++itr != tokens.end()) {
            return *itr;
        }
        static const std::string s;
        return s;
    }

    bool exist(const std::string &option) const {
        return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
    }

    std::vector<std::string> tokens;
};

#endif //ARGS_PARSER_H
