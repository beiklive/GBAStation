
#include "UpdatePage.hpp"
#include "core/Tools.hpp"
#include <borealis/views/label.hpp>
#include <borealis/views/rectangle.hpp>
#include <borealis/views/button.hpp>
#include <chrono>
#include <cmath>

namespace beiklive {

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

UpdatePage::UpdatePage() {
    _initLayout();
}

UpdatePage::~UpdatePage() {
    m_cancelled.store(true);
}

void UpdatePage::_initLayout() {
    this->showHeader(false);
    this->showFooter(false);

    auto* root = this->getContentBox();
    root->setAxis(brls::Axis::COLUMN);
    root->setWidthPercentage(100.f);
    root->setHeightPercentage(100.f);
    root->setAlignItems(brls::AlignItems::CENTER);
    root->setJustifyContent(brls::JustifyContent::CENTER);
    root->setGrow(1.0f);
    root->setBackgroundColor(nvgRGBA(0, 0, 0, 170));

    // 主卡片
    auto* card = new brls::Box(brls::Axis::COLUMN);
    card->setWidth(620.f);
    card->setHeight(480.f);
    card->setCornerRadius(28.f);
    card->setBackgroundColor(nvgRGBA(18, 20, 30, 245));
    card->setShadowType(brls::ShadowType::GENERIC);
    card->setShadowVisibility(true);
    card->setPadding(32.f, 42.f, 32.f, 42.f);
    card->setAlignItems(brls::AlignItems::STRETCH);
    card->setJustifyContent(brls::JustifyContent::CENTER);

    root->addView(card);

    // 标题
    m_titleLabel = new brls::Label();
    m_titleLabel->setText("模拟器更新");
    m_titleLabel->setFontSize(40.f);
    m_titleLabel->setTextColor(nvgRGB(255,255,255));
    m_titleLabel->setMarginBottom(18.f);
    card->addView(m_titleLabel);

    // 分割线
    auto* line = new brls::Rectangle(nvgRGBA(255,255,255,20));
    line->setHeight(1.f);
    line->setWidth(536.f);
    line->setMarginBottom(26.f);
    card->addView(line);

    // 状态
    m_statusLabel = new brls::Label();
    m_statusLabel->setText("正在连接服务器...");
    m_statusLabel->setFontSize(22.f);
    m_statusLabel->setTextColor(nvgRGBA(255,255,255,180));
    m_statusLabel->setMarginBottom(28.f);
    card->addView(m_statusLabel);


    // 进度条
    m_progressBar = new brls::Rectangle(nvgRGB(59,167,255));
    m_progressBar->setHeight(10.f);
    m_progressBar->setWidth(0.f);
    m_progressBar->setCornerRadius(2.f);
    m_progressBar->setMarginBottom(10.f);
    card->addView(m_progressBar);

    // 百分比
    m_pctLabel = new brls::Label();
    m_pctLabel->setText("0%");
    m_pctLabel->setFontSize(48.f);
    m_pctLabel->setTextColor(nvgRGB(255,255,255));
    m_pctLabel->setMarginBottom(20.f);
    card->addView(m_pctLabel);

    // 信息行
    auto* infoRow = new brls::Box(brls::Axis::ROW);
    infoRow->setWidth(536.f);
    infoRow->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    infoRow->setMarginBottom(8.f);

    m_speedLabel = new brls::Label();
    m_speedLabel->setFontSize(18.f);
    m_speedLabel->setTextColor(nvgRGBA(255,255,255,120));

    m_sizeLabel = new brls::Label();
    m_sizeLabel->setFontSize(18.f);
    m_sizeLabel->setTextColor(nvgRGBA(255,255,255,120));

    infoRow->addView(m_speedLabel);
    infoRow->addView(m_sizeLabel);
    card->addView(infoRow);

    // ETA
    m_etaLabel = new brls::Label();
    m_etaLabel->setFontSize(18.f);
    m_etaLabel->setTextColor(nvgRGBA(255,255,255,120));
    m_etaLabel->setMarginBottom(40.f);
    card->addView(m_etaLabel);

    // 按钮区域
    m_btnBox = new brls::Box(brls::Axis::ROW);
    m_btnBox->setAlignItems(brls::AlignItems::CENTER);
    m_btnBox->setJustifyContent(brls::JustifyContent::CENTER);
    m_btnBox->setMarginTop(20.f);
    card->addView(m_btnBox);

    // 默认取消按钮
    m_cancelBtn = new brls::Button();
    m_cancelBtn->setText("取消");
    m_cancelBtn->setWidth(170);
    m_cancelBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
    m_cancelBtn->setBorderThickness(1);
    m_cancelBtn->registerClickAction([this](brls::View*) -> bool {
        m_cancelled.store(true);

        if (m_onCancel)
            m_onCancel();

        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        return true;
    });

    m_btnBox->addView(m_cancelBtn);
}

void UpdatePage::_updateProgress(float pct,
                                 const std::string& speed,
                                 const std::string& size,
                                 const std::string& eta) {

    brls::sync([this, pct, speed, size, eta]() {

        m_progressBar->setWidth(536.f * pct / 100.f);

        m_pctLabel->setText(
            std::to_string(static_cast<int>(pct)) + "%"
        );

        m_speedLabel->setText(speed);
        m_sizeLabel->setText(size);
        m_etaLabel->setText(eta);

        if (pct >= 100.0f) {
            m_progressBar->setColor(nvgRGB(60,220,120));
            m_statusLabel->setText("下载完成");
        }
    });
}

void UpdatePage::startDownload() {

    brls::sync([this]() {
        m_statusLabel->setText("正在下载...");
        brls::Application::giveFocus(m_cancelBtn);
    });

    brls::async([this]() {

        using Clock = std::chrono::steady_clock;

        auto lastUpdate = Clock::now();
        size_t lastBytes = 0;
        double smoothedSpeed = 0;

        size_t totalSize = AppUpdater::instance().info().fileSize;

        _updateProgress(
            0,
            "0 B/s",
            "0 B / " + formatSize(totalSize),
            ""
        );

        bool ok = AppUpdater::instance().download(

            [&](size_t total, size_t now) -> bool {

                if (m_cancelled.load())
                    return false;

                auto t = Clock::now();

                double dt =
                    std::chrono::duration<double>(t - lastUpdate).count();

                if (dt >= 0.5) {

                    double instant =
                        dt > 0 ? (now - lastBytes) / dt : 0;

                    constexpr double alpha = 0.3;

                    smoothedSpeed =
                        (smoothedSpeed < 1.0)
                        ? instant
                        : alpha * instant +
                          (1.0 - alpha) * smoothedSpeed;

                    float pct =
                        total > 0
                        ? now * 100.0f / total
                        : 0;

                    int eta =
                        smoothedSpeed > 0
                        ? static_cast<int>((total - now) / smoothedSpeed)
                        : 0;

                    _updateProgress(
                        pct,
                        formatSpeed(smoothedSpeed),
                        formatSize(now) + " / " + formatSize(total),
                        formatETA(eta)
                    );

                    lastBytes = now;
                    lastUpdate = t;
                }

                return true;
            }
        );

        if (m_cancelled.load())
            return;

        {
        // 下载完成，强制更新进度到 100%
        size_t totalSize = AppUpdater::instance().info().fileSize;
        _updateProgress(
            100.0f,
            "  ",
            formatSize(totalSize) + " / " + formatSize(totalSize),
            "  "
        );
        }
        brls::sync([this, ok]() {

            m_btnBox->clearViews(true);
            brls::Application::giveFocus(nullptr);

            // 下载失败
            if (!ok) {

                m_statusLabel->setText("下载失败，请重试");
                m_progressBar->setColor(nvgRGB(255,120,120));

                auto* retryBtn = new brls::Button();
                retryBtn->setText("重试");
                retryBtn->setWidth(170);
                retryBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
                retryBtn->setBorderThickness(1);
                retryBtn->registerClickAction(
                    [this](brls::View*) -> bool {
                        startDownload();
                        return true;
                    }
                );

                m_btnBox->addView(retryBtn);

                auto* closeBtn = new brls::Button();
                closeBtn->setText("关闭");
                closeBtn->setWidth(170);
                closeBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
                closeBtn->setBorderThickness(1);
                closeBtn->registerClickAction(
                    [](brls::View*) -> bool {
                        brls::Application::popActivity(
                            brls::TransitionAnimation::NONE
                        );
                        return true;
                    }
                );

                m_btnBox->addView(closeBtn);
                brls::Application::giveFocus(retryBtn);
                return;
            }

            // 下载完成
            m_statusLabel->setText("下载完成，是否安装？");

            auto* installBtn = new brls::Button();
            installBtn->setText("安装");
            installBtn->setWidth(170);
            installBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
            installBtn->setBorderThickness(1);
            installBtn->registerClickAction(
                [this](brls::View*) -> bool {
                    startInstall();
                    return true;
                }
            );

            m_btnBox->addView(installBtn);

            auto* cancelBtn = new brls::Button();
            cancelBtn->setText("取消");

            cancelBtn->setWidth(170);
            cancelBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
            cancelBtn->setBorderThickness(1);
            cancelBtn->registerClickAction(
                [](brls::View*) -> bool {
                    brls::Application::popActivity(
                        brls::TransitionAnimation::NONE
                    );
                    return true;
                }
            );

            m_btnBox->addView(cancelBtn);

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

            auto* rebootBtn = new brls::Button();
            rebootBtn->setText("重启");
            rebootBtn->setWidth(170);
            rebootBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
            rebootBtn->setBorderThickness(1);
            rebootBtn->registerClickAction(
                [](brls::View*) -> bool {

#ifdef __SWITCH__
                    brls::Application::quit();
#else
                    brls::Application::notify("请手动重启");
#endif
                    return true;
                }
            );

            m_btnBox->addView(rebootBtn);
            brls::Application::giveFocus(rebootBtn);
        }
        else {

            m_statusLabel->setText("安装失败");
            m_progressBar->setColor(nvgRGB(255,120,120));

            auto* closeBtn = new brls::Button();
            closeBtn->setText("关闭");
            closeBtn->setWidth(170);
                closeBtn->setBorderColor(nvgRGBA(255, 255, 255, 255));
                closeBtn->setBorderThickness(1);
            closeBtn->registerClickAction(
                [](brls::View*) -> bool {
                    brls::Application::popActivity(
                        brls::TransitionAnimation::NONE
                    );
                    return true;
                }
            );

            m_btnBox->addView(closeBtn);
            brls::Application::giveFocus(closeBtn);
        }
    });
}

} // namespace beiklive
