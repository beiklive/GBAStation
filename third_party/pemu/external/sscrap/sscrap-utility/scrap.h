//
// Created by cpasjuste on 21/11/2019.
//

#ifndef SSCRAP_SCRAP_H
#define SSCRAP_SCRAP_H

#include <pthread.h>

#ifndef _MSC_VER

#include <semaphore.h>

#endif

#include "args.h"

class Scrap {

public:

    struct MissFile {
        MissFile(const std::string &n, const std::string &p,
                 const std::string &z, const std::string &r) {
            name = n;
            path = p;
            zipCrc = z;
            romCrc = r;
        }

        std::string name;
        std::string path;
        std::string zipCrc;
        std::string romCrc;
    };

    explicit Scrap(const ArgumentParser &parser);

    ss_api::Game scrapGame(int tid, int tryCount, int sid, int remainingFiles, const std::string &fileName,
                           const std::string &filePath, const std::string &searchName);

    void run();

    void parseSid(int sid);

    bool isFbnClone(const ss_api::Io::File &file);

    ss_api::Game *getGameByParent(const ss_api::Io::File &file);

    ArgumentParser args;
    std::string usr;
    std::string pwd;
    std::string romPath;
    ss_api::MediasGameList mediasGameList;
    ss_api::SystemList systemList;
    ss_api::GameList gameList;
    ss_api::GameList fbnGameList;
    std::vector<ss_api::Io::File> filesList;
    std::vector<ss_api::Io::File> cloneList;
    std::vector<MissFile> missList;
    ss_api::System system;
    int sscrapSystemId = 0;
    bool isFbNeoSid = false;
    ss_api::User user;
    int filesCount = 0;
    pthread_t threads[15]{};
    pthread_mutex_t mutex;
};

#endif //SSCRAP_SCRAP_H
