#include "UpdatePage.hpp"
#include "core/Tools.hpp"
#include <borealis/views/label.hpp>
#include <borealis/views/rectangle.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/applet_frame.hpp>
#include <chrono>
#include <cmath>

namespace beiklive {

UpdatePage::UpdatePage() {
    _initLayout();
}

UpdatePage::~UpdatePage() {
    m_cancelled.store(true);
}

void UpdatePage::_initLayout() {
    this->showHeader(false);
    this->showFooter(false);
    this->getContentBox()->setAxis(brls::Axis::COLUMN);
    this->getContentBox()->setAlignItems(brls::AlignItems::CENTER);
    this->getContentBox()->setJustifyContent(brls::JustifyContent::CENTER);
    this->getContentBox()->setGrow(1.0f);
    this->getContentBox()->setBackgroundColor(nvgRGBA(20, 20, 28, 255));

    // 主容器
    auto* mainBox = new brls::Box(brls::Axis::COLUMN);
    mainBox->setFocusable(false);
    mainBox->setAlignItems(brls::AlignItems::CENTER);
    mainBox->setJustifyContent(brls::JustifyContent::CENTER);
    mainBox->setGrow(1.0f);
    mainBox->setWidthPercentage(70.f);

    // 标题区
    m_titleLabel = new brls::Label();
    m_titleLabel->setText("系统更新");
    m_titleLabel->setFontSize(30.f);
    m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_titleLabel->setMarginBottom(8.f);
    m_titleLabel->setFocusable(false);
    mainBox->addView(m_titleLabel);

    // 分割线
    auto* div = new brls::Rectangle(nvgRGBA(79, 193, 255, 100));
    div->setWidth(60.f);
    div->setHeight(3.f);
    div->setCornerRadius(1.5f);
    div->setMarginBottom(30.f);
    mainBox->addView(div);

    // 状态文字
    m_statusLabel = new brls::Label();
    m_statusLabel->setText("正在连接服务器...");
    m_statusLabel->setFontSize(20.f);
    m_statusLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_statusLabel->setMarginBottom(24.f);
    m_statusLabel->setFocusable(false);
    mainBox->addView(m_statusLabel);

    // 进度条容器
    auto* progressBox = new brls::Box(brls::Axis::COLUMN);
    progressBox->setFocusable(false);
    progressBox->setAlignItems(brls::AlignItems::CENTER);
    progressBox->setWidthPercentage(80.f);

    // 进度条背景
    m_progressBg = new brls::Rectangle(nvgRGBA(50, 50, 60, 255));
    m_progressBg->setWidthPercentage(100.f);
    m_progressBg->setHeight(10.f);
    m_progressBg->setCornerRadius(5.f);
    m_progressBg->setFocusable(false);
    progressBox->addView(m_progressBg);

    // 进度条填充
    m_progressBar = new brls::Rectangle(nvgRGBA(79, 193, 255, 255));
    m_progressBar->setWidth(0.f);
    m_progressBar->setHeight(10.f);
    m_progressBar->setCornerRadius(5.f);
    m_progressBar->setFocusable(false);
    m_progressBar->setPositionType(brls::PositionType::ABSOLUTE);
    progressBox->addView(m_progressBar);

    // 百分比文字
    m_pctLabel = new brls::Label();
    m_pctLabel->setText("0%");
    m_pctLabel->setFontSize(32.f);
    m_pctLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_pctLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_pctLabel->setMarginTop(16.f);
    m_pctLabel->setMarginBottom(12.f);
    m_pctLabel->setFocusable(false);
    progressBox->addView(m_pctLabel);

    mainBox->addView(progressBox);

    // 详情区（速度和大小）
    auto* detailBox = new brls::Box(brls::Axis::COLUMN);
    detailBox->setFocusable(false);
    detailBox->setAlignItems(brls::AlignItems::CENTER);
    detailBox->setMarginTop(20.f);

    m_speedLabel = new brls::Label();
    m_speedLabel->setText("");
    m_speedLabel->setFontSize(16.f);
    m_speedLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_speedLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_speedLabel->setFocusable(false);
    detailBox->addView(m_speedLabel);

    m_sizeLabel = new brls::Label();
    m_sizeLabel->setText("");
    m_sizeLabel->setFontSize(16.f);
    m_sizeLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_sizeLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_sizeLabel->setMarginTop(4.f);
    m_sizeLabel->setFocusable(false);
    detailBox->addView(m_sizeLabel);

    m_etaLabel = new brls::Label();
    m_etaLabel->setText("");
    m_etaLabel->setFontSize(16.f);
    m_etaLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_etaLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_etaLabel->setMarginTop(4.f);
    m_etaLabel->setFocusable(false);
    detailBox->addView(m_etaLabel);

    mainBox->addView(detailBox);

    // 按钮区
    m_btnBox = new brls::Box(brls::Axis::ROW);
    m_btnBox->setFocusable(false);
    m_btnBox->setAlignItems(brls::AlignItems::CENTER);
    m_btnBox->setJustifyContent(brls::JustifyContent::CENTER);
    m_btnBox->setMarginTop(30.f);

    auto* cancelBtn = new brls::DetailCell();
    cancelBtn->setText("取消下载");
    cancelBtn->setDetailText("\uE03E");
    cancelBtn->registerClickAction(
        [this](brls::View*) -> bool {
            m_cancelled.store(true);
            if (m_onCancel) m_onCancel();
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return true;
        });
    m_btnBox->addView(cancelBtn);

    mainBox->addView(m_btnBox);

    this->getContentBox()->addView(mainBox);
}

void UpdatePage::_updateProgress(float pct, const std::string& speed,
                                  const std::string& size, const std::string& eta) {
    brls::sync([this, pct, speed, size, eta]() {
        float maxW = m_progressBg->getWidth();
        m_progressBar->setWidth(maxW * pct / 100.f);
        m_pctLabel->setText(std::to_string(static_cast<int>(pct)) + "%");
        m_speedLabel->setText(speed);
        m_sizeLabel->setText(size);
        m_etaLabel->setText(eta);

        if (pct >= 99.5f) {
            m_progressBar->setColor(nvgRGB(129, 199, 132));
            m_statusLabel->setText("下载完成");
            m_statusLabel->setTextColor(nvgRGB(129, 199, 132));
        }
    });
}

static std::string formatSpeed(double bytesPerSec) {
    if (bytesPerSec < 1024) return std::to_string((int)bytesPerSec) + " B/s";
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
    if (bytes < 1024) return std::to_string(bytes) + " B";
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
    if (seconds <= 0) return "";
    if (seconds < 60) return "剩余 " + std::to_string(seconds) + " 秒";
    int m = seconds / 60;
    int s = seconds % 60;
    return "剩余 " + std::to_string(m) + " 分 " + std::to_string(s) + " 秒";
}

void UpdatePage::startDownload() {
    brls::async([this]() {
        using Clock = std::chrono::steady_clock;
        auto startTime = Clock::now();
        auto lastUpdate = startTime;
        size_t lastBytes = 0;
        double smoothedSpeed = 0;
        size_t totalSize = AppUpdater::instance().info().fileSize;

        m_statusLabel->setText("正在下载...");
        _updateProgress(0, "", "0 B / " + formatSize(totalSize), "");

        bool ok = AppUpdater::instance().download(
            [&](size_t total, size_t now) -> bool {
                if (m_cancelled.load()) return false;

                auto t = Clock::now();
                double dt = std::chrono::duration<double>(t - lastUpdate).count();
                if (dt >= 0.5) {
                    double instant = dt > 0 ? (now - lastBytes) / dt : 0;
                    constexpr double alpha = 0.3;
                    smoothedSpeed = (smoothedSpeed < 1.0)
                        ? instant : alpha * instant + (1.0 - alpha) * smoothedSpeed;

                    float pct = total > 0 ? now * 100.0f / total : 0;
                    int eta = smoothedSpeed > 0 ? static_cast<int>((total - now) / smoothedSpeed) : 0;

                    _updateProgress(pct,
                        formatSpeed(smoothedSpeed),
                        formatSize(now) + " / " + formatSize(total),
                        formatETA(eta));

                    lastBytes = now;
                    lastUpdate = t;
                }
                return true;
            });

        if (m_cancelled.load()) return;

        brls::sync([this, ok]() {
            if (!ok) {
                m_statusLabel->setText("下载失败，请重试");
                m_statusLabel->setTextColor(nvgRGB(255, 100, 100));

                m_btnBox->clearViews(true);
                auto* retryBtn = new brls::DetailCell();
                retryBtn->setText("重试");
                retryBtn->registerClickAction([this](brls::View*) -> bool {
                    startDownload();
                    return true;
                });
                m_btnBox->addView(retryBtn);

                auto* closeBtn = new brls::DetailCell();
                closeBtn->setText("关闭");
                closeBtn->registerClickAction([](brls::View*) -> bool {
                    brls::Application::popActivity(brls::TransitionAnimation::NONE);
                    return true;
                });
                m_btnBox->addView(closeBtn);
                return;
            }

            m_statusLabel->setText("下载完成，是否安装更新？");
            m_statusLabel->setTextColor(nvgRGB(129, 199, 132));

            m_btnBox->clearViews(true);
            auto* installBtn = new brls::DetailCell();
            installBtn->setText("安装更新");
            installBtn->registerClickAction([this](brls::View*) -> bool {
                startInstall();
                return true;
            });
            m_btnBox->addView(installBtn);

            auto* closeBtn2 = new brls::DetailCell();
            closeBtn2->setText("稍后");
            closeBtn2->registerClickAction([](brls::View*) -> bool {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
                return true;
            });
            m_btnBox->addView(closeBtn2);
        });
    });
}

void UpdatePage::startInstall() {
    bool ok = AppUpdater::instance().install();
    if (ok) {
        m_statusLabel->setText("安装完成，请重启应用");
        m_statusLabel->setTextColor(nvgRGB(129, 199, 132));
        m_btnBox->clearViews(true);

        auto* rebootBtn = new brls::DetailCell();
        rebootBtn->setText("重启");
        rebootBtn->registerClickAction([](brls::View*) -> bool {
#ifdef __SWITCH__
            brls::Application::quit();
#else
            brls::Application::notify("请手动重启");
#endif
            return true;
        });
        m_btnBox->addView(rebootBtn);
    } else {
        m_statusLabel->setText("安装失败");
        m_statusLabel->setTextColor(nvgRGB(255, 100, 100));
    }
}

} // namespace beiklive
