#include "UpdateDialog.hpp"

#include <borealis/views/label.hpp>
#include <borealis/views/scrolling_frame.hpp>

namespace
{

brls::Box* buildUpdateDialogContent(const std::string& title, const std::string& body)
{
    auto* content = new brls::Box(brls::Axis::COLUMN);
    content->setWidth(720.f);
    content->setHeight(420.f);
    content->setPadding(24.f, 28.f, 8.f, 28.f);
    content->setFocusable(false);

    auto* titleLabel = new brls::Label();
    titleLabel->setText(title);
    titleLabel->setFontSize(26.f);
    titleLabel->setTextColor(nvgRGB(255, 255, 255));
    titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    titleLabel->setSingleLine(false);
    titleLabel->setMarginBottom(18.f);
    titleLabel->setFocusable(false);
    content->addView(titleLabel);

    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setWidthPercentage(100.f);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);

    auto* bodyLabel = new brls::Label();
    bodyLabel->setText(body);
    bodyLabel->setFontSize(16.f);
    bodyLabel->setWidthPercentage(100.f);
    bodyLabel->setTextColor(nvgRGBA(200, 200, 210, 255));
    bodyLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    bodyLabel->setSingleLine(false);
    bodyLabel->setIsWrapping(true);
    bodyLabel->setFocusable(false);
    scroll->setContentView(bodyLabel);

    content->addView(scroll);
    return content;
}

} // namespace

namespace beiklive {

UpdateDialog::UpdateDialog(const std::string& title, const std::string& body)
    : brls::Dialog(buildUpdateDialogContent(title, body))
{
    this->setCancelable(true);
}

void UpdateDialog::addButton(const std::string& label, std::function<void()> cb)
{
    brls::Dialog::addButton(label, std::move(cb));
}

void UpdateDialog::open()
{
    brls::Dialog::open();
}

void UpdateDialog::close()
{
    brls::Dialog::close();
}

void UpdateDialog::setCancelable(bool cancelable)
{
    brls::Dialog::setCancelable(cancelable);
}

} // namespace beiklive
