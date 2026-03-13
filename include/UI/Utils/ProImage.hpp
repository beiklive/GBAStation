#pragma once

#include <borealis/views/image.hpp>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

#ifdef BOREALIS_USE_OPENGL
#  include <glad/glad.h>
#endif

namespace beiklive::UI
{

/// Shader animation types supported by ProImage
enum class ShaderAnimationType
{
    NONE,           ///< No animation
    PSP_XMB_RIPPLE, ///< PSP XMB-style wavy ribbon ripple background (GL shader)
};

/**
 * ProImage – an enhanced brls::Image widget.
 *
 * Features:
 *  - Kawase Blur: optional multi-pass box-blur approximation drawn via NanoVG.
 *  - Animated GIF: decodes all frames with stb_image and cycles through them,
 *    with frame timing driven by std::chrono for accurate playback speed.
 *  - Shader Animation: PSP XMB ripple waves rendered via OpenGL GLSL shaders.
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

    /// Scaling mode for animated GIF frames.
    /// FILL    – stretch the frame to fill the entire widget area (default).
    /// CONTAIN – scale the frame to fit within the widget while preserving
    ///           aspect ratio; empty space is left transparent.
    enum class GifScalingMode { FILL, CONTAIN };

    /**
     * Load an animated GIF from the given file path.
     * If the file is a static image, falls back to the standard setImageFromFile().
     */
    void setImageFromGif(const std::string& path);

    /// Set the scaling mode used when drawing GIF frames. Default is FILL.
    void setGifScalingMode(GifScalingMode mode);
    GifScalingMode getGifScalingMode() const;

    // ── Shader Animation ─────────────────────────────────────────────────────

    void setShaderAnimation(ShaderAnimationType type);
    ShaderAnimationType getShaderAnimation() const;

    /// Set the background base colour for the PSP XMB ripple shader.
    /// r, g, b are in the [0, 1] range. Default is a dark navy blue.
    void setXmbBgColor(float r, float g, float b);

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
    GifScalingMode m_gifScalingMode = GifScalingMode::FILL;
    /// Timestamp of last GIF frame-advance check (for delta-time calculation).
    std::chrono::steady_clock::time_point m_gifLastTime;
    bool m_gifTimerStarted = false;

    // Shader animation
    ShaderAnimationType m_shaderAnimation = ShaderAnimationType::NONE;
    float m_animTime = 0.0f; ///< elapsed time in seconds (advances by real delta)
    /// Timestamp of last shader time update.
    std::chrono::steady_clock::time_point m_shaderLastTime;
    bool m_shaderTimerStarted = false;

    // PSP XMB background colour (RGB, [0,1])
    float m_xmbBgR = 0.05f;
    float m_xmbBgG = 0.10f;
    float m_xmbBgB = 0.25f;

#ifdef BOREALIS_USE_OPENGL
    // ── PSP XMB GL shader resources ──────────────────────────────────────────
    GLuint m_xmbProgram  = 0;  ///< compiled GLSL shader program
    GLuint m_xmbVAO      = 0;  ///< vertex array object for fullscreen quad
    GLuint m_xmbVBO      = 0;  ///< vertex buffer for fullscreen quad
    GLuint m_xmbFbo      = 0;  ///< framebuffer object for off-screen render
    GLuint m_xmbFboTex   = 0;  ///< colour attachment texture
    int    m_xmbFboW     = 0;  ///< current FBO texture width
    int    m_xmbFboH     = 0;  ///< current FBO texture height
    int    m_xmbNvgImage = -1; ///< NanoVG image handle for m_xmbFboTex
    bool   m_xmbInited   = false;

    GLint  m_xmbUTime       = -1;
    GLint  m_xmbUResolution = -1;
    GLint  m_xmbUBgColor    = -1;

    void   initXmbShader();
    void   resizeXmbFbo(int w, int h, NVGcontext* vg);
    void   drawPspXmbShader(NVGcontext* vg, float x, float y, float w, float h);
    void   freeXmbResources(NVGcontext* vg);
#endif

    void freeGifFrames();
    void drawGifFrame(NVGcontext* vg, float x, float y, float w, float h, int nvgTex);
    void drawBlur(NVGcontext* vg, float x, float y, float w, float h, NVGpaint basePaint);
};

} // namespace beiklive::UI
