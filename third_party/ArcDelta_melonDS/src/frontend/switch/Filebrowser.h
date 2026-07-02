#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include "BoxGui.h"

namespace Filebrowser
{

void InferSystemLanguage();

void Init();
void DeInit();

void EnterDirectory(const char* path);

void DoGui(BoxGui::Frame& parent);

}

#endif