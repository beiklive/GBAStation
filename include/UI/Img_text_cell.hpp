#pragma once

#include <borealis.hpp>



class Img_text_cell : public brls::RecyclerCell
{
  public:
    Img_text_cell();

    void onFocusGained() override;
    void onFocusLost() override;

    BRLS_BIND(brls::Rectangle, accent, "brls/sidebar/item_accent");
    BRLS_BIND(brls::Label, label, "title");
    BRLS_BIND(brls::Image, image, "image");


    void setTitle(std::string title);
    void setImage(std::string res);

    static brls::View* create();
};
