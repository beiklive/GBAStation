#include "InputMap.hpp"

namespace beiklive::input
{

std::string parseBrlsButton(const int code)
{
    if(
        (code >= brls::BRLS_KBD_KEY_SPACE && code <= brls::BRLS_KBD_KEY_WORLD_2) ||
        (code >= brls::BRLS_KBD_KEY_ESCAPE && code <= brls::BRLS_KBD_KEY_MENU) ||
        (code >= brls::BUTTON_LT && code <= brls::BUTTON_NAV_LEFT)
    ) {
        for (const auto& bn : k_brlsNames) {
            if (code == bn.id) 
            {
                brls::Logger::debug("parseBrlsButton: code={} -> name={}", code, bn.name);
                return bn.name;
            }
        }
    }
    return "";
}



} // namespace beiklive::input
