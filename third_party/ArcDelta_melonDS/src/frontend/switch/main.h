#ifndef MAIN_H
#define MAIN_H

enum
{
    uiScreen_Start, // either the start screen or pause screen depending on whether a game is running
    uiScreen_BrowseROM,
    uiScreen_EmulationSettings,
    uiScreen_DisplaySettings,
    uiScreen_InputSettings,
    uiScreen_RetroAchievements,
};

extern bool Done;
extern int CurrentUiScreen;

namespace Emulation
{

enum
{
    emuState_Nothing,
    emuState_Paused,
    emuState_Running,
    emuState_Quit
};

extern int State;

void LoadROM(const char* file);
void LoadBIOS();
void SetPause(bool pause);
void Stop();
void Reset();
void UpdateScreenLayout();

extern bool LidClosed;

}

#endif