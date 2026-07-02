#pragma once
#include <string>
#include <ctime>
#include <cstdarg>
#include "Gfx.h"
#include "PlatformConfig.h"
#include <unordered_map>

struct TriggerNotification {
    int textureId;
    int width;
    int height;
    bool active;
    int id;

    void Render(Gfx::Vector2f position) const {
        if (!active || textureId < 0) return;

        Gfx::Color brightColor = Gfx::Color{1.0f, 1.0f, 1.0f, 1.0f};

        float iconSize = 40.f;
        Gfx::DrawRectangle(textureId, position, {iconSize, iconSize},
                           {0.f, 0.f}, {static_cast<float>(width), static_cast<float>(height)}, brightColor);
    }
};

class TriggerNotificationManager {
public:
    std::vector<TriggerNotification> notifications;
    std::unordered_map<int, bool> conditionStates;

    void UpdateState(int id, int texId, int width, int height, bool conditionActive) {
        conditionStates[id] = conditionActive;

        auto it = std::find_if(notifications.begin(), notifications.end(),
                               [id](const TriggerNotification& n) { return n.id == id; });

        if (conditionActive) {
            if (it == notifications.end()) {
                notifications.push_back({texId, width, height, true, id});
            } else {
                it->active = true;
            }
        } else {
            if (it != notifications.end()) {
                it->active = false;
            }
        }
    }

    void RenderAll() {
        const float iconSize = 40.f;
        const float spacing = 10.f;
        Gfx::Vector2f startPos;

        switch (Config::GlobalRotation) {
            case 1: startPos = {20.f, 20.f}; break;
            case 2: startPos = {20.f, 720.f - iconSize - 20.f}; break;
            case 3: startPos = {1280.f - iconSize - 20.f, 720.f - iconSize - 20.f}; break;
            default: startPos = {20.f, 720.f - iconSize - 20.f}; break;
        }

        int index = 0;
        for (const auto& notif : notifications) {
            if (!notif.active) continue;
            Gfx::Vector2f pos = startPos + Gfx::Vector2f{(iconSize + spacing) * index, 0.f};
            notif.Render(pos);
            ++index;
        }
    }

    void RemoveNotificationById(int id) {
        notifications.erase(
            std::remove_if(notifications.begin(), notifications.end(), [id](const TriggerNotification& n) { return n.id == id; }),
            notifications.end());
    }


    void ClearAll() {
        notifications.clear();
        conditionStates.clear();
    }

    void DeactivateAll() {
        for (auto& notif : notifications) {
            notif.active = false;
        }
    }

    void ReactiveAll() {
        for (auto& notif : notifications) {
            notif.active = true;
        }
    }
};

extern TriggerNotificationManager g_triggerNotifications;