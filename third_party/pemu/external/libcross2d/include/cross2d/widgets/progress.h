//
// Created by cpasjuste on 03/10/18.
//

#ifndef C2D_PROGRESS_H
#define C2D_PROGRESS_H

namespace c2d {

    class Progress : public RectangleShape {

    public:

        Progress(const FloatRect &rect);

        void setProgress(float progress);

        float getProgressWidth();

        void setBgColor(const Color &color);

        void setFgColor(const Color &color);

    private:

        RectangleShape *progress_fg = nullptr;

    };
}

#endif //C2D_PROGRESS_H
