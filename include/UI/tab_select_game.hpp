#pragma once

#include <borealis.hpp>


#include "UI/Img_text_cell.hpp"


class SelectGameTab : public brls::Box
{
  public:
    SelectGameTab();

    void applyBackTheme(brls::ThemeVariant theme);




    BRLS_BIND(Img_text_cell, select_file, "select-file");
    BRLS_BIND(Img_text_cell, select_recent, "select-recent");
    BRLS_BIND(Img_text_cell, select_favorites, "select-favorites");



    static brls::View* create();
};
