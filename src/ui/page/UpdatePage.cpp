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

    // 卡片容器
    auto* card = new brls::Box(brls::Axis::COLUMN);
    card->setFocusable(true);
    card->setCornerRadius(16.f);
    card->setBackgroundColor(nvgRGBA(30, 30, 40, 240));
    card->setShadowType(brls::ShadowType::GENERIC);
    card->setShadowVisibility(true);
    card->setAlignItems(brls::AlignItems::CENTER);
    card->setPadding(30.f, 50.f, 30.f, 50.f);
    card->setWidth(480.f);

    // 标题
    m_titleLabel = new brls::Label();
    m_titleLabel->setText("系统更新");
    m_titleLabel->setFontSize(26.f);
    m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_titleLabel->setMarginBottom(4.f);
    m_titleLabel->setFocusable(false);
    card->addView(m_titleLabel);

    auto* div = new brls::Rectangle(nvgRGBA(79, 193, 255, 80));
    div->setWidth(50.f);
    div->setHeight(2.f);
    div->setCornerRadius(1.f);
    div->setMarginBottom(20.f);
    card->addView(div);

    // 状态
    m_statusLabel = new brls::Label();
    m_statusLabel->setText("正在连接...");
    m_statusLabel->setFontSize(18.f);
    m_statusLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_statusLabel->setMarginBottom(16.f);
    m_statusLabel->setFocusable(false);
    card->addView(m_statusLabel);

    // 进度条背景
    m_progressBg = new brls::Rectangle(nvgRGBA(50, 50, 60, 255));
    m_progressBg->setWidth(380.f);
    m_progressBg->setHeight(8.f);
    m_progressBg->setCornerRadius(4.f);
    m_progressBg->setFocusable(false);
    card->addView(m_progressBg);

    // 进度条
    m_progressBar = new brls::Rectangle(nvgRGBA(79, 193, 255, 255));
    m_progressBar->setWidth(0.f);
    m_progressBar->setHeight(8.f);
    m_progressBar->setCornerRadius(4.f);
    m_progressBar->setFocusable(false);
    m_progressBar->setPositionType(brls::PositionType::ABSOLUTE);
    card->addView(m_progressBar);

    // 百分比
    m_pctLabel = new brls::Label();
    m_pctLabel->setText("0%");
    m_pctLabel->setFontSize(28.f);
    m_pctLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_pctLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_pctLabel->setMarginTop(12.f);
    m_pctLabel->setMarginBottom(8.f);
    m_pctLabel->setFocusable(false);
    card->addView(m_pctLabel);

    // 详情
    m_speedLabel = new brls::Label();
    m_speedLabel->setText("");
    m_speedLabel->setFontSize(15.f);
    m_speedLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_speedLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_speedLabel->setFocusable(false);
    card->addView(m_speedLabel);

    m_sizeLabel = new brls::Label();
    m_sizeLabel->setText("");
    m_sizeLabel->setFontSize(15.f);
    m_sizeLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_sizeLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_sizeLabel->setMarginTop(2.f);
    m_sizeLabel->setFocusable(false);
    card->addView(m_sizeLabel);

    m_etaLabel = new brls::Label();
    m_etaLabel->setText("");
    m_etaLabel->setFontSize(15.f);
    m_etaLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_etaLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_etaLabel->setMarginTop(2.f);
    m_etaLabel->setFocusable(false);
    card->addView(m_etaLabel);

    // 按钮区
    m_btnBox = new brls::Box(brls::Axis::ROW);
    m_btnBox->setFocusable(true);
    m_btnBox->setAlignItems(brls::AlignItems::CENTER);
    m_btnBox->setJustifyContent(brls::JustifyContent::CENTER);
    m_btnBox->setMarginTop(20.f);
    card->addView(m_btnBox);

    this->getContentBox()->addView(card);

    // 默认取消按钮
    m_cancelBtn = new brls::DetailCell();
    m_cancelBtn->setText("取消");
    m_cancelBtn->registerClickAction([this](brls::View*) -> bool {
        m_cancelled.store(true);
        if (m_onCancel) m_onCancel();
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        return true;
    });
    m_btnBox->addView(m_cancelBtn);
}

void UpdatePage::_updateProgress(float pct, const std::string& speed,
                                  const std::string& size, const std::string& eta) {
    brls::sync([this, pct, speed, size, eta]() {
        m_progressBar->setWidth(380.f * pct / 100.f);
        m_pctLabel->setText(std::to_string(static_cast<int>(pct)) + "%");
        m_speedLabel->setText(speed);
        m_sizeLabel->setText(size);
        m_etaLabel->setText(eta);

        if (pct >= 99.5f) {
            m_progressBar->setColor(nvgRGB(129, 199, 132));
            m_statusLabel->setText("下载完成");
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
        auto lastUpdate = Clock::now();
        size_t lastBytes = 0;
        double smoothedSpeed = 0;
        size_t totalSize = AppUpdater::instance().info().fileSize;

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
            // 先移除取消按钮
            m_btnBox->clearViews(true);

            if (!ok) {
                m_statusLabel->setText("下载失败，请重试");

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

                brls::Application::giveFocus(retryBtn);
                return;
            }

            m_statusLabel->setText("下载完成，是否安装？");

            auto* installBtn = new brls::DetailCell();
            installBtn->setText("安装");
            installBtn->registerClickAction([this](brls::View*) -> bool {
                startInstall();
                return true;
            });
            m_btnBox->addView(installBtn);

            auto* laterBtn = new brls::DetailCell();
            laterBtn->setText("稍后");
            laterBtn->registerClickAction([](brls::View*) -> bool {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
                return true;
            });
            m_btnBox->addView(laterBtn);

            brls::Application::giveFocus(installBtn);
        });
    });
}

void UpdatePage::startInstall() {
    bool ok = AppUpdater::instance().install();
    brls::sync([this, ok]() {
        m_btnBox->clearViews(true);

        if (ok) {
            m_statusLabel->setText("安装完成，请重启");

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
            brls::Application::giveFocus(rebootBtn);
        } else {
            m_statusLabel->setText("安装失败");

            auto* closeBtn = new brls::DetailCell();
            closeBtn->setText("关闭");
            closeBtn->registerClickAction([](brls::View*) -> bool {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
                return true;
            });
            m_btnBox->addView(closeBtn);
            brls::Application::giveFocus(closeBtn);
        }
    });
}

} // namespace beiklive
