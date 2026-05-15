#include "UpdateDialog.hpp"

namespace beiklive {

UpdateDialog::UpdateDialog(const std::string& title, const std::string& body) {

    this->showHeader(false);
    this->showFooter(false);
    this->getContentBox()->setAlignItems(brls::AlignItems::CENTER);
this->getContentBox()->setJustifyContent(brls::JustifyContent::CENTER);

    auto* card = new brls::Box(brls::Axis::COLUMN);
    card->setWidth(700);
    card->setHeight(500);
    card->setCornerRadius(16);
    card->setBackgroundColor(nvgRGBA(25, 28, 40, 250));
    card->setShadowType(brls::ShadowType::GENERIC);
    card->setShadowVisibility(true);
    card->setPadding(32, 36, 24, 36);
    card->setAlignItems(brls::AlignItems::STRETCH);
    this->getContentBox()->addView(card);

    m_titleLabel = new brls::Label();
    m_titleLabel->setText(title);
    m_titleLabel->setFontSize(28);
    m_titleLabel->setTextColor(nvgRGB(255, 255, 255));
    m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_titleLabel->setSingleLine(false);
    m_titleLabel->setMarginBottom(18);
    card->addView(m_titleLabel);

    auto* separator = new brls::Rectangle(nvgRGBA(255, 255, 255, 20));
    separator->setHeight(1);
    separator->setWidth(588);
    separator->setMarginBottom(22);
    card->addView(separator);

    auto* scrollbox = new brls::ScrollingFrame();
    scrollbox->setGrow(1);
    scrollbox->setWidthPercentage(100.f);
    scrollbox->setScrollingIndicatorVisible(false);

    m_bodyLabel = new brls::Label();
    m_bodyLabel->setText(body);
    m_bodyLabel->setFontSize(15);
    m_bodyLabel->setTextColor(nvgRGBA(200, 200, 210, 255));
    scrollbox->addView(m_bodyLabel);
    card->addView(scrollbox);

    m_buttonBox = new brls::Box(brls::Axis::ROW);
    m_buttonBox->setWidth(588);
    m_buttonBox->setHeight(40);
    m_buttonBox->setJustifyContent(brls::JustifyContent::CENTER);
    m_buttonBox->setAlignItems(brls::AlignItems::CENTER);
    m_buttonBox->setMarginTop(24);
    card->addView(m_buttonBox);

    this->registerAction(
        "返回", brls::BUTTON_B, [this](brls::View*) {
            if (m_cancelable)
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
            return m_cancelable;
        },
        false, false, brls::SOUND_BACK);
}

void UpdateDialog::addButton(const std::string& label, std::function<void()> cb) {
    auto* btn = new brls::Button();
    btn->setText(label);
    btn->setFontSize(22);
    btn->setPadding(12, 24, 12, 24);
    btn->setMarginLeft(8);
    btn->setMarginRight(8);
    btn->setWidth(170);
    btn->setBorderColor(nvgRGBA(255, 255, 255, 255));
    btn->setBorderThickness(1);
    btn->registerClickAction([this, cb](brls::View*) -> bool {
        brls::Application::giveFocus(nullptr);
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        if (cb) cb();
        return true;
    });

    if (m_buttonBox->getChildren().empty())
        brls::Application::giveFocus(btn);

    m_buttonBox->addView(btn);
}

void UpdateDialog::open() {
    brls::Application::pushActivity(new brls::Activity(this));
    brls::Application::giveFocus(m_buttonBox->getChildren().front());
}

void UpdateDialog::close() {
    brls::Application::popActivity(brls::TransitionAnimation::NONE);
}

void UpdateDialog::setCancelable(bool cancelable) {
    m_cancelable = cancelable;
}

} // namespace beiklive
