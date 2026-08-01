//
// Created by cpasjuste on 05/02/18.
//

#include "cross2d/c2d.h"

using namespace c2d;

MessageBox::MessageBox(const c2d::FloatRect &rect, c2d::Input *ipt,
                       c2d::Font *font, int fontSize) : RectangleShape(rect) {
    input = ipt;

    float ratio = (float) fontSize / (float) C2D_DEFAULT_CHAR_SIZE;
    title = new Text("TITLE", (unsigned int) ((float) fontSize + (10 * ratio)), font);
    title->setPosition(16 * ratio, 16 * ratio);
    title->setSizeMax(rect.width - (float) fontSize * 2, (float) fontSize + 4);
    title->setOutlineColor(Color::Black);
    title->setOutlineThickness(2);
    add(title);

    message = new Text("MESSAGE", fontSize, font);
    message->setPosition(16 * ratio, (float) fontSize + (64 * ratio));
    message->setSizeMax(rect.width - (float) fontSize * 2, rect.height * 0.6f);
    message->setLineSpacingModifier(6 * ratio);
    message->setOverflow(Text::Overflow::NewLine);
    message->setOutlineColor(Color::Black);
    message->setOutlineThickness(2);
    add(message);

    for (int i = 0; i < 2; i++) {
        buttons[i] = new Button(
                FloatRect(0, 0, rect.width / 4, rect.height / 6),
                font, fontSize, "BUTTON");
        buttons[i]->setOutlineThickness(2);
        buttons[i]->setOrigin(Origin::Top);
        buttons[i]->setFillColor(getOutlineColor());
        buttons[i]->setOutlineColor(Color::White);
        add(buttons[i]);
    }

    timeout = new Text("9", fontSize, font);
    timeout->setOutlineColor(Color::Black);
    timeout->setOutlineThickness(1);
    timeout->setSize(rect.width - (16 * ratio), 0);
    timeout->setPosition(rect.width / 2, rect.height - buttons[0]->getSize().y - (16 * ratio));
    timeout->setOrigin(Origin::Center);
    timeout->setLineSpacingModifier(4 * ratio);
    add(timeout);

    setVisibility(Visibility::Hidden);
}

int MessageBox::show(const std::string &txt, const std::string &msg,
                     const std::string &buttonLeftText, const std::string &buttonRightText,
                     int *pressed, int t) { // if "pressed" is supplied, pressed is set to the pressed key..

    int ret = 0, index = 0, choices = 0;
    unsigned int key = 0;
    int button = 0;
    C2DClock clock;

    title->setString(txt);
    message->setString(msg);
    timeout->setVisibility(t > 0 ? Visibility::Visible : Visibility::Hidden);

    buttons[0]->getText()->setFillColor(selectedOutlineColor);
    buttons[0]->setFillColor(selectedFillColor);
    buttons[0]->setOutlineColor(selectedOutlineColor);
    buttons[0]->setVisibility(Visibility::Hidden);
    buttons[1]->getText()->setFillColor(notSelectedOutlineColor);
    buttons[1]->setFillColor(notSelectedFillColor);
    buttons[1]->setOutlineColor(notSelectedOutlineColor);
    buttons[1]->setVisibility(Visibility::Hidden);

    // buttons
    if (!buttonLeftText.empty() || !buttonRightText.empty()) {

        if (buttonRightText.empty()) {
            buttons[0]->setText(buttonLeftText);
            buttons[0]->setPosition(getSize().x / 2, getSize().y - buttons[0]->getSize().y - 16);
            buttons[0]->setVisibility(Visibility::Visible);
            buttons[1]->setVisibility(Visibility::Hidden);
            choices = 1;
        } else {
            buttons[0]->setText(buttonLeftText);
            buttons[0]->setPosition((getSize().x / 3) - 8,
                                    getSize().y - buttons[0]->getSize().y - 16);
            buttons[0]->setVisibility(Visibility::Visible);

            buttons[1]->setText(buttonRightText);
            buttons[1]->setPosition(((getSize().x / 3) * 2) + 8,
                                    getSize().y - buttons[1]->getSize().y - 16);
            buttons[1]->setVisibility(Visibility::Visible);
            choices = 2;
        }
    } else {
        buttons[0]->setVisibility(Visibility::Hidden);
        buttons[1]->setVisibility(Visibility::Hidden);
    }

    setVisibility(Visibility::Visible);
    setLayer(10000);

    if (choices > 0 || t > 0) {

        input->clear(0);
        clock.restart();

        while (true) {

            if (t > 0) {
                int elapsed = (int) clock.getElapsedTime().asSeconds();
                if (elapsed >= t) {
                    setVisibility(Visibility::Hidden);
                    ret = TIMEOUT;
                    break;
                }
                snprintf(timeout_str, 16, "%i", t - elapsed);
                timeout->setString(timeout_str);
                timeout->setOrigin(Origin::Center);
            }

            if (pressed != nullptr) {
                button = input->waitButton();
                if (button > -1) {
                    setVisibility(Visibility::Hidden);
                    break;
                }
            } else {
                key = input->update()->buttons;
                if (key > 0 && key != Input::Button::Delay) {
                    if (key & Input::Button::Left) {
                        if (index > 0) {
                            index--;
                        }
                    } else if (key & Input::Button::Right) {
                        if (index < choices - 1) {
                            index++;
                        }
                    } else if (key & Input::Button::A) {
                        setVisibility(Visibility::Hidden);
                        ret = index == 1 ? RIGHT : LEFT;
                        break;
                    } else if (key & Input::Button::B) {
                        setVisibility(Visibility::Hidden);
                        ret = CANCEL;
                        break;
                    }

                    if (index == 0) {
                        buttons[0]->getText()->setFillColor(selectedOutlineColor);
                        buttons[0]->setFillColor(selectedFillColor);
                        buttons[0]->setOutlineColor(selectedOutlineColor);
                        buttons[1]->getText()->setFillColor(notSelectedOutlineColor);
                        buttons[1]->setFillColor(notSelectedFillColor);
                        buttons[1]->setOutlineColor(notSelectedOutlineColor);
                    } else {
                        buttons[0]->getText()->setFillColor(notSelectedOutlineColor);
                        buttons[0]->setFillColor(notSelectedFillColor);
                        buttons[0]->setOutlineColor(notSelectedOutlineColor);
                        buttons[1]->getText()->setFillColor(selectedOutlineColor);
                        buttons[1]->setFillColor(selectedFillColor);
                        buttons[1]->setOutlineColor(selectedOutlineColor);
                    }
                }
            }

            c2d_renderer->flip(true, false);
        }

        if (pressed != nullptr) {
            *pressed = button;
        }

        input->clear();

    } else {
        c2d_renderer->flip();
    }

    return ret;
}

void MessageBox::hide() {
    setVisibility(Visibility::Hidden);
}

void MessageBox::setSelectedColor(const c2d::Color &fillColor, const c2d::Color &outlineColor) {
    selectedFillColor = fillColor;
    selectedOutlineColor = outlineColor;
}

void MessageBox::setNotSelectedColor(const c2d::Color &fillColor, const c2d::Color &outlineColor) {
    notSelectedFillColor = fillColor;
    notSelectedOutlineColor = outlineColor;
}

Text *MessageBox::getTitleText() {
    return title;
}

Text *MessageBox::getMessageText() {
    return message;
}

Button *MessageBox::getButton(int index) {
    return buttons[index];
}

void MessageBox::setSize(const Vector2f &size) {
    MessageBox::setSize(size.x, size.y);
}

void MessageBox::setSize(float width, float height) {
    RectangleShape::setSize(width, height);

    float fontSize = (float) title->getCharacterSize();
    title->setSizeMax(width - fontSize * 2, fontSize + 4);

    fontSize = (float) message->getCharacterSize();
    message->setSizeMax(width - fontSize * 2, height * 0.6f);
}
