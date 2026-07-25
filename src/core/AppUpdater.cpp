#include "AppUpdater.hpp"
#include "core/Tools.hpp"
#include "core/constexpr.h"
#include <borealis.hpp>
#include <curl/curl.h>
#include <miniz.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cctype>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive {

static const char* VERSION_INI_URL = "https://download.nswiki.cn/hahappify/xlcj/version.ini";
static const char* DOWNLOAD_URL = "https://download.nswiki.cn/hahappify/xlcj/nro/GBAStation.zip";
static const char* CHANGELOG_BASE_URL = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main";

// 缓存目录中的 update.nro 路径
static std::string cacheNroPath() {
    return beiklive::path::cachePath() + "/update.nro";
}

static std::string cacheNdsStubPath() {
    return beiklive::path::cachePath() + "/update_nds_stub.nro";
}

static std::string cache3dsStubPath() {
    return beiklive::path::cachePath() + "/update_3ds_stub.nro";
}

// 缓存目录中的 update.zip 路径
static std::string cacheZipPath() {
    return beiklive::path::cachePath() + "/update.zip";
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

static std::string trimCopy(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
        ++start;

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
        --end;

    return value.substr(start, end - start);
}

static std::string normalizeVersionLabel(const std::string& version) {
    if (!version.empty() && (version[0] == 'v' || version[0] == 'V'))
        return version.substr(1);
    return version;
}

static std::string zipBaseName(const std::string& name) {
    auto pos = name.find_last_of("/\\");
    return pos == std::string::npos ? name : name.substr(pos + 1);
}

static std::string normalizeZipPath(const std::string& name) {
    std::string normalized = name;
    for (char& ch : normalized)
        if (ch == '\\')
            ch = '/';
    return normalized;
}

static bool extractUpdateFilesFromZip(const std::string& zipPath,
                                      const std::string& mainNroPath,
                                      const std::string& ndsStubPath,
                                      const std::string& threeDsStubPath) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zipPath.c_str(), 0))
        return false;

    int mainPreferredIndex = -1;
    int mainFallbackIndex = -1;
    int stubPreferredIndex = -1;
    int stubFallbackIndex = -1;
    int threeDsStubPreferredIndex = -1;
    int threeDsStubFallbackIndex = -1;
    mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < numFiles; ++i) {
        char filename[512];
        mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
        std::string normalized = normalizeZipPath(filename);
        if (normalized == "switch/GBAStation.nro") {
            mainPreferredIndex = static_cast<int>(i);
        } else if (normalized == "GBAStation/core/GBAStationNDSStub.nro") {
            stubPreferredIndex = static_cast<int>(i);
        } else if (normalized == "GBAStation/core/GBAStation3DSStub.nro") {
            threeDsStubPreferredIndex = static_cast<int>(i);
        }
        const std::string baseName = zipBaseName(normalized);
        if (baseName == "GBAStation.nro" && mainFallbackIndex < 0)
            mainFallbackIndex = static_cast<int>(i);
        else if (baseName == "GBAStationNDSStub.nro" && stubFallbackIndex < 0)
            stubFallbackIndex = static_cast<int>(i);
        else if (baseName == "GBAStation3DSStub.nro" && threeDsStubFallbackIndex < 0)
            threeDsStubFallbackIndex = static_cast<int>(i);
    }

    const int mainIndex = mainPreferredIndex >= 0 ? mainPreferredIndex : mainFallbackIndex;
    const int stubIndex = stubPreferredIndex >= 0 ? stubPreferredIndex : stubFallbackIndex;
    const int threeDsStubIndex = threeDsStubPreferredIndex >= 0
        ? threeDsStubPreferredIndex
        : threeDsStubFallbackIndex;
    bool mainOk = false;
    bool stubOk = false;
    bool threeDsStubOk = false;
    if (mainIndex >= 0)
        mainOk = mz_zip_reader_extract_to_file(
            &zip, static_cast<mz_uint>(mainIndex), mainNroPath.c_str(), 0);
    if (stubIndex >= 0)
        stubOk = mz_zip_reader_extract_to_file(
            &zip, static_cast<mz_uint>(stubIndex), ndsStubPath.c_str(), 0);
    if (threeDsStubIndex >= 0)
        threeDsStubOk = mz_zip_reader_extract_to_file(
            &zip, static_cast<mz_uint>(threeDsStubIndex), threeDsStubPath.c_str(), 0);

    mz_zip_reader_end(&zip);
    return mainOk && stubOk && threeDsStubOk;
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

static size_t fetchContentLength(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return 0;

    setCommonOptions(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    size_t fileSize = 0;
    if (res == CURLE_OK && code == 200) {
#ifdef CURLINFO_CONTENT_LENGTH_DOWNLOAD_T
        curl_off_t length = 0;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &length) == CURLE_OK &&
            length > 0) {
            fileSize = static_cast<size_t>(length);
        }
#else
        double length = 0;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length) == CURLE_OK &&
            length > 0) {
            fileSize = static_cast<size_t>(length);
        }
#endif
    }

    curl_easy_cleanup(curl);
    return fileSize;
}

static std::string readVersionFromIni(const std::string& iniText, const std::string& targetKey) {
    std::istringstream stream(iniText);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
            continue;
        if (trimmed.front() == '[' && trimmed.back() == ']')
            continue;

        auto pos = trimmed.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = trimCopy(trimmed.substr(0, pos));
        if (key != targetKey)
            continue;

        return trimCopy(trimmed.substr(pos + 1));
    }

    return "";
}

static bool isRemoteVersionNewer(const std::string& remoteVersion, const std::string& localVersion) {
    try {
        return tools::versionCode(remoteVersion) > tools::versionCode(localVersion);
    } catch (...) {
        return remoteVersion != localVersion;
    }
}

// ── AppUpdater ────────────────────────────────────────────

void AppUpdater::check() {
    brls::async([this]() {
        checkSync();
    });
}

bool AppUpdater::checkSync() {
    m_info = UpdateInfo{};
    m_info.hasUpdate = false;
    m_info.changelog = "检测到新版本后，将更新 GBAStation 主程序与 NDS 运行核心。";
    m_aborted.store(false);

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string url = tools::appendDeviceIdParameter(
        std::string(VERSION_INI_URL) + "?t=" + std::to_string(ts));
    m_info.downloadUrl = tools::appendDeviceIdParameter(
        std::string(DOWNLOAD_URL) + "?t=" + std::to_string(ts));

    std::string iniText = fetchUrl(url);
    if (iniText.empty()) {
        brls::Logger::warning("AppUpdater: 无法获取版本信息 {}", VERSION_INI_URL);
        return false;
    }

    m_info.version = readVersionFromIni(iniText, "GBAStation");
    if (m_info.version.empty()) {
        brls::Logger::warning("AppUpdater: version.ini 中未找到 GBAStation 版本号");
        return false;
    }

    m_info.fileSize = fetchContentLength(m_info.downloadUrl);
    m_info.hasUpdate = isRemoteVersionNewer(m_info.version, APP_VERSION);
    if (m_info.hasUpdate) {
        std::string versionLabel = normalizeVersionLabel(m_info.version);
        std::string changelogUrl = tools::appendDeviceIdParameter(
            std::string(CHANGELOG_BASE_URL) + "/" + versionLabel
            + ".txt?t=" + std::to_string(ts));
        std::string changelogText = fetchUrl(changelogUrl);
        if (!changelogText.empty())
            m_info.changelog = changelogText;
        else
            m_info.changelog = "检测到新版本 " + m_info.version + "，但暂时无法获取更新说明。";
    }

    brls::Logger::info("AppUpdater: 本地={}, 远程={}, 有更新={}",
        APP_VERSION, m_info.version, m_info.hasUpdate);

    return m_info.hasUpdate;
}

bool AppUpdater::download(std::function<bool(size_t, size_t)> onProgress) {
    if (m_info.downloadUrl.empty()) return false;

    brls::Logger::info("Download Url : {}", m_info.downloadUrl);

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    m_downloadedData.clear();
    m_aborted.store(false);

    size_t totalSize = m_info.fileSize;
    ProgressCtx ctx{&onProgress, &m_aborted, totalSize};
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
        if (res == CURLE_ABORTED_BY_CALLBACK || m_aborted.load()) {
            brls::Logger::warning("AppUpdater: 下载被取消");
            return false;
        }
        brls::Logger::error("AppUpdater: 下载失败 code={}", code);
        return false;
    }

    // 写入 cache 目录中的 zip 包，然后解压出 update.nro
    std::error_code ec;
    std::filesystem::create_directories(beiklive::path::cachePath(), ec);
    std::ofstream f(cacheZipPath(), std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(m_downloadedData.data()), m_downloadedData.size());
    f.close();

    if (!extractUpdateFilesFromZip(cacheZipPath(), cacheNroPath(), cacheNdsStubPath(),
                                   cache3dsStubPath())) {
        std::filesystem::remove(cacheZipPath(), ec);
        std::filesystem::remove(cacheNroPath(), ec);
        std::filesystem::remove(cacheNdsStubPath(), ec);
        std::filesystem::remove(cache3dsStubPath(), ec);
        brls::Logger::error(
            "AppUpdater: 更新包解压失败，缺少 GBAStation.nro、GBAStationNDSStub.nro 或 "
            "GBAStation3DSStub.nro");
        return false;
    }

    std::filesystem::remove(cacheZipPath(), ec);
    m_info.fileSize = m_downloadedData.size();

    brls::Logger::info("AppUpdater: 下载并解压完成 {} bytes -> {}, {}, {}",
        m_downloadedData.size(), cacheNroPath(), cacheNdsStubPath(), cache3dsStubPath());
    return true;
}

bool AppUpdater::install() {
#ifdef __SWITCH__
    // 校验缓存 NRO 是否存在
    {
        std::ifstream mainNro(cacheNroPath(), std::ios::binary);
        std::ifstream ndsStub(cacheNdsStubPath(), std::ios::binary);
        std::ifstream threeDsStub(cache3dsStubPath(), std::ios::binary);
        if (!mainNro.good() || !ndsStub.good() || !threeDsStub.good()) {
            brls::Logger::error("AppUpdater: 缓存主程序、NDS Stub 或 3DS Stub 不存在");
            return false;
        }
    }

    brls::Logger::info("AppUpdater: 安装准备工作完成");
    return true;
#else
    brls::Logger::warning("AppUpdater: NRO 安装仅在 Switch 平台可用");
    return false;
#endif
}

bool AppUpdater::finishInstall() {
#ifdef __SWITCH__
    const std::string nroPath = "sdmc:/switch/GBAStation.nro";
    const std::string ndsStubPath = "sdmc:/GBAStation/core/GBAStationNDSStub.nro";
    const std::string threeDsStubPath = "sdmc:/GBAStation/core/GBAStation3DSStub.nro";
    const std::string nroBackupPath = nroPath + ".update_backup";
    const std::string ndsStubBackupPath = ndsStubPath + ".update_backup";
    const std::string threeDsStubBackupPath = threeDsStubPath + ".update_backup";

    romfsExit();

    std::error_code ec;
    std::filesystem::create_directories(
        std::filesystem::path(ndsStubPath).parent_path(), ec);
    if (ec) {
        brls::Logger::error("AppUpdater: 创建核心目录失败: {}", ec.message());
        return false;
    }

    std::remove(nroBackupPath.c_str());
    std::remove(ndsStubBackupPath.c_str());
    std::remove(threeDsStubBackupPath.c_str());

    const bool hadMainNro = std::filesystem::exists(nroPath, ec);
    ec.clear();
    const bool hadNdsStub = std::filesystem::exists(ndsStubPath, ec);
    ec.clear();
    const bool hadThreeDsStub = std::filesystem::exists(threeDsStubPath, ec);

    auto restoreBackup = [](const std::string& target,
                            const std::string& backup,
                            bool hadOriginal) {
        std::remove(target.c_str());
        if (hadOriginal)
            std::rename(backup.c_str(), target.c_str());
    };

    if (hadMainNro && std::rename(nroPath.c_str(), nroBackupPath.c_str()) != 0) {
        brls::Logger::error("AppUpdater: 无法备份现有主程序");
        return false;
    }
    if (hadNdsStub && std::rename(ndsStubPath.c_str(), ndsStubBackupPath.c_str()) != 0) {
        restoreBackup(nroPath, nroBackupPath, hadMainNro);
        brls::Logger::error("AppUpdater: 无法备份现有 NDS Stub");
        return false;
    }
    if (hadThreeDsStub &&
        std::rename(threeDsStubPath.c_str(), threeDsStubBackupPath.c_str()) != 0) {
        restoreBackup(ndsStubPath, ndsStubBackupPath, hadNdsStub);
        restoreBackup(nroPath, nroBackupPath, hadMainNro);
        brls::Logger::error("AppUpdater: 无法备份现有 3DS Stub");
        return false;
    }

    if (std::rename(cacheNdsStubPath().c_str(), ndsStubPath.c_str()) != 0) {
        restoreBackup(threeDsStubPath, threeDsStubBackupPath, hadThreeDsStub);
        restoreBackup(ndsStubPath, ndsStubBackupPath, hadNdsStub);
        restoreBackup(nroPath, nroBackupPath, hadMainNro);
        brls::Logger::error("AppUpdater: NDS Stub 替换失败");
        return false;
    }
    if (std::rename(cache3dsStubPath().c_str(), threeDsStubPath.c_str()) != 0) {
        restoreBackup(threeDsStubPath, threeDsStubBackupPath, hadThreeDsStub);
        restoreBackup(ndsStubPath, ndsStubBackupPath, hadNdsStub);
        restoreBackup(nroPath, nroBackupPath, hadMainNro);
        brls::Logger::error("AppUpdater: 3DS Stub 替换失败");
        return false;
    }
    if (std::rename(cacheNroPath().c_str(), nroPath.c_str()) != 0) {
        restoreBackup(threeDsStubPath, threeDsStubBackupPath, hadThreeDsStub);
        restoreBackup(ndsStubPath, ndsStubBackupPath, hadNdsStub);
        restoreBackup(nroPath, nroBackupPath, hadMainNro);
        brls::Logger::error("AppUpdater: 主程序 NRO 替换失败");
        return false;
    }

    std::remove(nroBackupPath.c_str());
    std::remove(ndsStubBackupPath.c_str());
    std::remove(threeDsStubBackupPath.c_str());
    brls::Logger::info("AppUpdater: 更新文件替换完成 -> {}, {}, {}",
                       nroPath, ndsStubPath, threeDsStubPath);
    return true;
#else
    return false;
#endif
}

} // namespace beiklive
