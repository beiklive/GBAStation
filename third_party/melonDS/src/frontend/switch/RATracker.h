#pragma once
#include <string>
#include <ctime>
#include "Gfx.h"
#include <unordered_map>

struct LeaderboardTracker {
    bool active = false;
    std::string id;
    std::string value;
    time_t startTime = 0;
    int durationSeconds = 0;

    void Show(const std::string& trackerId, const std::string& display, int duration = 0);
    void Update(const std::string& display);
    void Hide();
    void Render();
};

extern std::unordered_map<std::string, LeaderboardTracker> g_leaderboardTrackers;

LeaderboardTracker* find_tracker(const std::string& id);
void create_tracker(const std::string& id, const std::string& display);
void destroy_tracker(const std::string& id);