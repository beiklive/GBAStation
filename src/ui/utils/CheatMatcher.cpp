#include "CheatMatcher.hpp"
#include "core/Tools.hpp"
#include "ui/utils/Box.hpp"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

namespace beiklive {

static const char* kPlatformNames[] = {"", "GBA", "GBC", "GB"};
static const char* kRetroPlatformDirs[] = {"",
    "Nintendo%20-%20Game%20Boy%20Advance",
    "Nintendo%20-%20Game%20Boy%20Color",
    "Nintendo%20-%20Game%20Boy"};
static const char* kBaseUrl = "https://cdn.jsdelivr.net/gh/libretro/libretro-database@master/cht";

// ── 统一匹配页面（单 Activity，多阶段切换）───────────────

class CheatMatchActivity : public beiklive::Box {
public:
    CheatMatchActivity(int platform, const std::string& romPath,
                       std::function<void(const std::string&)> onDone)
        : m_platform(platform), m_romPath(romPath), m_onDone(std::move(onDone)) {
        this->showHeader(false);
        this->showFooter(false);
        if (this->getBottomBar())
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        auto* root = this->getContentBox();
        root->setAxis(brls::Axis::COLUMN);
        root->setAlignItems(brls::AlignItems::CENTER);
        root->setJustifyContent(brls::JustifyContent::CENTER);

        m_card = new brls::Box(brls::Axis::COLUMN);
        m_card->setWidth(700.f);
        m_card->setHeight(700.f);
        m_card->setCornerRadius(16.f);
        m_card->setBackgroundColor(nvgRGBA(25, 28, 40, 245));
        m_card->setShadowType(brls::ShadowType::GENERIC);
        m_card->setShadowVisibility(true);
        m_card->setPadding(20.f, 24.f, 20.f, 24.f);

        m_titleLabel = new brls::Label();
        m_titleLabel->setText("正在匹配金手指...");
        m_titleLabel->setFontSize(24.f);
        m_titleLabel->setTextColor(nvgRGB(255,255,255));
        m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_titleLabel->setIsWrapping(true);
        m_titleLabel->setMarginBottom(12.f);
        m_card->addView(m_titleLabel);

        m_statusLabel = new brls::Label();
        m_statusLabel->setText("请稍候...");
        m_statusLabel->setFontSize(15.f);
        m_statusLabel->setTextColor(nvgRGBA(200,200,200,200));
        m_statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_statusLabel->setIsWrapping(true);
        m_statusLabel->setMarginBottom(16.f);
        m_card->addView(m_statusLabel);

        m_dynamicArea = new brls::Box(brls::Axis::COLUMN);
        m_dynamicArea->setGrow(1.f);

        // 初始：取消按钮
        auto* btn = new brls::Button();
        btn->setText("取消");
        btn->setWidth(140.f);
        btn->registerClickAction([this](brls::View*) -> bool {
            m_cancelled.store(true);
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });
        m_dynamicArea->addView(btn);

        m_card->addView(m_dynamicArea);
        root->addView(m_card);

        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });

        // 启动后台线程
        _startMatching();
    }

private:
    int m_platform;
    std::string m_romPath;
    std::function<void(const std::string&)> m_onDone;
    std::atomic<bool> m_cancelled{false};

    brls::Box* m_card = nullptr;
    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_statusLabel = nullptr;
    brls::Box* m_dynamicArea = nullptr;
    brls::Label* m_previewLabel = nullptr;

    std::vector<CheatMatchResult> m_results;
    int m_selectedIdx = 0;

    void _showSelectPhase() {
        m_dynamicArea->clearViews(true);

        std::vector<std::string> names;
        for (auto& r : m_results) names.push_back(r.filename);
        auto* selector = new brls::SelectorCell();
        selector->init("匹配列表", names, 0,
            [this](int idx) {
                if (idx >= 0 && idx < (int)m_results.size()) {
                    m_selectedIdx = idx;
                    _updatePreview();
                }
            });
        selector->setHeight(70.f);
        m_dynamicArea->addView(selector);

        auto* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);
        scroll->setScrollingIndicatorVisible(false);

        m_previewLabel = new brls::Label();
        m_previewLabel->setFontSize(14.f);
        m_previewLabel->setTextColor(nvgRGBA(200,200,200,255));
        m_previewLabel->setIsWrapping(true);
        m_previewLabel->setFocusable(true);
        UP_DOWN_NAVIGATION(m_previewLabel, m_previewLabel);
        scroll->setContentView(m_previewLabel);

        m_dynamicArea->addView(scroll);

        // 确认按钮
        auto* btnBox = new brls::Box(brls::Axis::ROW);
        btnBox->setJustifyContent(brls::JustifyContent::CENTER);
        btnBox->setMarginTop(14.f);
        auto* confirmBtn = new brls::Button();
        confirmBtn->setText("确认选择");
        confirmBtn->setWidth(170.f);
        confirmBtn->registerClickAction([this](brls::View*) -> bool {
            if (m_selectedIdx < 0 || m_selectedIdx >= (int)m_results.size())
                return false;
            auto& r = m_results[m_selectedIdx];
            auto* dlg = new brls::Dialog("是否选择 " + r.filename + " ？");
            dlg->addButton("取消", []() {});
            dlg->addButton("确认", [this, path = r.filePath]() {
                if (m_onDone) m_onDone(path);
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
            });
            dlg->open();
            return true;
        });
        btnBox->addView(confirmBtn);
        m_dynamicArea->addView(btnBox);

        if (!m_results.empty()) _updatePreview();
        brls::Application::giveFocus(selector);
    }

    void _showResult(const std::string& text) {
        m_statusLabel->setText("");
        m_dynamicArea->clearViews(true);
        auto* lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(17.f);
        lbl->setTextColor(nvgRGBA(200,200,200,200));
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setIsWrapping(true);
        m_dynamicArea->addView(lbl);

        auto* btn = new brls::Button();
        btn->setText("确定");
        btn->setWidth(140.f);
        btn->registerClickAction([](brls::View*) -> bool {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });
        m_dynamicArea->addView(btn);
        brls::Application::giveFocus(btn);
    }

    void _updatePreview() {
        if (!m_previewLabel) return;
        if (m_selectedIdx < 0 || m_selectedIdx >= (int)m_results.size()) return;
        auto& r = m_results[m_selectedIdx];
        if (r.content.empty()) return;
        m_previewLabel->setText(r.content);
    }

    // ── 后台匹配 ──────────────────────────────────────────

    void _startMatching() {
        int platform = m_platform;
        std::string romPath = m_romPath;
        new std::thread([this, platform, romPath]() {
            // 检查数据库
            {
                std::string db = beiklive::path::dbsPath();
                static const char* kRequired[] = {
                    "gba_db.json", "gbc_db.json", "gb_db.json",
                    "gba_cht.json", "gbc_cht.json", "gb_cht.json"
                };
                bool missing = false;
                for (auto& f : kRequired) {
                    if (!std::filesystem::exists(db + beiklive::path::SPLIT_CHAR + f)) {
                        missing = true;
                        break;
                    }
                }
                if (missing) {
                    brls::sync([this]() {
                        m_titleLabel->setText("数据库缺失");
                        _showResult("数据库文件缺失，请去 关于-更新 中更新数据库");
                    });
                    return;
                }
            }
            if (m_cancelled.load()) return;

            brls::sync([this]() { m_statusLabel->setText("正在计算 ROM 特征码..."); });

            auto results = _doMatch(platform, romPath);
            if (m_cancelled.load()) return;

            if (results.empty()) {
                brls::sync([this]() {
                    m_titleLabel->setText("未匹配到");
                    _showResult("未匹配到，请在游戏中手动设置金手指");
                });
                return;
            }

            brls::sync([this, count = (int)results.size()]() {
                m_titleLabel->setText("正在下载金手指...");
                m_statusLabel->setText("已匹配 " + std::to_string(count) + " 个，正在下载...");
            });

            // 下载
            std::string cacheDir = _chtCacheDir(platform);
            std::error_code ec;
            std::filesystem::create_directories(cacheDir, ec);
            std::string retroDir = _retroPlatformDir(platform);

            for (auto& r : results) {
                if (m_cancelled.load()) break;
                std::string localPath = cacheDir + beiklive::path::SPLIT_CHAR + r.filename;
                if (!std::filesystem::exists(localPath)) {
                    std::string url = std::string(kBaseUrl) + "/" + retroDir + "/" + r.filename;
                    std::string body = _fetchUrl(url);
                    if (!body.empty() && !m_cancelled.load()) {
                        std::ofstream out(localPath, std::ios::binary | std::ios::trunc);
                        if (out) { out << body; out.close(); }
                    }
                }
                if (std::filesystem::exists(localPath)) {
                    r.filePath = localPath;
                    std::ifstream in(localPath);
                    if (in) {
                        std::stringstream ss;
                        ss << in.rdbuf();
                        r.content = ss.str();
                    }
                }
            }

            if (m_cancelled.load()) return;

            std::vector<CheatMatchResult> validResults;
            for (auto& r : results)
                if (!r.filePath.empty()) validResults.push_back(std::move(r));

            if (validResults.empty()) {
                brls::sync([this]() {
                    m_titleLabel->setText("下载失败");
                    _showResult("下载失败，请检查网络后重试");
                });
                return;
            }

            brls::sync([this, vr = std::move(validResults)]() mutable {
                m_results = std::move(vr);
                m_titleLabel->setText("选择金手指");
                m_statusLabel->setText("共 " + std::to_string(m_results.size()) + " 个匹配项");
                _showSelectPhase();
            });
        });
    }

    // ── 静态工具 ──────────────────────────────────────────

    static std::string _chtCacheDir(int platform) {
        if (platform < 1 || platform > 3) return "";
        return beiklive::path::cheatPath() + "/Retroarch/" + kPlatformNames[platform];
    }

    static std::string _retroPlatformDir(int platform) {
        if (platform < 1 || platform > 3) return "";
        return kRetroPlatformDirs[platform];
    }

    static std::string _fetchUrl(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        std::string body;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            static_cast<size_t(*)(void*, size_t, size_t, void*)>(
                [](void* ptr, size_t size, size_t nmemb, void* ud) -> size_t {
                    static_cast<std::string*>(ud)->append((const char*)ptr, size * nmemb);
                    return size * nmemb;
                }));
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        CURLcode res = curl_easy_perform(curl);
        long code = 0;
        if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK || code != 200) return "";
        return body;
    }

    static std::vector<CheatMatchResult> _doMatch(int platform, const std::string& romPath);
    static std::vector<std::string> _extractKeywords(const std::string& name);
    static bool _containsAllKeywords(const std::vector<std::string>& haystack,
                                     const std::vector<std::string>& needles);
};

// ── 匹配逻辑实现 ────────────────────────────────────────

std::vector<std::string> CheatMatchActivity::_extractKeywords(const std::string& name) {
    // clean: 去括号
    std::string s;
    int depth = 0;
    for (char c : name) {
        if (c == '(' || c == '[') ++depth;
        else if (c == ')' || c == ']') { if (depth > 0) --depth; }
        else if (depth == 0) s += c;
    }
    auto dot = s.rfind('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    size_t b = s.find_first_not_of(" \t-_");
    size_t e = s.find_last_not_of(" \t-_");
    if (b == std::string::npos) return {};
    s = s.substr(b, e - b + 1);

    std::vector<std::string> words;
    std::istringstream iss(s);
    std::string w;
    while (iss >> w) {
        if (w.size() <= 1) continue;
        std::transform(w.begin(), w.end(), w.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        words.push_back(w);
    }
    return words;
}

bool CheatMatchActivity::_containsAllKeywords(const std::vector<std::string>& haystack,
                                               const std::vector<std::string>& needles) {
    for (const auto& n : needles) {
        bool found = false;
        for (const auto& h : haystack) {
            if (h == n) { found = true; break; }
        }
        if (!found) return false;
    }
    return true;
}

std::vector<CheatMatchResult> CheatMatchActivity::_doMatch(int platform,
                                                           const std::string& romPath) {
    std::vector<CheatMatchResult> results;
    if (platform < 1 || platform > 3) return results;

    uint32_t crc = beiklive::tools::crc32(romPath);
    std::string crcHex = beiklive::tools::crc32ToHex(crc);
    std::string gameId = beiklive::tools::readGbaGameID(romPath);

    std::string platLower = kPlatformNames[platform];
    std::transform(platLower.begin(), platLower.end(), platLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::string dbPath = beiklive::path::dbsPath() + "/" + platLower + "_db.json";

    std::ifstream dbFile(dbPath);
    if (!dbFile.is_open()) return results;
    json db;
    try { dbFile >> db; } catch (...) { return results; }

    std::vector<std::string> matchedNames;
    for (const auto& entry : db) {
        std::string eCrc = entry.value("crc32", "");
        std::string eSerial = entry.value("serial", "");
        std::string eName = entry.value("name", "");
        if (eName.empty()) continue;
        bool crcMatch = !crcHex.empty() && !eCrc.empty() &&
            std::equal(crcHex.begin(), crcHex.end(), eCrc.begin(),
                       [](char a, char b) { return std::toupper(a) == std::toupper(b); });
        bool serialMatch = !gameId.empty() && !eSerial.empty() &&
            std::equal(gameId.begin(), gameId.end(), eSerial.begin(),
                       [](char a, char b) { return std::toupper(a) == std::toupper(b); });
        if (crcMatch || serialMatch)
            matchedNames.push_back(eName);
    }
    if (matchedNames.empty()) return results;

    std::set<std::string> allKeywords;
    for (const auto& name : matchedNames) {
        auto kw = _extractKeywords(name);
        allKeywords.insert(kw.begin(), kw.end());
    }
    std::vector<std::string> keywords(allKeywords.begin(), allKeywords.end());

    std::string chtDbPath = beiklive::path::dbsPath() + "/" + platLower + "_cht.json";
    std::ifstream chtFile(chtDbPath);
    if (!chtFile.is_open()) return results;
    json chtDb;
    try { chtFile >> chtDb; } catch (...) { return results; }

    for (const auto& entry : chtDb) {
        std::string chtName = entry.value("name", "");
        if (chtName.empty()) continue;
        auto chtKw = _extractKeywords(chtName);
        if (_containsAllKeywords(chtKw, keywords)) {
            CheatMatchResult r;
            r.filename = chtName;
            results.push_back(r);
        }
    }
    return results;
}

// ── 入口 ──────────────────────────────────────────────────

void startCheatMatching(int platform, const std::string& romPath,
                        std::function<void(const std::string& cheatPath)> onDone) {
    auto* activity = new CheatMatchActivity(platform, romPath, std::move(onDone));
    auto* frame = new brls::AppletFrame(activity);
    HIDE_BRLS_BAR(frame);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);
}

} // namespace beiklive
