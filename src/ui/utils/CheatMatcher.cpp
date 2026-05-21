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

// ── 自定义进度对话框 ──────────────────────────────────────

class ProgressDialog : public beiklive::Box {
public:
    ProgressDialog(const std::string& title, std::function<void()> onCancel = nullptr)
        : m_onCancel(std::move(onCancel)) {
        this->showHeader(false);
        this->showFooter(false);
        if (this->getBottomBar())
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        auto* root = this->getContentBox();
        root->setAxis(brls::Axis::COLUMN);
        root->setAlignItems(brls::AlignItems::CENTER);
        root->setJustifyContent(brls::JustifyContent::CENTER);

        auto* card = new brls::Box(brls::Axis::COLUMN);
        card->setWidth(500.f);
        card->setHeight(brls::View::AUTO);
        card->setCornerRadius(16.f);
        card->setBackgroundColor(nvgRGBA(25, 28, 40, 245));
        card->setShadowType(brls::ShadowType::GENERIC);
        card->setShadowVisibility(true);
        card->setPadding(24.f, 32.f, 24.f, 32.f);
        card->setAlignItems(brls::AlignItems::CENTER);

        m_titleLabel = new brls::Label();
        m_titleLabel->setText(title);
        m_titleLabel->setFontSize(22.f);
        m_titleLabel->setTextColor(nvgRGB(255,255,255));
        m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_titleLabel->setIsWrapping(true);
        m_titleLabel->setMarginBottom(20.f);
        card->addView(m_titleLabel);

        m_statusLabel = new brls::Label();
        m_statusLabel->setText(" ");
        m_statusLabel->setFontSize(15.f);
        m_statusLabel->setTextColor(nvgRGBA(200,200,200,200));
        m_statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_statusLabel->setIsWrapping(true);
        card->addView(m_statusLabel);

        m_buttonBox = new brls::Box(brls::Axis::ROW);
        m_buttonBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_buttonBox->setMarginTop(18.f);

        if (m_onCancel) {
            auto* btn = new brls::Button();
            btn->setText("取消");
            btn->setWidth(140.f);
            btn->registerClickAction([this](brls::View*) -> bool {
                if (m_onCancel) m_onCancel();
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
                return true;
            });
            m_buttonBox->addView(btn);
        }

        card->addView(m_buttonBox);
        root->addView(card);
    }

    void setText(const std::string& text) {
        if (m_titleLabel) m_titleLabel->setText(text);
    }

    void setStatus(const std::string& status) {
        if (m_statusLabel) m_statusLabel->setText(status);
    }

    void showResult(const std::string& text) {
        m_statusLabel->setText(text);
        m_onCancel = nullptr;
        m_buttonBox->clearViews(true);
        auto* btn = new brls::Button();
        btn->setText("确定");
        btn->setWidth(140.f);
        btn->registerClickAction([](brls::View*) -> bool {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });
        m_buttonBox->addView(btn);
        brls::Application::giveFocus(btn);
    }

    void close() {
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
    }

private:
    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_statusLabel = nullptr;
    brls::Box* m_buttonBox = nullptr;
    std::function<void()> m_onCancel;
};

// ── 工具函数 ──────────────────────────────────────────────

static std::string cleanNameForMatch(const std::string& name) {
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
    if (b == std::string::npos) return "";
    s = s.substr(b, e - b + 1);
    return s;
}

static std::vector<std::string> extractKeywords(const std::string& name) {
    std::vector<std::string> words;
    std::istringstream iss(cleanNameForMatch(name));
    std::string w;
    while (iss >> w) {
        if (w.size() <= 1) continue;
        std::transform(w.begin(), w.end(), w.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        words.push_back(w);
    }
    return words;
}

static bool containsAllKeywords(const std::vector<std::string>& haystack,
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

static std::string platformDir(int platform) {
    if (platform < 1 || platform > 3) return "";
    return kPlatformNames[platform];
}

static std::string retroPlatformDir(int platform) {
    if (platform < 1 || platform > 3) return "";
    return kRetroPlatformDirs[platform];
}

static std::string chtCacheDir(int platform) {
    return beiklive::path::cheatPath() + "/Retroarch/" + platformDir(platform);
}

// ── HTTP 下载 ──────────────────────────────────────────────

static std::string fetchUrl(const std::string& url) {
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

// ── 核心匹配逻辑 ──────────────────────────────────────────

static std::vector<CheatMatchResult> doMatch(int platform, const std::string& romPath) {
    std::vector<CheatMatchResult> results;

    std::string pDir = platformDir(platform);
    if (pDir.empty()) return results;

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
        auto kw = extractKeywords(name);
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
        auto chtKw = extractKeywords(chtName);
        if (containsAllKeywords(chtKw, keywords)) {
            CheatMatchResult r;
            r.filename = chtName;
            results.push_back(r);
        }
    }

    return results;
}

// ── 选择界面 ──────────────────────────────────────────────

class CheatSelectActivity : public beiklive::Box {
public:
    CheatSelectActivity(std::vector<CheatMatchResult> results,
                        std::function<void(const std::string&)> onDone)
        : m_results(std::move(results)), m_onDone(std::move(onDone)) {
        this->showHeader(false);
        this->showFooter(false);
        if (this->getBottomBar())
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        auto* root = this->getContentBox();
        root->setAxis(brls::Axis::COLUMN);
        root->setAlignItems(brls::AlignItems::CENTER);
        root->setJustifyContent(brls::JustifyContent::CENTER);

        auto* card = new brls::Box(brls::Axis::COLUMN);
        card->setWidth(700.f);
        card->setHeight(700.f);
        card->setCornerRadius(16.f);
        card->setBackgroundColor(nvgRGBA(25, 28, 40, 245));
        card->setShadowType(brls::ShadowType::GENERIC);
        card->setShadowVisibility(true);
        card->setPadding(20.f, 24.f, 20.f, 24.f);

        auto* titleLbl = new brls::Label();
        titleLbl->setText("选择金手指");
        titleLbl->setFontSize(24.f);
        titleLbl->setTextColor(nvgRGB(255,255,255));
        titleLbl->setMarginBottom(16.f);
        card->addView(titleLbl);

        std::vector<std::string> names;
        for (auto& r : m_results) names.push_back(r.filename);
        auto* dropdown = new brls::Dropdown("匹配列表", names,
            [this](int idx) {
                if (idx >= 0 && idx < (int)m_results.size()) {
                    m_selectedIdx = idx;
                    _updatePreview();
                }
            },
            0);
        dropdown->setHeight(60.f);
        card->addView(dropdown);

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

        card->addView(scroll);
        root->addView(card);

        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });

        this->registerAction("确认", brls::BUTTON_A, [this](brls::View*) {
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

        if (!m_results.empty()) _updatePreview();
    }

private:
    std::vector<CheatMatchResult> m_results;
    int m_selectedIdx = 0;
    std::function<void(const std::string&)> m_onDone;
    brls::Label* m_previewLabel = nullptr;

    void _updatePreview() {
        if (m_selectedIdx < 0 || m_selectedIdx >= (int)m_results.size()) return;
        auto& r = m_results[m_selectedIdx];
        if (r.content.empty()) return;
        m_previewLabel->setText(r.content);
    }
};

// ── 入口 ──────────────────────────────────────────────────

void startCheatMatching(int platform, const std::string& romPath,
                        std::function<void(const std::string& cheatPath)> onDone) {
    auto* cancelFlag = new std::atomic<bool>(false);

    auto* prog = new ProgressDialog("正在匹配金手指...",
        [cancelFlag]() { cancelFlag->store(true); });
    auto* frame = new brls::AppletFrame(prog);
    HIDE_BRLS_BAR(frame);
    brls::sync([frame]() {
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);
                                    brls::Application::blockInputs();
    });

    new std::thread([platform, romPath, onDone = std::move(onDone), prog, cancelFlag]() {
        // 检查数据库文件
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
                brls::sync([prog, cancelFlag]() {
                    delete cancelFlag;
                    brls::Application::unblockInputs();
                    prog->showResult("数据库文件缺失，请去 关于-更新 中更新数据库");
                });
                return;
            }
        }

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                brls::Application::unblockInputs();
                prog->close();
            });
            return;
        }

        brls::sync([prog]() { prog->setStatus("正在计算 ROM 特征码..."); });

        auto results = doMatch(platform, romPath);

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                brls::Application::unblockInputs();
                prog->close();
            });
            return;
        }

        if (results.empty()) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                brls::Application::unblockInputs();
                prog->showResult("未匹配到，请在游戏中手动设置金手指");
            });
            return;
        }

        brls::sync([prog, count = (int)results.size()]() {
            prog->setText("正在下载金手指...");
            prog->setStatus("已匹配 " + std::to_string(count) + " 个，正在下载...");
        });

        std::string cacheDir = chtCacheDir(platform);
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
        std::string retroDir = retroPlatformDir(platform);

        int dlCount = 0;
        for (auto& r : results) {
            if (cancelFlag->load()) break;
            std::string localPath = cacheDir + beiklive::path::SPLIT_CHAR + r.filename;
            if (!std::filesystem::exists(localPath)) {
                std::string url = std::string(kBaseUrl) + "/" + retroDir + "/" + r.filename;
                std::string body = fetchUrl(url);
                if (!body.empty() && !cancelFlag->load()) {
                    std::ofstream out(localPath, std::ios::binary | std::ios::trunc);
                    if (out) { out << body; out.close(); ++dlCount; }
                }
            } else {
                ++dlCount;
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

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                brls::Application::unblockInputs();
                prog->close();
            });
            return;
        }

        std::vector<CheatMatchResult> validResults;
        for (auto& r : results)
            if (!r.filePath.empty()) validResults.push_back(std::move(r));

        if (validResults.empty()) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                brls::Application::unblockInputs();
                prog->showResult("下载失败，请检查网络后重试");
            });
            return;
        }

        brls::sync([prog, cancelFlag, validResults = std::move(validResults),
                    onDone = std::move(onDone)]() mutable {
            delete cancelFlag;
            brls::Application::unblockInputs();
            // prog->close();
            // brls::Application::popActivity();
            
            auto* activity = new CheatSelectActivity(std::move(validResults), std::move(onDone));
            auto* frame = new brls::AppletFrame(activity);
            HIDE_BRLS_BAR(frame);
            brls::Application::pushActivity(new brls::Activity(frame),
                                            brls::TransitionAnimation::NONE);
        });
    });
}

} // namespace beiklive
