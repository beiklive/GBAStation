#pragma once

#include <borealis.hpp>




namespace beiklive::UI
{

class BrowserHeader : public brls::Box
{
  public:
    BrowserHeader();
    void setTitle(const std::string& title);
    void setPath(const std::string& path);
    void setInfo(const std::string& info);

  private:
    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_pathLabel = nullptr;
    brls::Label* m_infoLabel = nullptr;

    brls::Box* m_titleBox = nullptr;
    brls::Box* m_subtitleBox = nullptr;


    

};




    
} // namespace beiklive
