#include "Header.hpp"


namespace beiklive
{
    
    HeaderBar::HeaderBar()
    {
        // this->setWireframeEnabled(true);
        this->setAxis(brls::Axis::ROW);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setHeight(GET_STYLE("brls/applet_frame/header_height"));
        this->setPaddingTop(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
        this->setPaddingBottom(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
        this->setMarginRight(GET_STYLE("brls/applet_frame/padding_sides"));
        this->setMarginLeft(GET_STYLE("brls/applet_frame/padding_sides"));
    
        this->setPaddingRight(GET_STYLE("brls/applet_frame/header_padding_sides"));
        this->setPaddingLeft(GET_STYLE("brls/applet_frame/header_padding_sides"));
    
        this->setLineColor(GET_THEME_COLOR("brls/applet_frame/separator"));
        this->setLineBottom(1.f);
        
    
        float FontSize = GET_STYLE("brls/applet_frame/header_title_font_size");
        m_titleBox = new brls::Box();
        m_titleBox->setMarginRight(GET_STYLE("brls/applet_frame/header_image_title_spacing"));
        m_titleBox->setHeight(FontSize);
        m_titleLabel = new brls::Label();
        m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_titleLabel->setFontSize(FontSize);
        
        m_titleBox->addView(m_titleLabel);
        this->addView(m_titleBox);
    
        m_subtitleBox = new brls::Box(brls::Axis::COLUMN);
        m_subtitleBox->setHeight(FontSize);
        m_subtitleBox->setJustifyContent(brls::JustifyContent::CENTER);
    
        m_pathLabel = new brls::Label();
        m_pathLabel->setWidth(500.f);
        m_pathLabel->setSingleLine(true);
        m_pathLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        m_pathLabel->setVerticalAlign(brls::VerticalAlign::CENTER);
        m_pathLabel->setAnimated(true);
        m_pathLabel->setFontSize(FontSize/2 + 2);
        m_pathLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    
        m_subtitleBox->addView(new brls::Padding());
        m_subtitleBox->addView(m_pathLabel);
        this->addView(m_subtitleBox);
        
        this->addView(new brls::Padding());
    
        m_infoLabel = new brls::Label();
        m_infoLabel->setFontSize(FontSize);
        m_infoLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        this->addView(m_infoLabel);
    
    }
    
    void HeaderBar::setTitle(const std::string& title)
    {
        if (m_titleLabel)
            m_titleLabel->setText(title);
    }
    void HeaderBar::setPath(const std::string& path)
    {
        if (m_pathLabel)
            m_pathLabel->setText(path);
    }
    void HeaderBar::setInfo(const std::string& info)
    {
        if (m_infoLabel)
            m_infoLabel->setText(info);
    }


} // namespace beiklive
