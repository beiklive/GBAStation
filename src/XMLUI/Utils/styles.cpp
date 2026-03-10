#include "common.hpp"

namespace beiklive
{
    
static brls::StyleValues bkstyleValues = {
    {"beiklive/body/padding_left", 25.0f},
    {"beiklive/body/padding_right", 25.0f},
    {"beiklive/header/header_height", 70.0f},
    {"beiklive/header/font_size", 24.0f},
    // Header
    { "brls/header/padding_top_bottom", 11.0f },
    { "brls/header/padding_right", 11.0f },
    { "brls/header/rectangle_width", 5.0f },
    { "brls/header/rectangle_height", 33.0f },
    { "brls/header/rectangle_margin", 10.0f },
    { "brls/header/font_size", 18.0f },

};

static brls::ThemeValues lightThemeValues = {
    // Generic values
    { "beiklive/subtitle", nvgRGBA(31, 31, 31, 128) },
    { "brls/background", nvgRGB(235, 235, 235) },
    { "brls/text", nvgRGB(45, 45, 45) },
    { "brls/text_disabled", nvgRGB(140, 140, 140) },
    { "brls/backdrop", nvgRGBA(0, 0, 0, 178) },
    { "brls/click_pulse", nvgRGBA(13, 182, 213, 38) }, // same as highlight color1 with different opacity
    { "brls/accent", nvgRGB(49, 79, 235) },
};
static brls::Style bkstyle(&bkstyleValues);

// getStyle().getMetric("brls/applet_frame/header_title_font_size")
brls::Style getStyle()
{
    return bkstyle;
}
// getTheme().getColor("brls/clear")
brls::Theme& getTheme(int type)
{
    if (type == 0){
        static brls::Theme lightTheme(&lightThemeValues);
        return lightTheme;
    }
}
brls::Theme getTheme()
{
    return getTheme(0);
    // if (brls::Application::getThemeVariant() == brls::ThemeVariant::LIGHT)
        // return Theme::getLightTheme();
    // else
        // return Theme::getDarkTheme();
}


} // namespace beiklive
