#pragma once
#include <string>
#include <vector>

struct Achievement {
    std::string title;
    std::string description;
    std::string progress;
    int textureId = -1;
    int width = 0;
    int height = 0;
};

extern std::vector<Achievement> g_achievements;

extern bool g_loadAchievements;

void InitRetroAchievements(const char* username, const char* password, bool isToken);

void load_game_from_file(const char* path);

void rc_client_process_ra();

bool isConnected();

std::vector<Achievement> achievements_list();

void resetRCClient();

void capture_retroachievements_state(FILE* file);

int restore_retroachievements_state(FILE* file);

void rc_client_paused();