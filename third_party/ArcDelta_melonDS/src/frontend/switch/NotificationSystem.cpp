#include "NotificationSystem.h"
#include "Style.h"
#include "PlatformConfig.h"
#include <cmath>
#include <cstdio>
#include <queue>
#include <cstdarg>
#include <list>

Notification g_notification;
static std::queue<Notification> notificationQueue;
static Notification* currentNotification = nullptr;

void Notification::Show(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    Notification newNotif;
    newNotif.message = buffer;
    newNotif.startTime = time(nullptr);
    newNotif.active = true;
    notificationQueue.push(newNotif);
}

void DestroyNotification(const Notification& notification) {
    if (notification.textureId != -1) {
        Gfx::TextureDelete(notification.textureId);
    }
}

void Notification::ShowWithIcon(int texId, int width, int height, const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    Notification newNotif;
    newNotif.message = buffer;
    newNotif.textureId = texId;
    newNotif.nwidth = width;
    newNotif.nheight = height;
    newNotif.startTime = time(nullptr);
    newNotif.active = true;
    notificationQueue.push(newNotif);
}

void Notification::Render() {
    if (currentNotification == nullptr && !notificationQueue.empty()) {
        currentNotification = new Notification(notificationQueue.front());
        notificationQueue.pop();
    }

    if (currentNotification != nullptr) {
        Notification& notif = *currentNotification;

        if (!notif.active || Config::notification) {
            delete currentNotification;
            currentNotification = nullptr;
            return;
        }

        if (!notif.persistent && difftime(time(nullptr), notif.startTime) > notif.durationSeconds) {
            notif.active = false;
            DestroyNotification(notif);
            delete currentNotification;
            currentNotification = nullptr;
            return;
        }

        Gfx::Vector2f textSize = Gfx::MeasureText(Gfx::SystemFontStandard, TextLineHeight, notif.message.c_str());
        float padding = 20.f;
        float avatarSize = (notif.textureId >= 0 && notif.nwidth > 0 && notif.nheight > 0) ? 40.f : 0.f;
        float spacing = (avatarSize > 0) ? 10.f : 0.f;
        float rectWidth = textSize.X + padding * 2 + avatarSize + spacing;
        float rectHeight = std::max(textSize.Y + padding * 2, avatarSize + padding * 2);

        Gfx::Vector2f boxPos;
        Gfx::Vector2f boxSize = {rectWidth, rectHeight};

        switch (Config::GlobalRotation) {
            case 1:  // 90°
                boxPos = {20.f, 20.f};
                break;
            case 2:  // 180°
                boxPos = {1280.f - rectWidth - 20.f, 720.f - rectHeight - 20.f};
                break;
            case 3:  // 270°
                boxPos = {20.f, 720.f - rectWidth - 20.f};
                break;
            default:  // 0°
                boxPos = {1280.f - rectWidth - 20.f, 20.f};
                break;
        }

        Gfx::DrawRectangle(boxPos, boxSize, WidgetColorBright, true);

        Gfx::Vector2f localTextOffset = {
            padding + avatarSize + spacing,
            (rectHeight - textSize.Y) / 2.f
        };

        Gfx::Vector2f rotatedTextOffset;
        switch (Config::GlobalRotation) {
            case 1:  // 90°
                rotatedTextOffset = {
                    boxPos.X + localTextOffset.Y,
                    boxPos.Y + localTextOffset.X
                };
                break;
            case 2:  // 180°
                rotatedTextOffset = {
                    boxPos.X + rectWidth - localTextOffset.X - textSize.X,
                    boxPos.Y + rectHeight - localTextOffset.Y - textSize.Y
                };
                break;
            case 3:  // 270°
                rotatedTextOffset = {
                    boxPos.X + localTextOffset.Y,
                    boxPos.Y + localTextOffset.X
                };
                break;
            default:  // 0°
                rotatedTextOffset = {
                    boxPos.X + localTextOffset.X,
                    boxPos.Y + localTextOffset.Y
                };
                break;
        }

        if (notif.textureId >= 0 && notif.nwidth > 0 && notif.nheight > 0) {
            Gfx::Vector2f avatarPos = boxPos + Gfx::Vector2f{padding, (rectHeight - avatarSize) / 2.f};
            Gfx::Vector2f avatarDrawSize = {avatarSize, avatarSize};
            Gfx::DrawRectangle(notif.textureId, avatarPos, avatarDrawSize,
                               {0.f, 0.f}, {static_cast<float>(notif.nwidth), static_cast<float>(notif.nheight)}, WidgetColorBright);
        }

        if (!notif.message.empty()) {
            Gfx::DrawText(Gfx::SystemFontStandard, rotatedTextOffset, TextLineHeight, DarkColor,
                          Gfx::align_Left, Gfx::align_Center, notif.message.c_str());
        }
    }
}
