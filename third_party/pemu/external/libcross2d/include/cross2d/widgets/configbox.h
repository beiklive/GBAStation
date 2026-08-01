//
// Created by cpasjuste on 19/10/18.
//

#ifndef C2D_CONFIGBOX_H
#define C2D_CONFIGBOX_H

#include "cross2d/skeleton/sfml/RectangleShape.hpp"
#include "cross2d/skeleton/config.h"
#include "cross2d/widgets/listbox.h"

namespace c2d {

    class ConfigBox : public RectangleShape {

    public:

        enum class Navigation {
            Up,
            Down,
            Left,
            Right,
            Enter,
            Exit
        };

        explicit ConfigBox(Font *font, int fontSize,
                           const c2d::Vector2f &size = c2d::Vector2f(0, 0), float lineHeight = 0);

        explicit ConfigBox(Font *font, int fontSize,
                           const c2d::FloatRect &rect = c2d::FloatRect(0, 0, 0, 0),
                           float lineHeight = 0);

        virtual void load(config::Group *group);

        virtual void setLeftWidth(float width);

        virtual ListBox *getListBoxLeft();

        virtual ListBox *getListBoxRight();

        virtual config::Group *getGroup();

        virtual config::Option *getSelection();

        virtual void reset();

        virtual void setLoopEnable(bool enableLoop);

        ///
        /// \param navigation
        /// \return the selected option (only on "Navigation::Enter")
        virtual config::Option *navigate(const Navigation &navigation);

    private:

        ListBox *listBoxLeft;
        ListBox *listBoxRight;
        config::Group *group;
        std::vector<config::Group *> history;
        int index = 0;
        bool loop_choices = false;
    };
}

#endif //C2D_CONFIGBOX_H
