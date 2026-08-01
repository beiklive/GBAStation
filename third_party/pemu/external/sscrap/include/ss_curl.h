//
// Created by cpasjuste on 29/03/19.
//

#ifndef SS_CURL_H
#define SS_CURL_H

#include <string>
#if !defined(__WINDOWS__) && !defined(fd_set)
#include <sys/select.h>
#endif
#include <curl/curl.h>

namespace ss_api {

    class Curl {

    public:

        Curl();

        ~Curl();

        std::string getString(const std::string &url, int timeout, long *http_code);

        int getData(const std::string &url, const std::string &dstPath, int timeout, long *http_code);

        std::string escape(const std::string &url);

    private:

        CURL *curl = nullptr;
    };

}

#endif //SS_CURL_H
