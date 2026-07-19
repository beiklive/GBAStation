#include "ui/page/AboutPage.hpp"
#include "ui/page/UpdatePage.hpp"
#include "ui/widget/UpdateDialog.hpp"
#include "ui/widget/DetailCell.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "core/AppUpdater.hpp"
#include "core/Tools.hpp"
#include <borealis/views/applet_frame.hpp>
#include <curl/curl.h>
#include <miniz.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <sstream>

namespace beiklive {

static constexpr const char* RESOURCE_MANIFEST_URL =
    "https://file.beiklive.top/file/GBAStation/res_version.json";

struct OnlineResourceItem {
    char32_t materialIcon = material::SEARCH;
    std::string name;
    std::string type;
    std::string url;
    std::string path;
    std::string dialog;
    std::string version;
    bool needsUpdate = true;
};

struct OnlineResourceGroup {
    std::string header;
    std::vector<OnlineResourceItem> items;
};

struct OnlineResourceManifest {
    std::vector<OnlineResourceGroup> groups;
};

static std::string trimText(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

static std::string cacheBustedUrl(const std::string& url) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    const std::string timestampedUrl = url
        + (url.find('?') == std::string::npos ? "?t=" : "&t=")
        + std::to_string(timestamp);
    return beiklive::tools::appendDeviceIdParameter(timestampedUrl);
}

static bool fetchTextUrl(const std::string& url,
                         std::string& body,
                         const std::atomic<bool>* cancelFlag = nullptr) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    body.clear();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "GBAStation-ResourceManifest");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        static_cast<size_t(*)(void*, size_t, size_t, void*)>(
            [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* text = static_cast<std::string*>(userdata);
                text->append(static_cast<const char*>(ptr), size * nmemb);
                return size * nmemb;
            }));
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    CURLcode result = CURLE_ABORTED_BY_CALLBACK;
    if (!cancelFlag || !cancelFlag->load())
        result = curl_easy_perform(curl);

    long statusCode = 0;
    if (result == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_cleanup(curl);

    return (!cancelFlag || !cancelFlag->load())
        && result == CURLE_OK && statusCode == 200 && !body.empty();
}

static std::filesystem::path resourceVersionIniPath() {
#ifdef __SWITCH__
    return std::filesystem::path("sdmc:/GBAStation/update/res_version.ini");
#else
    return std::filesystem::path(beiklive::path::ROOT)
        / beiklive::path::PROGRAM_NAME / "update" / "res_version.ini";
#endif
}

static std::map<std::string, std::string> readResourceVersions() {
    std::map<std::string, std::string> versions;
    std::ifstream input(resourceVersionIniPath());
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos)
            continue;
        std::string name = trimText(line.substr(0, separator));
        std::string version = trimText(line.substr(separator + 1));
        if (!name.empty())
            versions[name] = version;
    }
    return versions;
}

static bool writeResourceVersion(const std::string& name, const std::string& version) {
    if (name.empty() || name.find_first_of("\r\n=") != std::string::npos)
        return false;

    auto versions = readResourceVersions();
    versions[name] = version;

    const auto iniPath = resourceVersionIniPath();
    std::error_code ec;
    std::filesystem::create_directories(iniPath.parent_path(), ec);
    if (ec)
        return false;

    std::ofstream output(iniPath, std::ios::binary | std::ios::trunc);
    if (!output)
        return false;
    for (const auto& [itemName, itemVersion] : versions)
        output << itemName << '=' << itemVersion << '\n';
    return output.good();
}

static char32_t parseMaterialIcon(const json& value) {
    try {
        if (value.is_number_unsigned() || value.is_number_integer())
            return static_cast<char32_t>(value.get<uint32_t>());
        if (value.is_string()) {
            const std::string text = trimText(value.get<std::string>());
            size_t parsed = 0;
            const auto codepoint = std::stoul(text, &parsed, 0);
            if (parsed == text.size() && codepoint <= 0x10FFFF)
                return static_cast<char32_t>(codepoint);
        }
    } catch (...) {
    }
    return material::SEARCH;
}

static std::string jsonString(const json& object,
                              const char* key,
                              const std::string& fallback = "") {
    const auto value = object.find(key);
    return value != object.end() && value->is_string()
        ? value->get<std::string>() : fallback;
}

static bool parseResourceManifest(const std::string& text,
                                  OnlineResourceManifest& manifest,
                                  std::string& error) {
    const json root = json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        error = "资源清单 JSON 格式无效";
        return false;
    }

    const auto listIt = root.find("list");
    if (listIt == root.end() || !listIt->is_array()) {
        error = "资源清单缺少 list 数组";
        return false;
    }

    const auto localVersions = readResourceVersions();
    OnlineResourceManifest parsedManifest;
    for (const auto& groupValue : *listIt) {
        if (!groupValue.is_object())
            continue;

        OnlineResourceGroup group;
        group.header = jsonString(groupValue, "header", "未分类资源");
        const auto itemsIt = groupValue.find("items");
        if (itemsIt == groupValue.end() || !itemsIt->is_array())
            continue;

        for (const auto& itemValue : *itemsIt) {
            if (!itemValue.is_object())
                continue;

            OnlineResourceItem item;
            const auto iconIt = itemValue.find("material icon");
            item.materialIcon = iconIt == itemValue.end()
                ? material::SEARCH : parseMaterialIcon(*iconIt);
            item.name = trimText(jsonString(itemValue, "name"));
            item.type = trimText(jsonString(itemValue, "type"));
            item.url = trimText(jsonString(itemValue, "url"));
            item.path = trimText(jsonString(itemValue, "path"));
            item.dialog = jsonString(itemValue, "dialog", "是否下载此资源？");
            item.version = trimText(jsonString(itemValue, "version"));

            std::transform(item.type.begin(), item.type.end(), item.type.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (item.name.empty() || item.url.empty() || item.path.empty()
                || item.version.empty() || (item.type != "zip" && item.type != "file")) {
                continue;
            }

            const auto localIt = localVersions.find(item.name);
            item.needsUpdate = localIt == localVersions.end() || localIt->second != item.version;
            group.items.push_back(std::move(item));
        }

        if (!group.items.empty())
            parsedManifest.groups.push_back(std::move(group));
    }

    if (parsedManifest.groups.empty()) {
        error = "资源清单中没有可用项目";
        return false;
    }

    manifest = std::move(parsedManifest);
    return true;
}

static std::string readTextFile(const std::string& path, const std::string& fallback = "") {
    std::ifstream file(path);
    if (!file)
        return fallback;

    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

static void showMessageDialog(const std::string& message) {
    auto* dlg = new brls::Dialog(message);
    dlg->addButton("确定", []() {});
    dlg->open();
}

static bool downloadFileToPath(const std::string& url,
                               const std::string& outPath,
                               const std::atomic<bool>* cancelFlag = nullptr,
                               std::function<void(curl_off_t, curl_off_t)> progressCallback = nullptr) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    std::vector<uint8_t> body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "GBAStation-ResourceDownloader");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        static_cast<size_t(*)(void*, size_t, size_t, void*)>(
            [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* data = static_cast<std::vector<uint8_t>*>(userdata);
                data->insert(data->end(), static_cast<uint8_t*>(ptr),
                             static_cast<uint8_t*>(ptr) + size * nmemb);
                return size * nmemb;
            }));
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    struct ProgressContext {
        const std::atomic<bool>* cancelFlag;
        std::function<void(curl_off_t, curl_off_t)> callback;
    } progressContext{cancelFlag, std::move(progressCallback)};

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
        static_cast<int(*)(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t)>(
            [](void* userdata, curl_off_t downloadTotal, curl_off_t downloaded,
               curl_off_t, curl_off_t) -> int {
                auto* context = static_cast<ProgressContext*>(userdata);
                if (context->cancelFlag && context->cancelFlag->load())
                    return 1;
                if (context->callback)
                    context->callback(downloaded, downloadTotal);
                return 0;
            }));
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressContext);

    CURLcode res = CURLE_ABORTED_BY_CALLBACK;
    if (!cancelFlag || !cancelFlag->load())
        res = curl_easy_perform(curl);

    long code = 0;
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (cancelFlag && cancelFlag->load())
        return false;

    if (res != CURLE_OK || code != 200 || body.empty())
        return false;

    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    out.write(reinterpret_cast<const char*>(body.data()), body.size());
    return out.good();
}

static std::string zipBaseName(const std::string& name) {
    auto pos = name.find_last_of("/\\");
    return pos == std::string::npos ? name : name.substr(pos + 1);
}

static bool extractZipFilesToDir(const std::string& zipPath,
                                 const std::string& outDir,
                                 const std::vector<std::string>& expectedFiles,
                                 const std::atomic<bool>* cancelFlag,
                                 int& extractCount) {
    extractCount = 0;

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zipPath.c_str(), 0))
        return false;

    std::vector<std::string> remaining = expectedFiles;
    mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < numFiles && (!cancelFlag || !cancelFlag->load()); ++i) {
        char filename[512];
        mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
        std::string baseName = zipBaseName(filename);
        if (baseName.empty())
            continue;

        auto it = std::find(remaining.begin(), remaining.end(), baseName);
        if (it == remaining.end())
            continue;

        std::string outPath = (std::filesystem::path(outDir) / baseName).string();
        if (mz_zip_reader_extract_to_file(&zip, i, outPath.c_str(), 0)) {
            ++extractCount;
            remaining.erase(it);
        }
    }

    mz_zip_reader_end(&zip);
    return remaining.empty() && (!cancelFlag || !cancelFlag->load());
}

static bool isSafeZipEntry(const std::filesystem::path& relativePath) {
    if (relativePath.empty() || relativePath.is_absolute() || relativePath.has_root_name())
        return false;
    for (const auto& part : relativePath) {
        if (part == "..")
            return false;
    }
    return true;
}

static std::string normalizeZipEntryNameForFilesystem(const std::string& entryName) {
    std::string normalized;
    normalized.reserve(entryName.size());
    for (size_t i = 0; i < entryName.size();) {
        const auto byte0 = static_cast<unsigned char>(entryName[i]);
        if (i + 2 < entryName.size() && byte0 == 0xEF) {
            const auto byte1 = static_cast<unsigned char>(entryName[i + 1]);
            const auto byte2 = static_cast<unsigned char>(entryName[i + 2]);
            if (byte1 == 0xBC && byte2 >= 0x81 && byte2 <= 0xBF) {
                normalized.push_back(static_cast<char>(0x21 + byte2 - 0x81));
                i += 3;
                continue;
            }
            if (byte1 == 0xBD && byte2 >= 0x80 && byte2 <= 0x9E) {
                normalized.push_back(static_cast<char>(0x60 + byte2 - 0x80));
                i += 3;
                continue;
            }
        }
        if (i + 2 < entryName.size() && byte0 == 0xE3
            && static_cast<unsigned char>(entryName[i + 1]) == 0x80
            && static_cast<unsigned char>(entryName[i + 2]) == 0x80) {
            normalized.push_back(' ');
            i += 3;
            continue;
        }
        normalized.push_back(entryName[i]);
        ++i;
    }
    return normalized;
}

static bool extractZipToDirectory(const std::filesystem::path& zipPath,
                                  const std::filesystem::path& outputDirectory,
                                  const std::atomic<bool>* cancelFlag,
                                  int& extractedCount,
                                  const std::function<void(int, int, const std::string&)>& progressCallback) {
    extractedCount = 0;
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zipPath.string().c_str(), 0))
        return false;

    bool success = true;
    std::error_code ec;
    const mz_uint fileCount = mz_zip_reader_get_num_files(&zip);
    for (mz_uint index = 0; index < fileCount; ++index) {
        if (cancelFlag && cancelFlag->load()) {
            success = false;
            break;
        }

        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, index, &stat)) {
            success = false;
            break;
        }

        const std::string archiveEntryName = stat.m_filename;
        std::string entryName = normalizeZipEntryNameForFilesystem(archiveEntryName);
        if (entryName != archiveEntryName) {
            brls::Logger::info("Resource ZIP entry normalized: '{}' -> '{}'",
                               archiveEntryName, entryName);
        }
        std::replace(entryName.begin(), entryName.end(), '\\', '/');
        const std::filesystem::path relativePath(entryName);
        if (!isSafeZipEntry(relativePath)) {
            brls::Logger::error("Resource ZIP contains unsafe entry: index={} name='{}'",
                                index, entryName);
            success = false;
            break;
        }

        const auto outputPath = outputDirectory / relativePath;
        if (mz_zip_reader_is_file_a_directory(&zip, index)) {
            std::filesystem::create_directories(outputPath, ec);
            if (ec) {
                brls::Logger::error("Resource ZIP directory creation failed: index={} path='{}' error='{}'",
                                    index, outputPath.string(), ec.message());
                success = false;
                break;
            }
            if (progressCallback)
                progressCallback(static_cast<int>(index + 1), static_cast<int>(fileCount), entryName);
            continue;
        }

        std::filesystem::create_directories(outputPath.parent_path(), ec);
        if (ec || !mz_zip_reader_extract_to_file(&zip, index, outputPath.string().c_str(), 0)) {
            const auto zipError = mz_zip_get_last_error(&zip);
            brls::Logger::error(
                "Resource ZIP extraction failed: index={} entry='{}' path='{}' fsError='{}' zipError='{}'",
                index, entryName, outputPath.string(), ec.message(),
                mz_zip_get_error_string(zipError));
            success = false;
            break;
        }
        ++extractedCount;
        if (progressCallback)
            progressCallback(static_cast<int>(index + 1), static_cast<int>(fileCount), entryName);
    }

    mz_zip_reader_end(&zip);
    return success && (!cancelFlag || !cancelFlag->load());
}

static std::string downloadFileName(const std::string& url) {
    std::string pathPart = url.substr(0, url.find_first_of("?#"));
    const auto separator = pathPart.find_last_of("/\\");
    std::string name = separator == std::string::npos
        ? pathPart : pathPart.substr(separator + 1);
    if (name.empty() || name == "." || name == "..")
        name = "resource_download";
    return name;
}

static bool installDownloadedResource(const OnlineResourceItem& item,
                                      const std::filesystem::path& downloadedPath,
                                      const std::atomic<bool>* cancelFlag,
                                      std::string& resultText,
                                      const std::function<void(int, int, const std::string&)>& progressCallback) {
    const std::filesystem::path targetDirectory(item.path);
    std::error_code ec;
    std::filesystem::create_directories(targetDirectory, ec);
    if (ec) {
        resultText = "创建目标目录失败：\n" + targetDirectory.string();
        return false;
    }

    if (item.type == "zip") {
        int extractedCount = 0;
        if (!extractZipToDirectory(downloadedPath, targetDirectory, cancelFlag,
                                   extractedCount, progressCallback)) {
            resultText = "解压失败，请检查压缩包内容和目标目录";
            return false;
        }
        resultText = "安装完成（解压 " + std::to_string(extractedCount) + " 个文件）";
        return true;
    }

    const auto targetPath = targetDirectory / downloadFileName(item.url);
    std::filesystem::remove(targetPath, ec);
    ec.clear();
    std::filesystem::rename(downloadedPath, targetPath, ec);
    if (ec) {
        ec.clear();
        std::filesystem::copy_file(downloadedPath, targetPath,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            std::error_code removeError;
            std::filesystem::remove(downloadedPath, removeError);
        }
    }
    if (ec) {
        resultText = "移动文件失败：\n" + targetPath.string();
        return false;
    }

    resultText = "下载完成：\n" + targetPath.string();
    return true;
}

static void openChangelogApplet(const std::string& title, const std::string& content);

static std::string encodeMaterialIcon(char32_t codepoint) {
    std::string result;
    if (codepoint <= 0x7F) {
        result.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return result;
}

enum class ChangelogLineKind {
    TEXT,
    SECTION,
    BULLET,
};

struct ChangelogLine {
    std::string text;
    ChangelogLineKind kind = ChangelogLineKind::TEXT;
    int indent = 0;
};

struct ChangelogVersion {
    std::string version;
    std::vector<ChangelogLine> lines;
};

static bool isChangelogVersion(const std::string& text) {
    if (text.size() < 2 || text[0] != 'v'
        || !std::isdigit(static_cast<unsigned char>(text[1]))) {
        return false;
    }
    return std::all_of(text.begin() + 1, text.end(), [](unsigned char c) {
        return std::isdigit(c) || c == '.' || c == '-' || c == '_';
    });
}

static bool isChangelogSection(const std::string& text) {
    static constexpr const char* headings[] = {
        "声明", "新变化", "新功能", "修复", "bug修复", "Bug修复",
        "BUG修复", "bug修复和优化", "Bug修复和优化", "优化"
    };
    return std::any_of(std::begin(headings), std::end(headings),
        [&text](const char* heading) {
            return text == heading || text == std::string(heading) + "："
                || text == std::string(heading) + ":";
        });
}

static bool isChangelogBullet(const std::string& text) {
    size_t cursor = 0;
    while (cursor < text.size()
           && std::isdigit(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
    if (cursor == 0 || cursor >= text.size())
        return false;
    if (text[cursor] == '.' || text[cursor] == ',' || text[cursor] == ')')
        return true;
    return text.compare(cursor, std::string("，").size(), "，") == 0
        || text.compare(cursor, std::string("、").size(), "、") == 0;
}

static std::vector<ChangelogVersion> parseChangelog(const std::string& content) {
    std::vector<ChangelogVersion> versions;
    std::istringstream stream(content);
    std::string rawLine;
    while (std::getline(stream, rawLine)) {
        if (!rawLine.empty() && rawLine.back() == '\r')
            rawLine.pop_back();
        const std::string text = trimText(rawLine);
        if (text.empty())
            continue;
        if (isChangelogVersion(text)) {
            versions.push_back({text, {}});
            continue;
        }
        if (versions.empty())
            versions.push_back({"更新日志", {}});

        int spaces = 0;
        for (const char c : rawLine) {
            if (c == ' ')
                ++spaces;
            else if (c == '\t')
                spaces += 4;
            else
                break;
        }
        ChangelogLineKind kind = ChangelogLineKind::TEXT;
        if (isChangelogSection(text))
            kind = ChangelogLineKind::SECTION;
        else if (isChangelogBullet(text))
            kind = ChangelogLineKind::BULLET;
        versions.back().lines.push_back({text, kind, std::clamp(spaces / 4, 0, 2)});
    }
    if (versions.empty())
        versions.push_back({"更新日志", {{"暂无更新日志", ChangelogLineKind::TEXT, 0}}});
    return versions;
}

static float detailClamp(float value) {
    return std::max(0.f, std::min(1.f, value));
}

static float detailSmooth(float value) {
    value = detailClamp(value);
    return value * value * (3.f - 2.f * value);
}

static float detailBack(float value) {
    value = detailClamp(value);
    constexpr float c1 = 1.16f;
    constexpr float c3 = c1 + 1.f;
    const float shifted = value - 1.f;
    return 1.f + c3 * shifted * shifted * shifted + c1 * shifted * shifted;
}

static unsigned char detailAlpha(float value) {
    return static_cast<unsigned char>(255.f * detailClamp(value));
}

class ChangelogCanvas final : public brls::View {
public:
    ChangelogCanvas(std::string title, std::vector<ChangelogVersion> versions,
                    std::function<void()> onBack)
        : m_title(std::move(title))
        , m_versions(std::move(versions))
        , m_onBack(std::move(onBack)) {
        this->setFocusable(true);
        this->setGrow(1.f);
        this->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);
        this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
        this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        auto previous = [this](brls::View*) -> bool {
            _selectVersion(-1);
            return true;
        };
        auto next = [this](brls::View*) -> bool {
            _selectVersion(1);
            return true;
        };
        auto scrollUp = [this](brls::View*) -> bool {
            _scrollDetail(-180.f);
            return true;
        };
        auto scrollDown = [this](brls::View*) -> bool {
            _scrollDetail(180.f);
            return true;
        };
        this->registerAction("", brls::BUTTON_UP, previous, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_DOWN, next, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_UP, scrollUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_DOWN, scrollDown, true, true, brls::SOUND_NONE);
        this->registerAction("上翻", brls::BUTTON_LB, scrollUp, true, false, brls::SOUND_NONE);
        this->registerAction("下翻", brls::BUTTON_RB, scrollDown, true, false, brls::SOUND_NONE);
        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            _beginClose();
            return true;
        }, false, false, brls::SOUND_NONE);
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    void frame(brls::FrameContext* ctx) override {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;
        m_time += dt;
        if (m_closing) {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 3.8f);
            if (m_pageEntrance <= 0.f && !m_closeQueued) {
                m_closeQueued = true;
                const auto onBack = m_onBack;
                brls::sync([onBack]() {
                    if (onBack)
                        onBack();
                });
            }
        } else {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 2.8f);
            m_detailEntrance = std::min(1.f, m_detailEntrance + dt * 5.2f);
        }
        m_versionScroll += (m_targetVersionScroll - m_versionScroll)
            * std::min(1.f, dt * 14.f);
        m_detailScroll += (m_targetDetailScroll - m_detailScroll)
            * std::min(1.f, dt * 12.f);
        this->invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override {
        (void)style;
        (void)ctx;
        _ensureFonts();
        const float pageProgress = detailBack(m_pageEntrance);
        const float alpha = detailSmooth(m_pageEntrance);
        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        _drawHeader(vg, x, y - (1.f - pageProgress) * 52.f, w);

        const float contentY = y + 108.f;
        const float contentH = std::max(1.f, h - 174.f);
        const float leftW = std::min(270.f, w * 0.23f);
        const Rect versions{x + 36.f, contentY, leftW, contentH};
        const Rect detail{x + 36.f + leftW + 20.f, contentY,
                          w - leftW - 92.f, contentH};
        const float contentScale = 0.97f + pageProgress * 0.03f;
        nvgTranslate(vg, x + w * 0.5f,
                     contentY + contentH * 0.5f + (1.f - pageProgress) * 20.f);
        nvgScale(vg, contentScale, contentScale);
        nvgTranslate(vg, -(x + w * 0.5f), -(contentY + contentH * 0.5f));
        _drawVersionList(vg, versions);
        _drawDetails(vg, detail);
        nvgRestore(vg);
        _drawFooter(vg, x, y, w, h, alpha);
    }

private:
    struct Rect {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    std::string m_title;
    std::vector<ChangelogVersion> m_versions;
    std::function<void()> m_onBack;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_switchFont = -1;
    int m_selectedVersion = 0;
    float m_time = 0.f;
    float m_pageEntrance = 0.f;
    float m_detailEntrance = 1.f;
    float m_versionScroll = 0.f;
    float m_targetVersionScroll = 0.f;
    float m_detailScroll = 0.f;
    float m_targetDetailScroll = 0.f;
    float m_detailContentHeight = 0.f;
    float m_detailViewportHeight = 0.f;
    bool m_closing = false;
    bool m_closeQueued = false;
    std::chrono::steady_clock::time_point m_lastFrameTime;

    void _ensureFonts() {
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
    }

    void _drawExternalShadow(NVGcontext* vg, const Rect& r, float radius,
                             float alpha = 1.f) {
        const NVGpaint shadow = nvgBoxGradient(
            vg, r.x + 5.f, r.y + 6.f, r.w, r.h, radius, 5.f,
            nvgRGBA(0, 0, 0, detailAlpha(0.30f * alpha)),
            nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, r.x - 3.f, r.y - 3.f, r.w + 16.f, r.h + 17.f);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
    }

    void _drawPanel(NVGcontext* vg, const Rect& r, float radius = 8.f) {
        _drawExternalShadow(vg, r, radius, 0.8f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 7));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x + 1.f, r.y + 1.f,
                       r.w - 2.f, r.h - 2.f, radius - 1.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
    }

    void _drawHeader(NVGcontext* vg, float x, float y, float w) {
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 27.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, y + 43.f, "更新日志", nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 180));
        const std::string summary = std::to_string(m_versions.size()) + " 个版本记录";
        nvgText(vg, x + 36.f, y + 72.f, summary.c_str(), nullptr);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 17.f);
        nvgFillColor(vg, nvgRGBA(225, 230, 238, 200));
        nvgText(vg, x + w - 36.f, y + 51.f, m_title.c_str(), nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 36.f, y + 94.f);
        nvgLineTo(vg, x + w - 36.f, y + 94.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 46));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

    void _drawVersionList(NVGcontext* vg, const Rect& r) {
        _drawPanel(vg, r);
        constexpr float padding = 16.f;
        constexpr float rowH = 52.f;
        constexpr float gap = 9.f;
        const float viewportH = r.h - padding * 2.f;
        const float contentH = m_versions.size() * (rowH + gap) - gap;
        const float maximum = std::max(0.f, contentH - viewportH);
        m_targetVersionScroll = std::clamp(m_targetVersionScroll, 0.f, maximum);
        m_versionScroll = std::clamp(m_versionScroll, 0.f, maximum);

        nvgSave(vg);
        nvgIntersectScissor(vg, r.x + 2.f, r.y + 2.f, r.w - 4.f, r.h - 4.f);
        for (size_t index = 0; index < m_versions.size(); ++index) {
            const float rowY = r.y + padding + index * (rowH + gap) - m_versionScroll;
            if (rowY + rowH < r.y || rowY > r.y + r.h)
                continue;
            const bool selected = static_cast<int>(index) == m_selectedVersion;
            if (selected) {
                nvgBeginPath(vg);
                nvgRoundedRect(vg, r.x + padding, rowY,
                               r.w - padding * 2.f, rowH, 7.f);
                nvgFillColor(vg, nvgRGBA(79, 193, 255, 35));
                nvgFill(vg);
                beiklive::ui::drawGradientFocusBorder(
                    vg, r.x + padding, rowY, r.w - padding * 2.f, rowH,
                    7.f, 3.f, 1.f,
                    beiklive::ui::gradientFocusAnimationOffset(m_time));
            }
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, selected ? 20.f : 17.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected
                ? nvgRGBA(255, 255, 255, 255)
                : nvgRGBA(215, 220, 229, 185));
            nvgText(vg, r.x + padding + 18.f, rowY + rowH * 0.5f,
                    m_versions[index].version.c_str(), nullptr);
        }
        nvgRestore(vg);
    }

    void _drawDetails(NVGcontext* vg, const Rect& r) {
        _drawPanel(vg, r);
        if (m_versions.empty())
            return;
        const auto& version = m_versions[m_selectedVersion];
        const float detailProgress = detailBack(m_detailEntrance);
        const float innerX = r.x + 32.f;
        const float innerY = r.y + 24.f;
        const float innerW = r.w - 64.f;
        const float innerH = r.h - 48.f;
        m_detailViewportHeight = innerH;

        nvgSave(vg);
        nvgIntersectScissor(vg, r.x + 2.f, r.y + 2.f, r.w - 4.f, r.h - 4.f);
        nvgGlobalAlpha(vg, detailSmooth(m_detailEntrance));
        nvgTranslate(vg, (1.f - detailProgress) * 46.f, 0.f);

        float cursorY = innerY - m_detailScroll;
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontSize(vg, 36.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, innerX, cursorY, version.version.c_str(), nullptr);
        cursorY += 58.f;

        for (const auto& line : version.lines) {
            const bool section = line.kind == ChangelogLineKind::SECTION;
            const bool bullet = line.kind == ChangelogLineKind::BULLET;
            const float lineX = innerX + (bullet ? 28.f : 0.f)
                + static_cast<float>(line.indent) * 14.f;
            const float lineW = std::max(40.f, innerW - (lineX - innerX));
            if (section) {
                cursorY += 8.f;
                nvgFontSize(vg, 18.f);
                float bounds[4]{};
                nvgTextBounds(vg, 0.f, 0.f, line.text.c_str(), nullptr, bounds);
                const float badgeW = bounds[2] - bounds[0] + 28.f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, innerX, cursorY, badgeW, 32.f, 6.f);
                nvgFillColor(vg, nvgRGBA(79, 193, 255, 28));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, innerX + 1.f, cursorY + 1.f,
                               badgeW - 2.f, 30.f, 5.f);
                nvgStrokeColor(vg, nvgRGBA(79, 193, 255, 145));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(245, 248, 252, 240));
                nvgText(vg, innerX + badgeW * 0.5f, cursorY + 16.f,
                        line.text.c_str(), nullptr);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                cursorY += 48.f;
                continue;
            }

            nvgFontSize(vg, bullet ? 17.f : 18.f);
            nvgTextLineHeight(vg, 1.35f);
            float bounds[4]{};
            nvgTextBoxBounds(vg, lineX, cursorY, lineW,
                             line.text.c_str(), nullptr, bounds);
            const float textH = std::max(25.f, bounds[3] - bounds[1]);
            if (bullet) {
                nvgBeginPath(vg);
                nvgCircle(vg, lineX - 15.f, cursorY + 11.f, 3.5f);
                nvgFillColor(vg, nvgRGBA(79, 193, 255, 220));
                nvgFill(vg);
            }
            nvgFillColor(vg, bullet
                ? nvgRGBA(235, 239, 245, 230)
                : nvgRGBA(215, 221, 231, 205));
            nvgTextBox(vg, lineX, cursorY, lineW,
                       line.text.c_str(), nullptr);
            cursorY += textH + (bullet ? 11.f : 14.f);
        }
        m_detailContentHeight = std::max(innerH, cursorY + m_detailScroll - innerY + 8.f);
        const float maximum = std::max(0.f, m_detailContentHeight - innerH);
        m_targetDetailScroll = std::clamp(m_targetDetailScroll, 0.f, maximum);
        m_detailScroll = std::clamp(m_detailScroll, 0.f, maximum);
        nvgRestore(vg);

        if (maximum > 1.f) {
            const float trackH = innerH - 10.f;
            const float thumbH = std::max(42.f, trackH * innerH / m_detailContentHeight);
            const float thumbY = innerY + 5.f
                + (trackH - thumbH) * m_detailScroll / maximum;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, r.x + r.w - 10.f, thumbY, 3.f, thumbH, 1.5f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 100));
            nvgFill(vg);
        }
    }

    void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                   const char* label, float& cursor, float y, float alpha) {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
        cursor -= bounds[2] - bounds[0] + 43.f;
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, detailAlpha(alpha)));
        nvgText(vg, cursor + 13.f, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(230, 234, 241, detailAlpha(alpha)));
        nvgText(vg, cursor + 30.f, y, label, nullptr);
        cursor -= 16.f;
    }

    void _drawFooter(NVGcontext* vg, float x, float y, float w, float h,
                     float alpha) {
        const float hintY = y + h - 27.f + (1.f - detailBack(m_pageEntrance)) * 46.f;
        float cursor = x + w - 32.f;
        _drawHint(vg, brls::BUTTON_B, "返回", cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_RB, "下翻", cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_LB, "上翻", cursor, hintY, alpha);
    }

    void _selectVersion(int direction) {
        if (m_closing || m_pageEntrance < 0.75f || m_versions.empty())
            return;
        const int next = std::clamp(m_selectedVersion + direction, 0,
                                    static_cast<int>(m_versions.size()) - 1);
        if (next == m_selectedVersion)
            return;
        m_selectedVersion = next;
        m_detailEntrance = 0.f;
        m_detailScroll = 0.f;
        m_targetDetailScroll = 0.f;
        constexpr float rowH = 52.f;
        constexpr float gap = 9.f;
        const float itemTop = m_selectedVersion * (rowH + gap);
        const float viewport = std::max(1.f, this->getHeight() - 206.f);
        if (itemTop < m_targetVersionScroll)
            m_targetVersionScroll = itemTop;
        else if (itemTop + rowH > m_targetVersionScroll + viewport)
            m_targetVersionScroll = itemTop + rowH - viewport;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    }

    void _scrollDetail(float amount) {
        if (m_closing || m_pageEntrance < 0.75f)
            return;
        const float maximum = std::max(0.f,
            m_detailContentHeight - m_detailViewportHeight);
        const float next = std::clamp(m_targetDetailScroll + amount, 0.f, maximum);
        if (std::abs(next - m_targetDetailScroll) < 0.5f)
            return;
        m_targetDetailScroll = next;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    }

    void _beginClose() {
        if (m_closing)
            return;
        m_closing = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
    }
};

static void openChangelogApplet(const std::string& title, const std::string& content) {
    auto* page = new beiklive::Box(brls::Axis::COLUMN);
    page->showHeader(false);
    page->showFooter(false);
    page->setGrow(1.f);
    auto* canvas = new ChangelogCanvas(
        title, parseChangelog(content), []() {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
        });
    page->getContentBox()->addView(canvas);
    auto* frame = new brls::AppletFrame(page);
    HIDE_BRLS_BAR(frame);
    brls::Application::pushActivity(
        new brls::Activity(frame), brls::TransitionAnimation::NONE);
    brls::Application::giveFocus(canvas);
}

static void startResourceDownload(const OnlineResourceItem& item,
                                  std::function<void()> onSuccess);

class OnlineResourceCanvas final : public brls::View {
public:
    OnlineResourceCanvas(OnlineResourceManifest manifest, std::function<void()> onBack)
        : m_manifest(std::move(manifest))
        , m_onBack(std::move(onBack)) {
        this->setFocusable(true);
        this->setGrow(1.0f);
        this->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);

        this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
        this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        auto moveLeft = [this](brls::View*) -> bool {
            if (_acceptNavigation(1))
                _moveHorizontal(-1);
            return true;
        };
        auto moveRight = [this](brls::View*) -> bool {
            if (_acceptNavigation(2))
                _moveHorizontal(1);
            return true;
        };
        auto moveUp = [this](brls::View*) -> bool {
            if (_acceptNavigation(3))
                _moveVertical(-1);
            return true;
        };
        auto moveDown = [this](brls::View*) -> bool {
            if (_acceptNavigation(4))
                _moveVertical(1);
            return true;
        };

        this->registerAction("", brls::BUTTON_LEFT, moveLeft, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_RIGHT, moveRight, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_UP, moveUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_DOWN, moveDown, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_LEFT, moveLeft, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_RIGHT, moveRight, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_UP, moveUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_DOWN, moveDown, true, true, brls::SOUND_NONE);
        this->registerAction("上一类", brls::BUTTON_LB, moveLeft, true, false, brls::SOUND_NONE);
        this->registerAction("下一类", brls::BUTTON_RB, moveRight, true, false, brls::SOUND_NONE);
        this->registerAction("下载", brls::BUTTON_A, [this](brls::View*) -> bool {
            _activateFocused();
            return true;
        }, false, false, brls::SOUND_NONE);
        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            _beginClose();
            return true;
        }, false, false, brls::SOUND_NONE);

        m_groupFocusIndices.assign(m_manifest.groups.size(), 0);
        m_groupScrollOffsets.assign(m_manifest.groups.size(), 0.f);
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override {
        (void)style;
        (void)ctx;
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);

        _rebuildLayout(w, h);
        m_scrollOffset = std::clamp(m_scrollOffset, 0.f, _maximumScroll());
        m_targetScroll = std::clamp(m_targetScroll, 0.f, _maximumScroll());

        const float pageProgress = detailBack(m_pageEntrance);
        const float pageAlpha = detailSmooth(m_pageEntrance);
        _drawResourceHeader(vg, x, y - (1.f - pageProgress) * 54.f, w);

        const float viewportY = y + 112.f;
        const float viewportH = std::max(1.f, h - 178.f);
        nvgSave(vg);
        nvgGlobalAlpha(vg, pageAlpha);
        nvgScissor(vg, x, viewportY, w, viewportH);
        const float drawOffsetY = viewportY - m_scrollOffset;
        const float contentScale = 0.97f + pageProgress * 0.03f;
        nvgTranslate(vg, x + w * 0.5f,
                     viewportY + viewportH * 0.5f + (1.f - pageProgress) * 22.f);
        nvgScale(vg, contentScale, contentScale);
        nvgTranslate(vg, -(x + w * 0.5f), -(viewportY + viewportH * 0.5f));
        const float groupProgress = detailBack(m_groupEntrance);
        nvgTranslate(vg, static_cast<float>(m_groupDirection)
                         * (1.f - groupProgress) * 54.f, 0.f);

        for (size_t index = 0; index < m_itemLayouts.size(); ++index) {
            const auto& layout = m_itemLayouts[index];
            const float itemY = drawOffsetY + layout.y;
            if (itemY + layout.h < viewportY || itemY > viewportY + viewportH)
                continue;
            const auto& item = m_manifest.groups[layout.groupIndex].items[layout.itemIndex];
            _drawResourceButton(vg, x + layout.x, itemY, layout.w, layout.h,
                                item, static_cast<int>(index) == m_focusedIndex);
        }

        if (m_contentHeight > viewportH + 1.f) {
            const float trackH = std::max(40.f, viewportH - 24.f);
            const float thumbH = std::max(42.f,
                trackH * viewportH / m_contentHeight);
            const float travel = trackH - thumbH;
            const float thumbY = viewportY + 12.f + (_maximumScroll() <= 0.f
                ? 0.f : travel * m_scrollOffset / _maximumScroll());
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + w - 39.f, thumbY, 3.f, thumbH, 1.5f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 90));
            nvgFill(vg);
        }

        nvgRestore(vg);
        _drawResourceFooter(vg, x, y, w, h, pageAlpha);
    }

    void frame(brls::FrameContext* ctx) override {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.5f)
            dt = 0.016f;
        m_animTime += dt;
        if (m_closing) {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 3.8f);
            if (m_pageEntrance <= 0.f && !m_closeQueued) {
                m_closeQueued = true;
                const auto onBack = m_onBack;
                brls::sync([onBack]() {
                    if (onBack)
                        onBack();
                });
            }
        } else {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 2.8f);
            m_groupEntrance = std::min(1.f, m_groupEntrance + dt * 5.f);
        }

        const float difference = m_targetScroll - m_scrollOffset;
        if (std::abs(difference) > 0.2f)
            m_scrollOffset += difference * std::min(1.f, dt * 12.f);
        else
            m_scrollOffset = m_targetScroll;
        this->invalidate();
    }

private:
    struct ItemLayout {
        size_t groupIndex = 0;
        size_t itemIndex = 0;
        int row = 0;
        int column = 0;
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    struct GroupLayout {
        size_t groupIndex = 0;
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    OnlineResourceManifest m_manifest;
    std::function<void()> m_onBack;
    std::vector<ItemLayout> m_itemLayouts;
    std::vector<GroupLayout> m_groupLayouts;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_switchFont = -1;
    int m_focusedIndex = 0;
    int m_selectedGroup = 0;
    int m_groupDirection = 1;
    float m_viewportHeight = 0.f;
    float m_contentHeight = 0.f;
    float m_scrollOffset = 0.f;
    float m_targetScroll = 0.f;
    float m_animTime = 0.f;
    float m_pageEntrance = 0.f;
    float m_groupEntrance = 1.f;
    float m_layoutWidth = -1.f;
    std::vector<int> m_groupFocusIndices;
    std::vector<float> m_groupScrollOffsets;
    bool m_closing = false;
    bool m_closeQueued = false;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastNavigationTime;
    int m_lastNavigationAction = 0;

    bool _acceptNavigation(int action) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastNavigationTime).count();
        if (action == m_lastNavigationAction && elapsed >= 0 && elapsed < 80)
            return false;
        m_lastNavigationAction = action;
        m_lastNavigationTime = now;
        return true;
    }

    void _rebuildLayout(float width, float height) {
        m_viewportHeight = std::max(1.f, height - 178.f);
        if (std::abs(m_layoutWidth - width) < 0.5f && !m_itemLayouts.empty())
            return;

        m_layoutWidth = width;
        m_itemLayouts.clear();
        m_groupLayouts.clear();

        constexpr float pagePadding = 36.f;
        constexpr float itemGap = 12.f;
        constexpr float itemHeight = 94.f;

        const float blockWidth = std::max(1.f, width - pagePadding * 2.f);
        float cursorY = 8.f;
        if (!m_manifest.groups.empty()) {
            m_selectedGroup = std::clamp(
                m_selectedGroup, 0, static_cast<int>(m_manifest.groups.size()) - 1);
            const auto& group = m_manifest.groups[static_cast<size_t>(m_selectedGroup)];
            for (size_t itemIndex = 0; itemIndex < group.items.size(); ++itemIndex) {
                m_itemLayouts.push_back({
                    static_cast<size_t>(m_selectedGroup),
                    itemIndex,
                    static_cast<int>(itemIndex),
                    0,
                    pagePadding,
                    cursorY + itemIndex * (itemHeight + itemGap),
                    blockWidth,
                    itemHeight,
                });
            }
            cursorY += group.items.size() * itemHeight
                + std::max(0, static_cast<int>(group.items.size()) - 1) * itemGap;
        }
        m_contentHeight = cursorY + 8.f;
        m_focusedIndex = std::clamp(m_focusedIndex, 0,
            std::max(0, static_cast<int>(m_itemLayouts.size()) - 1));
        _ensureFocusedVisible();
    }

    float _maximumScroll() const {
        return std::max(0.f, m_contentHeight - m_viewportHeight);
    }

    void _ensureFocusedVisible() {
        if (m_itemLayouts.empty() || m_focusedIndex < 0
            || m_focusedIndex >= static_cast<int>(m_itemLayouts.size()))
            return;
        const auto& item = m_itemLayouts[m_focusedIndex];
        constexpr float margin = 24.f;
        if (item.y < m_targetScroll + margin)
            m_targetScroll = item.y - margin;
        else if (item.y + item.h > m_targetScroll + m_viewportHeight - margin)
            m_targetScroll = item.y + item.h - m_viewportHeight + margin;
        m_targetScroll = std::clamp(m_targetScroll, 0.f, _maximumScroll());
    }

    bool _moveHorizontal(int direction) {
        if (m_manifest.groups.size() <= 1 || m_closing
            || m_pageEntrance < 0.72f || m_groupEntrance < 0.72f) {
            return true;
        }
        if (m_selectedGroup >= 0
            && m_selectedGroup < static_cast<int>(m_groupFocusIndices.size())) {
            m_groupFocusIndices[static_cast<size_t>(m_selectedGroup)] = m_focusedIndex;
            m_groupScrollOffsets[static_cast<size_t>(m_selectedGroup)] = m_targetScroll;
        }
        const int count = static_cast<int>(m_manifest.groups.size());
        m_selectedGroup = (m_selectedGroup + (direction < 0 ? -1 : 1) + count) % count;
        m_groupDirection = direction;
        m_groupEntrance = 0.f;
        m_focusedIndex = m_groupFocusIndices[static_cast<size_t>(m_selectedGroup)];
        m_scrollOffset = m_groupScrollOffsets[static_cast<size_t>(m_selectedGroup)];
        m_targetScroll = m_scrollOffset;
        m_layoutWidth = -1.f;
        m_itemLayouts.clear();
        m_groupLayouts.clear();
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        this->invalidate();
        return true;
    }

    bool _moveVertical(int direction) {
        if (m_itemLayouts.empty() || m_closing || m_pageEntrance < 0.72f)
            return true;
        const int candidate = std::clamp(
            m_focusedIndex + direction, 0,
            static_cast<int>(m_itemLayouts.size()) - 1);
        if (candidate != m_focusedIndex)
            _setFocus(candidate);
        return true;
    }

    void _setFocus(int index) {
        if (index == m_focusedIndex)
            return;
        m_focusedIndex = index;
        if (m_selectedGroup >= 0
            && m_selectedGroup < static_cast<int>(m_groupFocusIndices.size())) {
            m_groupFocusIndices[static_cast<size_t>(m_selectedGroup)] = index;
        }
        _ensureFocusedVisible();
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
        this->invalidate();
    }

    void _activateFocused() {
        if (m_itemLayouts.empty() || m_closing || m_pageEntrance < 0.85f)
            return;
        const auto layout = m_itemLayouts[m_focusedIndex];
        const auto item = m_manifest.groups[layout.groupIndex].items[layout.itemIndex];
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);

        auto* dialog = new brls::Dialog(item.dialog.empty() ? "是否下载此资源？" : item.dialog);
        dialog->addButton("取消", []() {});
        dialog->addButton("确认", [this, item, layout]() {
            startResourceDownload(item, [this, layout]() {
                if (layout.groupIndex < m_manifest.groups.size()
                    && layout.itemIndex < m_manifest.groups[layout.groupIndex].items.size()) {
                    m_manifest.groups[layout.groupIndex].items[layout.itemIndex].needsUpdate = false;
                    this->invalidate();
                }
            });
        });
        dialog->open();
    }

    void _drawResourceButton(NVGcontext* vg, float x, float y, float w, float h,
                             const OnlineResourceItem& item, bool focused) {
        const NVGpaint shadow = nvgBoxGradient(
            vg, x + 5.f, y + 6.f, w, h, 8.f, 5.f,
            nvgRGBA(0, 0, 0, 72), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x - 3.f, y - 3.f, w + 16.f, h + 17.f);
        nvgRoundedRect(vg, x, y, w, h, 8.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 8.f);
        nvgFillColor(vg, focused
            ? nvgRGBA(79, 193, 255, 34) : nvgRGBA(255, 255, 255, 7));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 7.f);
        nvgStrokeColor(vg, focused
            ? nvgRGBA(255, 255, 255, 145) : nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);

        if (focused && this->isFocused()) {
            beiklive::ui::drawGradientFocusBorder(vg, x, y, w, h, 8.f, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_animTime));
        }

        const std::string icon = encodeMaterialIcon(item.materialIcon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 38.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused
            ? nvgRGBA(255, 255, 255, 255) : nvgRGBA(225, 230, 238, 220));
        nvgText(vg, x + 54.f, y + h * 0.5f, icon.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 21.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused
            ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 96.f, y + 29.f, item.name.c_str(), nullptr);

        nvgFontSize(vg, 14.f);
        nvgFillColor(vg, nvgRGBA(205, 212, 223, focused ? 220 : 175));
        const std::string metadata = "版本 " + item.version + "  ·  "
            + (item.type == "zip" ? "压缩资源" : "单文件资源");
        nvgText(vg, x + 96.f, y + 54.f, metadata.c_str(), nullptr);

        nvgFontSize(vg, 13.f);
        nvgFillColor(vg, nvgRGBA(190, 198, 211, focused ? 190 : 145));
        nvgSave(vg);
        nvgIntersectScissor(vg, x + 96.f, y + 64.f,
                           std::max(20.f, w - 310.f), 22.f);
        nvgText(vg, x + 96.f, y + 76.f, item.path.c_str(), nullptr);
        nvgRestore(vg);

        const float statusW = item.needsUpdate ? 82.f : 76.f;
        const float statusX = x + w - statusW - 24.f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, statusX, y + (h - 34.f) * 0.5f,
                       statusW, 34.f, 6.f);
        nvgFillColor(vg, item.needsUpdate
            ? nvgRGBA(255, 190, 80, 28) : nvgRGBA(100, 220, 150, 25));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, statusX + 1.f, y + (h - 34.f) * 0.5f + 1.f,
                       statusW - 2.f, 32.f, 5.f);
        nvgStrokeColor(vg, item.needsUpdate
            ? nvgRGBA(255, 190, 80, 150) : nvgRGBA(100, 220, 150, 135));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgFontSize(vg, 15.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, item.needsUpdate
            ? nvgRGBA(255, 205, 120, 245) : nvgRGBA(135, 235, 175, 235));
        const char* status = item.needsUpdate ? "可更新" : "已安装";
        nvgText(vg, statusX + statusW * 0.5f, y + h * 0.5f,
                status, nullptr);
    }

    size_t _currentGroupIndex() const {
        if (m_manifest.groups.empty())
            return 0;
        return static_cast<size_t>(std::clamp(
            m_selectedGroup, 0, static_cast<int>(m_manifest.groups.size()) - 1));
    }

    void _drawResourceHeader(NVGcontext* vg, float x, float y, float w) {
        const float alpha = detailSmooth(m_pageEntrance);
        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 27.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, y + 43.f, "在线资源", nullptr);

        size_t itemCount = 0;
        size_t pendingCount = 0;
        for (const auto& group : m_manifest.groups) {
            itemCount += group.items.size();
            pendingCount += static_cast<size_t>(std::count_if(
                group.items.begin(), group.items.end(),
                [](const OnlineResourceItem& item) { return item.needsUpdate; }));
        }
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 180));
        const std::string summary = std::to_string(itemCount) + " 项资源  ·  "
            + std::to_string(pendingCount) + " 项可更新";
        nvgText(vg, x + 36.f, y + 72.f, summary.c_str(), nullptr);

        if (!m_manifest.groups.empty()) {
            const float centerX = x + w * 0.5f;
            const float centerY = y + 45.f;
            constexpr float spacing = 152.f;
            constexpr float selectorW = 132.f;
            constexpr float selectorH = 42.f;
            constexpr float selectorRadius = 21.f;
            const float eased = 1.f - std::pow(1.f - m_groupEntrance, 3.f);
            const float carouselShift = static_cast<float>(m_groupDirection)
                * spacing * (1.f - eased);
            const int count = static_cast<int>(m_manifest.groups.size());
            const int current = static_cast<int>(_currentGroupIndex());

            _drawResourceSwitchButton(vg, brls::BUTTON_LB,
                                      centerX - 290.f, centerY);
            _drawResourceSwitchButton(vg, brls::BUTTON_RB,
                                      centerX + 290.f, centerY);

            const int firstOffset = count == 1 ? 0 : -2;
            const int lastOffset = count == 1 ? 0 : 2;
            for (int relative = firstOffset; relative <= lastOffset; ++relative) {
                int index = (current + relative) % count;
                if (index < 0)
                    index += count;
                const float labelX = centerX + relative * spacing + carouselShift;
                const float distance = std::abs(labelX - centerX) / spacing;
                if (distance > 1.55f)
                    continue;
                const float prominence = std::max(0.f, 1.f - distance);
                const float labelAlpha = 0.42f + prominence * 0.58f;
                if (prominence > 0.55f) {
                    const float selectorX = labelX - selectorW * 0.5f;
                    const float selectorY = centerY - selectorH * 0.5f;
                    const NVGpaint selectorShadow = nvgBoxGradient(
                        vg, selectorX + 3.f, selectorY + 3.f,
                        selectorW, selectorH, selectorRadius, 5.f,
                        nvgRGBA(0, 0, 0,
                            static_cast<unsigned char>(72.f * prominence)),
                        nvgRGBA(0, 0, 0, 0));
                    nvgBeginPath(vg);
                    nvgRect(vg, selectorX - 2.f, selectorY - 2.f,
                            selectorW + 10.f, selectorH + 10.f);
                    nvgRoundedRect(vg, selectorX, selectorY,
                                   selectorW, selectorH, selectorRadius);
                    nvgPathWinding(vg, NVG_HOLE);
                    nvgFillPaint(vg, selectorShadow);
                    nvgFill(vg);

                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, selectorX, selectorY,
                                   selectorW, selectorH, selectorRadius);
                    nvgFillColor(vg, nvgRGBA(255, 255, 255,
                        static_cast<unsigned char>(22.f + 22.f * prominence)));
                    nvgFill(vg);
                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, selectorX + 1.f, selectorY + 1.f,
                                   selectorW - 2.f, selectorH - 2.f,
                                   selectorRadius - 1.f);
                    nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                        static_cast<unsigned char>(70.f + 65.f * prominence)));
                    nvgStrokeWidth(vg, 1.f);
                    nvgStroke(vg);
                }
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 17.f + 5.f * prominence);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(255.f * labelAlpha)));
                nvgText(vg, labelX, centerY,
                        m_manifest.groups[static_cast<size_t>(index)].header.c_str(),
                        nullptr);
            }
        }
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 36.f, y + 94.f);
        nvgLineTo(vg, x + w - 36.f, y + 94.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 46));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgRestore(vg);
    }

    void _drawResourceSwitchButton(NVGcontext* vg,
                                   brls::ControllerButton button,
                                   float x, float y) {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 240));
        nvgText(vg, x, y, glyph.c_str(), nullptr);
    }

    void _drawResourceHint(NVGcontext* vg, brls::ControllerButton button,
                           const char* label, float& cursor, float y, float alpha) {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
        cursor -= bounds[2] - bounds[0] + 43.f;
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, detailAlpha(alpha)));
        nvgText(vg, cursor + 13.f, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(230, 234, 241, detailAlpha(alpha)));
        nvgText(vg, cursor + 30.f, y, label, nullptr);
        cursor -= 16.f;
    }

    void _drawResourceFooter(NVGcontext* vg, float x, float y, float w, float h,
                             float alpha) {
        const float hintY = y + h - 27.f
            + (1.f - detailBack(m_pageEntrance)) * 46.f;
        float cursor = x + w - 32.f;
        _drawResourceHint(vg, brls::BUTTON_B, "返回", cursor, hintY, alpha);
        _drawResourceHint(vg, brls::BUTTON_A, "下载", cursor, hintY, alpha);
        _drawResourceHint(vg, brls::BUTTON_RB, "下一类", cursor, hintY, alpha);
        _drawResourceHint(vg, brls::BUTTON_LB, "上一类", cursor, hintY, alpha);
    }

    void _beginClose() {
        if (m_closing)
            return;
        m_closing = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
    }
};

class ResourceProgressCanvas final : public brls::View {
public:
    explicit ResourceProgressCanvas(std::string resourceName)
        : m_title("正在更新 " + std::move(resourceName)) {
        this->setWidth(620.f);
        this->setHeight(250.f);
        this->setFocusable(false);
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    void setProgress(std::string stage, std::string detail, float progress) {
        m_stage = std::move(stage);
        m_detail = detail.empty() ? "准备中" : std::move(detail);
        m_targetProgress = std::clamp(progress, 0.f, 1.f);
        _updateStageStyle();
        this->invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override {
        (void)style;
        (void)ctx;
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);

        constexpr float iconSize = 68.f;
        constexpr float sidePadding = 38.f;
        const float iconX = x + sidePadding;
        const float iconY = y + 34.f;
        const float textX = iconX + iconSize + 22.f;
        const float progressX = x + sidePadding;
        const float progressW = w - sidePadding * 2.f;
        const float progressY = y + 184.f;
        const float progressH = 10.f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, iconX, iconY, iconSize, iconSize, 8.f);
        nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 34));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, iconX + 1.f, iconY + 1.f, iconSize - 2.f, iconSize - 2.f, 7.f);
        nvgStrokeColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 105));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        const std::string iconText = encodeMaterialIcon(m_icon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 38.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        const float pulse = 0.82f + std::sin(m_animTime * 3.2f) * 0.12f;
        nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB,
                                static_cast<unsigned char>(255.f * pulse)));
        nvgText(vg, iconX + iconSize * 0.5f, iconY + iconSize * 0.5f,
                iconText.c_str(), nullptr);

        nvgSave(vg);
        nvgIntersectScissor(vg, textX, y + 28.f, w - textX + x - sidePadding, 88.f);
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontSize(vg, 23.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, textX, y + 39.f, m_title.c_str(), nullptr);
        nvgFontSize(vg, 18.f);
        nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 235));
        nvgText(vg, textX, y + 76.f, m_stage.c_str(), nullptr);
        nvgRestore(vg);

        nvgBeginPath(vg);
        nvgMoveTo(vg, x + sidePadding, y + 124.f);
        nvgLineTo(vg, x + w - sidePadding, y + 124.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 24));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgSave(vg);
        nvgIntersectScissor(vg, x + sidePadding, y + 138.f, progressW - 90.f, 28.f);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 15.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(205, 210, 220, 210));
        nvgText(vg, x + sidePadding, y + 151.f, m_detail.c_str(), nullptr);
        nvgRestore(vg);

        const std::string percentText = std::to_string(
            static_cast<int>(m_targetProgress * 100.f + 0.5f)) + "%";
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 17.f);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + w - sidePadding, y + 151.f, percentText.c_str(), nullptr);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, progressX, progressY, progressW, progressH, 5.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 28));
        nvgFill(vg);

        const float fillW = progressW * std::clamp(m_displayProgress, 0.f, 1.f);
        if (fillW > 0.5f) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, progressX, progressY, fillW, progressH, 5.f);
            nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 235));
            nvgFill(vg);

            if (fillW > 12.f) {
                nvgBeginPath(vg);
                nvgCircle(vg, progressX + fillW - 5.f, progressY + progressH * 0.5f, 2.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 205));
                nvgFill(vg);
            }
        }

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 13.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(175, 180, 192, 165));
        nvgText(vg, x + sidePadding, y + 220.f, "在线资源", nullptr);
    }

    void frame(brls::FrameContext* ctx) override {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.5f)
            dt = 0.016f;
        m_animTime += dt;
        const float difference = m_targetProgress - m_displayProgress;
        m_displayProgress += difference * std::min(1.f, dt * 10.f);
        if (std::abs(difference) < 0.001f)
            m_displayProgress = m_targetProgress;
        this->invalidate();
    }

private:
    std::string m_title;
    std::string m_stage = "正在连接服务器...";
    std::string m_detail = "准备中";
    char32_t m_icon = 0xE2C4;
    unsigned char m_accentR = 79;
    unsigned char m_accentG = 193;
    unsigned char m_accentB = 255;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    float m_targetProgress = 0.f;
    float m_displayProgress = 0.f;
    float m_animTime = 0.f;
    std::chrono::steady_clock::time_point m_lastFrameTime;

    void _updateStageStyle() {
        if (m_stage.find("解压") != std::string::npos) {
            m_icon = 0xE149;
            m_accentR = 255;
            m_accentG = 184;
            m_accentB = 77;
        } else if (m_stage.find("安装") != std::string::npos) {
            m_icon = material::INSTALL_APP;
            m_accentR = 111;
            m_accentG = 207;
            m_accentB = 151;
        } else if (m_stage.find("版本") != std::string::npos) {
            m_icon = material::DESCRIPTION;
            m_accentR = 111;
            m_accentG = 207;
            m_accentB = 151;
        } else {
            m_icon = 0xE2C4;
            m_accentR = 79;
            m_accentG = 193;
            m_accentB = 255;
        }
    }
};

class ResourceTransferDialog final : public brls::Dialog {
public:
    explicit ResourceTransferDialog(const std::string& resourceName)
        : ResourceTransferDialog(_buildContent(resourceName)) {
        this->setCancelable(false);
        this->setFocusable(true);
        HIDE_BRLS_HIGHLIGHT(this);
    }

    void setProgress(const std::string& stage,
                     const std::string& detail,
                     float progress) {
        m_canvas->setProgress(stage, detail, progress);
    }

private:
    struct ContentParts {
        brls::Box* root = nullptr;
        ResourceProgressCanvas* canvas = nullptr;
    };

    ResourceProgressCanvas* m_canvas = nullptr;

    explicit ResourceTransferDialog(const ContentParts& parts)
        : brls::Dialog(parts.root)
        , m_canvas(parts.canvas) {
    }

    static ContentParts _buildContent(const std::string& resourceName) {
        ContentParts parts;
        parts.root = new brls::Box(brls::Axis::COLUMN);
        parts.root->setWidth(620.f);
        parts.root->setHeight(250.f);
        parts.root->setFocusable(false);
        parts.canvas = new ResourceProgressCanvas(resourceName);
        parts.root->addView(parts.canvas);
        return parts;
    }
};

static std::string formatTransferSize(curl_off_t bytes) {
    if (bytes < 1024)
        return std::to_string(static_cast<long long>(bytes)) + " B";
    if (bytes < 1024 * 1024)
        return fmt::format("{:.1f} KB", static_cast<double>(bytes) / 1024.0);
    return fmt::format("{:.1f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
}

static void openOnlineResourceActivity(OnlineResourceManifest manifest,
                                       beiklive::Box* previousPage) {
    auto* page = new beiklive::Box(brls::Axis::COLUMN);
    page->showHeader(false);
    page->showFooter(false);
    page->setGrow(1.f);

    auto* canvas = new OnlineResourceCanvas(std::move(manifest), [page]() {
        beiklive::popActivity(page);
    });
    page->getContentBox()->addView(canvas);

    auto* frame = new brls::AppletFrame(page);
    HIDE_BRLS_BAR(frame);
    beiklive::pushActivity(frame, previousPage, page, [canvas]() {
        brls::Application::giveFocus(canvas);
    });
}

static void checkOnlineResources(beiklive::Box* previousPage) {
    auto* progressDialog = new brls::Dialog("正在检测在线资源...\n\n请稍候");
    progressDialog->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(progressDialog);
    progressDialog->open();

    new std::thread([progressDialog, previousPage]() {
        std::string manifestText;
        const bool downloadOk = fetchTextUrl(cacheBustedUrl(RESOURCE_MANIFEST_URL),
                                             manifestText);
        if (!downloadOk) {
            brls::sync([progressDialog]() {
                progressDialog->close([]() {});
                showMessageDialog("资源清单下载失败，请检查网络或资源地址");
            });
            return;
        }

        brls::Logger::info("Online resource manifest URL: {}", RESOURCE_MANIFEST_URL);
        brls::Logger::info("res_version.json content:\n{}", manifestText);

        OnlineResourceManifest manifest;
        std::string error;
        if (!parseResourceManifest(manifestText, manifest, error)) {
            brls::sync([progressDialog, error]() {
                progressDialog->close([]() {});
                showMessageDialog(error);
            });
            return;
        }

        brls::sync([progressDialog, previousPage, manifest = std::move(manifest)]() mutable {
            progressDialog->close([]() {});
            openOnlineResourceActivity(std::move(manifest), previousPage);
        });
    });
}

static void startResourceDownload(const OnlineResourceItem& item,
                                  std::function<void()> onSuccess) {
    auto* progressDialog = new ResourceTransferDialog(item.name);
    progressDialog->open();

    new std::thread([progressDialog, item, onSuccess = std::move(onSuccess)]() {
        std::error_code ec;
        const auto cacheDirectory = std::filesystem::path(beiklive::path::cachePath())
            / "online_resources";
        std::filesystem::create_directories(cacheDirectory, ec);
        if (ec) {
            brls::sync([progressDialog]() {
                progressDialog->close([]() {});
                showMessageDialog("创建下载缓存目录失败");
            });
            return;
        }

        const auto uniqueId = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const auto downloadPath = cacheDirectory
            / (std::to_string(uniqueId) + "_" + downloadFileName(item.url));

        auto lastDownloadUpdate = std::make_shared<std::chrono::steady_clock::time_point>();
        auto downloadProgress = [progressDialog, lastDownloadUpdate](
                                    curl_off_t downloaded, curl_off_t total) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - *lastDownloadUpdate).count();
            if (total > 0 && downloaded < total && elapsed >= 0 && elapsed < 100)
                return;
            *lastDownloadUpdate = now;

            const float progress = total > 0
                ? static_cast<float>(static_cast<double>(downloaded) / static_cast<double>(total))
                : 0.f;
            std::string detail = formatTransferSize(downloaded);
            if (total > 0)
                detail += " / " + formatTransferSize(total);
            brls::sync([progressDialog, detail = std::move(detail), progress]() {
                progressDialog->setProgress("正在下载...", detail, progress);
            });
        };

        if (!downloadFileToPath(cacheBustedUrl(item.url), downloadPath.string(),
                                nullptr, std::move(downloadProgress))) {
            std::filesystem::remove(downloadPath, ec);
            brls::sync([progressDialog]() {
                progressDialog->close([]() {});
                showMessageDialog("下载失败，请稍后重试");
            });
            return;
        }

        if (item.type == "zip") {
            brls::sync([progressDialog]() {
                progressDialog->setProgress("正在解压...", "正在读取压缩包", 0.f);
            });
        } else {
            brls::sync([progressDialog, item]() {
                progressDialog->setProgress("正在安装文件...", downloadFileName(item.url), 0.f);
            });
        }

        auto lastExtractUpdate = std::make_shared<std::chrono::steady_clock::time_point>();
        auto extractProgress = [progressDialog, lastExtractUpdate](
                                   int current, int total, const std::string& entryName) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - *lastExtractUpdate).count();
            if (current < total && elapsed >= 0 && elapsed < 50)
                return;
            *lastExtractUpdate = now;

            const float progress = total > 0
                ? static_cast<float>(current) / static_cast<float>(total) : 0.f;
            const std::string detail = std::to_string(current) + " / "
                + std::to_string(total) + "  " + zipBaseName(entryName);
            brls::sync([progressDialog, detail, progress]() {
                progressDialog->setProgress("正在解压...", detail, progress);
            });
        };

        std::string resultText;
        std::function<void(int, int, const std::string&)> installProgress;
        if (item.type == "zip")
            installProgress = extractProgress;
        const bool installOk = installDownloadedResource(
            item, downloadPath, nullptr, resultText,
            installProgress);
        std::filesystem::remove(downloadPath, ec);

        if (!installOk) {
            brls::sync([progressDialog, resultText]() {
                progressDialog->close([]() {});
                showMessageDialog(resultText);
            });
            return;
        }

        brls::sync([progressDialog]() {
            progressDialog->setProgress("正在保存版本信息...", "即将完成", 1.f);
        });
        const bool versionSaved = writeResourceVersion(item.name, item.version);
        brls::sync([progressDialog, resultText, versionSaved, onSuccess]() {
            progressDialog->close([]() {});
            if (versionSaved) {
                if (onSuccess)
                    onSuccess();
                showMessageDialog("更新完成\n\n" + resultText);
            } else {
                showMessageDialog("资源已安装\n\n" + resultText
                    + "\n但版本记录写入失败");
            }
        });
    });
}

class UpdateTabCanvas final : public brls::View {
public:
    UpdateTabCanvas(std::string version,
                    std::string updateSource,
                    std::function<void()> onCheckUpdate,
                    std::function<void()> onChangelog,
                    std::function<void()> onResourceCheck)
        : m_version(std::move(version))
        , m_updateSource(std::move(updateSource))
        , m_onCheckUpdate(std::move(onCheckUpdate))
        , m_onChangelog(std::move(onChangelog))
        , m_onResourceCheck(std::move(onResourceCheck)) {
        this->setFocusable(true);
        this->setGrow(1.0f);
        this->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);

        this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
        this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        auto moveLeft = [this](brls::View*) -> bool {
            if (m_focusedIndex == 1)
                _setFocus(0);
            return true;
        };
        auto moveRight = [this](brls::View*) -> bool {
            if (m_focusedIndex == 0)
                _setFocus(1);
            return true;
        };
        auto moveUp = [this](brls::View*) -> bool {
            if (m_focusedIndex == 2)
                _setFocus(0);
            return true;
        };
        auto moveDown = [this](brls::View*) -> bool {
            if (m_focusedIndex != 2)
                _setFocus(2);
            return true;
        };

        this->registerAction("", brls::BUTTON_LEFT, moveLeft, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_RIGHT, moveRight, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_UP, moveUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_DOWN, moveDown, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_LEFT, moveLeft, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_RIGHT, moveRight, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_UP, moveUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_DOWN, moveDown, true, true, brls::SOUND_NONE);
        this->registerAction("打开", brls::BUTTON_A, [this](brls::View*) -> bool {
            _activateFocused();
            return true;
        }, false, false, brls::SOUND_NONE);
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override {
        (void)style;
        (void)ctx;

        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);

        const float padX = 40.f;
        const float padY = 24.f;
        const float gap = 18.f;
        const float contentW = std::max(1.f, w - padX * 2.f);
        const float square = std::min(154.f, std::max(128.f, contentW * 0.15f));
        const float topH = square;
        const float versionW = std::max(260.f, contentW - square * 2.f - gap * 2.f);

        const float topX = x + padX;
        const float topY = y + padY;
        m_checkRect = {topX + versionW + gap, topY, square, topH};
        m_changelogRect = {m_checkRect.x + square + gap, topY, square, topH};

        _drawVersionCard(vg, topX, topY, versionW, topH);
        _drawSquareButton(vg, m_checkRect, material::UPDATE, "检测更新", m_focusedIndex == 0);
        _drawSquareButton(vg, m_changelogRect, material::DESCRIPTION, "更新日志", m_focusedIndex == 1);

        const float dividerY = topY + topH + 30.f;
        nvgBeginPath(vg);
        nvgMoveTo(vg, topX, dividerY);
        nvgLineTo(vg, x + w - padX, dividerY);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 32));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        m_resourceRect = {topX, dividerY + 26.f, contentW, 72.f};
        _drawWideButton(vg, m_resourceRect, material::SEARCH, "在线资源检测", m_focusedIndex == 2);
    }

    void frame(brls::FrameContext* ctx) override {
        brls::View::frame(ctx);

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.5f)
            dt = 0.016f;
        m_animTime += dt;

        if (!this->isFocused() || this->isHidden())
            return;

        this->invalidate();
    }

    void onFocusGained() override {
        brls::View::onFocusGained();
        this->invalidate();
    }

    void onFocusLost() override {
        brls::View::onFocusLost();
        this->invalidate();
    }

private:
    struct Rect {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    std::string m_version;
    std::string m_updateSource;
    std::function<void()> m_onCheckUpdate;
    std::function<void()> m_onChangelog;
    std::function<void()> m_onResourceCheck;

    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_focusedIndex = 0;
    float m_animTime = 0.f;
    std::chrono::steady_clock::time_point m_lastFrameTime;

    Rect m_checkRect;
    Rect m_changelogRect;
    Rect m_resourceRect;

    void _drawVersionCard(NVGcontext* vg, float x, float y, float w, float h) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 14.f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 28));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 13.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 18));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        nvgFontSize(vg, 22.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 28.f, y + 30.f, "当前版本信息", nullptr);

        _drawInfoRow(vg, x + 28.f, y + 66.f, "版本号", m_version);
        _drawInfoRow(vg, x + 28.f, y + 92.f, "更新源", m_updateSource);
    }

    void _drawInfoRow(NVGcontext* vg, float x, float y, const char* label, const std::string& value) {
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 17.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 210));
        nvgText(vg, x, y, label, nullptr);

        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 86.f, y, value.c_str(), nullptr);
    }

    void _drawSquareButton(NVGcontext* vg, const Rect& r, char32_t icon, const char* label, bool focused) {
        _drawButtonBase(vg, r, focused, 14.f);

        const std::string iconText = encodeMaterialIcon(icon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 46.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        nvgText(vg, r.x + r.w * 0.5f, r.y + r.h * 0.38f, iconText.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 17.f);
        nvgFillColor(vg, focused ? nvgRGBA(255, 255, 255, 255) : nvgRGBA(220, 220, 220, 230));
        nvgText(vg, r.x + r.w * 0.5f, r.y + r.h - 28.f, label, nullptr);
    }

    void _drawWideButton(NVGcontext* vg, const Rect& r, char32_t icon, const char* label, bool focused) {
        _drawButtonBase(vg, r, focused, 12.f);

        const float centerY = r.y + r.h * 0.5f;
        const float groupX = r.x + 36.f;
        const std::string iconText = encodeMaterialIcon(icon);

        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 34.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        nvgText(vg, groupX, centerY, iconText.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 20.f);
        nvgFillColor(vg, focused ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        nvgText(vg, groupX + 50.f, centerY, label, nullptr);
    }

    void _drawButtonBase(NVGcontext* vg, const Rect& r, bool focused, float radius) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgFillColor(vg, focused ? nvgRGBA(79, 193, 255, 56) : nvgRGBA(0, 0, 0, 28));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x + 1.f, r.y + 1.f, r.w - 2.f, r.h - 2.f, radius - 1.f);
        nvgStrokeColor(vg, focused ? nvgRGBA(79, 193, 255, 180) : nvgRGBA(255, 255, 255, 18));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        if (focused && this->isFocused()) {
            beiklive::ui::drawGradientFocusBorder(
                vg,
                r.x,
                r.y,
                r.w,
                r.h,
                radius,
                3.f,
                1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_animTime));
        }
    }

    void _setFocus(int index) {
        if (m_focusedIndex == index)
            return;
        m_focusedIndex = index;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
        this->invalidate();
    }

    void _activateFocused() {
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        if (m_focusedIndex == 0 && m_onCheckUpdate)
            m_onCheckUpdate();
        else if (m_focusedIndex == 1 && m_onChangelog)
            m_onChangelog();
        else if (m_focusedIndex == 2 && m_onResourceCheck)
            m_onResourceCheck();
    }

};

static float aboutClamp(float value) {
    return std::max(0.f, std::min(1.f, value));
}

static float aboutSmooth(float value) {
    value = aboutClamp(value);
    return value * value * (3.f - 2.f * value);
}

static float aboutBack(float value) {
    value = aboutClamp(value);
    constexpr float c1 = 1.12f;
    constexpr float c3 = c1 + 1.f;
    const float shifted = value - 1.f;
    return 1.f + c3 * shifted * shifted * shifted
        + c1 * shifted * shifted;
}

static unsigned char aboutAlpha(float value) {
    return static_cast<unsigned char>(255.f * aboutClamp(value));
}

class AboutMainCanvas final : public brls::View {
public:
    AboutMainCanvas(std::string version,
                    std::string updateSource,
                    std::function<void()> onCheckUpdate,
                    std::function<void()> onChangelog,
                    std::function<void()> onResourceCheck,
                    std::function<void()> onBack)
        : m_version(std::move(version))
        , m_updateSource(std::move(updateSource))
        , m_onCheckUpdate(std::move(onCheckUpdate))
        , m_onChangelog(std::move(onChangelog))
        , m_onResourceCheck(std::move(onResourceCheck))
        , m_onBack(std::move(onBack)) {
        this->setFocusable(true);
        this->setGrow(1.f);
        this->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);
        this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
        this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        auto previousTab = [this](brls::View*) -> bool {
            if (_acceptNavigation(1))
                _switchTab(-1);
            return true;
        };
        auto nextTab = [this](brls::View*) -> bool {
            if (_acceptNavigation(2))
                _switchTab(1);
            return true;
        };
        auto moveUp = [this](brls::View*) -> bool {
            if (_acceptNavigation(3))
                _moveFocus(-1);
            return true;
        };
        auto moveDown = [this](brls::View*) -> bool {
            if (_acceptNavigation(4))
                _moveFocus(1);
            return true;
        };

        this->registerAction("", brls::BUTTON_LEFT, previousTab,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_RIGHT, nextTab,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_LEFT, previousTab,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_RIGHT, nextTab,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_LB, previousTab,
                             true, false, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_RB, nextTab,
                             true, false, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_UP, moveUp,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_DOWN, moveDown,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_UP, moveUp,
                             true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_DOWN, moveDown,
                             true, true, brls::SOUND_NONE);
        this->registerAction("打开", brls::BUTTON_A,
            [this](brls::View*) -> bool {
                _activateFocused();
                return true;
            }, false, false, brls::SOUND_NONE);
        this->registerAction("返回", brls::BUTTON_B,
            [this](brls::View*) -> bool {
                _beginClose();
                return true;
            }, false, false, brls::SOUND_NONE);
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

    ~AboutMainCanvas() override {
        if (auto* vg = brls::Application::getNVGContext()) {
            if (m_authorImage > 0)
                nvgDeleteImage(vg, m_authorImage);
            if (m_qqImage > 0)
                nvgDeleteImage(vg, m_qqImage);
            if (m_payImage > 0)
                nvgDeleteImage(vg, m_payImage);
        }
    }

    void frame(brls::FrameContext* ctx) override {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;
        m_time += dt;
        if (m_closing) {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 3.7f);
            if (m_pageEntrance <= 0.f && !m_closeQueued) {
                m_closeQueued = true;
                const auto onBack = m_onBack;
                brls::sync([onBack]() {
                    if (onBack)
                        onBack();
                });
            }
        } else {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 2.8f);
            m_tabEntrance = std::min(1.f, m_tabEntrance + dt * 4.4f);
        }
        if (m_clicking) {
            m_clickTime += dt;
            if (m_clickTime >= 0.22f) {
                m_clicking = false;
                m_clickTime = 0.f;
                _runFocusedAction();
            }
        }
        this->invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override {
        (void)style;
        (void)ctx;
        _ensureAssets(vg);
        _drawHeader(vg, x, y, w);

        const float contentX = x + 36.f;
        const float contentY = y + 112.f;
        const float contentW = w - 72.f;
        const float contentH = h - 178.f;
        const float progress = aboutBack(m_tabEntrance);
        const float contentOffset = static_cast<float>(m_tabDirection)
            * (1.f - progress) * 76.f;
        const float pageProgress = aboutBack(m_pageEntrance);
        const float alpha = aboutSmooth(m_pageEntrance)
            * aboutSmooth(m_tabEntrance);
        const float contentScale = 0.965f + pageProgress * 0.035f;
        const float contentCenterX = contentX + contentW * 0.5f;
        const float contentCenterY = contentY + contentH * 0.5f;

        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        nvgTranslate(vg, contentCenterX + contentOffset,
                     contentCenterY + (1.f - pageProgress) * 22.f);
        nvgScale(vg, contentScale, contentScale);
        nvgTranslate(vg, -contentCenterX, -contentCenterY);
        if (m_tab == 0)
            _drawInfo(vg, contentX, contentY, contentW, contentH);
        else if (m_tab == 1)
            _drawUpdate(vg, contentX, contentY, contentW, contentH);
        else
            _drawSupport(vg, contentX, contentY, contentW, contentH);
        nvgRestore(vg);

        _drawFooter(vg, x, y, w, h);
    }

private:
    struct Rect {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    std::string m_version;
    std::string m_updateSource;
    std::function<void()> m_onCheckUpdate;
    std::function<void()> m_onChangelog;
    std::function<void()> m_onResourceCheck;
    std::function<void()> m_onBack;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_switchFont = -1;
    int m_authorImage = 0;
    int m_qqImage = 0;
    int m_payImage = 0;
    int m_tab = 0;
    int m_tabDirection = 1;
    int m_updateFocus = 0;
    float m_time = 0.f;
    float m_pageEntrance = 0.f;
    float m_tabEntrance = 0.f;
    bool m_clicking = false;
    bool m_closing = false;
    bool m_closeQueued = false;
    float m_clickTime = 0.f;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastNavigationTime;
    int m_lastNavigationAction = 0;

    void _ensureAssets(NVGcontext* vg) {
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        if (m_authorImage == 0)
            m_authorImage = nvgCreateImage(vg, BK_RES("img/beiklive.png").c_str(), 0);
        if (m_qqImage == 0)
            m_qqImage = nvgCreateImage(vg, BK_RES("img/QQ.png").c_str(), 0);
        if (m_payImage == 0)
            m_payImage = nvgCreateImage(vg, BK_RES("img/pay.png").c_str(), 0);
    }

    bool _acceptNavigation(int action) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastNavigationTime).count();
        if (action == m_lastNavigationAction && elapsed >= 0 && elapsed < 90)
            return false;
        m_lastNavigationAction = action;
        m_lastNavigationTime = now;
        return true;
    }

    void _switchTab(int direction) {
        if (m_clicking || m_closing || m_pageEntrance < 0.72f)
            return;
        const int next = (m_tab + direction + 3) % 3;
        if (next == m_tab)
            return;
        m_tab = next;
        m_tabDirection = direction;
        m_tabEntrance = 0.f;
        m_updateFocus = 0;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        this->invalidate();
    }

    void _moveFocus(int direction) {
        if (m_tab != 1 || m_clicking || m_closing
            || m_pageEntrance < 0.72f)
            return;
        const int next = std::clamp(m_updateFocus + direction, 0, 2);
        if (next == m_updateFocus)
            return;
        m_updateFocus = next;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
        this->invalidate();
    }

    void _activateFocused() {
        if (m_tab != 1 || m_clicking || m_closing
            || m_pageEntrance < 0.85f || m_tabEntrance < 0.85f)
            return;
        m_clicking = true;
        m_clickTime = 0.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    void _runFocusedAction() {
        if (m_updateFocus == 0 && m_onCheckUpdate)
            m_onCheckUpdate();
        else if (m_updateFocus == 1 && m_onChangelog)
            m_onChangelog();
        else if (m_updateFocus == 2 && m_onResourceCheck)
            m_onResourceCheck();
    }

    void _beginClose() {
        if (m_closing || m_clicking)
            return;
        m_closing = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
        this->invalidate();
    }

    void _drawExternalShadow(NVGcontext* vg, const Rect& r, float radius,
                             float alpha = 1.f) {
        const NVGpaint shadow = nvgBoxGradient(
            vg, r.x + 5.f, r.y + 6.f, r.w, r.h, radius, 5.f,
            nvgRGBA(0, 0, 0, aboutAlpha(0.32f * alpha)),
            nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, r.x - 3.f, r.y - 3.f, r.w + 16.f, r.h + 17.f);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
    }

    void _drawPanel(NVGcontext* vg, const Rect& r, float radius = 8.f,
                    bool focused = false, float scale = 1.f) {
        Rect draw = r;
        draw.w *= scale;
        draw.h *= scale;
        draw.x += (r.w - draw.w) * 0.5f;
        draw.y += (r.h - draw.h) * 0.5f;
        _drawExternalShadow(vg, draw, radius, 0.85f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, draw.x, draw.y, draw.w, draw.h, radius);
        nvgFillColor(vg, focused
            ? nvgRGBA(79, 193, 255, 36) : nvgRGBA(255, 255, 255, 7));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, draw.x + 1.f, draw.y + 1.f,
                       draw.w - 2.f, draw.h - 2.f, std::max(1.f, radius - 1.f));
        nvgStrokeColor(vg, focused
            ? nvgRGBA(255, 255, 255, 150) : nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
        if (focused && this->isFocused()) {
            beiklive::ui::drawGradientFocusBorder(
                vg, draw.x, draw.y, draw.w, draw.h, radius, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
        }
    }

    void _drawHeader(NVGcontext* vg, float x, float y, float w) {
        const float headerProgress = aboutBack(m_pageEntrance / 0.72f);
        const float headerY = y + 25.f - (1.f - headerProgress) * 58.f;
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 25.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, headerY + 24.f, "GBAStation", nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 180));
        const std::string version = "v" + m_version;
        nvgText(vg, x + 36.f, headerY + 52.f, version.c_str(), nullptr);

        constexpr const char* labels[] = {
            "项目信息", "更新与资源", "支持作者"
        };
        constexpr float tabW = 154.f;
        constexpr float gap = 12.f;
        constexpr float tabH = 48.f;
        const float totalW = tabW * 3.f + gap * 2.f;
        const float startX = x + (w - totalW) * 0.5f;
        for (int index = 0; index < 3; ++index) {
            const float tx = startX + index * (tabW + gap);
            const bool selected = index == m_tab;
            if (selected) {
                const Rect tabRect{tx, headerY + 10.f, tabW, tabH};
                _drawExternalShadow(vg, tabRect, 7.f, 0.55f);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, tx, headerY + 10.f, tabW, tabH, 7.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 15));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, tx + 1.f, headerY + 11.f,
                               tabW - 2.f, tabH - 2.f, 6.f);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 72));
                nvgStrokeWidth(vg, 1.5f);
                nvgStroke(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, tx + 26.f, headerY + 54.f,
                               tabW - 52.f, 3.f, 1.5f);
                nvgFillColor(vg, nvgRGBA(79, 193, 255, 230));
                nvgFill(vg);
            }
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, selected ? 20.f : 18.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected
                ? nvgRGBA(255, 255, 255, 250)
                : nvgRGBA(210, 216, 226, 175));
            nvgText(vg, tx + tabW * 0.5f, headerY + 34.f,
                    labels[index], nullptr);
        }

        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 36.f, y + 95.f);
        nvgLineTo(vg, x + w - 36.f, y + 95.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
            aboutAlpha(0.20f * aboutSmooth(m_pageEntrance))));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

    void _drawImageFit(NVGcontext* vg, int image, const Rect& r,
                       float radius, float alpha = 1.f) {
        if (image <= 0)
            return;
        int iw = 0;
        int ih = 0;
        nvgImageSize(vg, image, &iw, &ih);
        if (iw <= 0 || ih <= 0)
            return;
        const float scale = std::min(r.w / static_cast<float>(iw),
                                     r.h / static_cast<float>(ih));
        const float dw = iw * scale;
        const float dh = ih * scale;
        const float dx = r.x + (r.w - dw) * 0.5f;
        const float dy = r.y + (r.h - dh) * 0.5f;
        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, dx, dy, dw, dh, radius);
        nvgFillPaint(vg, nvgImagePattern(
            vg, dx, dy, dw, dh, 0.f, image, 1.f));
        nvgFill(vg);
        nvgRestore(vg);
    }

    void _drawAvatar(NVGcontext* vg, float cx, float cy, float size) {
        if (m_authorImage <= 0)
            return;
        int iw = 0;
        int ih = 0;
        nvgImageSize(vg, m_authorImage, &iw, &ih);
        if (iw <= 0 || ih <= 0)
            return;
        const float scale = std::max(size / iw, size / ih);
        const float dw = iw * scale;
        const float dh = ih * scale;
        const float dx = cx - dw * 0.5f;
        const float dy = cy - dh * 0.5f;
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, size * 0.5f);
        nvgFillPaint(vg, nvgImagePattern(
            vg, dx, dy, dw, dh, 0.f, m_authorImage, 1.f));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, size * 0.5f - 1.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 90));
        nvgStrokeWidth(vg, 2.f);
        nvgStroke(vg);
    }

    void _drawBadge(NVGcontext* vg, float x, float y,
                    const char* text, NVGcolor color) {
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 15.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, text, nullptr, bounds);
        const float width = bounds[2] - bounds[0] + 24.f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, 28.f, 6.f);
        nvgFillColor(vg, nvgRGBAf(color.r, color.g, color.b, 28.f / 255.f));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 1.f, y + 1.f, width - 2.f, 26.f, 5.f);
        nvgStrokeColor(vg, nvgRGBAf(color.r, color.g, color.b, 125.f / 255.f));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(245, 248, 252, 235));
        nvgText(vg, x + width * 0.5f, y + 14.f, text, nullptr);
    }

    void _drawInfo(NVGcontext* vg, float x, float y, float w, float h) {
        const float gap = 20.f;
        const float leftW = std::min(345.f, w * 0.30f);
        const Rect author{x, y, leftW, h};
        const Rect project{x + leftW + gap, y, w - leftW - gap, h};
        _drawPanel(vg, author);
        _drawPanel(vg, project);

        _drawAvatar(vg, author.x + author.w * 0.5f, author.y + 92.f, 108.f);
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 27.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, author.x + author.w * 0.5f,
                author.y + 170.f, "beiklive", nullptr);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 190));
        nvgText(vg, author.x + author.w * 0.5f,
                author.y + 207.f, "项目作者与维护者", nullptr);

        const Rect github{author.x + 24.f, author.y + 250.f,
                          author.w - 48.f, 62.f};
        const Rect bilibili{author.x + 24.f, author.y + 328.f,
                            author.w - 48.f, 62.f};
        nvgBeginPath(vg);
        nvgRoundedRect(vg, github.x, github.y, github.w, github.h, 7.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 22));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bilibili.x, bilibili.y,
                       bilibili.w, bilibili.h, 7.f);
        nvgFillColor(vg, nvgRGBA(0, 188, 212, 20));
        nvgFill(vg);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, github.x + 18.f, github.y + 21.f, "GitHub", nullptr);
        nvgText(vg, bilibili.x + 18.f, bilibili.y + 21.f, "BiliBili", nullptr);
        nvgFontSize(vg, 14.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 185));
        nvgText(vg, github.x + 18.f, github.y + 44.f,
                "beiklive/GBAStation", nullptr);
        nvgText(vg, bilibili.x + 18.f, bilibili.y + 44.f,
                "BEIKLIVE", nullptr);

        const float px = project.x + 30.f;
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontSize(vg, 25.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, px, project.y + 27.f, "关于本项目", nullptr);
        nvgFontSize(vg, 17.f);
        nvgFillColor(vg, nvgRGBA(220, 225, 234, 210));
        nvgTextBox(vg, px, project.y + 68.f, project.w - 60.f,
            "GBAStation 是面向 Switch 平台的模拟器前端，统一管理游戏、核心、存档、封面与输入配置；3DS 游戏由 Azahar 独立运行时启动。", nullptr);

        float badgeX = px;
        const float badgeY = project.y + 126.f;
        const std::array<std::pair<const char*, NVGcolor>, 7> badges{{
            {"GB / GBC", nvgRGB(79, 193, 255)},
            {"GBA", nvgRGB(0, 188, 212)},
            {"FC", nvgRGB(255, 119, 168)},
            {"SFC", nvgRGB(150, 130, 255)},
            {"NDS", nvgRGB(100, 220, 150)},
            {"3DS", nvgRGB(230, 79, 91)},
            {"PICO-8", nvgRGB(255, 190, 80)},
        }};
        for (const auto& badge : badges) {
            _drawBadge(vg, badgeX, badgeY, badge.first, badge.second);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 15.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, badge.first, nullptr, bounds);
            badgeX += bounds[2] - bounds[0] + 36.f;
        }

        nvgBeginPath(vg);
        nvgMoveTo(vg, px, project.y + 178.f);
        nvgLineTo(vg, project.x + project.w - 30.f, project.y + 178.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 34));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        constexpr const char* features[] = {
            "游戏库、封面与游玩记录",
            "目录扫描与 RetroArch 导入",
            "即时、自动存档与倒带",
            "按机型独立输入映射",
            "金手指与多核心切换",
            "着色器、遮罩与画面模式",
            "远程管理与资源检测",
            "原生 NDS、3DS 与 PICO-8 运行时",
        };
        for (int index = 0; index < 8; ++index) {
            const int column = index % 2;
            const int row = index / 2;
            const float fx = px + column * (project.w - 60.f) * 0.5f;
            const float fy = project.y + 215.f + row * 58.f;
            const std::string icon = encodeMaterialIcon(material::CHECK_BOX);
            nvgFontFaceId(vg, m_materialFont);
            nvgFontSize(vg, 24.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(79, 193, 255, 225));
            nvgText(vg, fx, fy, icon.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 16.f);
            nvgFillColor(vg, nvgRGBA(235, 238, 244, 225));
            nvgText(vg, fx + 34.f, fy, features[index], nullptr);
        }
    }

    void _drawUpdate(NVGcontext* vg, float x, float y, float w, float h) {
        const float gap = 22.f;
        const float leftW = std::min(390.f, w * 0.34f);
        const Rect version{x, y, leftW, h};
        _drawPanel(vg, version);
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontSize(vg, 20.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 190));
        nvgText(vg, version.x + 28.f, version.y + 30.f,
                "当前版本", nullptr);
        nvgFontSize(vg, 46.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        const std::string versionText = "v" + m_version;
        nvgText(vg, version.x + 28.f, version.y + 72.f,
                versionText.c_str(), nullptr);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 185));
        nvgText(vg, version.x + 28.f, version.y + 145.f,
                "更新源", nullptr);
        nvgFontSize(vg, 19.f);
        nvgFillColor(vg, nvgRGBA(245, 248, 252, 235));
        nvgTextBox(vg, version.x + 28.f, version.y + 174.f,
                   version.w - 56.f, m_updateSource.c_str(), nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, version.x + 28.f, version.y + 236.f);
        nvgLineTo(vg, version.x + version.w - 28.f, version.y + 236.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 34));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 195));
        nvgTextBox(vg, version.x + 28.f, version.y + 270.f,
                   version.w - 56.f,
                   "程序更新与资源更新相互独立。在线资源可单独检查并安装，不会覆盖用户配置。",
                   nullptr);

        const float actionsX = x + leftW + gap;
        const float actionsW = w - leftW - gap;
        const float itemGap = 18.f;
        const float itemH = (h - itemGap * 2.f) / 3.f;
        constexpr char32_t icons[] = {
            material::UPDATE, material::DESCRIPTION, material::SEARCH
        };
        constexpr const char* titles[] = {
            "检测程序更新", "查看更新日志", "在线资源检测"
        };
        constexpr const char* descriptions[] = {
            "检查新版本并进入下载安装流程",
            "浏览当前版本包含的功能与修复",
            "检测 BIOS、数据库和扩展资源"
        };
        for (int index = 0; index < 3; ++index) {
            const Rect item{actionsX,
                y + index * (itemH + itemGap), actionsW, itemH};
            const bool focused = index == m_updateFocus;
            float clickScale = 1.f;
            if (focused && m_clicking)
                clickScale = 1.f - 0.035f * std::sin(
                    3.14159265f * aboutClamp(m_clickTime / 0.22f));
            _drawPanel(vg, item, 8.f, focused, clickScale);
            const std::string icon = encodeMaterialIcon(icons[index]);
            nvgFontFaceId(vg, m_materialFont);
            nvgFontSize(vg, 42.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, focused
                ? nvgRGBA(255, 255, 255, 255)
                : nvgRGBA(220, 225, 234, 210));
            nvgText(vg, item.x + 58.f, item.y + item.h * 0.5f,
                    icon.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFontSize(vg, 22.f);
            nvgFillColor(vg, focused
                ? nvgRGBA(255, 255, 255, 255)
                : GET_THEME_COLOR("brls/text"));
            nvgText(vg, item.x + 108.f, item.y + item.h * 0.42f,
                    titles[index], nullptr);
            nvgFontSize(vg, 15.f);
            nvgFillColor(vg, nvgRGBA(210, 216, 226,
                focused ? 225 : 175));
            nvgText(vg, item.x + 108.f, item.y + item.h * 0.68f,
                    descriptions[index], nullptr);
        }
    }

    void _drawSupport(NVGcontext* vg, float x, float y, float w, float h) {
        const float gap = 22.f;
        const float leftW = std::min(390.f, w * 0.34f);
        const Rect message{x, y, leftW, h};
        const Rect payment{x + leftW + gap, y, w - leftW - gap, h};
        _drawPanel(vg, message);
        _drawPanel(vg, payment);

        _drawAvatar(vg, message.x + message.w * 0.5f,
                    message.y + 82.f, 86.f);
        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFontSize(vg, 24.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, message.x + message.w * 0.5f,
                message.y + 142.f, "感谢你的支持", nullptr);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(215, 220, 230, 195));
        nvgTextBox(vg, message.x + 34.f, message.y + 190.f,
                   message.w - 68.f,
                   "反馈、测试与分享同样是对项目的重要帮助。也许下一次更新的灵感，就来自你的建议。",
                   nullptr);
        nvgFontSize(vg, 18.f);
        nvgFillColor(vg, nvgRGBA(245, 248, 252, 235));
        nvgText(vg, message.x + message.w * 0.5f,
                message.y + 300.f, "交流与反馈", nullptr);
        _drawImageFit(vg, m_qqImage,
            {message.x + 24.f, message.y + 330.f,
             message.w - 48.f, std::min(150.f, h - 350.f)}, 6.f);

        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontSize(vg, 24.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, payment.x + 30.f, payment.y + 25.f,
                "请作者喝杯咖啡", nullptr);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 190));
        nvgText(vg, payment.x + 30.f, payment.y + 60.f,
                "所有支持都会用于项目维护与功能开发", nullptr);
        _drawImageFit(vg, m_payImage,
            {payment.x + 28.f, payment.y + 96.f,
             payment.w - 56.f, payment.h - 122.f}, 7.f);
    }

    void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                   const char* label, float& cursor, float y, float alpha) {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
        const float labelW = bounds[2] - bounds[0];
        cursor -= labelW + 43.f;
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, aboutAlpha(alpha)));
        nvgText(vg, cursor + 13.f, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(230, 234, 241, aboutAlpha(alpha)));
        nvgText(vg, cursor + 30.f, y, label, nullptr);
        cursor -= 16.f;
    }

    void _drawFooter(NVGcontext* vg, float x, float y, float w, float h) {
        const float alpha = aboutSmooth(m_pageEntrance);
        const float hintY = y + h - 27.f;
        float cursor = x + w - 32.f;
        _drawHint(vg, brls::BUTTON_B, "返回", cursor, hintY, alpha);
        if (m_tab == 1)
            _drawHint(vg, brls::BUTTON_A, "打开", cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_RB, "下一页", cursor, hintY, alpha);
        _drawHint(vg, brls::BUTTON_LB, "上一页", cursor, hintY, alpha);
    }
};

AboutPage::AboutPage() {
    brls::sync([this]() {
        this->showFooter(false);
        this->showHeader(false);
        const std::string localVersion = APP_VERSION;
        const std::string changelogText = readTextFile(
            BK_RES("changelog"), "暂无更新日志");
        m_aboutCanvas = new AboutMainCanvas(
            localVersion,
            "download.nswiki.cn",
            [this]() { _checkUpdate(); },
            [localVersion, changelogText]() {
                openChangelogApplet(
                    "当前版本更新内容  " + localVersion,
                    changelogText.empty() ? "暂无更新日志" : changelogText);
            },
            [this]() { checkOnlineResources(this); },
            [this]() { beiklive::popActivity(this); });
        this->getContentBox()->addView(m_aboutCanvas);
        brls::Application::giveFocus(m_aboutCanvas);
    });
}

// ── 关于本项目 ─────────────────────────────────────────────

brls::View* AboutPage::_buildInfoTab() {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);

    // 作者卡片
    auto* authorCard = new brls::Box(brls::Axis::ROW);
    authorCard->setCornerRadius(16.f);
    authorCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
    authorCard->setShadowVisibility(true);
    authorCard->setShadowType(brls::ShadowType::GENERIC);
    authorCard->setPadding(24.f, 36.f, 24.f, 36.f);
    authorCard->setAlignItems(brls::AlignItems::CENTER);
    authorCard->setFocusable(true);
    authorCard->setHideHighlightBackground(true);
    authorCard->setHideHighlightBorder(true);
    authorCard->setHeight(brls::View::AUTO);

    auto* authorImage = new brls::Image();
    authorImage->setImageFromFile(BK_RES("img/beiklive.png"));
    authorImage->setWidth(80.f);
    authorImage->setHeight(80.f);
    authorImage->setCornerRadius(40.f);
    authorImage->setScalingType(brls::ImageScalingType::FIT);
    authorImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    authorImage->setFocusable(false);
    authorImage->setMarginRight(30.f);

    auto* infoBox = new brls::Box(brls::Axis::COLUMN);
    infoBox->setAlignItems(brls::AlignItems::FLEX_START);
    infoBox->setJustifyContent(brls::JustifyContent::CENTER);
    infoBox->setFocusable(false);

    auto* nameLabel = new brls::Label();
    nameLabel->setText("beiklive");
    nameLabel->setFontSize(28.f);
    nameLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    nameLabel->setMarginBottom(16.f);
    nameLabel->setFocusable(false);

    auto* githubLabel = new brls::Label();
    githubLabel->setText("GitHub:  https://github.com/beiklive/GBAStation");
    githubLabel->setFontSize(18.f);
    githubLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    githubLabel->setFocusable(false);

    auto* githubBadge = new brls::Box(brls::Axis::ROW);
    githubBadge->setCornerRadius(8.f);
    githubBadge->setBackgroundColor(nvgRGBA(79, 193, 255, 30));
    githubBadge->setPadding(6.f, 12.f, 6.f, 12.f);
    githubBadge->setMarginBottom(10.f);
    githubBadge->setFocusable(false);
    githubBadge->setHideHighlightBackground(true);
    githubBadge->addView(githubLabel);

    auto* biliLabel = new brls::Label();
    biliLabel->setText("BiliBili:   BEIKLIVE");
    biliLabel->setFontSize(18.f);
    biliLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    biliLabel->setFocusable(false);

    auto* biliBadge = new brls::Box(brls::Axis::ROW);
    biliBadge->setCornerRadius(8.f);
    biliBadge->setBackgroundColor(nvgRGBA(0, 168, 107, 30));
    biliBadge->setPadding(6.f, 12.f, 6.f, 12.f);
    biliBadge->setFocusable(false);
    biliBadge->setHideHighlightBackground(true);
    biliBadge->addView(biliLabel);

    infoBox->addView(nameLabel);
    infoBox->addView(githubBadge);
    infoBox->addView(biliBadge);

    authorCard->addView(authorImage);
    authorCard->addView(infoBox);
    box->addView(authorCard);

    // 项目说明
    auto* sectionHeader = new brls::Header();
    sectionHeader->setTitle("关于本项目");
    sectionHeader->setMarginTop(30.f);
    sectionHeader->setMarginBottom(15.f);
    box->addView(sectionHeader);

    auto* descCard = new brls::Box(brls::Axis::COLUMN);
    descCard->setCornerRadius(16.f);
    descCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
    descCard->setShadowVisibility(true);
    descCard->setShadowType(brls::ShadowType::GENERIC);
    descCard->setPadding(20.f, 24.f, 20.f, 24.f);
    descCard->setFocusable(false);
    descCard->setHideHighlightBackground(true);
    descCard->setHideHighlightBorder(true);
    descCard->setHeight(brls::View::AUTO);

    std::vector<std::string> descLines = {
        "GBAStation 是一个基于 borealis UI 的跨平台模拟器前端，整合 libretro 核心并移植 melonDS 核心代码。",
        "当前支持 GB、GBC、GBA、FC、SFC、NDS 与 3DS（NDS/3DS 性能仍在优化中）。",
        "内置核心包含 mGBA、Nestopia、FCEUmm、Snes9x 2005、Snes9x、melonDS 与 Azahar。",
        "",
        "目前已实现功能：",
        "  •  游戏库功能、游戏封面、游玩时长、游戏次数",
        "  •  支持目录扫描、RetroArch 游戏库导入、Web 局域网管理游戏库与封面自定义",
        "  •  支持即时存档 / 读档、自动存档 / 自动存读档",
        "  •  支持金手指（不支持raw格式）",
        "  •  按机型独立按键映射（含 3DS 双摇杆与 ZL / ZR）、A / B 连发",
        "  •  快进、倒带",
        "  •  遮罩、RetroArch GLSL 着色器与参数调整",
        "  •  多种画面模式"
    };

    for (const auto& line : descLines) {
        auto* label = new brls::Label();
        label->setText(line);
        label->setFontSize(20.f);
        label->setHeight(line.empty() ? 8.f : 26.f);
        label->setWidth(brls::View::AUTO);
        label->setTextColor(GET_THEME_COLOR("brls/text"));
        label->setFocusable(false);
        descCard->addView(label);
    }

    box->addView(descCard);
    box->addView(new brls::Padding());
    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

// ── 更新 ──────────────────────────────────────────────────

brls::View* AboutPage::_buildUpdateTab() {
    std::string localVersion = APP_VERSION;
    std::string changelogText = readTextFile(BK_RES("changelog"), "暂无更新日志");

    return new UpdateTabCanvas(
        localVersion,
        "download.nswiki.cn",
        [this]() {
            _checkUpdate();
        },
        [localVersion, changelogText]() {
            openChangelogApplet(
                "当前版本更新内容  " + localVersion,
                changelogText.empty() ? "暂无更新日志" : changelogText);
        },
        [this]() {
            checkOnlineResources(this);
        });
}

void AboutPage::_checkUpdate() {
    // 显示检测中弹窗
    auto* dlg = new brls::Dialog("正在检测更新...\n\n请稍候");
    dlg->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(dlg);
    dlg->open();

    new std::thread([dlg]() {
        auto& updater = AppUpdater::instance();
        updater.checkSync();

        brls::sync([dlg]() {
            // 关闭检测中弹窗
            dlg->close([]{});

            auto& info = AppUpdater::instance().info();
            if (info.hasUpdate) {
                auto* confirmDlg = new beiklive::UpdateDialog(
                    "版本更新  " + info.version,
                    info.changelog
                );
                confirmDlg->addButton("更新", []() {
                    brls::sync([]() {
                        auto* dialog = new UpdatePage();
                        dialog->open();
                        brls::sync([dialog]() {
                            dialog->startDownload();
                        });
                    });
                });
                confirmDlg->addButton("取消", []() {});
                confirmDlg->open();
            } else if (!info.version.empty()) {
                auto* latestDlg = new brls::Dialog(
                    "当前已是最新版本，是否再次更新？");
                latestDlg->addButton("再次更新", []() {
                    brls::sync([]() {
                        auto* dialog = new UpdatePage();
                        dialog->open();
                        brls::sync([dialog]() {
                            dialog->startDownload();
                        });
                    });
                });
                latestDlg->addButton("取消", []() {});
                latestDlg->open();
            } else {
                auto* okDlg = new brls::Dialog("更新检测失败，请检查网络后重试");
                okDlg->addButton("确定", []() {});
                okDlg->open();
            }
        });
    });
}

void AboutPage::_updateCheatDatabase() {
    auto* cancelFlag = new std::atomic<bool>(false);

    auto* prog = new beiklive::ProgressDialog("正在更新金手指数据库...",
        [cancelFlag]() { cancelFlag->store(true); });
    auto* frame = new brls::AppletFrame(prog);
    HIDE_BRLS_BAR(frame);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);

    new std::thread([prog, cancelFlag]() {
        static const char* kUrl = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/cheat_db/cheat_db.zip";

        std::string dbDir = beiklive::path::dbsPath();
        std::error_code ec;
        std::filesystem::create_directories(dbDir, ec);

        std::string zipPath = dbDir + beiklive::path::SPLIT_CHAR + "cheat_db.zip";

        // ── 下载 ──
        brls::sync([prog]() { prog->setStatus("正在下载..."); });

        bool downloadOk = false;
        {
            CURL* curl = curl_easy_init();
            if (curl && !cancelFlag->load()) {
                std::vector<uint8_t> body;
                curl_easy_setopt(curl, CURLOPT_URL, kUrl);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                    static_cast<size_t(*)(void*, size_t, size_t, void*)>(
                        [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                            auto* v = static_cast<std::vector<uint8_t>*>(userdata);
                            v->insert(v->end(), (uint8_t*)ptr, (uint8_t*)ptr + size * nmemb);
                            return size * nmemb;
                        }));
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

                if (!cancelFlag->load()) {
                    CURLcode res = curl_easy_perform(curl);
                    long code = 0;
                    if (res == CURLE_OK)
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
                    if (res == CURLE_OK && code == 200 && !body.empty()) {
                        std::ofstream out(zipPath, std::ios::binary | std::ios::trunc);
                        if (out) {
                            out.write(reinterpret_cast<const char*>(body.data()), body.size());
                            out.close();
                            downloadOk = true;
                        }
                    }
                }
                curl_easy_cleanup(curl);
            }
        }

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            return;
        }

        if (!downloadOk) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                prog->showResult("下载失败，请稍后重试或者去网盘手动下载");
            });
            return;
        }

        // ── 解压 ──
        int extractCount = 0;
        std::string nestedZipPath;
        {
            brls::sync([prog]() { prog->setStatus("正在解压..."); });

            mz_zip_archive zip;
            memset(&zip, 0, sizeof(zip));
            if (mz_zip_reader_init_file(&zip, zipPath.c_str(), 0)) {
                mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
                for (mz_uint i = 0; i < numFiles && !cancelFlag->load(); ++i) {
                    char filename[256];
                    mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
                    std::string name = filename;

                    if (name == "RetroArch.zip") {
                        nestedZipPath = dbDir + beiklive::path::SPLIT_CHAR + "RetroArch.zip";
                        if (mz_zip_reader_extract_to_file(&zip, i, nestedZipPath.c_str(), 0))
                            ++extractCount;
                    } else {
                        std::string outPath = dbDir + beiklive::path::SPLIT_CHAR + name;
                        if (mz_zip_reader_extract_to_file(&zip, i, outPath.c_str(), 0))
                            ++extractCount;
                    }
                }
                mz_zip_reader_end(&zip);
            }
        }

        // 解压嵌套的 RetroArch.zip
        if (!nestedZipPath.empty() && !cancelFlag->load()) {
            brls::sync([prog]() { prog->setStatus("正在解压 RetroArch.zip..."); });

            std::string retroArchDir = beiklive::path::cheatPath() + "/RetroArch";
            std::filesystem::create_directories(retroArchDir, ec);

            mz_zip_archive nestedZip;
            memset(&nestedZip, 0, sizeof(nestedZip));
            if (mz_zip_reader_init_file(&nestedZip, nestedZipPath.c_str(), 0)) {
                mz_uint nestedCount = mz_zip_reader_get_num_files(&nestedZip);
                for (mz_uint i = 0; i < nestedCount && !cancelFlag->load(); ++i) {
                    char fn[512];
                    mz_zip_reader_get_filename(&nestedZip, i, fn, sizeof(fn));
                    std::string outPath = retroArchDir + "/" + fn;
                    mz_zip_reader_extract_to_file(&nestedZip, i, outPath.c_str(), 0);
                }
                mz_zip_reader_end(&nestedZip);
            }
            std::filesystem::remove(nestedZipPath, ec);
        }

        std::filesystem::remove(zipPath, ec);

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            return;
        }

        brls::sync([prog, cancelFlag, extractCount]() {
            delete cancelFlag;
            prog->setText("更新完成");
            std::string msg = "数据库已更新（解压 " + std::to_string(extractCount) + " 个文件）";
            if (extractCount > 0)
                msg = "数据库已更新（解压 " + std::to_string(extractCount) + " 个文件），\n金手指文件已就绪";
            prog->showResult(msg);
        });
    });
}

void AboutPage::_downloadNdsFirmware() {
    static const char* kUrl = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/firmware/nds.zip";
    const std::array<std::string, 3> firmwareFiles = {
        "bios7.bin",
        "bios9.bin",
        "firmware.bin",
    };

    std::error_code ec;
    const auto ndsDir = std::filesystem::path(beiklive::path::biosPath()) / "nds";
    std::filesystem::create_directories(ndsDir, ec);
    if (ec) {
        showMessageDialog("创建 NDS 固件目录失败：\n" + ndsDir.string());
        return;
    }

    bool allExists = true;
    for (const auto& file : firmwareFiles) {
        if (!std::filesystem::exists(ndsDir / file)) {
            allExists = false;
            break;
        }
    }

    if (allExists) {
        showMessageDialog("NDS 固件文件已存在，无需下载");
        return;
    }

    auto* cancelFlag = new std::atomic<bool>(false);
    auto* prog = new beiklive::ProgressDialog("正在下载 NDS 固件...",
        [cancelFlag]() { cancelFlag->store(true); });
    auto* frame = new brls::AppletFrame(prog);
    HIDE_BRLS_BAR(frame);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);

    new std::thread([prog, cancelFlag, ndsDir, firmwareFiles]() {
        std::error_code ec;
        const auto cacheDir = std::filesystem::path(beiklive::path::cachePath());
        std::filesystem::create_directories(cacheDir, ec);
        if (ec) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                prog->showResult("创建缓存目录失败");
            });
            return;
        }

        const auto zipPath = cacheDir / "nds_firmware.zip";

        brls::sync([prog]() { prog->setStatus("正在下载..."); });
        if (!downloadFileToPath(kUrl, zipPath.string(), cancelFlag)) {
            if (cancelFlag->load()) {
                brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            } else {
                brls::sync([prog, cancelFlag]() {
                    delete cancelFlag;
                    prog->showResult("下载失败，请稍后重试或者去网盘手动下载");
                });
            }
            return;
        }

        brls::sync([prog]() { prog->setStatus("正在解压..."); });
        std::vector<std::string> expectedFiles(firmwareFiles.begin(), firmwareFiles.end());
        int extractCount = 0;
        bool extractOk = extractZipFilesToDir(zipPath.string(), ndsDir.string(), expectedFiles,
                                              cancelFlag, extractCount);
        std::filesystem::remove(zipPath, ec);

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            return;
        }

        brls::sync([prog, cancelFlag, extractOk, extractCount]() {
            delete cancelFlag;
            prog->setText(extractOk ? "下载完成" : "解压失败");
            if (extractOk) {
                prog->showResult("NDS 固件已就绪（解压 " + std::to_string(extractCount) + " 个文件）");
            } else {
                prog->showResult("解压失败，压缩包中缺少必要的 NDS 固件文件");
            }
        });
    });
}

void AboutPage::_downloadNdsCheatDatabase() {
    static const char* kUrl = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/cheat_db/usrcheat.zip";
    static const char* kCheatFile = "usrcheat.dat";

    std::error_code ec;
    const auto cheatDir = std::filesystem::path(beiklive::path::cheatPath());
    std::filesystem::create_directories(cheatDir, ec);
    if (ec) {
        showMessageDialog("创建金手指目录失败：\n" + cheatDir.string());
        return;
    }

    const auto cheatPath = cheatDir / kCheatFile;
    if (std::filesystem::exists(cheatPath)) {
        showMessageDialog("NDS 金手指文件已存在，无需下载");
        return;
    }

    auto* cancelFlag = new std::atomic<bool>(false);
    auto* prog = new beiklive::ProgressDialog("正在下载 NDS 金手指...",
        [cancelFlag]() { cancelFlag->store(true); });
    auto* frame = new brls::AppletFrame(prog);
    HIDE_BRLS_BAR(frame);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);

    new std::thread([prog, cancelFlag, cheatDir]() {
        std::error_code ec;
        const auto cacheDir = std::filesystem::path(beiklive::path::cachePath());
        std::filesystem::create_directories(cacheDir, ec);
        if (ec) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                prog->showResult("创建缓存目录失败");
            });
            return;
        }

        const auto zipPath = cacheDir / "usrcheat.zip";

        brls::sync([prog]() { prog->setStatus("正在下载..."); });

        if (!downloadFileToPath(kUrl, zipPath.string(), cancelFlag)) {
            if (cancelFlag->load()) {
                brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            } else {
                brls::sync([prog, cancelFlag]() {
                    delete cancelFlag;
                    prog->showResult("下载失败，请稍后重试或者去网盘手动下载");
                });
            }
            return;
        }

        brls::sync([prog]() { prog->setStatus("正在解压..."); });
        int extractCount = 0;
        bool extractOk = extractZipFilesToDir(zipPath.string(), cheatDir.string(), {kCheatFile},
                                              cancelFlag, extractCount);
        std::filesystem::remove(zipPath, ec);

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            return;
        }

        brls::sync([prog, cancelFlag, extractOk]() {
            delete cancelFlag;
            prog->setText(extractOk ? "下载完成" : "解压失败");
            prog->showResult(extractOk
                ? "NDS 金手指文件已就绪"
                : "解压失败，压缩包中缺少 usrcheat.dat");
        });
    });
}

// ── 支持作者 ─────────────────────────────────────────────

brls::View* AboutPage::_buildSupportTab() {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);
    box->setAlignItems(brls::AlignItems::CENTER);
    box->setJustifyContent(brls::JustifyContent::CENTER);
    box->setFocusable(false);

    auto* label1 = new brls::Label();
    label1->setText("喜欢这个项目的话，不妨请作者喝杯咖啡吧");
    label1->setFontSize(20.f);
    label1->setTextColor(GET_THEME_COLOR("brls/text"));
    label1->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    label1->setMarginBottom(16.f);
    label1->setFocusable(false);
    box->addView(label1);

    auto* label2 = new brls::Label();
    label2->setText("也许下一次更新的灵感，就来自这杯咖啡里的能量");
    label2->setFontSize(14.f);
    label2->setTextColor(nvgRGBA(200, 200, 200, 200));
    label2->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    label2->setMarginBottom(32.f);
    label2->setFocusable(false);
    box->addView(label2);

    auto* QQImage = new brls::Image();
    QQImage->setImageFromFile(BK_RES("img/QQ.png"));
    QQImage->setScalingType(brls::ImageScalingType::FIT);
    QQImage->setInterpolation(brls::ImageInterpolation::NEAREST);
    QQImage->setCornerRadius(16.f);
    QQImage->setWidth(400.f);
    QQImage->setHeight(150.f);
    QQImage->setFocusable(false);
    box->addView(QQImage);


    auto* payImage = new brls::Image();
    payImage->setImageFromFile(BK_RES("img/pay.png"));
    payImage->setScalingType(brls::ImageScalingType::FIT);
    payImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    payImage->setCornerRadius(16.f);
    payImage->setWidth(800.f);
    payImage->setHeight(400.f);
    payImage->setFocusable(false);
    box->addView(payImage);

    box->addView(new brls::Padding());
    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}


} // namespace beiklive
