//
// Created by cpasjuste on 16/09/2019.
//

#include <windows.h>
#include <filesystem>
#include "ss_io.h"

namespace fs = std::filesystem;
using namespace ss_api;

std::vector<std::string> Io::getDirList(const std::string &path, const std::string &ext) {

    std::vector<std::string> files;

    if (!fs::exists(path) || !fs::is_directory(path)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(path)) {

        if (!entry.is_regular_file()) {
            continue;
        }

        fs::path file = entry.path().filename();
        if (!ext.empty()) {
            if (file.extension().string() == "." + ext) {
                files.emplace_back(file.string());
            }
        }
        else {
            files.emplace_back(file.string());
        }
    }

    return files;
}

void Io::makedir(const std::string &path) {
    fs::create_directories(path);
}

bool Io::exist(const std::string &file) {
    return fs::exists(file);
}

void Io::delay(int seconds) {
    Sleep(seconds * 1000);
}
