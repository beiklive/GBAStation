//
// Created by cpasjuste on 05/02/18.
//

#ifndef C2D_MESSAGEBOX_H
#define C2D_MESSAGEBOX_H

namespace c2d {
    class MessageBox : public RectangleShape {
    public:

        enum MessageBoxResult : int {
            LEFT = 0, RIGHT = 1, CANCEL = 2, TIMEOUT = 3
        };

        MessageBox(const c2d::FloatRect &rect, c2d::Input *input,
                   c2d::Font *font, int fontSize);

        // if "pressed" is supplied, pressed is set to the pressed key..
        int show(const std::string &title, const std::string &message,
                 const std::string &buttonLeftText = "", const std::string &buttonRightText = "",
                 int *pressed = nullptr, int timeout = 0);

        void hide();

        void setSelectedColor(const c2d::Color &fillColor, const c2d::Color &outlineColor);

        void setNotSelectedColor(const c2d::Color &fillColor, const c2d::Color &outlineColor);

        c2d::Text *getTitleText();

        c2d::Text *getMessageText();

        c2d::Button *getButton(int index);

        void setSize(const c2d::Vector2f &size) override;

        void setSize(float width, float height) override;

    private:
        c2d::Input *input = nullptr;
        c2d::Text *title = nullptr;
        c2d::Text *message = nullptr;
        c2d::Text *timeout = nullptr;
        char timeout_str[16];
        c2d::Button *buttons[2];
        c2d::Color selectedFillColor = Color::White;
        c2d::Color selectedOutlineColor = Color::Red;
        c2d::Color notSelectedFillColor = Color::White;
        c2d::Color notSelectedOutlineColor = Color::White;
    };
}

#endif //C2D_MESSAGEBOX_H
