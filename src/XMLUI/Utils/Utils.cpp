#include "XMLUI/Utils/Utils.hpp"
#include "common.hpp"

void beiklive::swallow(brls::View* v, brls::ControllerButton btn)
{
    v->registerAction("", btn, [](brls::View*) { return true; },
                      /*hidden=*/true);
}


beiklive::UI::BrowserHeader::BrowserHeader()
{
    
    this->setJustifyContent(brls::JustifyContent::CENTER);

    this->setHeight(GET_STYLE("beiklive/header/header_height"));
    this->setPaddingTop(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
    this->setPaddingBottom(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
    this->setMarginRight(GET_STYLE("brls/applet_frame/padding_sides"));
    this->setMarginLeft(GET_STYLE("brls/applet_frame/padding_sides"));

    this->setPaddingRight(GET_STYLE("brls/applet_frame/header_padding_sides"));
    this->setPaddingLeft(GET_STYLE("brls/applet_frame/header_padding_sides"));

    this->setLineColor(GET_THEME_COLOR("brls/applet_frame/separator"));
    this->setLineBottom(1.f);
    

    m_titleBox = new brls::Box();
    m_titleBox->setMarginRight(GET_STYLE("brls/applet_frame/header_image_title_spacing"));

    m_titleLabel = new brls::Label();
    float FontSize = GET_STYLE("brls/applet_frame/header_title_font_size");
    m_titleLabel->setFontSize(FontSize + 2);
    
    m_titleBox->addView(m_titleLabel);
    this->addView(m_titleBox);

    m_subtitleBox = new brls::Box(brls::Axis::COLUMN);
    m_subtitleBox->setJustifyContent(brls::JustifyContent::CENTER);

    m_pathLabel = new brls::Label();
    m_pathLabel->setFontSize(FontSize/2 + 2);
    m_pathLabel->setTextColor(GET_THEME_COLOR("beiklive/subtitle"));

    m_fileNameLabel = new brls::Label();
    m_fileNameLabel->setFontSize(FontSize/2);
    m_fileNameLabel->setTextColor(GET_THEME_COLOR("beiklive/subtitle"));
    m_subtitleBox->addView(m_fileNameLabel);
    m_subtitleBox->addView(m_pathLabel);
    this->addView(m_subtitleBox);

    this->addView(new brls::Padding());

}

void beiklive::UI::BrowserHeader::setTitle(const std::string& title)
{
    if (m_titleLabel)
        m_titleLabel->setText(title);
}
void beiklive::UI::BrowserHeader::setPath(const std::string& path)
{
    if (m_pathLabel)
        m_pathLabel->setText(path);
}
void beiklive::UI::BrowserHeader::setFileName(const std::string& fileName)
{
    if (m_fileNameLabel)
        m_fileNameLabel->setText(fileName);
}