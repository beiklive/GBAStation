//
// Created by cpasjuste on 10/12/2019.
//

#include <cstring>
#include <algorithm>

#ifndef __VITA__

#include <minizip/unzip.h>

#endif

#include <ss_api.h>
#include <dirent.h>
#include "utility.h"

using namespace ss_api;

static hashwrapper *md5Wrapper;
static hashwrapper *sha1Wrapper;

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
    if (end != nullptr) {
        return (int) i;
    }
    return defValue;
}

std::string Utility::getExt(const std::string &file) {

    char ext[3];

    if (file.length() < 4) {
        return "";
    }

    ext[0] = file[file.size() - 3];
    ext[1] = file[file.size() - 2];
    ext[2] = file[file.size() - 1];

    return ext;
}

std::string Utility::getRomCrc(const std::string &zipPath, std::vector<std::string> whiteList) {

    char *zipFileName, *data, buffer[16];
    memset(buffer, 0, 16);

    if (!Io::endsWith(zipPath, ".zip", false)) {
        return "";
    }

#ifndef __VITA__
    unzFile zip = unzOpen(zipPath.c_str());
    if (zip == nullptr) {
        SS_PRINT("could not open zip file for crc check (%s)\n", zipPath.c_str());
        return buffer;
    }

    if (unzGoToFirstFile(zip) == UNZ_OK) {
        do {
            if (unzOpenCurrentFile(zip) == UNZ_OK) {
                unz_file_info fileInfo;
                memset(&fileInfo, 0, sizeof(unz_file_info));
                if (unzGetCurrentFileInfo(zip, &fileInfo, nullptr, 0,
                                          nullptr, 0, nullptr, 0) == UNZ_OK) {
                    zipFileName = (char *) malloc((uLong) fileInfo.size_filename + 1);
                    unzGetCurrentFileInfo(zip, &fileInfo, zipFileName, fileInfo.size_filename + 1,
                                          nullptr, 0, nullptr, 0);
                    zipFileName[(uLong) fileInfo.size_filename] = '\0';
                    std::string ext = getExt(std::string(zipFileName));
                    if (whiteList.empty() || std::find(whiteList.begin(), whiteList.end(), ext) != whiteList.end()) {
                        data = (char *) malloc(fileInfo.uncompressed_size);
                        unzReadCurrentFile(zip, data, (unsigned int) fileInfo.uncompressed_size);
                        uLong crc = crc32(0L, (const Bytef *) data, fileInfo.uncompressed_size) & 0xffffffff;
                        free(data);
                        free(zipFileName);
                        unzClose(zip);
                        snprintf(buffer, 16, "%08lx", crc);
                        return buffer;
                    }
                    free(zipFileName);
                }
                unzCloseCurrentFile(zip);
            }
        } while (unzGoToNextFile(zip) == UNZ_OK);
    }

    unzClose(zip);
#endif
    return buffer;
}

Utility::ZipInfo Utility::getZipInfo(const std::string &path, const std::string &file) {

    ZipInfo info;
    info.name = file;
    std::string fullPath = path + "/" + file;

    if (!Io::exist(fullPath)) {
        return info;
    }

    if (!md5Wrapper) {
        md5Wrapper = new md5wrapper();
        sha1Wrapper = new sha1wrapper();
    }

    info.name = file;
    info.size = std::to_string(Io::getSize(fullPath));
    info.crc = Api::getFileCrc(fullPath);
    info.md5 = md5Wrapper->getHashFromFile(fullPath);
    info.sha1 = sha1Wrapper->getHashFromFile(fullPath);

    return info;
}

std::string Utility::getZipInfoStr(const std::string &path, const std::string &file) {

    if (!md5Wrapper) {
        md5Wrapper = new md5wrapper();
        sha1Wrapper = new sha1wrapper();
    }

    ZipInfo info = getZipInfo(path, file);
    return info.name + "|" + info.size + "|" + info.serial + "|" + info.crc + "|" + info.md5 + "|" + info.sha1;
}
