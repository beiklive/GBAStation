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

namespace beiklive {

static constexpr const char* RESOURCE_MANIFEST_URL =
    "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/emu_res/res_version.json";

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
    return url + (url.find('?') == std::string::npos ? "?t=" : "&t=")
        + std::to_string(timestamp);
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
    return std::filesystem::path(beiklive::path::ROOT)
        / beiklive::path::PROGRAM_NAME / "update" / "res_version.ini";
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
                               const std::atomic<bool>* cancelFlag = nullptr) {
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

static bool extractZipToDirectory(const std::filesystem::path& zipPath,
                                  const std::filesystem::path& outputDirectory,
                                  const std::atomic<bool>* cancelFlag,
                                  int& extractedCount) {
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

        std::string entryName = stat.m_filename;
        std::replace(entryName.begin(), entryName.end(), '\\', '/');
        const std::filesystem::path relativePath(entryName);
        if (!isSafeZipEntry(relativePath)) {
            success = false;
            break;
        }

        const auto outputPath = outputDirectory / relativePath;
        if (mz_zip_reader_is_file_a_directory(&zip, index)) {
            std::filesystem::create_directories(outputPath, ec);
            if (ec) {
                success = false;
                break;
            }
            continue;
        }

        std::filesystem::create_directories(outputPath.parent_path(), ec);
        if (ec || !mz_zip_reader_extract_to_file(&zip, index, outputPath.string().c_str(), 0)) {
            success = false;
            break;
        }
        ++extractedCount;
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
                                      std::string& resultText) {
    const std::filesystem::path targetDirectory(item.path);
    std::error_code ec;
    std::filesystem::create_directories(targetDirectory, ec);
    if (ec) {
        resultText = "创建目标目录失败：\n" + targetDirectory.string();
        return false;
    }

    if (item.type == "zip") {
        int extractedCount = 0;
        if (!extractZipToDirectory(downloadedPath, targetDirectory, cancelFlag, extractedCount)) {
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

static void openChangelogApplet(const std::string& title, const std::string& content) {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setWidthPercentage(100.f);
    box->setHeightPercentage(100.f);
    box->setPadding(26.f, 34.f, 26.f, 34.f);
    box->setAlignItems(brls::AlignItems::FLEX_START);
    box->setJustifyContent(brls::JustifyContent::FLEX_START);

    auto* bodyLabel = new brls::Label();
    bodyLabel->setText(content.empty() ? "暂无更新日志" : content);
    bodyLabel->setFontSize(18.f);
    bodyLabel->setWidthPercentage(100.f);
    bodyLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    bodyLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    bodyLabel->setSingleLine(false);
    bodyLabel->setIsWrapping(true);
    bodyLabel->setFocusable(true);

    HIDE_BRLS_HIGHLIGHT(bodyLabel);

    bodyLabel->registerAction("确认", brls::BUTTON_A, [](brls::View*) -> bool {
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        return true;
    });
    bodyLabel->registerAction("返回", brls::BUTTON_B, [](brls::View*) -> bool {
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        return true;
    });
    box->addView(bodyLabel);

    scroll->setContentView(box);

    auto* frame = new brls::AppletFrame(scroll);
    frame->setTitle(title);

    brls::Application::pushActivity(new brls::Activity(frame), brls::TransitionAnimation::NONE);
    brls::Application::giveFocus(bodyLabel);
}

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

static void startResourceDownload(const OnlineResourceItem& item,
                                  std::function<void()> onSuccess);

class OnlineResourceCanvas final : public brls::View {
public:
    explicit OnlineResourceCanvas(OnlineResourceManifest manifest)
        : m_manifest(std::move(manifest)) {
        this->setFocusable(true);
        this->setGrow(1.0f);
        this->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);

        this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
        this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        auto moveLeft = [this](brls::View*) -> bool { return _moveHorizontal(-1); };
        auto moveRight = [this](brls::View*) -> bool { return _moveHorizontal(1); };
        auto moveUp = [this](brls::View*) -> bool { return _moveVertical(-1); };
        auto moveDown = [this](brls::View*) -> bool { return _moveVertical(1); };

        this->registerAction("", brls::BUTTON_LEFT, moveLeft, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_RIGHT, moveRight, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_UP, moveUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_DOWN, moveDown, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_LEFT, moveLeft, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_RIGHT, moveRight, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_UP, moveUp, true, true, brls::SOUND_NONE);
        this->registerAction("", brls::BUTTON_NAV_DOWN, moveDown, true, true, brls::SOUND_NONE);
        this->registerAction("下载", brls::BUTTON_A, [this](brls::View*) -> bool {
            _activateFocused();
            return true;
        }, false, false, brls::SOUND_NONE);
        this->registerAction("返回", brls::BUTTON_B, [](brls::View*) -> bool {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });

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

        _rebuildLayout(w, h);
        m_scrollOffset = std::clamp(m_scrollOffset, 0.f, _maximumScroll());
        m_targetScroll = std::clamp(m_targetScroll, 0.f, _maximumScroll());

        nvgSave(vg);
        nvgScissor(vg, x, y, w, h);
        const float drawOffsetY = y - m_scrollOffset;

        for (const auto& group : m_groupLayouts) {
            const float groupY = drawOffsetY + group.y;
            if (groupY + group.h < y || groupY > y + h)
                continue;

            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + group.x, groupY, group.w, group.h, 10.f);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 22));
            nvgFill(vg);

            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + group.x + 1.f, groupY + 1.f,
                           group.w - 2.f, group.h - 2.f, 9.f);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 18));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 22.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
            nvgText(vg, x + group.x + 20.f, groupY + 31.f,
                    m_manifest.groups[group.groupIndex].header.c_str(), nullptr);
        }

        for (size_t index = 0; index < m_itemLayouts.size(); ++index) {
            const auto& layout = m_itemLayouts[index];
            const float itemY = drawOffsetY + layout.y;
            if (itemY + layout.h < y || itemY > y + h)
                continue;
            const auto& item = m_manifest.groups[layout.groupIndex].items[layout.itemIndex];
            _drawResourceButton(vg, x + layout.x, itemY, layout.w, layout.h,
                                item, static_cast<int>(index) == m_focusedIndex);
        }

        if (m_contentHeight > h + 1.f) {
            const float trackH = std::max(40.f, h - 32.f);
            const float thumbH = std::max(36.f, trackH * h / m_contentHeight);
            const float travel = trackH - thumbH;
            const float thumbY = y + 16.f + (_maximumScroll() <= 0.f
                ? 0.f : travel * m_scrollOffset / _maximumScroll());
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + w - 7.f, thumbY, 3.f, thumbH, 1.5f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 90));
            nvgFill(vg);
        }

        nvgRestore(vg);
    }

    void frame(brls::FrameContext* ctx) override {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.5f)
            dt = 0.016f;
        m_animTime += dt;

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
    std::vector<ItemLayout> m_itemLayouts;
    std::vector<GroupLayout> m_groupLayouts;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_focusedIndex = 0;
    float m_viewportHeight = 0.f;
    float m_contentHeight = 0.f;
    float m_scrollOffset = 0.f;
    float m_targetScroll = 0.f;
    float m_animTime = 0.f;
    float m_layoutWidth = -1.f;
    std::chrono::steady_clock::time_point m_lastFrameTime;

    void _rebuildLayout(float width, float height) {
        m_viewportHeight = height;
        if (std::abs(m_layoutWidth - width) < 0.5f && !m_itemLayouts.empty())
            return;

        m_layoutWidth = width;
        m_itemLayouts.clear();
        m_groupLayouts.clear();

        constexpr float pagePadding = 30.f;
        constexpr float blockPadding = 18.f;
        constexpr float itemGap = 16.f;
        constexpr float minimumSquare = 124.f;
        constexpr float maximumSquare = 154.f;
        constexpr float headerHeight = 48.f;
        constexpr float groupGap = 18.f;

        const float blockWidth = std::max(1.f, width - pagePadding * 2.f);
        const float availableWidth = std::max(1.f, blockWidth - blockPadding * 2.f);
        const int columns = std::max(1, static_cast<int>(
            std::floor((availableWidth + itemGap) / (minimumSquare + itemGap))));
        const float square = std::min(maximumSquare,
            (availableWidth - itemGap * static_cast<float>(columns - 1)) / columns);
        const float gridWidth = square * columns + itemGap * (columns - 1);
        const float gridStartX = pagePadding + blockPadding
            + std::max(0.f, (availableWidth - gridWidth) * 0.5f);

        float cursorY = 18.f;
        int visualRow = 0;
        for (size_t groupIndex = 0; groupIndex < m_manifest.groups.size(); ++groupIndex) {
            const auto& group = m_manifest.groups[groupIndex];
            const int rows = static_cast<int>((group.items.size() + columns - 1) / columns);
            const float groupHeight = blockPadding + headerHeight
                + rows * square + std::max(0, rows - 1) * itemGap + blockPadding;
            m_groupLayouts.push_back({groupIndex, pagePadding, cursorY, blockWidth, groupHeight});

            const float itemsY = cursorY + blockPadding + headerHeight;
            for (size_t itemIndex = 0; itemIndex < group.items.size(); ++itemIndex) {
                const int rowInGroup = static_cast<int>(itemIndex) / columns;
                const int column = static_cast<int>(itemIndex) % columns;
                m_itemLayouts.push_back({
                    groupIndex,
                    itemIndex,
                    visualRow + rowInGroup,
                    column,
                    gridStartX + column * (square + itemGap),
                    itemsY + rowInGroup * (square + itemGap),
                    square,
                    square,
                });
            }

            visualRow += rows;
            cursorY += groupHeight + groupGap;
        }
        m_contentHeight = cursorY - groupGap + 18.f;
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
        if (m_itemLayouts.empty())
            return true;
        const auto& current = m_itemLayouts[m_focusedIndex];
        int candidate = -1;
        for (size_t index = 0; index < m_itemLayouts.size(); ++index) {
            const auto& item = m_itemLayouts[index];
            if (item.row != current.row)
                continue;
            if ((direction < 0 && item.column == current.column - 1)
                || (direction > 0 && item.column == current.column + 1)) {
                candidate = static_cast<int>(index);
                break;
            }
        }
        if (candidate >= 0)
            _setFocus(candidate);
        return true;
    }

    bool _moveVertical(int direction) {
        if (m_itemLayouts.empty())
            return true;
        const auto& current = m_itemLayouts[m_focusedIndex];
        int targetRow = direction < 0 ? -1 : std::numeric_limits<int>::max();
        for (const auto& item : m_itemLayouts) {
            if (direction < 0 && item.row < current.row)
                targetRow = std::max(targetRow, item.row);
            else if (direction > 0 && item.row > current.row)
                targetRow = std::min(targetRow, item.row);
        }
        if (targetRow < 0 || targetRow == std::numeric_limits<int>::max())
            return true;

        int candidate = -1;
        int columnDistance = std::numeric_limits<int>::max();
        for (size_t index = 0; index < m_itemLayouts.size(); ++index) {
            const auto& item = m_itemLayouts[index];
            if (item.row != targetRow)
                continue;
            const int distance = std::abs(item.column - current.column);
            if (distance < columnDistance) {
                columnDistance = distance;
                candidate = static_cast<int>(index);
            }
        }
        if (candidate >= 0)
            _setFocus(candidate);
        return true;
    }

    void _setFocus(int index) {
        if (index == m_focusedIndex)
            return;
        m_focusedIndex = index;
        _ensureFocusedVisible();
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
        this->invalidate();
    }

    void _activateFocused() {
        if (m_itemLayouts.empty())
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
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 10.f);
        nvgFillColor(vg, focused ? nvgRGBA(79, 193, 255, 54) : nvgRGBA(255, 255, 255, 10));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 9.f);
        nvgStrokeColor(vg, focused ? nvgRGBA(79, 193, 255, 190) : nvgRGBA(255, 255, 255, 24));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        if (focused && this->isFocused()) {
            beiklive::ui::drawGradientFocusBorder(vg, x, y, w, h, 10.f, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_animTime));
        }

        const std::string icon = encodeMaterialIcon(item.materialIcon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 43.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + w * 0.5f, y + h * 0.34f, icon.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 17.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, focused ? nvgRGBA(255, 255, 255, 255) : GET_THEME_COLOR("brls/text"));
        nvgSave(vg);
        nvgIntersectScissor(vg, x + 8.f, y + h * 0.56f, w - 16.f, h * 0.29f);
        nvgTextBox(vg, x + 10.f, y + h * 0.58f, w - 20.f, item.name.c_str(), nullptr);
        nvgRestore(vg);

        nvgFontSize(vg, 13.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgFillColor(vg, item.needsUpdate
            ? nvgRGBA(255, 190, 80, 235) : nvgRGBA(100, 220, 150, 225));
        const char* status = item.needsUpdate ? "可更新" : "已安装";
        nvgText(vg, x + w * 0.5f, y + h - 11.f, status, nullptr);
    }
};

static void openOnlineResourceActivity(OnlineResourceManifest manifest) {
    auto* canvas = new OnlineResourceCanvas(std::move(manifest));
    auto* frame = new brls::AppletFrame(canvas);
    frame->setTitle("在线资源");
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);
    brls::Application::giveFocus(canvas);
}

static void checkOnlineResources() {
    auto* progressDialog = new brls::Dialog("正在检测在线资源...\n\n请稍候");
    progressDialog->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(progressDialog);
    progressDialog->open();

    new std::thread([progressDialog]() {
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

        brls::sync([progressDialog, manifest = std::move(manifest)]() mutable {
            progressDialog->close([]() {});
            openOnlineResourceActivity(std::move(manifest));
        });
    });
}

static void startResourceDownload(const OnlineResourceItem& item,
                                  std::function<void()> onSuccess) {
    auto* progressDialog = new brls::Dialog(
        "正在下载并安装 " + item.name + "...\n\n请稍候");
    progressDialog->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(progressDialog);
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

        if (!downloadFileToPath(cacheBustedUrl(item.url), downloadPath.string())) {
            std::filesystem::remove(downloadPath, ec);
            brls::sync([progressDialog]() {
                progressDialog->close([]() {});
                showMessageDialog("下载失败，请稍后重试");
            });
            return;
        }

        std::string resultText;
        const bool installOk = installDownloadedResource(item, downloadPath, nullptr, resultText);
        std::filesystem::remove(downloadPath, ec);

        if (!installOk) {
            brls::sync([progressDialog, resultText]() {
                progressDialog->close([]() {});
                showMessageDialog(resultText);
            });
            return;
        }

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

AboutPage::AboutPage() {
    brls::sync([this]() {
        this->showFooter(true);
        this->showHeader(false);
        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) { 
            beiklive::popActivity(this);
            return true;
        });
        m_tabFrame = new beiklive::TabFrame();
        this->getContentBox()->addView(m_tabFrame);

        m_tabFrame->addTab(
            "关于本项目",
            BK_RES("img/ui/setting/emu.png"),
            nullptr, nullptr, nullptr,
            _buildInfoTab()
        );
        m_tabFrame->addTab(
            "更新",
            BK_RES("img/ui/setting/debug.png"),
            nullptr, nullptr, nullptr,
            _buildUpdateTab()
        );
        m_tabFrame->addTab(
            "支持作者",
            BK_RES("img/ui/setting/display.png"),
            nullptr, nullptr, nullptr,
            _buildSupportTab()
        );
        m_tabFrame->addFinish();
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
        "当前支持 GB、GBC、GBA、FC、SFC、NDS(NDS性能较弱，仍在优化中)",
        "内置核心包含 mGBA、Nestopia、FCEUmm、Snes9x 2005、Snes9x 与 melonDS。",
        "",
        "目前已实现功能：",
        "  •  游戏库功能、游戏封面、游玩时长、游戏次数",
        "  •  支持目录扫描、RetroArch 游戏库导入、Web 局域网管理游戏库与封面自定义",
        "  •  支持即时存档 / 读档、自动存档 / 自动存读档",
        "  •  支持金手指（不支持raw格式）",
        "  •  按机型独立按键映射、A / B 连发",
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
        []() {
            checkOnlineResources();
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
            } else {
                auto* okDlg = new brls::Dialog("已是最新版本");
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
