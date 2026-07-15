/*
Borealis, a Nintendo Switch UI Library
Copyright (C) 2019  natinusala
Copyright (C) 2024  xfangfang

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/notification_manager.hpp>

#include <algorithm>

namespace brls
{

NotificationManager::NotificationManager()
{
    float width = Application::getStyle().getMetric("brls/notification/width");
    this->setWidth(width);
    this->setTranslationX(Application::ORIGINAL_WINDOW_WIDTH - width);
    this->setTranslationY(26.0f);
    this->setAxis(Axis::COLUMN);
}

void NotificationManager::notify(const std::string& text)
{
    // Create the notification
    brls::Logger::debug("Showing notification \"{}\"", text);

    auto* notification = new Notification(text);
    this->addView(notification, 0);

    // Timeout timer
    auto style    = Application::getStyle();
    float timeout = style.getMetric("brls/animations/notification_timeout");
    float show    = style.getMetric("brls/animations/notification_show");
    float slide   = style.getMetric("brls/notification/slide");
    notification->timeoutTimer.reset(slide);
    notification->timeoutTimer.addStep(0.0f, (int)show, EasingFunction::exponentialOut);
    notification->timeoutTimer.addStep(0.0f, (int)timeout, EasingFunction::linear);
    notification->timeoutTimer.addStep(slide, (int)show, EasingFunction::exponentialIn);

    notification->timeoutTimer.setTickCallback([notification, slide]()
        {
            float position = notification->timeoutTimer.getValue();
            notification->setTranslationX(position);
            notification->setAlpha(1.0f - position / slide);
        });

    notification->timeoutTimer.setEndCallback([this, notification](bool finished)
        { this->removeView(notification); });

    notification->timeoutTimer.start();
}

NotificationManager::~NotificationManager()
{
    std::vector<View*> views = this->getChildren();
    for (auto& view : views)
    {
        auto label = dynamic_cast<Notification*>(view);
        label->timeoutTimer.stop();
    }
}

Notification::Notification(const std::string& text)
{
    this->setBackground(ViewBackground::NONE);
    auto style    = Application::getStyle();
    float padding = style.getMetric("brls/notification/padding");
    this->setPadding(padding, padding + 2.0f, padding, padding + 18.0f);
    float width = style.getMetric("brls/notification/width");
    this->setWidth(width);
    this->setMinHeight(68.0f);
    this->setMarginBottom(10.0f);
    this->setCornerRadius(8.0f);
    this->label = new Label();
    this->label->setText(text);
    this->label->setFontSize(17.0f);
    this->label->setLineHeight(1.35f);
    this->label->setSingleLine(false);
    this->label->setTextColor(RGBA(238, 243, 249, 240));
    this->addView(label);
}

void Notification::draw(NVGcontext* vg, float x, float y, float width, float height,
                        Style style, FrameContext* ctx)
{
    const float radius = 8.0f;
    NVGpaint shadow = nvgBoxGradient(vg, x + 5.0f, y + 6.0f,
        width, height, radius, 5.0f,
        RGBA(0, 0, 0, 105), TRANSPARENT);
    nvgBeginPath(vg);
    nvgRect(vg, x - 3.0f, y - 3.0f, width + 16.0f, height + 17.0f);
    nvgRoundedRect(vg, x, y, width, height, radius);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, radius);
    nvgFillColor(vg, RGBA(24, 29, 36, 236));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + 1.0f, y + 1.0f,
        width - 2.0f, height - 2.0f, radius - 1.0f);
    nvgStrokeColor(vg, RGBA(255, 255, 255, 50));
    nvgStrokeWidth(vg, 1.5f);
    nvgStroke(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + 10.0f, y + 14.0f, 4.0f,
        std::max(16.0f, height - 28.0f), 2.0f);
    nvgFillColor(vg, RGBA(79, 193, 255, 230));
    nvgFill(vg);

    Box::draw(vg, x, y, width, height, style, ctx);
}

Notification::~Notification() = default;

}; // namespace brls
