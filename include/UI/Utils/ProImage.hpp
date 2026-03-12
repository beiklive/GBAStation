#pragma once

#include <borealis/views/image.hpp>
#include <functional>
#include <string>
#include <vector>

namespace beiklive::UI
{

/// Shader animation types supported by ProImage
enum class ShaderAnimationType
{
    NONE,       ///< No animation
    PSP_LINES,  ///< PSP XMB-style flowing diagonal line animation
};

/**
 * ProImage – an enhanced brls::Image widget.
 *
 * Features:
 *  - Kawase Blur: optional multi-pass box-blur approximation drawn via NanoVG.
 *  - Animated GIF: decodes all frames with stb_image and cycles through them.
 *  - Shader Animation: built-in NanoVG-drawn animations (e.g. PSP_LINES).
 */
class ProImage : public brls::Image
{
  public:
    ProImage();
    ~ProImage() override;

    // ── Kawase Blur ──────────────────────────────────────────────────────────

    /// Enable/disable Kawase-style blur overlay on the image.
    void setBlurEnabled(bool enabled);
    bool isBlurEnabled() const;

    /// Blur radius in pixels. Higher values produce stronger blur. Default 8.
    void setBlurRadius(float radius);
    float getBlurRadius() const;

    // ── Animated GIF ─────────────────────────────────────────────────────────

    /**
     * Load an animated GIF from the given file path.
     * If the file is a static image, falls back to the standard setImageFromFile().
     */
    void setImageFromGif(const std::string& path);

    // ── Shader Animation ─────────────────────────────────────────────────────

    void setShaderAnimation(ShaderAnimationType type);
    ShaderAnimationType getShaderAnimation() const;

    // ── Override ─────────────────────────────────────────────────────────────

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

  private:
    // Blur
    bool  m_blurEnabled = false;
    float m_blurRadius  = 8.0f;

    // GIF animation
    struct GifFrame
    {
        int texture   = 0;
        int delay_ms  = 100; ///< frame duration in milliseconds
    };
    std::vector<GifFrame> m_gifFrames;
    int   m_gifCurrentFrame = 0;
    float m_gifElapsedMs    = 0.0f;
    bool  m_isGif           = false;

    // Shader animation
    ShaderAnimationType m_shaderAnimation = ShaderAnimationType::NONE;
    float m_animTime = 0.0f;  ///< elapsed time in seconds (advances each draw)

    void freeGifFrames();
    void drawBlur(NVGcontext* vg, float x, float y, float w, float h, NVGpaint basePaint);
    void drawPspLines(NVGcontext* vg, float x, float y, float w, float h);
};

} // namespace beiklive::UI
