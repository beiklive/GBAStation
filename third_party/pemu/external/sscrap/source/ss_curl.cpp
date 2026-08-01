//
// Created by cpasjuste on 29/03/19.
//

#include <curl/curl.h>
#include "ss_api.h"
#include "ss_curl.h"

using namespace ss_api;

static size_t write_string_cb(void *buf, size_t len, size_t count, void *stream) {
    ((std::string *) stream)->append((char *) buf, 0, len * count);
    return len * count;
}

static size_t write_data_cb(void *buf, size_t len, size_t count, void *stream) {
    size_t written = fwrite(buf, len, count, (FILE *) stream);
    return written;
}

Curl::Curl() {
    curl = curl_easy_init();
}

Curl::~Curl() {
    if (curl != nullptr) {
        curl_easy_cleanup(curl);
    }
}

std::string Curl::getString(const std::string &url, int timeout, long *http_code) {

    std::string data;

    int res = 0;

    if (curl == nullptr) {
        SS_PRINT("Curl::getString: error: curl_easy_init failed\n");
        return data;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_cb);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

    res = curl_easy_perform(curl);
    if (http_code != nullptr) {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, http_code);
        if ((*http_code) == 200) {
            (*http_code) = 0;
        }
    }

    if (res != 0 && http_code != nullptr && (*http_code) == 0) {
        *http_code = res;
        SS_PRINT("Curl::getString: error: curl_easy_perform failed: %s, http_code: %li\n",
                 curl_easy_strerror((CURLcode) res), http_code ? *http_code : 0);
        return "";
    }

    return data;
}

int Curl::getData(const std::string &url, const std::string &dstPath, int timeout, long *http_code) {

    FILE *data;
    int res = 0;

    if (curl == nullptr) {
        SS_PRINT("Curl::getData: error: curl_easy_init failed\n");
        return -1;
    }

#ifdef _MSC_VER
    fopen_s(&data, dstPath.c_str(), "wb");
#else
    data = fopen(dstPath.c_str(), "wb");
#endif
    if (data == nullptr) {
        SS_PRINT("Curl::getData: error: fopen failed: %s\n", dstPath.c_str());
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_cb);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

    res = curl_easy_perform(curl);
    fclose(data);
    if (http_code != nullptr) {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, http_code);
        if ((*http_code) == 200) {
            (*http_code) = 0;
        }
    }

    if (res != 0 && http_code != nullptr && (*http_code) == 0) {
        *http_code = res;
        SS_PRINT("Curl::getData: error: curl_easy_perform failed: %s, http_code: %li\n",
                 curl_easy_strerror((CURLcode) res), http_code ? *http_code : 0);
        remove(dstPath.c_str());
        return res;
    }

    return 0;
}

std::string Curl::escape(const std::string &url) {

    std::string escaped = url;

    if (curl != nullptr) {
        char *ret = curl_easy_escape(curl, url.c_str(), (int) url.length());
        if (ret != nullptr) {
            escaped = ret;
            free(ret);
        }
    }

    return escaped;
}
