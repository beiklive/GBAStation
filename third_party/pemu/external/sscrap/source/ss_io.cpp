//
// Created by cpasjuste on 16/09/2019.
//

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>
#include "ss_api.h"
#include "ss_io.h"
#include "ss_dreamcast.h"

#ifdef __WINDOWS__

#include <windows.h>

#define mkdir(x, y) mkdir(x)
#define stat _stat64 // ?!
#elif __VITA__

#include <psp2/kernel/threadmgr.h>
#include <psp2/io/stat.h>

#define mkdir(x, y) sceIoMkdir(x, 06)
#endif

using namespace ss_api;

static std::string dcGetIpHeaderTitle(const std::string &path) {

    int offset = 0;
    char buffer[128];

    FILE *file = fopen(path.c_str(), "rb");
    if (!file) {
        //SS_PRINT("dcGetIpHeaderTitle: could not open file: \"%s\"\n", path.c_str());
        return "";
    }

    // NO OPT
    if (Io::getExt(path) == "bin") {
        offset = 16;
        fseek(file, 16, SEEK_SET);
    }

    // read header hardware id (our magic)
    size_t read = fread(buffer, 1, 15, file);
    if (read != 15) {
        SS_PRINT("dcGetIpHeaderTitle: could not read file (1): \"%s\"\n", path.c_str());
        fclose(file);
        return "";
    }

    if (strncmp(buffer, "SEGA SEGAKATANA", 15) != 0) {
        SS_PRINT("dcGetIpHeaderTitle: ip.bin header magic not found (SEGA SEGAKATANA) in \"%s\"\n", path.c_str());
        fclose(file);
        return "";
    }

    // read header title
    fseek(file, offset + 128, SEEK_SET);
    read = fread(buffer, 1, 128, file);
    if (read != 128) {
        SS_PRINT("dcGetIpHeaderTitle: could not read file (1): \"%s\"\n", path.c_str());
        fclose(file);
        return "";
    }
    fclose(file);

    // trim..
    for (int i = 126; i > 0; i--) {
        if (!isspace(buffer[i])) {
            buffer[i + 1] = '\0';
            break;
        }
    }

    std::string headerName = std::string(buffer);
    Dreamcast::HeaderFix headerFix = Dreamcast::getHeaderFix(headerName);
    if (!headerFix.searchName.empty()) {
        headerName = headerFix.searchName;
    }

    return headerName;
}

std::vector<Io::File> Io::getDirList(const std::string &path, bool recursive,
                                     const std::vector<std::string> &filters) {
    std::vector<Io::File> files;
    struct stat st{};
    struct dirent *ent;
    DIR *dir;

    if (!path.empty()) {
        if ((dir = opendir(path.c_str())) != nullptr) {
            while ((ent = readdir(dir)) != nullptr) {
                // skip "hidden" files
                if (ent->d_name[0] == '.') {
                    continue;
                }
#if defined(__SWITCH__) || defined(__VITA__) || defined(__PS4__) || defined(__3DS__)
                // stat is too slow on switch
                File file = {ent->d_name, path + "/" + ent->d_name};
                size_t len = file.name.length();
                if (len > 3 && (file.name[len - 4] == '.' || file.name[len - 3] == '.')) {
                    file.isFile = true;
                } else {
                    file.isFile = false;
                }
#else
                File file = {ent->d_name, path + "/" + ent->d_name};
                if (stat(file.path.c_str(), &st) == 0) {
                    file.size = (size_t) st.st_size;
                    file.isFile = S_ISDIR(st.st_mode) ? false : true;
                }
#endif
                if (!file.isFile) {
                    if (recursive) {
                        std::vector<Io::File> subFiles = getDirList(file.path, true, filters);
                        for (const auto &x: subFiles) {
                            files.push_back(x);
                        }
                    }
                    continue;
                }

                // DC, extract title from track0.bin/iso
                if (file.isFile && endsWith(file.name, ".gdi", false)) {
                    file.dc_header_title = dcGetIpHeaderTitle(path + "/track01.iso");
                    file.dc_track01 = path + "/track01.iso";
                    if (file.dc_header_title.empty()) {
                        file.dc_header_title = dcGetIpHeaderTitle(path + "/track01.bin");
                        file.dc_track01 = path + "/track01.bin";
                    }
                    if (!file.dc_header_title.empty()) {
                        // rename disc.gdi / disc_optimized.gdi
                        std::string lowerName = toLower(file.name);
                        if (lowerName == "disc.gdi" || lowerName == "disc_optimized.gdi") {
                            std::string newGdiName = toLower(file.dc_header_title) + ".gdi";
                            std::string newPath = path + "/";
                            newPath += newGdiName;
                            if (!rename(file.path.c_str(), newPath.c_str())) {
                                file.name = newGdiName;
                                file.path = newPath;
                            }
                        }
                    }
                }

                if (!filters.empty()) {
                    for (const auto &filter: filters) {
                        if (file.name.find(filter) != std::string::npos) {
                            files.push_back(file);
                            break;
                        }
                    }
                } else {
                    files.push_back(file);
                }
            }
            closedir(dir);
        }
    }

    return files;
}

void Io::makedir(const std::string &path) {
    mkdir(path.c_str(), 0755);
}

bool Io::exist(const std::string &file) {
    struct stat st{};
    return (stat(file.c_str(), &st) == 0);
}

size_t Io::getSize(const std::string &file) {
    struct stat st{};
    if (stat(file.c_str(), &st) != 0) {
        return 0;
    }
    return (size_t) st.st_size;
}

std::string Io::toLower(const std::string &str) {
    std::string ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ret;
}

std::string Io::toUpper(const std::string &str) {
    std::string ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return ret;
}

bool Io::endsWith(const std::string &value, const std::string &ending, bool sensitive) {
    if (ending.size() > value.size()) return false;
    if (sensitive) {
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    } else {
        std::string val_low = toLower(value);
        std::string end_low = toLower(ending);
        return std::equal(end_low.rbegin(), end_low.rend(), val_low.rbegin());
    }
}

std::string Io::getExt(const std::string &file) {

    char ext[3];

    if (file.length() < 4) {
        return "";
    }

    ext[0] = file[file.size() - 3];
    ext[1] = file[file.size() - 2];
    ext[2] = file[file.size() - 1];

    return ext;
}

void Io::delay(int seconds) {
#ifdef __VITA__
    sceKernelDelayThread(seconds * 1000000);
#else
    sleep(seconds);
#endif
}
