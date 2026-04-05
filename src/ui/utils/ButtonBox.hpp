#pragma once

#include <borealis.hpp>
#include <functional>

namespace beiklive
{
    
class ButtonBox : public brls::Box
{
public:
    void onFocusGained() override
    {
        Box::onFocusGained();
        if (onFocusGainedCallback) onFocusGainedCallback();
    }

    void onFocusLost() override
    {
        Box::onFocusLost();
        if (onFocusLostCallback) onFocusLostCallback();
    }

    std::function<void()> onFocusGainedCallback;
    std::function<void()> onFocusLostCallback;

};
} // namespace beiklive
