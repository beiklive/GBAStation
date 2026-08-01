//
// Created by cpasjuste on 29/12/17.
//

#include "cross2d/c2d.h"

using namespace c2d;

TextBox::TextBox(const std::string &string, Font *font,
                 const FloatRect &rect,
                 const Color &fillColor,
                 const Color &textColor) : RectangleShape(rect) {

    setFillColor(fillColor);

    text = new Text(string, C2D_DEFAULT_CHAR_SIZE, font);
    text->setFillColor(textColor);

    if (rect.width <= 0 || rect.height <= 0) {
        setSize(text->getLocalBounds().width + 16, text->getLocalBounds().height + 16);
    }

    text->setOrigin(Origin::Center);
    text->setPosition(getLocalBounds().width / 2, getLocalBounds().height / 2);

    add(text);
}

void TextBox::onDraw(Transform &transform, bool draw) {

    // update text position
    text->setPosition(getLocalBounds().width / 2, getLocalBounds().height / 2);
    Shape::onDraw(transform, draw);
}

Text *TextBox::getText() {
    return text;
}

TextBox::~TextBox() {

    if (text) {
        delete (text);
    }
}
