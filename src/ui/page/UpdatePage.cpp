#include "UpdatePage.hpp"
#include "core/Translation.hpp"
#include "ui/widget/VideoBackgroundView.hpp"

#include "core/Tools.hpp"
#include "ui/utils/MaterialIcons.hpp"

#include <borealis/views/button.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/rectangle.hpp>

#include <chrono>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive {

namespace
{

enum class UpdateVisualState {
    Connecting = 0,
    Downloading,
    Extracting,
    Installing,
    Success,
    Error,
    Manual,
};

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

class UpdateProgressCanvas final : public brls::View {
public:
    UpdateProgressCanvas() {
        this->setWidth(620.f);
        this->setHeight(310.f);
        this->setFocusable(false);
        m_lastFrameTime = std::chrono::steady_clock::now();
        setState(UpdateVisualState::Connecting, L("正在连接服务器...").c_str());
    }

    void setProgress(float pct, std::string speed, std::string size, std::string eta) {
        m_targetProgress = std::clamp(pct / 100.f, 0.f, 1.f);
        m_speed = std::move(speed);
        m_size = std::move(size);
        m_eta = eta.empty() ? L("计算剩余时间") : std::move(eta);
        this->invalidate();
    }

    void setState(UpdateVisualState state, std::string status) {
        m_state = state;
        m_status = std::move(status);
        switch (state) {
            case UpdateVisualState::Connecting:
            case UpdateVisualState::Downloading:
                m_icon = 0xE2C4;
                _setAccent(79, 193, 255);
                break;
            case UpdateVisualState::Extracting:
                m_icon = 0xE149;
                _setAccent(255, 184, 77);
                break;
            case UpdateVisualState::Installing:
                m_icon = material::INSTALL_APP;
                _setAccent(111, 207, 151);
                break;
            case UpdateVisualState::Success:
                m_icon = 0xE5CA;
                _setAccent(111, 207, 151);
                m_targetProgress = 1.f;
                break;
            case UpdateVisualState::Error:
                m_icon = 0xE000;
                _setAccent(255, 112, 112);
                break;
            case UpdateVisualState::Manual:
                m_icon = material::DESCRIPTION;
                _setAccent(255, 184, 77);
                m_targetProgress = 1.f;
                break;
        }
        this->invalidate();
    }

    void reset() {
        m_targetProgress = 0.f;
        m_displayProgress = 0.f;
        m_speed = "0 B/s";
        m_size = "0 B / 0 B";
        m_eta = L("计算剩余时间");
        setState(UpdateVisualState::Downloading, L("正在下载更新包...").c_str());
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override {
        (void)style;
        (void)ctx;
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);

        constexpr float pad = 34.f;
        constexpr float iconSize = 70.f;
        const float iconX = x + pad;
        const float iconY = y + 31.f;
        const float textX = iconX + iconSize + 22.f;
        const float barX = x + pad;
        const float barW = w - pad * 2.f;
        const float barY = y + 180.f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, iconX, iconY, iconSize, iconSize, 8.f);
        nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 34));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, iconX + 1.f, iconY + 1.f, iconSize - 2.f, iconSize - 2.f, 7.f);
        nvgStrokeColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 110));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        const std::string icon = encodeMaterialIcon(m_icon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 39.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        const float pulse = 0.84f + std::sin(m_animTime * 3.f) * 0.1f;
        nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB,
                                static_cast<unsigned char>(255.f * pulse)));
        nvgText(vg, iconX + iconSize * 0.5f, iconY + iconSize * 0.5f,
                icon.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontSize(vg, 25.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, textX, y + 37.f, L("模拟器更新").c_str(), nullptr);
        nvgFontSize(vg, 18.f);
        nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 235));
        nvgText(vg, textX, y + 78.f, m_status.c_str(), nullptr);

        nvgBeginPath(vg);
        nvgMoveTo(vg, x + pad, y + 125.f);
        nvgLineTo(vg, x + w - pad, y + 125.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 24));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 15.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(205, 210, 220, 210));
        nvgText(vg, x + pad, y + 151.f, m_size.c_str(), nullptr);

        const std::string percent = std::to_string(
            static_cast<int>(m_targetProgress * 100.f + 0.5f)) + "%";
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + w - pad, y + 151.f, percent.c_str(), nullptr);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, barX, barY, barW, 10.f, 5.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 28));
        nvgFill(vg);
        const float fillW = barW * std::clamp(m_displayProgress, 0.f, 1.f);
        if (fillW > 0.5f) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, barX, barY, fillW, 10.f, 5.f);
            nvgFillColor(vg, nvgRGBA(m_accentR, m_accentG, m_accentB, 235));
            nvgFill(vg);
            if (fillW > 12.f) {
                nvgBeginPath(vg);
                nvgCircle(vg, barX + fillW - 5.f, barY + 5.f, 2.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 205));
                nvgFill(vg);
            }
        }

        _drawMetric(vg, x + pad, y + 228.f, 0xE640, L("下载速度").c_str(), m_speed);
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + w * 0.5f, y + 216.f);
        nvgLineTo(vg, x + w * 0.5f, y + 273.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 24));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        _drawMetric(vg, x + w * 0.5f + 24.f, y + 228.f, 0xE8B5, L("剩余时间").c_str(), m_eta);
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
    UpdateVisualState m_state = UpdateVisualState::Connecting;
    std::string m_status = L("正在连接服务器...");
    std::string m_speed = "0 B/s";
    std::string m_size = "0 B / 0 B";
    std::string m_eta = L("计算剩余时间");
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

    void _setAccent(unsigned char r, unsigned char g, unsigned char b) {
        m_accentR = r;
        m_accentG = g;
        m_accentB = b;
    }

    void _drawMetric(NVGcontext* vg, float x, float y, char32_t icon,
                     const char* label, const std::string& value) {
        const std::string iconText = encodeMaterialIcon(icon);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 22.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(180, 188, 202, 185));
        nvgText(vg, x, y, iconText.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 13.f);
        nvgFillColor(vg, nvgRGBA(170, 178, 192, 170));
        nvgText(vg, x + 31.f, y - 1.f, label, nullptr);
        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 31.f, y + 22.f, value.c_str(), nullptr);
    }
};

struct UpdatePageRefs {
    UpdateProgressCanvas* canvas = nullptr;
};

static UpdatePageRefs g_updatePageRefs;

} // namespace

static std::string formatSpeed(double bytesPerSec) {
    if (bytesPerSec < 1024)
        return std::to_string((int)bytesPerSec) + " B/s";

    if (bytesPerSec < 1024 * 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB/s", bytesPerSec / 1024.0);
        return buf;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f MB/s", bytesPerSec / (1024.0 * 1024.0));
    return buf;
}

static std::string formatSize(size_t bytes) {
    if (bytes < 1024)
        return std::to_string(bytes) + " B";

    if (bytes < 1024 * 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
        return buf;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
    return buf;
}

static std::string formatETA(int seconds) {
    if (seconds <= 0)
        return "";

    if (seconds < 60)
        return L("剩余 ") + std::to_string(seconds) + L(" 秒");

    int m = seconds / 60;
    int s = seconds % 60;
    return L("剩余 ") + std::to_string(m) + L(" 分 ") + std::to_string(s) + L(" 秒");
}

brls::Box* UpdatePage::buildDialogContent(UpdatePage* self) {
    (void)self;
    g_updatePageRefs = {};

    auto* root = new brls::Box(brls::Axis::COLUMN);
    root->setWidth(620.f);
    root->setHeight(310.f);
    root->setFocusable(false);
    g_updatePageRefs.canvas = new UpdateProgressCanvas();
    root->addView(g_updatePageRefs.canvas);

    return root;
}

UpdatePage::UpdatePage()
    : brls::Dialog(buildDialogContent(this)) {
    m_progressCanvas = g_updatePageRefs.canvas;
    g_updatePageRefs = {};

    this->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(this);
    this->setCancelable(false);
    this->getAppletFrame()->setWidth(620.f);
    this->getAppletFrame()->setHeight(310.f);
    _resetActionButtons();

    this->registerAction(L("返回"), brls::BUTTON_B, [this](brls::View*) -> bool {
        if (m_canClose.load())
            _closeDialog();
        return true;
    });
}

UpdatePage::~UpdatePage() {
    m_cancelled.store(true);
    AppUpdater::instance().abort();
}

void UpdatePage::_closeDialog() {
    this->close();
}

void UpdatePage::_resetActionButtons() {
    m_canClose.store(false);
    this->clearButtons();
    this->getAppletFrame()->setHeight(310.f);
}

void UpdatePage::_showCloseButton() {
    m_canClose.store(true);
    this->clearButtons();
    this->getAppletFrame()->setHeight(382.f);
    this->addButton(L("关闭"), []() {});
    brls::Application::giveFocus(button1);
}

void UpdatePage::_updateProgress(
    float pct, const std::string& speed, const std::string& size, const std::string& eta) {
    brls::sync([this, pct, speed, size, eta]() {
        if (m_progressCanvas)
            static_cast<UpdateProgressCanvas*>(m_progressCanvas)->setProgress(
                pct, speed, size, eta);
    });
}

void UpdatePage::_setVisualState(const std::string& status, int state) {
    if (!m_progressCanvas)
        return;
    static_cast<UpdateProgressCanvas*>(m_progressCanvas)->setState(
        static_cast<UpdateVisualState>(state), status);
}

void UpdatePage::startDownload() {
    m_cancelled.store(false);

    brls::sync([this]() {
        _resetActionButtons();
        if (m_progressCanvas)
            static_cast<UpdateProgressCanvas*>(m_progressCanvas)->reset();
        brls::Application::giveFocus(this);
    });

    brls::async([this]() {
        using Clock = std::chrono::steady_clock;

        auto lastUpdate = Clock::now();
        size_t lastBytes = 0;
        double smoothedSpeed = 0;
        size_t totalSize = AppUpdater::instance().info().fileSize;
        bool extractionStateShown = false;

        _updateProgress(0, "0 B/s", "0 B / " + formatSize(totalSize), "");

        bool ok = AppUpdater::instance().download([&](size_t total, size_t now) -> bool {
            if (m_cancelled.load())
                return false;

            if (!extractionStateShown && total > 0 && now >= total) {
                extractionStateShown = true;
                _updateProgress(
                    100.f,
                    L("下载完成"),
                    formatSize(total) + " / " + formatSize(total),
                    L("正在处理压缩包"));
                brls::sync([this]() {
                    _setVisualState(L("正在解压更新包..."),
                        static_cast<int>(UpdateVisualState::Extracting));
                });
            }

            auto t = Clock::now();
            double dt = std::chrono::duration<double>(t - lastUpdate).count();

            if (dt >= 0.5) {
                double instant = dt > 0 ? (now - lastBytes) / dt : 0;
                constexpr double alpha = 0.3;
                smoothedSpeed = (smoothedSpeed < 1.0)
                    ? instant
                    : alpha * instant + (1.0 - alpha) * smoothedSpeed;

                float pct = total > 0 ? now * 100.0f / total : 0;
                int eta = smoothedSpeed > 0
                    ? static_cast<int>((total - now) / smoothedSpeed)
                    : 0;

                _updateProgress(
                    pct,
                    formatSpeed(smoothedSpeed),
                    formatSize(now) + " / " + formatSize(total),
                    formatETA(eta));

                lastBytes = now;
                lastUpdate = t;
            }

            return true;
        });

        if (m_cancelled.load())
            return;

        if (ok) {
            size_t finalTotal = AppUpdater::instance().info().fileSize;
            _updateProgress(
                100.0f,
                "  ",
                formatSize(finalTotal) + " / " + formatSize(finalTotal),
                "  ");
        }

        brls::sync([this, ok]() {
            if (!ok) {
                _setVisualState(L("下载失败，请重试"),
                    static_cast<int>(UpdateVisualState::Error));
                _showCloseButton();

                return;
            }

#ifdef __SWITCH__
            _setVisualState(L("下载完成，开始安装"),
                static_cast<int>(UpdateVisualState::Installing));
            startInstall();
#else
            _setVisualState(L("下载完成，请手动替换程序文件"),
                static_cast<int>(UpdateVisualState::Manual));
            _showCloseButton();

#endif
        });
    });
}

void UpdatePage::startInstall() {
    brls::sync([this]() {
        _setVisualState(L("正在安装更新..."),
            static_cast<int>(UpdateVisualState::Installing));
    });

    brls::async([this]() {
        bool ok = AppUpdater::instance().install();

        brls::sync([this, ok]() {

            if (ok) {
                
#ifdef __SWITCH__
                if (AppUpdater::instance().finishInstall()) {
                    _setVisualState(L("安装完成，请重启模拟器"),
                        static_cast<int>(UpdateVisualState::Success));
                    // envSetNextLoad("sdmc:/switch/GBAStation.nro", "sdmc:/switch/GBAStation.nro");
                    m_canClose.store(false);
                    this->clearButtons();
                    this->getAppletFrame()->setHeight(382.f);
                    this->addButton(L("重启模拟器"), [this]() {
                        VideoBackgroundView::setSharedAudioSuspended(true);
                        brls::Application::quit();
                    });
                    brls::Application::giveFocus(button1);
                } else {
                    _setVisualState(L("更新文件覆盖失败，已尝试恢复旧文件"),
                        static_cast<int>(UpdateVisualState::Error));
                    _showCloseButton();
                }
#else
                    brls::Application::notify(L("请手动重启"));
#endif

            } else {
#ifdef __SWITCH__
                _setVisualState(L("安装失败"),
                    static_cast<int>(UpdateVisualState::Error));
#else
                _setVisualState(L("当前平台不支持自动安装，请手动替换程序文件"),
                    static_cast<int>(UpdateVisualState::Manual));
#endif
                _showCloseButton();

            }
        });
    });
}

} // namespace beiklive
