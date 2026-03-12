#include "UI/Utils/ProImage.hpp"

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

// Use the stb_image bundled with borealis/nanovg for GIF decoding.
// The header guard avoids re-defining the implementation since image.cpp
// already does STB_IMAGE_IMPLEMENTATION in a separate translation unit.
#include <borealis/extern/nanovg/stb_image.h>

// ── OpenGL / NanoVG GL backend (needed for FBO and shader rendering) ─────────
#ifdef BOREALIS_USE_OPENGL
#  ifdef USE_GLES3
#    define NANOVG_GLES3
#  elif defined(USE_GLES2)
#    define NANOVG_GLES2
#  elif defined(USE_GL2)
#    define NANOVG_GL2
#  else
#    define NANOVG_GL3
#  endif
#  include <borealis/extern/nanovg/nanovg_gl.h>
#endif

namespace beiklive::UI
{

// ─────────────────────────────────────────────────────────────────────────────
//  PSP XMB Ripple — GLSL shader sources
//  Vertex shader: outputs a fullscreen quad (two NDC triangles).
//  Fragment shader: renders the PSP XMB wave-ribbon pattern.
// ─────────────────────────────────────────────────────────────────────────────

#ifdef BOREALIS_USE_OPENGL

// GLSL version header chosen at compile time so the shader compiles on both
// desktop OpenGL 3.x (core) and OpenGL ES 3.0 (Switch / mobile).
#if defined(USE_GLES3) || defined(USE_GLES2)
#  define GLSL_VER "#version 300 es\nprecision mediump float;\n"
#else
#  define GLSL_VER "#version 330 core\n"
#endif

static const char* k_xmbVertSrc = GLSL_VER
R"(
in  vec2 aPos;
out vec2 vUV;
void main()
{
    // aPos is in [-1,+1] NDC for the region; UV maps to [0,1]
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* k_xmbFragSrc = GLSL_VER
R"(
uniform float uTime;
uniform vec2  uResolution;
in  vec2 vUV;
out vec4 FragColor;

// Simple value-noise helpers (from RetroArch XMB ribbon shader)
float iqhash(float n)
{
    return fract(sin(n) * 43758.5453);
}

float noise3(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0 + 113.0 * p.z;
    return mix(
        mix(mix(iqhash(n +   0.0), iqhash(n +   1.0), f.x),
            mix(iqhash(n +  57.0), iqhash(n +  58.0), f.x), f.y),
        mix(mix(iqhash(n + 113.0), iqhash(n + 114.0), f.x),
            mix(iqhash(n + 170.0), iqhash(n + 171.0), f.x), f.y),
        f.z);
}

// XMB wave function – the heart of the PSP ribbon effect
float xmbWave(float x, float z)
{
    return cos(z * 4.0) * cos(z + uTime / 10.0 + x);
}

void main()
{
    vec2 p = vUV;

    // Dark blue gradient background
    vec3 color = mix(vec3(0.02, 0.03, 0.14),
                     vec3(0.04, 0.06, 0.25),
                     p.y);

    const int N = 24; // number of ribbons
    for (int i = 0; i < N; i++)
    {
        float fi = float(i);
        float t  = fi / float(N);

        // Ribbon centre Y, scrolling slowly downward
        float cy = fract(t + uTime * 0.025);

        // Wave displacement (XMB formula)
        float wave = xmbWave(p.x * 3.0, cy * 3.14159) * 0.04;

        // Additional noise turbulence (makes waves more organic)
        vec3 npos = vec3(p.x * 0.4 + uTime / 5.0,
                         cy  * 3.0  + uTime / 10.0,
                         p.y * 2.0  + uTime / 100.0);
        wave += noise3(npos * 7.0) * 0.012;

        float dist = abs(p.y - cy - wave);

        // Soft ribbon profile (smooth edge falloff)
        float ribbonW = 0.0025 + 0.0010 * sin(fi * 1.3 + uTime * 0.5);
        float alpha   = smoothstep(ribbonW * 4.0, 0.0, dist);

        // Colour: blend blue → purple based on ribbon index
        vec3 rc = mix(vec3(0.25, 0.55, 1.00),
                      vec3(0.65, 0.30, 1.00),
                      fract(t * 2.5));

        // Brighten ribbons near the bottom for variety
        rc *= 0.6 + 0.4 * p.y;

        color += rc * alpha * 0.5;
    }

    // Subtle horizontal scanline vignette
    float vig = 1.0 - 0.35 * pow(abs(p.y - 0.5) * 2.0, 2.5);
    color *= vig;

    FragColor = vec4(clamp(color, 0.0, 1.0), 0.90);
}
)";

// ─────────────────────────────────────────────────────────────────────────────
//  GL helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Compile a single GL shader stage and return the shader handle (or 0 on error).
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512] = {};
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        brls::Logger::error("ProImage XMB shader compile error: {}", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

/**
 * Import a raw GL texture into NanoVG without transferring ownership.
 * Selects the correct nvglCreateImageFromHandle* variant based on the
 * active GL backend (GL2 / GL3 / GLES2 / GLES3).
 */
static int nvgImageFromRawTexture(NVGcontext* vg, GLuint tex, int w, int h)
{
    constexpr int kFlags = NVG_IMAGE_NODELETE; // NVG must not free our texture
#if defined(USE_GLES2)
    return nvglCreateImageFromHandleGLES2(vg, tex, w, h, kFlags);
#elif defined(USE_GLES3)
    return nvglCreateImageFromHandleGLES3(vg, tex, w, h, kFlags);
#elif defined(USE_GL2)
    return nvglCreateImageFromHandleGL2(vg, tex, w, h, kFlags);
#else
    return nvglCreateImageFromHandleGL3(vg, tex, w, h, kFlags);
#endif
}

#endif // BOREALIS_USE_OPENGL

// ─────────────────────────────────────────────────────────────────────────────
//  ProImage
// ─────────────────────────────────────────────────────────────────────────────

ProImage::ProImage() = default;

ProImage::~ProImage()
{
    freeGifFrames();
#ifdef BOREALIS_USE_OPENGL
    NVGcontext* vg = brls::Application::getNVGContext();
    freeXmbResources(vg);
#endif
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
    m_gifCurrentFrame  = 0;
    m_gifElapsedMs     = 0.0f;
    m_isGif            = false;
    m_gifTimerStarted  = false;
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
        m_isGif            = true;
        m_gifCurrentFrame  = 0;
        m_gifElapsedMs     = 0.0f;
        m_gifTimerStarted  = false;
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
    m_shaderAnimation     = type;
    m_animTime            = 0.0f;
    m_shaderTimerStarted  = false;
    invalidate();
}

ShaderAnimationType ProImage::getShaderAnimation() const { return m_shaderAnimation; }

// ── Draw ─────────────────────────────────────────────────────────────────────

void ProImage::draw(NVGcontext* vg, float x, float y, float w, float h,
                    brls::Style style, brls::FrameContext* ctx)
{
    using Clock = std::chrono::steady_clock;
    const auto now = Clock::now();

    // ── GIF frame advance (clock-based) ───────────────────────────────────────
    if (m_isGif && !m_gifFrames.empty())
    {
        if (!m_gifTimerStarted)
        {
            m_gifLastTime      = now;
            m_gifTimerStarted  = true;
        }
        else
        {
            float deltaMs = std::chrono::duration<float, std::milli>(
                                now - m_gifLastTime).count();
            m_gifLastTime    = now;
            m_gifElapsedMs  += deltaMs;

            // Advance through as many frames as the elapsed time covers.
            // threshold is updated each iteration to the NEW current frame's
            // delay, correctly handling variable-delay GIFs over large deltas.
            float threshold = static_cast<float>(
                m_gifFrames[m_gifCurrentFrame].delay_ms);
            while (m_gifElapsedMs >= threshold)
            {
                m_gifElapsedMs   -= threshold;
                m_gifCurrentFrame = (m_gifCurrentFrame + 1)
                                    % static_cast<int>(m_gifFrames.size());
                threshold = static_cast<float>(
                    m_gifFrames[m_gifCurrentFrame].delay_ms);
            }
            texture = m_gifFrames[m_gifCurrentFrame].texture;
        }
        invalidate(); // keep redrawing
    }

    if (m_blurEnabled && texture)
    {
        brls::Image::draw(vg, x, y, w, h, style, ctx);
        drawBlur(vg, x, y, w, h, paint);
    }
    else
    {
        brls::Image::draw(vg, x, y, w, h, style, ctx);
    }

    // ── Shader animation overlay (clock-based time) ───────────────────────────
    if (m_shaderAnimation != ShaderAnimationType::NONE)
    {
        if (!m_shaderTimerStarted)
        {
            m_shaderLastTime     = now;
            m_shaderTimerStarted = true;
        }
        else
        {
            float deltaSec = std::chrono::duration<float>(
                                 now - m_shaderLastTime).count();
            m_shaderLastTime  = now;
            m_animTime       += deltaSec;
        }

#ifdef BOREALIS_USE_OPENGL
        if (m_shaderAnimation == ShaderAnimationType::PSP_XMB_RIPPLE)
            drawPspXmbShader(vg, x, y, w, h);
#endif
        invalidate();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Kawase Blur
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Kawase Blur approximation via multiple semi-transparent NanoVG draws.
 * Each pass samples at small pixel offsets to approximate a Gaussian blur.
 */
void ProImage::drawBlur(NVGcontext* vg, float x, float y, float w, float h,
                        NVGpaint basePaint)
{
    static constexpr int   BLUR_PASSES = 3;
    static constexpr float BASE_ALPHA  = 0.12f;

    const float r = m_blurRadius;

    static constexpr int   OFFSETS = 8;
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
            p.xform[4] += dx;
            p.xform[5] += dy;
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

// ─────────────────────────────────────────────────────────────────────────────
//  PSP XMB ripple — OpenGL shader rendering
// ─────────────────────────────────────────────────────────────────────────────

#ifdef BOREALIS_USE_OPENGL

void ProImage::initXmbShader()
{
    if (m_xmbInited)
        return;
    m_xmbInited = true; // mark even on failure to avoid repeated retries

    // ── Compile and link shader program ──────────────────────────────────────
    GLuint vert = compileShader(GL_VERTEX_SHADER,   k_xmbVertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, k_xmbFragSrc);
    if (!vert || !frag)
    {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[512] = {};
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        brls::Logger::error("ProImage XMB shader link error: {}", log);
        glDeleteProgram(prog);
        return;
    }
    m_xmbProgram = prog;

    // Uniform locations
    m_xmbUTime       = glGetUniformLocation(prog, "uTime");
    m_xmbUResolution = glGetUniformLocation(prog, "uResolution");

    // ── Create VAO/VBO for a fullscreen quad (NDC [-1,+1]) ────────────────────
    // Will be updated per-draw with the actual widget NDC coordinates.
    glGenVertexArrays(1, &m_xmbVAO);
    glGenBuffers(1, &m_xmbVBO);

    glBindVertexArray(m_xmbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_xmbVBO);
    // Reserve space; data is filled in drawPspXmbShader each frame
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    brls::Logger::info("ProImage: PSP XMB shader compiled successfully");
}

void ProImage::resizeXmbFbo(int w, int h, NVGcontext* vg)
{
    if (m_xmbFboW == w && m_xmbFboH == h)
        return;

    // Delete old NVG image handle (so NVG forgets the old texture)
    if (m_xmbNvgImage >= 0 && vg)
    {
        nvgDeleteImage(vg, m_xmbNvgImage);
        m_xmbNvgImage = -1;
    }

    // Delete old FBO / texture
    if (m_xmbFbo)    { glDeleteFramebuffers(1, &m_xmbFbo);  m_xmbFbo    = 0; }
    if (m_xmbFboTex) { glDeleteTextures(1, &m_xmbFboTex);   m_xmbFboTex = 0; }

    // Create new texture
    // GL_RGBA8 is unavailable in GLES2; use GL_RGBA there (same bits, different token).
    glGenTextures(1, &m_xmbFboTex);
    glBindTexture(GL_TEXTURE_2D, m_xmbFboTex);
#if defined(USE_GLES2)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create FBO and attach texture
    glGenFramebuffers(1, &m_xmbFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_xmbFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_xmbFboTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_xmbFboW = w;
    m_xmbFboH = h;
}

void ProImage::freeXmbResources(NVGcontext* vg)
{
    if (m_xmbNvgImage >= 0 && vg)
    {
        nvgDeleteImage(vg, m_xmbNvgImage);
        m_xmbNvgImage = -1;
    }
    if (m_xmbFbo)     { glDeleteFramebuffers(1, &m_xmbFbo);   m_xmbFbo    = 0; }
    if (m_xmbFboTex)  { glDeleteTextures(1, &m_xmbFboTex);    m_xmbFboTex = 0; }
    if (m_xmbVBO)     { glDeleteBuffers(1, &m_xmbVBO);        m_xmbVBO    = 0; }
    if (m_xmbVAO)     { glDeleteVertexArrays(1, &m_xmbVAO);   m_xmbVAO    = 0; }
    if (m_xmbProgram) { glDeleteProgram(m_xmbProgram);        m_xmbProgram = 0; }
    m_xmbFboW   = 0;
    m_xmbFboH   = 0;
    m_xmbInited = false;
}

/**
 * Render the PSP XMB ripple wave effect:
 *  1. Render the GLSL shader to an off-screen FBO texture.
 *  2. Save / restore all GL state touched (FBO binding, viewport,
 *     shader program, VAO, VBO) so NanoVG is not disturbed.
 *  3. Import the FBO texture into NanoVG and draw it as a NVG paint.
 */
void ProImage::drawPspXmbShader(NVGcontext* vg, float x, float y, float w, float h)
{
    if (!m_xmbInited)
        initXmbShader();
    if (!m_xmbProgram)
        return; // shader unavailable (compile failed)

    const int iw = static_cast<int>(w);
    const int ih = static_cast<int>(h);
    if (iw <= 0 || ih <= 0)
        return;

    // Ensure FBO matches current widget size
    resizeXmbFbo(iw, ih, vg);

    // ── Save relevant GL state ────────────────────────────────────────────────
    GLint prevFBO      = 0;
    GLint prevViewport[4] = {};
    GLint prevProgram  = 0;
    GLint prevVAO      = 0;
    GLint prevVBO      = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,  &prevFBO);
    glGetIntegerv(GL_VIEWPORT,              prevViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM,      &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVBO);

    // ── Render XMB pattern to FBO ─────────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, m_xmbFbo);
    glViewport(0, 0, iw, ih);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_xmbProgram);
    if (m_xmbUTime >= 0)
        glUniform1f(m_xmbUTime, m_animTime);
    if (m_xmbUResolution >= 0)
        glUniform2f(m_xmbUResolution, static_cast<float>(iw), static_cast<float>(ih));

    // The quad covers NDC [-1,+1] within the FBO (which IS the widget area)
    const float quad[8] = {
        -1.0f,  1.0f,   // top-left
         1.0f,  1.0f,   // top-right
        -1.0f, -1.0f,   // bottom-left
         1.0f, -1.0f,   // bottom-right
    };
    glBindVertexArray(m_xmbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_xmbVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ── Restore GL state ──────────────────────────────────────────────────────
    glBindBuffer(GL_ARRAY_BUFFER,      prevVBO);
    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER,  prevFBO);

    // ── Import FBO texture into NanoVG (lazy, one-time per size) ─────────────
    if (m_xmbNvgImage < 0)
        m_xmbNvgImage = nvgImageFromRawTexture(vg, m_xmbFboTex, iw, ih);

    // ── Draw FBO texture via NanoVG (blended on top of the base image) ────────
    if (m_xmbNvgImage >= 0)
    {
        NVGpaint paint = nvgImagePattern(vg, x, y, w, h,
                                         0.0f, m_xmbNvgImage, 1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }
}

#endif // BOREALIS_USE_OPENGL

} // namespace beiklive::UI
