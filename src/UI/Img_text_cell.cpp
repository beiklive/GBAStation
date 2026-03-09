#include "UI/Img_text_cell.hpp"


using namespace brls::literals;  // for _i18n

Img_text_cell::Img_text_cell()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/mgba_xml/cell/Img_text_cell.xml");


    // 映射属性
    this->forwardXMLAttribute("image", this->image);
    this->forwardXMLAttribute("imageWidth", this->image, "width");
    this->forwardXMLAttribute("imageHeight", this->image, "height");
    this->forwardXMLAttribute("caption", this->label, "text");


    // this->getView("img_text_cell_root")->setHighlightAlphaTransparent(true, 0.0f); // 取消选中高亮背景
}


brls::View* Img_text_cell::create() 
{
    return new Img_text_cell();
}

void Img_text_cell::setTitle(std::string title)
{
    this->label->setText(title);
}

void Img_text_cell::setImage(std::string res)
{
    this->image->setImageFromRes(res);
}


void Img_text_cell::onFocusGained()
{
    RecyclerCell::onFocusGained();

    this->accent->setVisibility(brls::Visibility::VISIBLE);
}

void Img_text_cell::onFocusLost()
{
    RecyclerCell::onFocusLost();

    this->accent->setVisibility(brls::Visibility::INVISIBLE);
}