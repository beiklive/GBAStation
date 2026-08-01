//
// Created by cpasjuste on 29/12/17.
//

#ifndef CROSS2D_TEXTBOX_H
#define CROSS2D_TEXTBOX_H

#include "cross2d/skeleton/sfml/RectangleShape.hpp"
#include "cross2d/skeleton/sfml/Text.hpp"

namespace c2d {

    class TextBox : public RectangleShape {

    public:

        TextBox(const std::string &string, Font *font,
                const FloatRect &rect = FloatRect(),
                const Color &fillColor = Color::GrayLight,
                const Color &textColor = Color::White);

        ~TextBox() override;

        virtual Text *getText();

    private:

        virtual void onDraw(Transform &transform, bool draw = true) override;

        Text *text;

    };
}

#endif //CROSS2D_TEXTBOX_H
