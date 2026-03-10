#pragma once

#include <borealis.hpp>
#include <string>

/// Full-screen image viewer with zoom/pan support.
/// - Black background
/// - BUTTON_B  : close (pop activity)
/// - BUTTON_A  : zoom in
/// - BUTTON_Y  : zoom out
/// - BUTTON_X  : reset zoom / pan
/// - D-pad     : pan image
/// - All other buttons are swallowed (disabled)
class ImageView : public brls::Box
{
  public:
    explicit ImageView(const std::string& imagePath);

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

  private:
    std::string m_imagePath;

    // NVG image handle (-1 = not loaded yet)
    int  m_nvgImage  = -1;
    int  m_imgW      = 0;
    int  m_imgH      = 0;
    bool m_loaded    = false;

    float m_scale   = 1.0f;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;

    static constexpr float ZOOM_STEP = 0.25f;
    static constexpr float ZOOM_MIN  = 0.25f;
    static constexpr float ZOOM_MAX  = 8.0f;
    static constexpr float PAN_STEP  = 40.0f;

    void zoomIn();
    void zoomOut();
    void resetView();
    void panLeft();
    void panRight();
    void panUp();
    void panDown();
};
