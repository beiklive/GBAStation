#include "core/SteamGridDb.hpp"

#include "core/Tools.hpp"
#include "core/constexpr.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <borealis/core/logger.hpp>

#include <borealis/extern/nanovg/stb_image.h>
#include "third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <sstream>
#include <unordered_set>

namespace beiklive::steamgriddb
{
namespace
{
    namespace fs = std::filesystem;
    constexpr const char* kApiBase = "https://www.steamgriddb.com/api/v2";

    struct HttpResponse
    {
        CURLcode curlCode = CURLE_OK;
        long status = 0;
        std::string body;
        bool fromCache = false;
    };

    std::once_flag g_curlInit;
    std::shared_mutex g_cacheAccessMutex;

    bool removeCacheTree(const fs::path& path, std::string& failedPath,
                         std::error_code& error)
    {
        error.clear();
        std::error_code statusError;
        const bool directory = fs::is_directory(path, statusError);
        if (statusError) {
            failedPath = path.string();
            error = statusError;
            return false;
        }

        if (directory) {
            std::vector<fs::path> children;
            {
                fs::directory_iterator iterator(path, error);
                if (error) {
                    failedPath = path.string();
                    return false;
                }
                const fs::directory_iterator end;
                while (iterator != end) {
                    children.push_back(iterator->path());
                    iterator.increment(error);
                    if (error) {
                        failedPath = path.string();
                        return false;
                    }
                }
            }
            for (const auto& child : children) {
                if (!removeCacheTree(child, failedPath, error)) return false;
            }
        }

        const bool removed = fs::remove(path, error);
        if (error) {
            failedPath = path.string();
            return false;
        }
        if (!removed) {
            std::error_code existsError;
            if (fs::exists(path, existsError)) {
                failedPath = path.string();
                error = std::make_error_code(std::errc::io_error);
                return false;
            }
        }
        return true;
    }

    void ensureCurl()
    {
        std::call_once(g_curlInit, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
    }

    std::string trim(std::string value)
    {
        auto whitespace = [](unsigned char c) { return std::isspace(c) != 0; };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
            [&](unsigned char c) { return !whitespace(c); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
            [&](unsigned char c) { return !whitespace(c); }).base(), value.end());
        return value;
    }

    std::string readText(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) return {};
        std::ostringstream out;
        out << file.rdbuf();
        return out.str();
    }

    bool writeBytes(const fs::path& path, const std::string& data,
                    std::string* error)
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec) {
            if (error) *error = ec.message();
            return false;
        }
        const fs::path temp = path.string() + ".tmp";
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file) {
            if (error) *error = "无法创建文件";
            return false;
        }
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        file.close();
        if (!file) {
            if (error) *error = "写入文件失败";
            return false;
        }
        fs::remove(path, ec);
        ec.clear();
        fs::rename(temp, path, ec);
        if (ec) {
            fs::remove(temp, ec);
            if (error) *error = ec.message();
            return false;
        }
        return true;
    }

    size_t writeString(void* data, size_t size, size_t count, void* user)
    {
        auto* output = static_cast<std::string*>(user);
        const size_t bytes = size * count;
        output->append(static_cast<const char*>(data), bytes);
        return bytes;
    }

    bool requestActive(const std::atomic<bool>* active)
    {
        return !active || active->load(std::memory_order_relaxed);
    }

    int cancelTransfer(void* user, curl_off_t, curl_off_t,
                       curl_off_t, curl_off_t)
    {
        const auto* active = static_cast<const std::atomic<bool>*>(user);
        return requestActive(active) ? 0 : 1;
    }

    std::uint64_t fnv1a(const std::string& text)
    {
        std::uint64_t hash = 1469598103934665603ULL;
        for (unsigned char c : text) {
            hash ^= c;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    std::string hashName(const std::string& text)
    {
        std::ostringstream out;
        out << std::hex << std::setw(16) << std::setfill('0') << fnv1a(text);
        return out.str();
    }

    std::string encoded(const std::string& text)
    {
        ensureCurl();
        CURL* curl = curl_easy_init();
        if (!curl) return text;
        char* escaped = curl_easy_escape(curl, text.c_str(),
                                         static_cast<int>(text.size()));
        std::string result = escaped ? escaped : text;
        if (escaped) curl_free(escaped);
        curl_easy_cleanup(curl);
        return result;
    }

    fs::path jsonCachePath(const std::string& url)
    {
        return fs::path(cacheDirectory()) / "api" / (hashName(url) + ".json");
    }

    void removeJsonCache(const std::string& url)
    {
        std::shared_lock<std::shared_mutex> cacheAccess(g_cacheAccessMutex);
        std::error_code ec;
        fs::remove(jsonCachePath(url), ec);
    }

    HttpResponse httpGet(const std::string& url, const std::string& apiKey,
                         bool useCache, bool writeCache,
                         const std::atomic<bool>* active = nullptr)
    {
        std::shared_lock<std::shared_mutex> cacheAccess(
            g_cacheAccessMutex, std::defer_lock);
        if (useCache || writeCache) cacheAccess.lock();
        HttpResponse response;
        if (!requestActive(active)) {
            response.curlCode = CURLE_ABORTED_BY_CALLBACK;
            return response;
        }
        const fs::path cachePath = jsonCachePath(url);
        if (useCache) {
            response.body = readText(cachePath);
            if (!response.body.empty()) {
                response.status = 200;
                response.fromCache = true;
                brls::Logger::info(
                    "[SteamGridDB] cache hit: {} ({} bytes)",
                    url, response.body.size());
                return response;
            }
        }

        ensureCurl();
        CURL* curl = curl_easy_init();
        if (!curl) {
            response.curlCode = CURLE_FAILED_INIT;
            return response;
        }
        struct curl_slist* headers = nullptr;
        if (!apiKey.empty()) {
            const std::string authorization = "Authorization: Bearer " + apiKey;
            headers = curl_slist_append(headers, authorization.c_str());
        }
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 45L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "GBAStation-SteamGridDB/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        if (active) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelTransfer);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, active);
        }
        brls::Logger::info("[SteamGridDB] GET {}", url);
        response.curlCode = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (response.curlCode == CURLE_OK && response.status >= 200 &&
            response.status < 300 && writeCache && !response.body.empty()) {
            std::string ignored;
            writeBytes(cachePath, response.body, &ignored);
        }
        brls::Logger::info(
            "[SteamGridDB] response: HTTP {}, curl={}, {} bytes",
            response.status, static_cast<int>(response.curlCode),
            response.body.size());
        return response;
    }

    template <typename T>
    Result<T> responseError(const HttpResponse& response)
    {
        Result<T> result;
        result.networkError = response.curlCode != CURLE_OK || response.status == 0;
        if (response.status == 401 || response.status == 403)
            result.error = "SteamGridDB API Key 无效";
        else if (result.networkError)
            result.error = "网络连接异常";
        else
            result.error = "SteamGridDB 请求失败（HTTP " +
                std::to_string(response.status) + "）";
        return result;
    }

    std::string jsonString(const nlohmann::json& value, const char* key)
    {
        auto it = value.find(key);
        return it != value.end() && it->is_string() ? it->get<std::string>() : "";
    }

    bool jsonBool(const nlohmann::json& value, const char* key)
    {
        auto it = value.find(key);
        return it != value.end() && it->is_boolean() && it->get<bool>();
    }

    int jsonInt(const nlohmann::json& value, const char* key)
    {
        auto it = value.find(key);
        return it != value.end() && it->is_number_integer() ? it->get<int>() : 0;
    }

    float assetScore(const Asset& asset)
    {
        float score = static_cast<float>(asset.width) * asset.height / 100000.f;
        const float ratio = asset.height > 0
            ? static_cast<float>(asset.width) / asset.height : 0.f;
        if (asset.type == AssetType::Grids) {
            if (std::abs(ratio - 2.f / 3.f) < 0.10f) score += 30.f;
            if (asset.style == "official") score += 50.f;
            else if (asset.style == "alternate") score += 25.f;
        } else if (asset.type == AssetType::Heroes) {
            if (ratio > 2.5f) score += 30.f;
            if (asset.width >= 1920) score += 30.f;
        } else if (asset.type == AssetType::Logos) {
            if (asset.mime == "image/png") score += 20.f;
            score += static_cast<float>(asset.width) / 30.f;
        }
        if (asset.language == "zh" || asset.language == "ja") score += 20.f;
        else if (asset.language == "en") score += 10.f;
        if (asset.humor) score -= 100.f;
        return score;
    }

    const char* endpoint(AssetType type)
    {
        switch (type) {
            case AssetType::Grids: return "grids";
            case AssetType::Heroes: return "heroes";
            case AssetType::Logos: return "logos";
            case AssetType::Icons: return "icons";
            default: return "grids";
        }
    }

    std::string extensionFromUrl(const std::string& url, const std::string& mime)
    {
        const size_t query = url.find('?');
        const std::string clean = url.substr(0, query);
        std::string ext = fs::path(clean).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp")
            return ext;
        return mime.find("jpeg") != std::string::npos ? ".jpg" : ".png";
    }

    fs::path assetCachePath(const Asset& asset, bool thumbnail)
    {
        const std::string& url = thumbnail && !asset.thumbnailUrl.empty()
            ? asset.thumbnailUrl : asset.url;
        const std::string name = asset.id != 0
            ? std::to_string(asset.id) : hashName(url);
        return fs::path(cacheDirectory()) / "images" / assetTypeName(asset.type) /
            (name + (thumbnail ? "_thumb" : "") +
             extensionFromUrl(url, asset.mime));
    }

    bool downloadFile(const std::string& url, const fs::path& destination,
                      std::string* error, const std::atomic<bool>* active)
    {
        if (!requestActive(active)) {
            if (error) *error = "下载已取消";
            return false;
        }
        const std::string existing = readText(destination);
        if (!existing.empty()) return true;
        const HttpResponse response = httpGet(
            url, "", false, false, active);
        if (response.curlCode != CURLE_OK || response.status < 200 ||
            response.status >= 300 || response.body.empty()) {
            if (response.curlCode == CURLE_ABORTED_BY_CALLBACK) {
                if (error) *error = "下载已取消";
                brls::Logger::info(
                    "[SteamGridDB] image download cancelled: {}", url);
                return false;
            }
            if (error) *error = response.curlCode != CURLE_OK
                ? "图片下载网络异常"
                : "图片下载失败（HTTP " + std::to_string(response.status) + "）";
            return false;
        }
        if (!requestActive(active)) {
            if (error) *error = "下载已取消";
            return false;
        }
        return writeBytes(destination, response.body, error);
    }

    std::vector<unsigned char> resizeBilinear(const unsigned char* source,
                                              int sourceWidth, int sourceHeight,
                                              int targetWidth, int targetHeight)
    {
        std::vector<unsigned char> output(
            static_cast<size_t>(targetWidth) * targetHeight * 4);
        for (int y = 0; y < targetHeight; ++y) {
            const float sourceY = (static_cast<float>(y) + 0.5f) * sourceHeight /
                targetHeight - 0.5f;
            const int y0 = std::clamp(static_cast<int>(std::floor(sourceY)), 0,
                                      sourceHeight - 1);
            const int y1 = std::min(sourceHeight - 1, y0 + 1);
            const float fy = std::clamp(sourceY - std::floor(sourceY), 0.f, 1.f);
            for (int x = 0; x < targetWidth; ++x) {
                const float sourceX = (static_cast<float>(x) + 0.5f) * sourceWidth /
                    targetWidth - 0.5f;
                const int x0 = std::clamp(static_cast<int>(std::floor(sourceX)), 0,
                                          sourceWidth - 1);
                const int x1 = std::min(sourceWidth - 1, x0 + 1);
                const float fx = std::clamp(sourceX - std::floor(sourceX), 0.f, 1.f);
                for (int channel = 0; channel < 4; ++channel) {
                    const float a = source[(y0 * sourceWidth + x0) * 4 + channel] * (1.f - fx) +
                        source[(y0 * sourceWidth + x1) * 4 + channel] * fx;
                    const float b = source[(y1 * sourceWidth + x0) * 4 + channel] * (1.f - fx) +
                        source[(y1 * sourceWidth + x1) * 4 + channel] * fx;
                    output[(y * targetWidth + x) * 4 + channel] =
                        static_cast<unsigned char>(std::clamp(a * (1.f - fy) + b * fy,
                                                              0.f, 255.f));
                }
            }
        }
        return output;
    }
}

std::string rootDirectory()
{
    return beiklive::path::ROOT + beiklive::path::SPLIT_CHAR +
        beiklive::path::PROGRAM_NAME + beiklive::path::SPLIT_CHAR +
        "SteamGirdDB";
}

std::string apiKeyPath()
{
    return (fs::path(rootDirectory()) / "api" / "SteamGridDB_api_key.ini").string();
}

std::string cacheDirectory()
{
    return (fs::path(rootDirectory()) / "cache").string();
}

std::string loadApiKey()
{
    std::string value = trim(readText(apiKeyPath()));
    const size_t equals = value.find('=');
    if (equals != std::string::npos)
        value = trim(value.substr(equals + 1));
    return value;
}

bool hasApiKey()
{
    return !loadApiKey().empty();
}

bool saveApiKey(const std::string& key, std::string* error)
{
    const std::string clean = trim(key);
    if (clean.empty()) {
        if (error) *error = "API Key 不能为空";
        return false;
    }
    return writeBytes(apiKeyPath(), clean + "\n", error);
}

Result<bool> validateApiKey(const std::string& key)
{
    const std::string clean = trim(key);
    if (clean.empty()) {
        Result<bool> result;
        result.error = "API Key 不能为空";
        return result;
    }
    const HttpResponse response = httpGet(
        std::string(kApiBase) + "/search/autocomplete/mario", clean,
        false, false);
    if (response.curlCode != CURLE_OK || response.status < 200 ||
        response.status >= 300)
        return responseError<bool>(response);
    try {
        const auto json = nlohmann::json::parse(response.body);
        Result<bool> result;
        result.ok = json.value("success", false);
        result.value = result.ok;
        if (!result.ok) result.error = "SteamGridDB API Key 无效";
        return result;
    } catch (...) {
        Result<bool> result;
        result.error = "SteamGridDB 返回了无法解析的数据";
        return result;
    }
}

bool clearCache(std::string* error)
{
    // Wait until all thumbnail/API cache jobs finish before touching the
    // directory tree. Multiple downloads may hold this lock concurrently;
    // clearing takes it exclusively.
    std::unique_lock<std::shared_mutex> cacheAccess(g_cacheAccessMutex);
    if (error) error->clear();
    const fs::path root(cacheDirectory());
    std::error_code ec;
    fs::create_directories(root, ec);
    if (ec) {
        if (error) *error = ec.message();
        brls::Logger::error(
            "[SteamGridDB] cannot prepare cache directory {}: {}",
            root.string(), ec.message());
        return false;
    }

    // Read all root entries first so the FAT directory iterator is closed
    // before deleting anything below it.
    std::vector<fs::path> entries;
    {
        fs::directory_iterator iterator(root, ec);
        if (ec) {
            if (error) *error = "无法读取缓存目录：" + ec.message();
            brls::Logger::error(
                "[SteamGridDB] cannot enumerate cache directory {}: {}",
                root.string(), ec.message());
            return false;
        }
        const fs::directory_iterator end;
        while (iterator != end) {
            entries.push_back(iterator->path());
            iterator.increment(ec);
            if (ec) {
                if (error) *error = "读取缓存项目失败：" + ec.message();
                return false;
            }
        }
    }

    for (const auto& entry : entries) {
        std::string failedPath;
        std::error_code removeError;
        if (!removeCacheTree(entry, failedPath, removeError)) {
            if (error) {
                *error = "无法删除 " + failedPath + "：" +
                    removeError.message();
            }
            brls::Logger::error(
                "[SteamGridDB] cannot remove cache path {}: {}",
                failedPath, removeError.message());
            return false;
        }
    }
    brls::Logger::info("[SteamGridDB] cache cleared: {}", root.string());
    return true;
}

Result<std::vector<SearchGame>> searchGames(
    const std::vector<std::string>& terms, const std::atomic<bool>* active)
{
    Result<std::vector<SearchGame>> result;
    if (!requestActive(active)) {
        result.error = "搜索已取消";
        return result;
    }
    const std::string key = loadApiKey();
    if (key.empty()) {
        result.error = "请先在设置中输入 SteamGridDB API Key";
        return result;
    }
    brls::Logger::info("[SteamGridDB] game search started: {} terms",
                       terms.size());
    std::unordered_set<std::int64_t> seen;
    for (const std::string& rawTerm : terms) {
        if (!requestActive(active)) {
            result.error = "搜索已取消";
            return result;
        }
        const std::string term = trim(rawTerm);
        if (term.empty()) continue;
        brls::Logger::info("[SteamGridDB] searching game term: {}", term);
        const std::string url = std::string(kApiBase) +
            "/search/autocomplete/" + encoded(term);
        const HttpResponse response = httpGet(
            url, key, true, true, active);
        if (response.curlCode != CURLE_OK || response.status < 200 ||
            response.status >= 300) {
            if (result.value.empty()) return responseError<std::vector<SearchGame>>(response);
            continue;
        }
        try {
            const auto json = nlohmann::json::parse(response.body);
            if (!json.value("success", false) || !json.contains("data") ||
                !json["data"].is_array())
                continue;
            for (const auto& item : json["data"]) {
                const std::int64_t id = item.value("id", std::int64_t(0));
                if (id == 0 || !seen.insert(id).second) continue;
                result.value.push_back({id, jsonString(item, "name"),
                                        jsonBool(item, "verified")});
            }
        } catch (const std::exception& exception) {
            brls::Logger::error(
                "[SteamGridDB] game search JSON parse failed: {}",
                exception.what());
        }
    }
    std::stable_sort(result.value.begin(), result.value.end(),
        [](const SearchGame& a, const SearchGame& b) {
            return a.verified && !b.verified;
        });
    result.ok = !result.value.empty();
    if (!result.ok) result.error = "没有找到匹配的游戏";
    brls::Logger::info("[SteamGridDB] game search finished: {} matches",
                       result.value.size());
    return result;
}

Result<AssetGroups> fetchAllAssets(
    const std::vector<SearchGame>& games, int page,
    const std::atomic<bool>* active)
{
    Result<AssetGroups> result;
    if (!requestActive(active)) {
        result.error = "素材读取已取消";
        return result;
    }
    const std::string key = loadApiKey();
    if (key.empty()) {
        result.error = "请先在设置中输入 SteamGridDB API Key";
        return result;
    }
    // Autocomplete can place obscure entries with no artwork before the useful
    // match. Probe more candidates, but stop each category as soon as enough
    // entries have been collected for several UI batches.
    const size_t gameCount = std::min<size_t>(games.size(), 10);
    bool receivedAnyResponse = false;
    size_t totalAssets = 0;
    brls::Logger::info(
        "[SteamGridDB] asset search started: {} candidate games, page {}",
        gameCount, page);
    for (int typeIndex = 0; typeIndex < static_cast<int>(AssetType::Count); ++typeIndex) {
        if (!requestActive(active)) {
            result.error = "素材读取已取消";
            return result;
        }
        const AssetType type = static_cast<AssetType>(typeIndex);
        std::unordered_set<std::string> seen;
        auto& output = result.value[static_cast<size_t>(type)];
        for (size_t gameIndex = 0; gameIndex < gameCount; ++gameIndex) {
            if (!requestActive(active)) {
                result.error = "素材读取已取消";
                return result;
            }
            if (output.size() >= 30) break;
            const std::string url = std::string(kApiBase) + "/" + endpoint(type) +
                "/game/" + std::to_string(games[gameIndex].id) +
                "?page=" + std::to_string(std::max(0, page));
            HttpResponse response = httpGet(
                url, key, true, true, active);
            if (response.curlCode != CURLE_OK || response.status < 200 ||
                response.status >= 300) {
                brls::Logger::warning(
                    "[SteamGridDB] {} request failed for game {}: HTTP {}, curl={}",
                    assetTypeName(type), games[gameIndex].id, response.status,
                    static_cast<int>(response.curlCode));
                continue;
            }
            receivedAnyResponse = true;
            try {
                const auto json = nlohmann::json::parse(response.body);
                if (!json.value("success", false) || !json.contains("data") ||
                    !json["data"].is_array()) {
                    brls::Logger::warning(
                        "[SteamGridDB] {} response has no data array for game {}",
                        assetTypeName(type), games[gameIndex].id);
                    continue;
                }
                brls::Logger::info(
                    "[SteamGridDB] {} game {} response: page={}, total={}, data={}",
                    assetTypeName(type), games[gameIndex].id,
                    json.value("page", page), json.value("total", 0),
                    json["data"].size());
                if (json["data"].empty()) {
                    // Empty lists change as artwork is added. Do not keep an
                    // empty response indefinitely and hide future results.
                    removeJsonCache(url);
                }
                const size_t before = output.size();
                for (const auto& item : json["data"]) {
                    Asset asset;
                    asset.id = item.value("id", std::int64_t(0));
                    asset.type = type;
                    asset.url = jsonString(item, "url");
                    asset.thumbnailUrl = jsonString(item, "thumb");
                    asset.width = jsonInt(item, "width");
                    asset.height = jsonInt(item, "height");
                    asset.style = jsonString(item, "style");
                    asset.mime = jsonString(item, "mime");
                    asset.language = jsonString(item, "language");
                    asset.nsfw = jsonBool(item, "nsfw");
                    asset.humor = jsonBool(item, "humor");
                    if (asset.url.empty() || asset.nsfw ||
                        !seen.insert(asset.url).second)
                        continue;
                    asset.score = assetScore(asset);
                    const fs::path cached = assetCachePath(asset, true);
                    std::error_code ec;
                    if (fs::exists(cached, ec) && fs::is_regular_file(cached, ec))
                        asset.localPath = cached.string();
                    output.push_back(std::move(asset));
                }
                brls::Logger::info(
                    "[SteamGridDB] {} game {}: {} usable assets",
                    assetTypeName(type), games[gameIndex].id,
                    output.size() - before);
            } catch (const std::exception& exception) {
                brls::Logger::error(
                    "[SteamGridDB] {} JSON parse failed for game {}: {}",
                    assetTypeName(type), games[gameIndex].id,
                    exception.what());
            }
        }
        std::stable_sort(output.begin(), output.end(),
            [](const Asset& a, const Asset& b) { return a.score > b.score; });
        totalAssets += output.size();
        brls::Logger::info("[SteamGridDB] {} total: {}",
                           assetTypeName(type), output.size());
    }
    result.ok = receivedAnyResponse && totalAssets > 0;
    if (!result.ok) {
        result.networkError = !receivedAnyResponse;
        result.error = receivedAnyResponse
            ? "找到游戏，但 SteamGridDB 没有返回可用图片"
            : "素材列表获取失败，请检查网络或 API Key";
    }
    brls::Logger::info(
        "[SteamGridDB] asset search finished: responses={}, total={}",
        receivedAnyResponse, totalAssets);
    return result;
}

std::vector<Asset> applyFilters(const std::vector<Asset>& source,
                                const Filters& filters)
{
    std::vector<Asset> output;
    for (const auto& asset : source) {
        if (asset.nsfw) continue;
        if (!filters.allowHumor && asset.humor) continue;
        if (filters.width > 0 && asset.width != filters.width) continue;
        if (filters.height > 0 && asset.height != filters.height) continue;
        if (!filters.style.empty() && asset.style != filters.style) continue;
        if (!filters.mime.empty() && asset.mime != filters.mime) continue;
        if (!filters.language.empty() && asset.language != filters.language) continue;
        output.push_back(asset);
    }
    return output;
}

bool ensureAssetCached(Asset& asset, bool thumbnail, std::string* error,
                       const std::atomic<bool>* active)
{
    std::shared_lock<std::shared_mutex> cacheAccess(g_cacheAccessMutex);
    if (!requestActive(active)) {
        if (error) *error = "下载已取消";
        return false;
    }
    const fs::path path = assetCachePath(asset, thumbnail);
    std::error_code ec;
    if (fs::exists(path, ec) && fs::is_regular_file(path, ec)) {
        asset.localPath = path.string();
        brls::Logger::info("[SteamGridDB] image cache hit: {}", path.string());
        return true;
    }
    const std::string& url = thumbnail && !asset.thumbnailUrl.empty()
        ? asset.thumbnailUrl : asset.url;
    if (!downloadFile(url, path, error, active)) {
        brls::Logger::warning(
            "[SteamGridDB] image download failed: {} ({})", url,
            error && !error->empty() ? *error : "unknown error");
        return false;
    }
    asset.localPath = path.string();
    brls::Logger::info("[SteamGridDB] image cached: {}", path.string());
    return true;
}

bool saveAssetAsCover(const Asset& sourceAsset, const GameEntry& entry,
                      std::string& outputPath, std::string* error)
{
    Asset asset = sourceAsset;
    if (!ensureAssetCached(asset, false, error)) return false;
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(asset.localPath.c_str(), &width, &height,
                                      &channels, 4);
    if (!pixels || width <= 0 || height <= 0) {
        if (pixels) stbi_image_free(pixels);
        if (error) *error = "图片解码失败";
        return false;
    }
    const float scale = std::min(1.f, 512.f /
        static_cast<float>(std::max(width, height)));
    const int targetWidth = std::max(1, static_cast<int>(std::round(width * scale)));
    const int targetHeight = std::max(1, static_cast<int>(std::round(height * scale)));
    std::vector<unsigned char> resized;
    const unsigned char* outputPixels = pixels;
    if (targetWidth != width || targetHeight != height) {
        resized = resizeBilinear(pixels, width, height, targetWidth, targetHeight);
        outputPixels = resized.data();
    }
    const std::string saveDir = entry.savePath.empty()
        ? beiklive::tools::defaultGameSavePath(entry.platform, entry.path)
        : entry.savePath;
    std::error_code ec;
    fs::create_directories(saveDir, ec);
    if (ec) {
        stbi_image_free(pixels);
        if (error) *error = ec.message();
        return false;
    }
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    outputPath = (fs::path(saveDir) /
        ("steamgriddb_cover_" + std::to_string(timestamp) + ".png")).string();
    const int written = stbi_write_png(outputPath.c_str(), targetWidth,
        targetHeight, 4, outputPixels, targetWidth * 4);
    stbi_image_free(pixels);
    if (!written) {
        if (error) *error = "封面写入失败";
        return false;
    }
    return true;
}

const char* assetTypeName(AssetType type)
{
    switch (type) {
        case AssetType::Grids: return "GRIDS";
        case AssetType::Heroes: return "HEROS";
        case AssetType::Logos: return "LOGOS";
        case AssetType::Icons: return "ICONS";
        default: return "GRIDS";
    }
}
}
