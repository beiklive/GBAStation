//
// Created by cpasjuste on 05/02/18.
//

#ifndef C2D_BUTTON_H
#define C2D_BUTTON_H

#include "cross2d/skeleton/sfml/RectangleShape.hpp"
#include "cross2d/skeleton/sfml/Text.hpp"

namespace c2d {

    class Button : public c2d::RectangleShape {

    public:

        Button(const c2d::FloatRect &rect, c2d::Font *font,
               int fontSize, const std::string &text);

        void setText(const std::string &text);
        
        c2d::Text *getText();

    private:

        c2d::Text *text;
    };
}

#endif //C2D_BUTTON_H
