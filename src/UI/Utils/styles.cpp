#include "common.hpp"

namespace beiklive
{

void RegisterStyles()
{
    ADD_STYLE("brls/highlight/stroke_width", 3.0f);


    ADD_STYLE("beiklive/header/header_height", 70.0f);
    ADD_STYLE("beiklive/header/font_size", 24.0f);
    ADD_STYLE("beiklive/file_list_page/margin_bottom", 24.0f);
    ADD_STYLE("beiklive/file_list_page/margin_top", 24.0f);
}

void RegisterThemes()
{
    ADD_THEME_COLOR("beiklive/subtitle", nvgRGBA(31, 31, 31, 128));
    ADD_THEME_COLOR("beiklive/line", nvgRGBA(31, 31, 31, 256));
}



} // namespace beiklive
