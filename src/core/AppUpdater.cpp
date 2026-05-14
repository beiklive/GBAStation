#include "AppUpdater.hpp"
#include "core/Tools.hpp"
#include "core/constexpr.h"
#include <borealis.hpp>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include <cstdio>
#include <cstring>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive {

static const char* BASE_URL = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main";

AppUpdater& AppUpdater::instance() {
    static AppUpdater s;
    return s;
}

// ── libcurl 回调 ──────────────────────────────────────────

static size_t writeToString(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* str = static_cast<std::string*>(userdata);
    str->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

static size_t writeToVector(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* vec = static_cast<std::vector<uint8_t>*>(userdata);
    auto bytes = size * nmemb;
    vec->insert(vec->end(), (uint8_t*)ptr, (uint8_t*)ptr + bytes);
    return bytes;
}

struct ProgressCtx {
    std::function<bool(size_t, size_t)>* onProgress;
    std::atomic<bool>* cancelled;
    size_t totalSize;
};

static int progressCallback(void* userdata, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t, curl_off_t) {
    auto* ctx = static_cast<ProgressCtx*>(userdata);
    if (ctx->cancelled && ctx->cancelled->load()) return 1;
    if (ctx->onProgress && *ctx->onProgress) {
        size_t total = ctx->totalSize ? ctx->totalSize : static_cast<size_t>(dltotal);
        return (*ctx->onProgress)(total, static_cast<size_t>(dlnow)) ? 0 : 1;
    }
    return 0;
}

static void setCommonOptions(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "GBAStation-Updater");
}

static std::string fetchUrl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string body;
    setCommonOptions(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK || code != 200) return "";
    return body;
}

// ── AppUpdater ────────────────────────────────────────────

void AppUpdater::check(const std::string& localVersion) {
    brls::async([this, localVersion]() {
        checkSync(localVersion);
    });
}

bool AppUpdater::checkSync(const std::string& localVersion) {
    m_info = UpdateInfo{};
    m_info.hasUpdate = false;

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string url = std::string(BASE_URL) + "/version.json?t=" + std::to_string(ts);

    std::string json = fetchUrl(url);
    if (json.empty()) {
        brls::Logger::warning("AppUpdater: 无法获取版本信息");
        return false;
    }

    try {
        auto j = nlohmann::json::parse(json);
        m_info.version = j.value("version", "");
        m_info.changelog = j.value("changelog", "");
        m_info.fileSize = j.value("size", size_t(0));
        std::string dl = j.value("download", "");
        m_info.downloadUrl = dl.empty() ? "" : std::string(BASE_URL) + "/" + dl;
    } catch (...) {
        brls::Logger::warning("AppUpdater: 版本 JSON 解析失败");
        return false;
    }

    try {
        std::string localPath = beiklive::path::configPath() + "/version.json";
        std::ofstream f(localPath, std::ios::trunc);
        if (f) { f << json; f.close(); }
    } catch (...) {}

    m_info.hasUpdate = (m_info.version != localVersion);

    brls::Logger::info("AppUpdater: 本地={}, 远程={}, 有更新={}",
        localVersion, m_info.version, m_info.hasUpdate);

    return m_info.hasUpdate;
}

bool AppUpdater::download(std::function<bool(size_t, size_t)> onProgress) {
    if (m_info.downloadUrl.empty()) return false;

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    m_downloadedData.clear();

    size_t totalSize = m_info.fileSize;
    ProgressCtx ctx{&onProgress, nullptr, totalSize};
    setCommonOptions(curl);
    curl_easy_setopt(curl, CURLOPT_URL, m_info.downloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToVector);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &m_downloadedData);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK || code != 200) {
        m_downloadedData.clear();
        brls::Logger::error("AppUpdater: 下载失败 code={}", code);
        return false;
    }

    std::string tmpPath = beiklive::path::configPath() + "/update.nro";
    std::ofstream f(tmpPath, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(m_downloadedData.data()), m_downloadedData.size());
    f.close();

    brls::Logger::info("AppUpdater: 下载完成 {} bytes", m_downloadedData.size());
    return true;
}

bool AppUpdater::install() {
#ifdef __SWITCH__
    std::string tmpPath = beiklive::path::configPath() + "/update.nro";
    std::string nroPath = "sdmc:/switch/GBAStation.nro";

    romfsExit();

    std::remove(nroPath.c_str());
    if (std::rename(tmpPath.c_str(), nroPath.c_str()) != 0) {
        std::remove(tmpPath.c_str());
        brls::Logger::error("AppUpdater: 安装失败");
        return false;
    }

    brls::Logger::info("AppUpdater: 安装完成 -> {}", nroPath);
    return true;
#else
    brls::Logger::warning("AppUpdater: 安装仅在 Switch 平台可用");
    return false;
#endif
}

} // namespace beiklive
