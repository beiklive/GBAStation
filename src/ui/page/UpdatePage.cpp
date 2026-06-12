#include "UpdatePage.hpp"

#include "core/Tools.hpp"

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

struct UpdatePageRefs
{
    brls::Label* titleLabel = nullptr;
    brls::Label* statusLabel = nullptr;
    brls::Label* speedLabel = nullptr;
    brls::Label* sizeLabel = nullptr;
    brls::Label* etaLabel = nullptr;
    brls::Label* pctLabel = nullptr;
    brls::Rectangle* progressBar = nullptr;
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
        return "剩余 " + std::to_string(seconds) + " 秒";

    int m = seconds / 60;
    int s = seconds % 60;
    return "剩余 " + std::to_string(m) + " 分 " + std::to_string(s) + " 秒";
}

brls::Box* UpdatePage::buildDialogContent(UpdatePage* self) {
    (void)self;
    g_updatePageRefs = {};

    auto* root = new brls::Box(brls::Axis::COLUMN);
    root->setWidthPercentage(100.f);
    root->setHeightPercentage(100.f);
    root->setPadding(24.f, 30.f, 10.f, 30.f);
    root->setFocusable(false);

    g_updatePageRefs.titleLabel = new brls::Label();
    g_updatePageRefs.titleLabel->setText("模拟器更新");
    g_updatePageRefs.titleLabel->setFontSize(30.f);
    g_updatePageRefs.titleLabel->setTextColor(nvgRGB(255, 255, 255));
    g_updatePageRefs.titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    g_updatePageRefs.titleLabel->setMarginBottom(18.f);
    g_updatePageRefs.titleLabel->setFocusable(false);
    root->addView(g_updatePageRefs.titleLabel);

    g_updatePageRefs.statusLabel = new brls::Label();
    g_updatePageRefs.statusLabel->setText("正在连接服务器...");
    g_updatePageRefs.statusLabel->setFontSize(20.f);
    g_updatePageRefs.statusLabel->setTextColor(nvgRGBA(255, 255, 255, 200));
    g_updatePageRefs.statusLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    g_updatePageRefs.statusLabel->setMarginBottom(24.f);
    g_updatePageRefs.statusLabel->setFocusable(false);
    root->addView(g_updatePageRefs.statusLabel);

    auto* progressTrack = new brls::Box(brls::Axis::ROW);
    progressTrack->setWidthPercentage(100.f);
    progressTrack->setHeight(12.f);
    progressTrack->setCornerRadius(6.f);
    progressTrack->setBackgroundColor(nvgRGBA(255, 255, 255, 32));
    progressTrack->setMarginBottom(12.f);
    progressTrack->setFocusable(false);

    g_updatePageRefs.progressBar = new brls::Rectangle(nvgRGB(59, 167, 255));
    g_updatePageRefs.progressBar->setHeight(12.f);
    g_updatePageRefs.progressBar->setWidth(0.f);
    g_updatePageRefs.progressBar->setCornerRadius(6.f);
    g_updatePageRefs.progressBar->setFocusable(false);
    progressTrack->addView(g_updatePageRefs.progressBar);
    root->addView(progressTrack);

    g_updatePageRefs.pctLabel = new brls::Label();
    g_updatePageRefs.pctLabel->setText("0%");
    g_updatePageRefs.pctLabel->setFontSize(46.f);
    g_updatePageRefs.pctLabel->setTextColor(nvgRGB(255, 255, 255));
    g_updatePageRefs.pctLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    g_updatePageRefs.pctLabel->setMarginBottom(18.f);
    g_updatePageRefs.pctLabel->setFocusable(false);
    root->addView(g_updatePageRefs.pctLabel);

    auto* infoRow = new brls::Box(brls::Axis::ROW);
    infoRow->setWidthPercentage(100.f);
    infoRow->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    infoRow->setMarginBottom(10.f);
    infoRow->setFocusable(false);

    g_updatePageRefs.speedLabel = new brls::Label();
    g_updatePageRefs.speedLabel->setFontSize(18.f);
    g_updatePageRefs.speedLabel->setTextColor(nvgRGBA(255, 255, 255, 120));
    g_updatePageRefs.speedLabel->setFocusable(false);

    g_updatePageRefs.sizeLabel = new brls::Label();
    g_updatePageRefs.sizeLabel->setFontSize(18.f);
    g_updatePageRefs.sizeLabel->setTextColor(nvgRGBA(255, 255, 255, 120));
    g_updatePageRefs.sizeLabel->setFocusable(false);

    infoRow->addView(g_updatePageRefs.speedLabel);
    infoRow->addView(g_updatePageRefs.sizeLabel);
    root->addView(infoRow);

    g_updatePageRefs.etaLabel = new brls::Label();
    g_updatePageRefs.etaLabel->setFontSize(18.f);
    g_updatePageRefs.etaLabel->setTextColor(nvgRGBA(255, 255, 255, 120));
    g_updatePageRefs.etaLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    g_updatePageRefs.etaLabel->setMarginBottom(30.f);
    g_updatePageRefs.etaLabel->setFocusable(false);
    root->addView(g_updatePageRefs.etaLabel);



    return root;
}

UpdatePage::UpdatePage()
    : brls::Dialog(buildDialogContent(this)) {
    m_titleLabel = g_updatePageRefs.titleLabel;
    m_statusLabel = g_updatePageRefs.statusLabel;
    m_speedLabel = g_updatePageRefs.speedLabel;
    m_sizeLabel = g_updatePageRefs.sizeLabel;
    m_etaLabel = g_updatePageRefs.etaLabel;
    m_pctLabel = g_updatePageRefs.pctLabel;
    m_progressBar = g_updatePageRefs.progressBar;
    g_updatePageRefs = {};

    this->setCancelable(false);
    this->getAppletFrame()->setWidth(620.f);
    this->getAppletFrame()->setHeight(340.f);
    this->addButton("取消", [this]() {
        m_cancelled.store(true);
        if (m_onCancel)
            m_onCancel();
    });
    _resetActionButtons();

    this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
        m_cancelled.store(true);
        if (m_onCancel)
            m_onCancel();
        _closeDialog();
        return true;
    });
}

UpdatePage::~UpdatePage() {
    m_cancelled.store(true);
}

void UpdatePage::_closeDialog() {
    this->close();
}

brls::Button* UpdatePage::_makeActionButton(
    const std::string& text, std::function<bool(brls::View*)> onClick) {
    auto* button = new brls::Button();
    button->setText(text);
    button->setWidth(176.f);
    button->setBorderColor(nvgRGBA(255, 255, 255, 255));
    button->setBorderThickness(1.f);
    button->registerClickAction(std::move(onClick));
    return button;
}

void UpdatePage::_resetActionButtons() {
}

void UpdatePage::_updateProgress(
    float pct, const std::string& speed, const std::string& size, const std::string& eta) {
    brls::sync([this, pct, speed, size, eta]() {
        m_progressBar->setWidth(560.f * pct / 100.f);
        m_pctLabel->setText(std::to_string(static_cast<int>(pct)) + "%");
        m_speedLabel->setText(speed);
        m_sizeLabel->setText(size);
        m_etaLabel->setText(eta);

        if (pct >= 100.0f) {
            m_progressBar->setColor(nvgRGB(60, 220, 120));
            m_statusLabel->setText("下载完成");
        }
    });
}

void UpdatePage::startDownload() {
    m_cancelled.store(false);

    brls::sync([this]() {
        _resetActionButtons();
        m_progressBar->setWidth(0.f);
        m_progressBar->setColor(nvgRGB(59, 167, 255));
        m_pctLabel->setText("0%");
        m_statusLabel->setText("正在下载...");
        m_speedLabel->setText("0 B/s");
        m_sizeLabel->setText("0 B / 0 B");
        m_etaLabel->setText("");
    });

    brls::async([this]() {
        using Clock = std::chrono::steady_clock;

        auto lastUpdate = Clock::now();
        size_t lastBytes = 0;
        double smoothedSpeed = 0;
        size_t totalSize = AppUpdater::instance().info().fileSize;

        _updateProgress(0, "0 B/s", "0 B / " + formatSize(totalSize), "");

        bool ok = AppUpdater::instance().download([&](size_t total, size_t now) -> bool {
            if (m_cancelled.load())
                return false;

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
            brls::Application::giveFocus(nullptr);

            if (!ok) {
                m_statusLabel->setText("下载失败，请重试");
                m_progressBar->setColor(nvgRGB(255, 120, 120));

                return;
            }

#ifdef __SWITCH__
            m_statusLabel->setText("下载完成，开始安装");
            startInstall();
#else
            m_statusLabel->setText("下载完成，请手动替换程序文件");

#endif
        });
    });
}

void UpdatePage::startInstall() {
    brls::sync([this]() {
        m_statusLabel->setText("正在安装...");
        m_etaLabel->setText(" ");
    });

    brls::async([this]() {
        bool ok = AppUpdater::instance().install();

        brls::sync([this, ok]() {

            if (ok) {
                
#ifdef __SWITCH__
                AppUpdater::instance().finishInstall();
                m_statusLabel->setText("安装完成，请重启模拟器");
                // envSetNextLoad("sdmc:/switch/GBAStation.nro", "sdmc:/switch/GBAStation.nro");
                this->clearButtons();
                this->addButton("重启模拟器", [this]() {
                    brls::Application::quit();
                });
#else
                    brls::Application::notify("请手动重启");
#endif

            } else {
#ifdef __SWITCH__
                m_statusLabel->setText("安装失败");
#else
                m_statusLabel->setText("当前平台不支持自动安装，请手动替换程序文件");
#endif
                m_progressBar->setColor(nvgRGB(255, 120, 120));

            }
        });
    });
}

} // namespace beiklive
