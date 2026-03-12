#pragma once

#include <borealis.hpp>




namespace beiklive::UI
{

// A common header for file browsers, with a title, a path and an info label
class BrowserHeader : public brls::Box
{
  public:
    BrowserHeader();
    void setTitle(const std::string& title);
    void setPath(const std::string& path);
    void setInfo(const std::string& info);

  private:
    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_pathLabel = nullptr;
    brls::Label* m_infoLabel = nullptr;

    brls::Box* m_titleBox = nullptr;
    brls::Box* m_subtitleBox = nullptr;
};



class RoundButton : public brls::Box
{
  public:
    // imagePath: path to the button image; text: label below the circle; onClick: callback fired after bounce
    RoundButton(const std::string& imagePath, const std::string& text, std::function<void()> onClick);

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void onChildFocusGained(brls::View* directChild, brls::View* focusedView) override;
    void onChildFocusLost(brls::View* directChild, brls::View* focusedView) override;
    // brls::View* getDefaultFocus() override;

    void setImage(const std::string& imagePath);
    void setText(const std::string& text);

  private:
    brls::Box*            m_imageWrapper = nullptr; // circular-radius container for the image
    brls::Image*          m_image        = nullptr;
    brls::Label*          m_label        = nullptr;
    std::function<void()> m_onClick;

    // Focus + hover scale animation
    bool  m_focused = false;
    float m_scale   = 1.0f;

    // Click bounce animation
    bool  m_clickAnimating = false;
    float m_clickT         = 0.0f;
    float m_clickScale     = 1.0f;
};

class ButtonBar : public brls::Box
{
  public:
    ButtonBar();
    void addButton(const std::string& imagePath, const std::string& text, std::function<void()> onClick);
    void addButton(RoundButton* button);

};



    
} // namespace beiklive
