#ifndef KEYEXPLANATIONS_H
#define KEYEXPLANATIONS_H

#include "BoxGui.h"

namespace KeyExplanation
{

enum
{
    button_A,
    button_B,
    button_X,
    button_Y,
    button_Plus,
    button_Minus,

    buttons_Count
};

void Reset();
void Explain(int button, const char* explanation);
void DoGui(BoxGui::Frame& parent);

}

#endif