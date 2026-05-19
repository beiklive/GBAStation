#ifndef STARTMENU_H
#define STARTMENU_H

#include "BoxGui.h"

#include <string>

namespace StartMenu
{

void Init();
void DeInit();

void DoGui(BoxGui::Frame& parent);

void PushLastPlayed(const std::string& newPath, int titleIconIdx);

}

#endif