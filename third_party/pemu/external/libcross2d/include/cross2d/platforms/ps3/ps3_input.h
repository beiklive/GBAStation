//
// Created by cpasjuste on 11/01/17.
//

#ifndef _PS3_INPUT_H
#define _PS3_INPUT_H

#include "cross2d/skeleton/input.h"

namespace c2d {

    class PS3Input : Input {

    public:
        PS3Input(Renderer *renderer);

        virtual ~PS3Input();

        virtual Player *Update(int rotate);

        virtual int GetButton(int player);

    private:
        void process_axis(Input::Player &player, int rotate);

        void process_buttons(Input::Player &player, int rotate);

    };
}

#endif //_PS3_INPUT_H
