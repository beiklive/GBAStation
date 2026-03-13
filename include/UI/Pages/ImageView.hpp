#pragma once

#include <borealis.hpp>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

/// Full-screen image viewer with zoom/pan support.
/// - Black background
/// - BUTTON_B  : close (pop activity)
/// - BUTTON_A  : zoom in
/// - BUTTON_Y  : zoom out
/// - BUTTON_X  : reset zoom / pan
/// - D-pad     : pan image
/// - All other buttons are swallowed (disabled)
/// Supports static images as well as animated GIFs.
class ImageView : public brls::Box
{
  public:
    explicit ImageView(const std::string& imagePath);
    ~ImageView() override;

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

  private:
    std::string m_imagePath;

    // NVG image handle for static images (-1 = not loaded yet)
    int  m_nvgImage  = -1;
    int  m_imgW      = 0;
    int  m_imgH      = 0;
    bool m_loaded    = false;

    // Async loading
    bool                  m_asyncLoading = false;
    std::atomic<bool>     m_asyncReady{false};
    std::vector<uint8_t>  m_asyncBytes;
    std::mutex            m_asyncMutex;

    // Animated GIF support
    struct GifFrame
    {
        static constexpr int DEFAULT_DELAY_MS = 100; ///< Fallback frame delay in ms
        int nvgTex   = -1; ///< NVG image handle for this frame
        int delay_ms = DEFAULT_DELAY_MS;
    };
    std::vector<GifFrame> m_gifFrames;
    int   m_gifCurrentFrame  = 0;
    float m_gifElapsedMs     = 0.0f;
    bool  m_isGif            = false;
    bool  m_gifTimerStarted  = false;
    std::chrono::steady_clock::time_point m_gifLastTime;

    void freeGifFrames();

    float m_scale   = 1.0f;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;

    static constexpr float ZOOM_STEP = 0.10f;
    static constexpr float ZOOM_MIN  = 0.10f;
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
