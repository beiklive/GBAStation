#include "UI/Utils/ProImage.hpp"
#include "UI/Utils/ImageFileCache.hpp"
#include "common.hpp"

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/thread.hpp>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

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
uniform vec3  uBgColor;
in  vec2 vUV;
out vec4 FragColor;

void main()
{
    vec2 p = vUV;

    // Configurable background colour
    vec3 color = uBgColor;

    // Two lines centred vertically, interleaving with sine/cosine waves
    float amplitude = 0.06;
    float freq      = 6.28318; // one full cycle across the width
    float speed     = 1.2;

    // Red line: sine wave around y = 0.5
    float waveRed  = amplitude * sin(p.x * freq - uTime * speed);
    float distRed  = abs(p.y - 0.5 - waveRed);
    float alphaRed = smoothstep(0.008, 0.0, distRed);
    color += vec3(1.0, 0.15, 0.10) * alphaRed;

    // Blue line: cosine wave (phase-shifted by π/2) around y = 0.5
    float waveBlue  = amplitude * cos(p.x * freq - uTime * speed);
    float distBlue  = abs(p.y - 0.5 - waveBlue);
    float alphaBlue = smoothstep(0.008, 0.0, distBlue);
    color += vec3(0.10, 0.40, 1.00) * alphaBlue;

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
    beiklive::UnregisterXmbBackground(this);
#ifdef BOREALIS_USE_OPENGL
    NVGcontext* vg = brls::Application::getNVGContext();
    freeXmbResources(vg);
#endif
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

// ── Async image loading ───────────────────────────────────────────────────────

void ProImage::setImageFromFileAsync(const std::string& path)
{
    if (path.empty())
        return;

    // Increment the generation counter to cancel any pending async load.
    int gen = ++m_asyncGen;

    // Check the file-byte cache first (main thread only).
    auto& byteCache = beiklive::UI::ImageFileCache::instance();
    if (const auto* cached = byteCache.getBytes(path))
    {
        // Cache hit – decode/load from cached bytes synchronously (no disk I/O).
        m_asyncLoading = false;

        const std::vector<uint8_t>& bytes = *cached;
        NVGcontext* vg2 = brls::Application::getNVGContext();
        int tex = nvgCreateImageMem(vg2, getImageFlags(),
            const_cast<unsigned char*>(bytes.data()), static_cast<int>(bytes.size()));
        if (tex > 0) innerSetImage(tex);
        return;
    }

    // Cache miss – start async file read.
    m_asyncLoading = true;
    invalidate();

    ASYNC_RETAIN
    std::string capturedPath = path;
    brls::async([ASYNC_TOKEN, capturedPath, gen]()
    {
        // Background thread: read the file into memory.
        std::vector<uint8_t> bytes;
        {
            std::ifstream f(capturedPath, std::ios::binary | std::ios::ate);
            if (f.is_open())
            {
                auto size = static_cast<std::streamsize>(f.tellg());
                f.seekg(0);
                bytes.resize(static_cast<size_t>(size));
                f.read(reinterpret_cast<char*>(bytes.data()), size);
                // Discard partial reads
                if (f.gcount() != size)
                    bytes.clear();
            }
        }

        // Marshal result back to the main thread.
        brls::sync([ASYNC_TOKEN, bytes = std::move(bytes), capturedPath, gen]()
        {
            ASYNC_RELEASE
            // If another load request was issued after this one, discard.
            if (gen != this->m_asyncGen.load())
                return;

            this->m_asyncLoading = false;

            if (bytes.empty())
                return;

            // Store in byte cache for future fast access.
            beiklive::UI::ImageFileCache::instance().storeBytes(capturedPath, bytes);

            NVGcontext* vg2 = brls::Application::getNVGContext();
            int tex = nvgCreateImageMem(vg2, this->getImageFlags(),
                const_cast<unsigned char*>(bytes.data()), static_cast<int>(bytes.size()));
            if (tex > 0) this->innerSetImage(tex);
        });
    });
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

void ProImage::setXmbBgColor(float r, float g, float b)
{
    m_xmbBgR = r;
    m_xmbBgG = g;
    m_xmbBgB = b;
    invalidate();
}

// ── Draw ─────────────────────────────────────────────────────────────────────

void ProImage::draw(NVGcontext* vg, float x, float y, float w, float h,
                    brls::Style style, brls::FrameContext* ctx)
{
    // ── Show loading placeholder while async load is in progress ─────────────
    if (m_asyncLoading)
    {
        nvgSave(vg);
        nvgFontSize(vg, 20.0f);
        nvgFillColor(vg, nvgRGBA(180, 180, 180, 200));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, "加载中...", nullptr);
        nvgRestore(vg);
        invalidate(); // keep ticking so we notice when loading completes
        return;
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

    if (m_shaderAnimation != ShaderAnimationType::NONE)
    {
        using Clock = std::chrono::steady_clock;
        const auto now = Clock::now();
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
    m_xmbUBgColor    = glGetUniformLocation(prog, "uBgColor");

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
    if (m_xmbUBgColor >= 0)
        glUniform3f(m_xmbUBgColor, m_xmbBgR, m_xmbBgG, m_xmbBgB);

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
