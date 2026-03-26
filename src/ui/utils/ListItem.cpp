#include "ListItem.hpp"

namespace beiklive
{
static constexpr float CELL_HEIGHT = 66.f;
static constexpr float ICON_HEIGHT = 50.f;
static constexpr float ACCENT_W    = 10.f;
static constexpr float CELL_PAD_H  = 12.f;
static constexpr float CELL_PAD_V  = 10.f;

ListItemCell::ListItemCell()
{
    this->setFocusable(true);
    // this->setWireframeEnabled(true);
    // this->setHeight(CELL_HEIGHT);
    this->setAxis(brls::Axis::ROW);
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setHideHighlightBackground(true);
    // this->setPaddingBottom(4.f);
    // HIDE_BRLS_BACKGROUND(this);

    // 左侧强调条
    m_accent = new brls::Rectangle();
    m_accent->setWidth(5.f);
    m_accent->setHeight(40.f);
    m_accent->setColor(nvgRGBA(79, 193, 255, 255));
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
    this->addView(m_accent);

    // 图标
    m_icon = new brls::Image();
    m_icon->setWidth(ICON_HEIGHT);
    m_icon->setHeight(ICON_HEIGHT);
    m_icon->setMarginLeft(CELL_PAD_H);
    m_icon->setScalingType(brls::ImageScalingType::FIT);
    m_icon->setInterpolation(brls::ImageInterpolation::LINEAR);
    this->addView(m_icon);

    // 中间文本区域
    auto textBox = new brls::Box(brls::Axis::ROW);
    textBox->setAlignItems(brls::AlignItems::CENTER);
    textBox->setHeight(ICON_HEIGHT);

    textBox->setMarginLeft(CELL_PAD_H);
    textBox->setMarginRight(CELL_PAD_H);

    textBox->setGrow(1);

    m_text = new brls::Label();
    m_text->setGrow(1);
    m_text->setFontSize(22);
    m_text->setTextColor(GET_THEME_COLOR("brls/text"));
    m_text->setSingleLine(true);
    m_text->setAutoAnimate(true);

    m_subtext = new brls::Label();
    m_subtext->setFontSize(16);
    m_subtext->setSingleLine(true);
    m_subtext->setTextColor(GET_THEME_COLOR("brls/text"));
    m_subtext->setHorizontalAlign(brls::HorizontalAlign::RIGHT);

    textBox->addView(m_text);
    textBox->addView(m_subtext);

    this->addView(textBox);
}

void ListItemCell::setTitle(const std::string& text)
{
    m_text->setText(text);
}

void ListItemCell::setSubTitle(const std::string& text)
{
    m_subtext->setText(text);
}

void ListItemCell::setIcon(const std::string& path)
{
    m_icon->setImageFromFile(path);
}

void ListItemCell::onFocusGained()
{
    brls::RecyclerCell::onFocusGained();
    m_accent->setVisibility(brls::Visibility::VISIBLE);
}

void ListItemCell::onFocusLost()
{
    brls::RecyclerCell::onFocusLost();
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
}

ListItemCell* ListItemCell::create()
{
    return new ListItemCell();
}

} // namespace beiklive
