//
// Created by cpasjuste on 05/02/18.
//

#include "cross2d/c2d.h"

using namespace c2d;

Button::Button(const c2d::FloatRect &rect, c2d::Font *font,
               int fontSize, const std::string &text)
        : RectangleShape(rect) {

    this->text = new Text(text, (unsigned int) fontSize, font);
    this->text->setPosition(getSize().x / 2, getSize().y / 2);
    this->text->setOutlineColor(Color::Black);
    this->text->setOutlineThickness(1);
    add(this->text);

    setText(text);
}

void Button::setText(const std::string &t) {

    text->setScale(1, 1);
    text->setString(t);
    text->setOrigin(Origin::Center);
    float width = text->getGlobalBounds().width;
    if (width > getSize().x - 8) {
        float scaling = (getSize().x - 16) / width;
        text->setScale(scaling, scaling);
    }
}

c2d::Text *Button::getText() {
    return text;
}
