#include "common.h"

namespace beiklive
{

void RegisterStyles()
{
    ADD_STYLE("brls/highlight/stroke_width", 3.0f);

    // ADD_STYLE("brls/shadow/width", 8.0f);
    // ADD_STYLE("brls/shadow/feather", 20.0f);
    // ADD_STYLE("brls/shadow/opacity", 93.0f);
    // ADD_STYLE("brls/shadow/offset", 10.0f);

    ADD_STYLE("beiklive/header/header_height", 70.0f);
    ADD_STYLE("beiklive/header/font_size", 24.0f);
    ADD_STYLE("beiklive/file_list_page/margin_bottom", 24.0f);
    ADD_STYLE("beiklive/file_list_page/margin_top", 24.0f);
}

void RegisterThemes()
{
    // ADD_THEME_COLOR("brls/text", nvgRGB(244, 249, 253));
    ADD_THEME_COLOR("beiklive/CardText/color", nvgRGB(156, 220, 218));
    ADD_THEME_COLOR("brls/highlight/color2", nvgRGB(248, 81, 73));
    ADD_THEME_COLOR("brls/highlight/background", nvgRGBA(0, 0, 0, 0));

    ADD_THEME_COLOR("brls/sidebar/background", nvgRGBA(240, 240, 240, 10));



    ADD_THEME_COLOR("beiklive/sidePanel", nvgRGBA(20, 20, 20, 128));
    ADD_THEME_COLOR("beiklive/subtitle", nvgRGBA(31, 31, 31, 128));
    ADD_THEME_COLOR("beiklive/line", nvgRGBA(31, 31, 31, 255));


    ADD_THEME_COLOR("brls/button/primary_enabled_background", nvgRGBA(50, 79, 241, 0));
    ADD_THEME_COLOR("brls/button/primary_disabled_background", nvgRGBA(201, 201, 209, 0));
    ADD_THEME_COLOR("brls/button/default_enabled_background", nvgRGBA(255, 255, 255, 0));
    ADD_THEME_COLOR("brls/button/default_disabled_background", nvgRGBA(255, 255, 255, 0));











}



} // namespace beiklive
