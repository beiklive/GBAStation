#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"
#include "core/AppUpdater.hpp"
#include <atomic>
#include <functional>

namespace beiklive {

class UpdatePage : public beiklive::Box {
public:
    UpdatePage();
    ~UpdatePage();

    /// 设置取消回调（关闭页面时调用）
    void setOnCancel(std::function<void()> cb) { m_onCancel = std::move(cb); }

    /// 开始下载
    void startDownload();

    /// 开始安装
    void startInstall();

private:
    void _initLayout();
    void _updateProgress(float pct, const std::string& speed, const std::string& size, const std::string& eta);

    std::atomic<bool> m_cancelled{false};
    std::function<void()> m_onCancel;

    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_statusLabel = nullptr;
    brls::Label* m_speedLabel = nullptr;
    brls::Label* m_sizeLabel = nullptr;
    brls::Label* m_etaLabel = nullptr;
    brls::Label* m_pctLabel = nullptr;
    brls::Rectangle* m_progressBg = nullptr;
    brls::Rectangle* m_progressBar = nullptr;
    brls::Box* m_btnBox = nullptr;
    brls::Button* m_cancelBtn = nullptr;
};

} // namespace beiklive
