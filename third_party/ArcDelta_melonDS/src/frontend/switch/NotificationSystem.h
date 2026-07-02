#pragma once
#include <string>
#include <ctime>
#include <cstdarg>
#include "Gfx.h"

struct Notification {
    bool active = false;
    std::string message;
    time_t startTime = 0;
    bool persistent = false;
    int durationSeconds = 3;
    int textureId = -1;
    int nwidth = 0;
    int nheight = 0;

    void Show(const char* fmt, ...);
    void ShowWithIcon(int texId, int width, int height, const char* fmt, ...);

    void Render();
};

extern Notification g_notification;
    
void DestroyNotification();