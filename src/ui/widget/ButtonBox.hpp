#pragma once

#include <borealis.hpp>
#include <functional>

namespace beiklive
{
    
class ButtonBox : public brls::Box
{
public:
    ButtonBox();
    void setIcon(const std::string& iconPath);
    void setText(const std::string& text);
    void onFocusGained() override;
    void onFocusLost() override;

    std::function<void()> onFocusGainedCallback = nullptr;
    std::function<void()> onFocusLostCallback = nullptr;

private:
    brls::Rectangle* m_accent = nullptr;
    brls::Image* m_icon = nullptr;
    brls::Label* m_label = nullptr; 
};
} // namespace beiklive
