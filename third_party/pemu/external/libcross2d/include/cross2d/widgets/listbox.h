//
// Created by cpasjuste on 05/01/18.
//

#ifndef CROSS2D_LISTBOX_H
#define CROSS2D_LISTBOX_H

#include "cross2d/skeleton/sfml/RectangleShape.hpp"
#include "cross2d/skeleton/sfml/Text.hpp"
#include "cross2d/skeleton/io.h"

namespace c2d {

    class ListBoxLine : public RectangleShape {

    public:

        ListBoxLine(const FloatRect &rect, const std::string &str, Font *font,
                    unsigned int fontSize, Texture *icon = nullptr, bool use_icons = false);

        ~ListBoxLine() override;

        void setSize(const Vector2f &size) override;

        void setSize(float width, float height) override;

        void setString(const std::string &string);

        void setColor(const Color &color);

        void setIcon(Texture *icon);

        Text *getText();

    private:

        void onDraw(Transform &transform, bool draw = true) override;

        RectangleShape *iconRect = nullptr;
        Texture *icon = nullptr;
        Text *text = nullptr;
        bool use_icons = false;
    };

    class ListBox : public RectangleShape {

    public:

        ListBox(Font *font, int fontSize, const FloatRect &rect,
                const std::vector<Io::File> &fileList, bool useIcons = false, float lineHeight = 0);

        ListBox(Font *font, int fontSize, const FloatRect &rect,
                const std::vector<Io::File *> &fileList, bool useIcons = false, float lineHeight = 0);

        ~ListBox() override;

        void setFiles(const std::vector<Io::File> &fileList);

        void setFiles(const std::vector<Io::File *> fileList);

        void setSelection(int index);

        void up();

        void down();

        void setSize(const Vector2f &size) override;

        void setSize(float width, float height) override;

        void setTextColor(const Color &fileColor, const Color &dirColor);

        void setTextOutlineColor(const Color &fileColor, const Color &dirColor);

        void setTextOutlineThickness(float thickness);

        void setHighlightEnabled(bool enable);

        RectangleShape *getHighlight();

        int getMaxLines();

        std::vector<ListBoxLine *> getLines();

        std::vector<c2d::Io::File *> getFiles();

        Io::File *getSelection();

        int getIndex();

    private:

        void init(Font *font, int fontSize, bool useIcons, float lineHeight);

        void updateLines();

        std::vector<Io::File *> files;
        std::vector<ListBoxLine *> lines;
        RectangleShape *highlight;
        Color textFileColor;
        Color textFileColorOutline;
        Color textDirColor;
        Color textDirColorOutline;
        float line_height;
        int max_lines;
        int file_index = 0;
        int start_index = 0;
        int highlight_index = 0;
        bool use_icons = false;
        bool use_highlight = true;
        bool files_are_mine = false;
    };
}

#endif //CROSS2D_LISTBOX_H
