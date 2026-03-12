#include "UI/Utils/ProImage.hpp"

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

// Use the stb_image bundled with borealis/nanovg for GIF decoding.
// The header guard avoids re-defining the implementation since image.cpp
// already does STB_IMAGE_IMPLEMENTATION in a separate translation unit.
#include <borealis/extern/nanovg/stb_image.h>

namespace beiklive::UI
{

// ─────────────────────────────────────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────────────────────────────────────

/// Pi with sufficient precision for all animation calculations.
static constexpr double PI = 3.14159265358979323846;

/// Assumed frame duration in milliseconds (used for GIF timing).
/// brls::FrameContext does not expose per-frame delta time via the draw API,
/// so we approximate 60 fps.  GIF frame timing is based on the encoded delays
/// and is only mildly sensitive to this value.
static constexpr float ASSUMED_FRAME_MS = 1000.0f / 60.0f;

/// Assumed seconds per frame (used for shader animation speed).
static constexpr float ASSUMED_FRAME_SEC = 1.0f / 60.0f;

// ─────────────────────────────────────────────────────────────────────────────
//  ProImage
// ─────────────────────────────────────────────────────────────────────────────

ProImage::ProImage() = default;

ProImage::~ProImage()
{
    freeGifFrames();
}

void ProImage::freeGifFrames()
{
    NVGcontext* vg = brls::Application::getNVGContext();
    for (auto& f : m_gifFrames)
    {
        if (f.texture && vg)
            nvgDeleteImage(vg, f.texture);
    }
    m_gifFrames.clear();
    m_gifCurrentFrame = 0;
    m_gifElapsedMs    = 0.0f;
    m_isGif           = false;
}

// ── Kawase Blur ───────────────────────────────────────────────────────────────

void ProImage::setBlurEnabled(bool enabled)
{
    m_blurEnabled = enabled;
    invalidate();
}

bool ProImage::isBlurEnabled() const { return m_blurEnabled; }

void ProImage::setBlurRadius(float radius)
{
    m_blurRadius = radius;
    invalidate();
}

float ProImage::getBlurRadius() const { return m_blurRadius; }

// ── Animated GIF ──────────────────────────────────────────────────────────────

void ProImage::setImageFromGif(const std::string& path)
{
    // Release previous GIF frames (if any)
    freeGifFrames();

    // Read the file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        brls::Logger::warning("ProImage: failed to open GIF file: {}", path);
        setImageFromFile(path); // fallback
        return;
    }
    auto fileSize = static_cast<std::streamsize>(file.tellg());
    file.seekg(0);
    std::vector<unsigned char> buf(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(buf.data()), fileSize);
    file.close();

    // Decode all GIF frames
    int frameW = 0, frameH = 0, frameCount = 0, comp = 0;
    int* delays = nullptr;
    unsigned char* pixels = stbi_load_gif_from_memory(
        buf.data(), static_cast<int>(buf.size()),
        &delays, &frameW, &frameH, &frameCount, &comp, 4 /*RGBA*/);

    if (!pixels || frameCount <= 0)
    {
        brls::Logger::warning("ProImage: not an animated GIF or decode failed: {}", path);
        if (pixels) stbi_image_free(pixels);
        if (delays) stbi_image_free(delays);
        setImageFromFile(path); // fallback to static image
        return;
    }

    NVGcontext* vg = brls::Application::getNVGContext();
    const size_t frameSizeBytes = static_cast<size_t>(frameW) * static_cast<size_t>(frameH) * 4;

    for (int i = 0; i < frameCount; ++i)
    {
        GifFrame gf;
        gf.delay_ms = (delays && delays[i] > 0) ? delays[i] : 100;
        gf.texture  = nvgCreateImageRGBA(vg, frameW, frameH, 0,
                                         pixels + i * frameSizeBytes);
        m_gifFrames.push_back(gf);
    }

    stbi_image_free(pixels);
    if (delays) stbi_image_free(delays);

    if (!m_gifFrames.empty())
    {
        m_isGif           = true;
        m_gifCurrentFrame = 0;
        m_gifElapsedMs    = 0.0f;
        // Store first frame as the base texture so brls::Image layout works
        innerSetImage(m_gifFrames[0].texture);
        originalImageWidth  = static_cast<float>(frameW);
        originalImageHeight = static_cast<float>(frameH);
        invalidate();
    }
}

// ── Shader Animation ─────────────────────────────────────────────────────────

void ProImage::setShaderAnimation(ShaderAnimationType type)
{
    m_shaderAnimation = type;
    m_animTime        = 0.0f;
    invalidate();
}

ShaderAnimationType ProImage::getShaderAnimation() const { return m_shaderAnimation; }

// ── Draw ─────────────────────────────────────────────────────────────────────

void ProImage::draw(NVGcontext* vg, float x, float y, float w, float h,
                    brls::Style style, brls::FrameContext* ctx)
{
    // ── GIF frame advance ──
    if (m_isGif && !m_gifFrames.empty())
    {
        m_gifElapsedMs += ASSUMED_FRAME_MS;
        float threshold = static_cast<float>(m_gifFrames[m_gifCurrentFrame].delay_ms);
        if (m_gifElapsedMs >= threshold)
        {
            m_gifElapsedMs -= threshold;
            m_gifCurrentFrame = (m_gifCurrentFrame + 1) % static_cast<int>(m_gifFrames.size());
            // Swap the active texture so brls::Image draws the right frame
            texture = m_gifFrames[m_gifCurrentFrame].texture;
        }
        invalidate(); // keep redrawing
    }

    if (m_blurEnabled && texture)
    {
        // ── Kawase Blur approximation via multiple semi-transparent draws ──
        // Compute the image bounds the same way brls::Image does.
        // We only need to call the parent once (which sets up paint / bounds),
        // then we draw the blurred overlay on top.
        brls::Image::draw(vg, x, y, w, h, style, ctx);
        drawBlur(vg, x, y, w, h, paint);
    }
    else
    {
        brls::Image::draw(vg, x, y, w, h, style, ctx);
    }

    // ── Shader animation overlay ──
    if (m_shaderAnimation != ShaderAnimationType::NONE)
    {
        m_animTime += ASSUMED_FRAME_SEC;
        if (m_shaderAnimation == ShaderAnimationType::PSP_LINES)
            drawPspLines(vg, x, y, w, h);
        invalidate();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Kawase Blur approximation.
 *
 * Kawase blur applies a sequence of box-filter passes at increasing radii
 * (e.g. offsets 0, 1, 2, 2, 3) to approximate a Gaussian.  In NanoVG we
 * cannot do real framebuffer passes, so we replicate the visual by drawing
 * the image N times at small pixel offsets with reduced opacity.  The result
 * is not physically identical to Kawase but gives a convincing soft-blur look
 * without any GL extensions.
 *
 * Passes: centre + 8 cardinal/diagonal neighbours per iteration × 2 rounds.
 */
void ProImage::drawBlur(NVGcontext* vg, float x, float y, float w, float h,
                        NVGpaint basePaint)
{
    // Number of blur iterations; more iterations → smoother but heavier
    static constexpr int   BLUR_PASSES  = 3;
    // Attenuation per sample: sum of all samples converges to ~1
    static constexpr float BASE_ALPHA   = 0.12f;

    const float r = m_blurRadius;

    // Offsets: 4 axis-aligned + 4 diagonal per pass
    static constexpr int OFFSETS = 8;
    static constexpr float OX[OFFSETS] = { 1, -1,  0,  0,  1, -1,  1, -1 };
    static constexpr float OY[OFFSETS] = { 0,  0,  1, -1,  1,  1, -1, -1 };

    nvgSave(vg);
    nvgScissor(vg, x, y, w, h);

    for (int pass = 1; pass <= BLUR_PASSES; ++pass)
    {
        float offset = r * static_cast<float>(pass) / static_cast<float>(BLUR_PASSES);
        for (int i = 0; i < OFFSETS; ++i)
        {
            float dx = OX[i] * offset;
            float dy = OY[i] * offset;

            NVGpaint p = basePaint;
            // Shift the paint origin
            p.xform[4] += dx;
            p.xform[5] += dy;
            // Reduce inner colour alpha so blurred copies don't overbrighten
            p.innerColor.a *= BASE_ALPHA;
            p.outerColor.a *= BASE_ALPHA;

            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillPaint(vg, p);
            nvgFill(vg);
        }
    }

    nvgRestore(vg);
}

/**
 * PSP XMB-style flowing line animation.
 *
 * Draws a set of semi-transparent diagonal lines that slowly drift downward,
 * creating the characteristic "scanning" effect of the PlayStation Portable
 * XMB (XrossMediaBar) background.
 */
void ProImage::drawPspLines(NVGcontext* vg, float x, float y, float w, float h)
{
    // Number of lines visible at once
    static constexpr int   LINE_COUNT   = 12;
    // Line thickness
    static constexpr float LINE_WIDTH   = 1.2f;
    // Speed (fraction of height per second)
    static constexpr float SPEED        = 0.08f;
    // Angle from vertical (radians) – ~20°
    static constexpr double ANGLE_DEG    = 20.0;
    static const float      TAN_A        = static_cast<float>(std::tan(ANGLE_DEG * PI / 180.0));

    // Vertical offset cycles in [0, 1) with wrapping
    float phase = std::fmod(m_animTime * SPEED, 1.0f);

    nvgSave(vg);
    nvgScissor(vg, x, y, w, h);
    nvgStrokeWidth(vg, LINE_WIDTH);

    for (int i = 0; i < LINE_COUNT; ++i)
    {
        // Fractional vertical position of each line's top in [0, 1)
        float t = std::fmod(static_cast<float>(i) / static_cast<float>(LINE_COUNT) + phase, 1.0f);
        float lineY = y + t * (h + w * TAN_A) - w * TAN_A;

        // Fade in/out based on position
        float alpha = 0.18f + 0.12f * static_cast<float>(std::sin(t * PI));

        nvgBeginPath(vg);
        // The line enters at the left with a diagonal: top-left → bottom-right
        nvgMoveTo(vg, x,     lineY + w * TAN_A);
        nvgLineTo(vg, x + w, lineY);
        nvgStrokeColor(vg, nvgRGBAf(1.0f, 1.0f, 1.0f, alpha));
        nvgStroke(vg);
    }

    nvgRestore(vg);
}

} // namespace beiklive::UI
