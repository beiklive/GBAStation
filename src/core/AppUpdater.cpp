#include "AppUpdater.hpp"
#include "core/Tools.hpp"
#include "core/constexpr.h"
#include <borealis.hpp>
#include <httplib.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include <cstdio>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive {

static const char* BASE_URL = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main";

AppUpdater& AppUpdater::instance() {
    static AppUpdater s;
    return s;
}

static std::string fetchUrl(const std::string& url) {
    httplib::Client cli("https://cdn.jsdelivr.net");
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    cli.set_follow_location(true);

    // 从完整 URL 中提取 path
    std::string path = "/" + url.substr(std::string(BASE_URL).size());
    auto res = cli.Get(path);
    if (res && res->status == 200)
        return res->body;
    return "";
}

void AppUpdater::check(const std::string& localVersion) {
    brls::async([this, localVersion]() {
        checkSync(localVersion);
    });
}

bool AppUpdater::checkSync(const std::string& localVersion) {
    m_info = UpdateInfo{};
    m_info.hasUpdate = false;

    // 构建带时间戳的 URL
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
        std::string dl = j.value("download", "");
        m_info.downloadUrl = dl.empty() ? "" : std::string(BASE_URL) + "/" + dl;
    } catch (...) {
        brls::Logger::warning("AppUpdater: 版本 JSON 解析失败");
        return false;
    }

    // 保存 version.json 到本地
    try {
        std::string localPath = beiklive::path::configPath() + "/version.json";
        std::ofstream f(localPath, std::ios::trunc);
        if (f) { f << json; f.close(); }
    } catch (...) {}

    // 版本比较
    int remoteVer = beiklive::tools::versionCode(m_info.version);
    int localVer  = beiklive::tools::versionCode(localVersion);
    m_info.hasUpdate = remoteVer > localVer;

    brls::Logger::info("AppUpdater: 本地={} ({}), 远程={} ({}), 有更新={}",
        localVersion, localVer, m_info.version, remoteVer, m_info.hasUpdate);

    return m_info.hasUpdate;
}

bool AppUpdater::download(std::function<bool(size_t, size_t)> onProgress) {
    if (m_info.downloadUrl.empty()) return false;

    // 从完整 URL 提取 host 和 path
    std::string host = "cdn.jsdelivr.net";
    std::string path = "/" + m_info.downloadUrl.substr(std::string(BASE_URL).size());

    httplib::Client cli("https://" + host);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(0);  // 下载不限速
    cli.set_follow_location(true);

    m_downloadedData.clear();

    auto res = cli.Get(path,
        [&](const char* data, size_t len) -> bool {
            m_downloadedData.insert(m_downloadedData.end(), data, data + len);
            if (onProgress)
                return onProgress(0, m_downloadedData.size());
            return true;
        },
        [](uint64_t current, uint64_t total) -> bool {
            return true;
        });

    if (!res || res->status != 200) {
        m_downloadedData.clear();
        brls::Logger::error("AppUpdater: 下载失败 status={}", res ? res->status : 0);
        return false;
    }

    // 保存到临时文件
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
    std::string nroPath = "sdmc:/switch/GBAStation.nro"; // 默认 NRO 路径

    // 释放 romfs 挂载，解除对运行中 NRO 文件的读句柄锁定
    romfsExit();

    // 删除旧文件并重命名
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
