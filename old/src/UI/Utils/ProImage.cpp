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

// ── OpenGL / NanoVG GL 后端（FBO 和着色器渲染所需） ──────────────────────────
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
//  PSP XMB 波纹 — GLSL 着色器源码
//  顶点着色器：输出全屏四边形（两个 NDC 三角形）
//  片元着色器：渲染 PSP XMB 波浪带图案
// ─────────────────────────────────────────────────────────────────────────────

#ifdef BOREALIS_USE_OPENGL

// 编译时选择 GLSL 版本头，使着色器同时兼容桌面 OpenGL 3.x 和 OpenGL ES 3.0（Switch/移动端）
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
    // aPos 为区域内 [-1,+1] NDC 坐标；UV 映射到 [0,1]
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

    // 可配置的背景颜色
    vec3 color = uBgColor;

    // 两条以正弦/余弦波交织的中心线
    float amplitude = 0.06;
    float freq      = 6.28318; // 宽度方向一个完整周期
    float speed     = 1.2;

    // 红线：以 y=0.5 为中心的正弦波
    float waveRed  = amplitude * sin(p.x * freq - uTime * speed);
    float distRed  = abs(p.y - 0.5 - waveRed);
    float alphaRed = smoothstep(0.008, 0.0, distRed);
    color += vec3(1.0, 0.15, 0.10) * alphaRed;

    // 蓝线：以 y=0.5 为中心的余弦波（相移 π/2）
    float waveBlue  = amplitude * cos(p.x * freq - uTime * speed);
    float distBlue  = abs(p.y - 0.5 - waveBlue);
    float alphaBlue = smoothstep(0.008, 0.0, distBlue);
    color += vec3(0.10, 0.40, 1.00) * alphaBlue;

    FragColor = vec4(clamp(color, 0.0, 1.0), 0.90);
}
)";

// ─────────────────────────────────────────────────────────────────────────────
//  GL 辅助函数
// ─────────────────────────────────────────────────────────────────────────────

/// 编译单个 GL 着色器阶段并返回句柄（失败时返回 0）
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
 * 将原始 GL 纹理导入 NanoVG，不转移所有权。
 * 根据当前 GL 后端（GL2/GL3/GLES2/GLES3）选择对应的 nvglCreateImageFromHandle* 变体。
 */
static int nvgImageFromRawTexture(NVGcontext* vg, GLuint tex, int w, int h)
{
    constexpr int kFlags = NVG_IMAGE_NODELETE; // NVG 不释放此纹理
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

// ── Kawase 模糊 ──────────────────────────────────────────────────────────────

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

// ── 异步图片加载 ──────────────────────────────────────────────────────────────

void ProImage::setImageFromFileAsync(const std::string& path)
{
    if (path.empty())
        return;

    // 递增生成计数器以取消待处理的异步加载
    int gen = ++m_asyncGen;

    // 优先从文件字节缓存中查找（仅主线程）
    auto& byteCache = beiklive::UI::ImageFileCache::instance();
    if (const auto* cached = byteCache.getBytes(path))
    {
        // 缓存命中：同步解码（无磁盘 I/O）
        m_asyncLoading = false;

        const std::vector<uint8_t>& bytes = *cached;
        NVGcontext* vg2 = brls::Application::getNVGContext();
        int tex = nvgCreateImageMem(vg2, getImageFlags(),
            const_cast<unsigned char*>(bytes.data()), static_cast<int>(bytes.size()));
        if (tex > 0) innerSetImage(tex);
        return;
    }

    // 缓存未命中：启动异步文件读取
    m_asyncLoading = true;
    invalidate();

    ASYNC_RETAIN
    std::string capturedPath = path;
    brls::async([ASYNC_TOKEN, capturedPath, gen]()
    {
        // 后台线程：将文件读入内存
        std::vector<uint8_t> bytes;
        {
            std::ifstream f(capturedPath, std::ios::binary | std::ios::ate);
            if (f.is_open())
            {
                auto size = static_cast<std::streamsize>(f.tellg());
                f.seekg(0);
                bytes.resize(static_cast<size_t>(size));
                f.read(reinterpret_cast<char*>(bytes.data()), size);
                // 丢弃不完整读取
                if (f.gcount() != size)
                    bytes.clear();
            }
        }

        // 将结果回传主线程
        brls::sync([ASYNC_TOKEN, bytes = std::move(bytes), capturedPath, gen]()
        {
            ASYNC_RELEASE
            // 若此后又发起了新的加载请求则丢弃当前结果
            if (gen != this->m_asyncGen.load())
                return;

            this->m_asyncLoading = false;

            if (bytes.empty())
                return;

            // 存入字节缓存以便后续快速访问
            beiklive::UI::ImageFileCache::instance().storeBytes(capturedPath, bytes);

            NVGcontext* vg2 = brls::Application::getNVGContext();
            int tex = nvgCreateImageMem(vg2, this->getImageFlags(),
                const_cast<unsigned char*>(bytes.data()), static_cast<int>(bytes.size()));
            if (tex > 0) this->innerSetImage(tex);
        });
    });
}

// ── 着色器动画 ────────────────────────────────────────────────────────────────

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

// ── 绘制 ──────────────────────────────────────────────────────────────────────

void ProImage::draw(NVGcontext* vg, float x, float y, float w, float h,
                    brls::Style style, brls::FrameContext* ctx)
{
    // ── 异步加载中时显示占位符 ───────────────────────────────────────────────
    if (m_asyncLoading)
    {
        nvgSave(vg);
        nvgFontSize(vg, 20.0f);
        nvgFillColor(vg, nvgRGBA(180, 180, 180, 200));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, "加载中...", nullptr);
        nvgRestore(vg);
        invalidate(); // 持续触发重绘以检测加载完成
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
//  Kawase 模糊
// ─────────────────────────────────────────────────────────────────────────────

/**
 * 基于多次半透明 NanoVG 绘制的 Kawase 模糊近似。
 * 每次采样在小像素偏移处叠加以近似高斯模糊。
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
    m_xmbInited = true; // 即使失败也标记，避免重复初始化

    // ── 编译并链接着色器程序 ──────────────────────────────────────────────────
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

    // Uniform 位置
    m_xmbUTime       = glGetUniformLocation(prog, "uTime");
    m_xmbUResolution = glGetUniformLocation(prog, "uResolution");
    m_xmbUBgColor    = glGetUniformLocation(prog, "uBgColor");

    // ── 创建全屏四边形 VAO/VBO（NDC [-1,+1]），每帧更新实际控件坐标 ──────────
    glGenVertexArrays(1, &m_xmbVAO);
    glGenBuffers(1, &m_xmbVBO);

    glBindVertexArray(m_xmbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_xmbVBO);
    // 预留空间；数据在每帧 drawPspXmbShader 中填充
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

    // 删除旧 NVG 图像句柄（使 NVG 忘记旧纹理）
    if (m_xmbNvgImage >= 0 && vg)
    {
        nvgDeleteImage(vg, m_xmbNvgImage);
        m_xmbNvgImage = -1;
    }

    // 删除旧 FBO / 纹理
    if (m_xmbFbo)    { glDeleteFramebuffers(1, &m_xmbFbo);  m_xmbFbo    = 0; }
    if (m_xmbFboTex) { glDeleteTextures(1, &m_xmbFboTex);   m_xmbFboTex = 0; }

    // 创建新纹理（GLES2 中 GL_RGBA8 不可用，使用 GL_RGBA）
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

    // 创建 FBO 并附加纹理
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
 * 渲染 PSP XMB 波纹效果：
 *  1. 将 GLSL 着色器渲染到离屏 FBO 纹理
 *  2. 保存/恢复所有接触到的 GL 状态（FBO、视口、着色器、VAO、VBO）以免干扰 NanoVG
 *  3. 将 FBO 纹理导入 NanoVG 并用 NVG 画刷绘制
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

    // ── 将 XMB 图案渲染到 FBO ─────────────────────────────────────────────────
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

    // 四边形覆盖 FBO 内的 NDC [-1,+1]（即控件区域）
    const float quad[8] = {
        -1.0f,  1.0f,   // 左上
         1.0f,  1.0f,   // 右上
        -1.0f, -1.0f,   // 左下
         1.0f, -1.0f,   // 右下
    };
    glBindVertexArray(m_xmbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_xmbVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ── 恢复 GL 状态 ──────────────────────────────────────────────────────────
    glBindBuffer(GL_ARRAY_BUFFER,      prevVBO);
    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER,  prevFBO);

    // ── 将 FBO 纹理导入 NanoVG（懒初始化，尺寸变化时重建） ───────────────────
    if (m_xmbNvgImage < 0)
        m_xmbNvgImage = nvgImageFromRawTexture(vg, m_xmbFboTex, iw, ih);

    // ── 通过 NanoVG 绘制 FBO 纹理（叠加在底图之上） ───────────────────────────
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
