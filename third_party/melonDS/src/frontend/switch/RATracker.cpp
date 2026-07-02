#include "RATracker.h"
#include "RetroAchievements.h"
#include "Gfx.h"
#include "Style.h" // Per WidgetColorBright, DarkColor, ecc.
#include "PlatformConfig.h" // Per Config::GlobalRotation
#include <unordered_map>
#include <cmath>

std::unordered_map<std::string, LeaderboardTracker> g_leaderboardTrackers;

void LeaderboardTracker::Show(const std::string& trackerId, const std::string& display, int duration) {
    id = trackerId;
    value = display;
    active = true;
    startTime = time(nullptr);
    durationSeconds = duration;
}

void LeaderboardTracker::Update(const std::string& display) {
    if (active) {
        value = display;
    }
}

void LeaderboardTracker::Hide() {
    active = false;
}

LeaderboardTracker* find_tracker(const std::string& id) {
    auto it = g_leaderboardTrackers.find(id);
    if (it != g_leaderboardTrackers.end()) {
        return &it->second;
    }
    return nullptr;
}

void create_tracker(const std::string& id, const std::string& display) {
    LeaderboardTracker tracker;
    tracker.Show(id, display);
    g_leaderboardTrackers[id] = tracker;
}

void destroy_tracker(const std::string& id) {
    g_leaderboardTrackers.erase(id);
}

void LeaderboardTracker::Render() {
    if (!active)
        return;

    // Controlla se il tracker è scaduto
    if (durationSeconds > 0 && difftime(time(nullptr), startTime) > durationSeconds) {
        active = false;
        return;
    }

    // Calcola dimensioni e posizione
    Gfx::Vector2f textSize = Gfx::MeasureText(Gfx::SystemFontStandard, TextLineHeight, value.c_str());
    float padding = 20.f;
    float rectWidth = textSize.X + padding * 2;
    float rectHeight = textSize.Y + padding * 2;

    Gfx::Vector2f boxPos;
    Gfx::Vector2f boxSize = {rectWidth, rectHeight};

    // Posiziona in basso a destra, considerando la rotazione dello schermo
    switch (Config::GlobalRotation) {
        case 1:  // 90°
            boxPos = {20.f, 720.f - rectWidth - 20.f};
            break;
        case 2:  // 180°
            boxPos = {20.f, 20.f};
            break;
        case 3:  // 270°
            boxPos = {1280.f - rectHeight - 20.f, 20.f};
            break;
        default:  // 0°
            boxPos = {1280.f - rectWidth - 20.f, 720.f - rectHeight - 20.f};
            break;
    }

    // Disegna il rettangolo di sfondo
    Gfx::DrawRectangle(boxPos, boxSize, WidgetColorBright, true);

    // Disegna il testo
    Gfx::DrawText(Gfx::SystemFontStandard, boxPos + Gfx::Vector2f{padding, padding}, TextLineHeight, DarkColor,
                  Gfx::align_Left, Gfx::align_Center, value.c_str());
}