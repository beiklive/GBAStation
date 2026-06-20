#pragma once
#include "core/common.h"
#include "ui/widget/Box.hpp"
#include <string>
#include <functional>

namespace beiklive {

/// 通用进度对话框：居中卡片，标题 + 状态文字 + 按钮区，支持实时更新
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

} // namespace beiklive
