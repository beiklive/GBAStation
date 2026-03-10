#include "XMLUI/Utils/Utils.hpp"
#include "common.hpp"

beiklive::UI::BrowserHeader::BrowserHeader()
{
    this->setJustifyContent(brls::JustifyContent::FLEX_START);
    this->setHeight(beiklive::getStyle().getMetric("beiklive/header/header_height"));
    this->setPaddingLeft(beiklive::getStyle().getMetric("beiklive/body/padding_left"));
    this->setPaddingRight(beiklive::getStyle().getMetric("beiklive/body/padding_right"));

    m_titleBox = new brls::Box();
    m_titleBox->setMarginRight(8.f);

    m_titleLabel = new brls::Label();
    float FontSize = beiklive::getStyle().getMetric("beiklive/header/font_size");
    m_titleLabel->setFontSize(FontSize + 2);
    
    m_titleBox->addView(m_titleLabel);
    this->addView(m_titleBox);

    m_subtitleBox = new brls::Box(brls::Axis::COLUMN);
    m_titleBox->setMarginRight(2.f);
    m_subtitleBox->setJustifyContent(brls::JustifyContent::CENTER);

    m_pathLabel = new brls::Label();
    m_pathLabel->setFontSize(FontSize/2);
    m_pathLabel->setTextColor(beiklive::getTheme().getColor("beiklive/subtitle"));

    m_fileNameLabel = new brls::Label();
    m_fileNameLabel->setFontSize(FontSize/2);
    m_fileNameLabel->setTextColor(beiklive::getTheme().getColor("beiklive/subtitle"));
    m_subtitleBox->addView(m_fileNameLabel);
    m_subtitleBox->addView(m_pathLabel);
    this->addView(m_subtitleBox);


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