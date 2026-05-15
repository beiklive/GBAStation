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

// 本地 version.json 路径（config 目录）
static std::string localVersionJsonPath() {
    return beiklive::path::configPath() + "/version.json";
}

// 缓存目录中的 version.json 路径
static std::string cacheVersionJsonPath() {
    return beiklive::path::cachePath() + "/version.json";
}

// 缓存目录中的 update.nro 路径
static std::string cacheNroPath() {
    return beiklive::path::cachePath() + "/update.nro";
}

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

// ── 读取本地 version.json（config 目录）────
// 返回 version 字段，文件不存在或解析失败返回空字符串
static std::string readLocalVersionFromConfig() {
    std::ifstream f(localVersionJsonPath());
    if (!f.is_open()) return "";
    try {
        nlohmann::json j;
        f >> j;
        return j.value("version", "");
    } catch (...) {
        return "";
    }
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
        // 远程获取失败时，检查本地 config/version.json 是否存在且与当前版本不同
        std::string localVer = readLocalVersionFromConfig();
        if (!localVer.empty() && localVer != localVersion) {
            m_info.version = localVer;
            m_info.hasUpdate = true;
            brls::Logger::info("AppUpdater: 使用本地缓存版本信息 version={}", localVer);
        }
        return m_info.hasUpdate;
    }

    brls::Logger::info("Version Json: {}", json);

    // 移除尾随逗号
    std::string cleanJson = json;
    size_t pos;
    while ((pos = cleanJson.find(",\n}")) != std::string::npos)
        cleanJson.erase(pos, 1);
    while ((pos = cleanJson.find(", }")) != std::string::npos)
        cleanJson.erase(pos, 1);

    try {
        auto j = nlohmann::json::parse(cleanJson);
        m_info.version = j.value("version", "");
        m_info.changelog = j.value("changelog", "");
        m_info.fileSize = j.value("size", size_t(0));
        std::string dl = j.value("download", "");
        m_info.downloadUrl = dl.empty() ? "" : std::string(BASE_URL) + "/" + dl;
    } catch (...) {
        brls::Logger::warning("AppUpdater: 版本 JSON 解析失败");
        return false;
    }

    m_info.hasUpdate = (m_info.version != localVersion);

    // 将远程 version.json 写入 cache 目录，供 download/install 使用
    if (m_info.hasUpdate) {
        std::error_code ec;
        std::filesystem::create_directories(beiklive::path::cachePath(), ec);
        std::ofstream f(cacheVersionJsonPath(), std::ios::trunc);
        if (f) { f << cleanJson; f.close(); }
    }

    brls::Logger::info("AppUpdater: 本地={}, 远程={}, 有更新={}",
        localVersion, m_info.version, m_info.hasUpdate);

    return m_info.hasUpdate;
}

bool AppUpdater::download(std::function<bool(size_t, size_t)> onProgress) {
    if (m_info.downloadUrl.empty()) return false;

    brls::Logger::info("Download Url : {}", m_info.downloadUrl);

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

    // 写入 cache 目录
    std::error_code ec;
    std::filesystem::create_directories(beiklive::path::cachePath(), ec);
    std::ofstream f(cacheNroPath(), std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(m_downloadedData.data()), m_downloadedData.size());
    f.close();

    brls::Logger::info("AppUpdater: 下载完成 {} bytes -> {}", m_downloadedData.size(), cacheNroPath());
    return true;
}

bool AppUpdater::install() {
#ifdef __SWITCH__
    // 1. 先复制 cache/version.json 到 config/version.json
    {
        std::error_code ec;
        if (std::filesystem::exists(cacheVersionJsonPath(), ec)) {
            std::filesystem::copy_file(
                cacheVersionJsonPath(),
                localVersionJsonPath(),
                std::filesystem::copy_options::overwrite_existing,
                ec);
            if (ec) {
                brls::Logger::error("AppUpdater: 复制 version.json 失败");
            }
        }
    }

    // 2. 替换 NRO
    std::string nroPath = "sdmc:/switch/GBAStation.nro";

    romfsExit();

    std::remove(nroPath.c_str());
    if (std::rename(cacheNroPath().c_str(), nroPath.c_str()) != 0) {
        brls::Logger::error("AppUpdater: 安装失败");
        return false;
    }

    brls::Logger::info("AppUpdater: 安装完成 -> {}", nroPath);
    return true;
#else
    // 非 Switch 平台也执行 version.json 替换（方便测试）
    {
        std::error_code ec;
        if (std::filesystem::exists(cacheVersionJsonPath(), ec)) {
            std::filesystem::copy_file(
                cacheVersionJsonPath(),
                localVersionJsonPath(),
                std::filesystem::copy_options::overwrite_existing,
                ec);
        }
    }
    brls::Logger::warning("AppUpdater: NRO 安装仅在 Switch 平台可用");
    return false;
#endif
}

} // namespace beiklive
