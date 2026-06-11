#pragma once

#include "core/common.h"
#include "core/AppUpdater.hpp"

#include <borealis/views/dialog.hpp>

#include <atomic>
#include <functional>

namespace beiklive {

class UpdatePage : public brls::Dialog {
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
    static brls::Box* buildDialogContent(UpdatePage* self);
    void _updateProgress(float pct, const std::string& speed, const std::string& size, const std::string& eta);
    brls::Button* _makeActionButton(
        const std::string& text, std::function<bool(brls::View*)> onClick);
    void _resetActionButtons();
    void _closeDialog();

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
};

} // namespace beiklive
