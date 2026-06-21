#include "game/render/GameRenderer.hpp"

#include <borealis.hpp>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace beiklive {

namespace {
struct ScopedGLCopyState {
    GLint activeTexture = 0;
    GLint texture2D = 0;
    GLint readFbo = 0;
    GLint drawFbo = 0;
    GLint readBuffer = GL_BACK;
    GLint drawBuffer = GL_BACK;
    GLint program = 0;
    GLint arrayBuffer = 0;
    GLint elementArrayBuffer = 0;
    GLint viewport[4] = {0, 0, 0, 0};
    GLint scissorBox[4] = {0, 0, 0, 0};
    GLboolean colorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    GLboolean depthMask = GL_TRUE;
    GLboolean blendEnabled = GL_FALSE;
    GLboolean depthEnabled = GL_FALSE;
    GLboolean stencilEnabled = GL_FALSE;
    GLboolean cullEnabled = GL_FALSE;
    GLboolean scissorEnabled = GL_FALSE;
    GLint blendSrcRGB = GL_ONE;
    GLint blendDstRGB = GL_ZERO;
    GLint blendSrcAlpha = GL_ONE;
    GLint blendDstAlpha = GL_ZERO;
#if !defined(USE_GLES2)
    GLint vao = 0;
#endif

    ScopedGLCopyState()
    {
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2D);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
        glGetIntegerv(GL_READ_BUFFER, &readBuffer);
        glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        blendEnabled = glIsEnabled(GL_BLEND);
        depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        stencilEnabled = glIsEnabled(GL_STENCIL_TEST);
        cullEnabled = glIsEnabled(GL_CULL_FACE);
        scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
#if !defined(USE_GLES2)
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
#endif
    }

    ~ScopedGLCopyState()
    {
        glUseProgram(static_cast<GLuint>(program));
#if !defined(USE_GLES2)
        glBindVertexArray(static_cast<GLuint>(vao));
#endif
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFbo));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFbo));
        glReadBuffer(static_cast<GLenum>(readBuffer));
        glDrawBuffer(static_cast<GLenum>(drawBuffer));
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        glDepthMask(depthMask);
        if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (depthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (stencilEnabled) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        if (cullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (scissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        glBlendFuncSeparate(static_cast<GLenum>(blendSrcRGB),
                            static_cast<GLenum>(blendDstRGB),
                            static_cast<GLenum>(blendSrcAlpha),
                            static_cast<GLenum>(blendDstAlpha));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(elementArrayBuffer));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture2D));
        glActiveTexture(static_cast<GLenum>(activeTexture));
    }
};
}

const GameTexture& GameRenderer::displayTexture() const
{
    if (m_usingCopiedTexture && m_copyTextures[m_copyDisplayIndex].isValid())
        return m_copyTextures[m_copyDisplayIndex];
    return m_texture;
}

bool GameRenderer::isReady() const
{
    return displayTexture().isValid() && m_renderChain.isDirectRendererReady();
}

unsigned GameRenderer::texWidth() const
{
    return displayTexture().width();
}

unsigned GameRenderer::texHeight() const
{
    return displayTexture().height();
}

GLuint GameRenderer::texId() const
{
    return displayTexture().texId();
}

// ============================================================
// init
// ============================================================
bool GameRenderer::init(unsigned width, unsigned height, bool linear,
                         const std::string& shaderPath)
{
    m_linear = linear;

    // 初始化 GL 纹理
    if (!m_texture.init(width, height, linear)) {
        brls::Logger::error("GameRenderer: 游戏帧纹理初始化失败 ({}x{})", width, height);
        return false;
    }

    // 初始化渲染链（含直接渲染器和可选的着色器管线）
    if (!m_renderChain.init(shaderPath)) {
        brls::Logger::error("GameRenderer: RenderChain 初始化失败");
        m_texture.deinit();
        return false;
    }

    brls::Logger::info("GameRenderer: 初始化完成 ({}x{} linear={} shader={})",
                       width, height, linear,
                       shaderPath.empty() ? "无" : shaderPath);
    return true;
}

// ============================================================
// deinit
// ============================================================
void GameRenderer::deinit()
{
    if (m_copyReadFbo) {
        glDeleteFramebuffers(1, &m_copyReadFbo);
        m_copyReadFbo = 0;
    }
    if (m_copyDrawFbo) {
        glDeleteFramebuffers(1, &m_copyDrawFbo);
        m_copyDrawFbo = 0;
    }
    if (m_copyDrawVbo) {
        glDeleteBuffers(1, &m_copyDrawVbo);
        m_copyDrawVbo = 0;
    }
#if !defined(USE_GLES2)
    if (m_copyDrawVao) {
        glDeleteVertexArrays(1, &m_copyDrawVao);
        m_copyDrawVao = 0;
    }
#endif
    if (m_copyDrawProgram) {
        glDeleteProgram(m_copyDrawProgram);
        m_copyDrawProgram = 0;
    }
    for (auto& texture : m_copyTextures)
        texture.deinit();
    m_copyWriteIndex = 0;
    m_copyDisplayIndex = 0;
    m_copyFrameActive = false;
    m_usingCopiedTexture = false;
    m_renderChain.deinit();
    m_texture.deinit();
}

// ============================================================
// uploadFrame – 将 libretro VideoFrame 上传至 GL 纹理
// ============================================================
void GameRenderer::uploadFrame(const LibretroLoader::VideoFrame& frame)
{
    if (frame.pixels.empty() || frame.width == 0 || frame.height == 0)
        return;

    m_copyFrameActive = false;
    m_usingCopiedTexture = false;

    // 若尺寸发生变化，重新初始化纹理
    if (frame.width != m_texture.width() || frame.height != m_texture.height()) {
        m_texture.init(frame.width, frame.height, m_linear);
    }

    // 上传 RGBA8888 数据（LibretroLoader 已将帧数据转换为 RGBA8888）
    FrameUploader::upload(m_texture.texId(),
                          frame.width, frame.height,
                          frame.pixels.data(),
                          m_texture.width(), m_texture.height());
}

bool GameRenderer::copyFromTexture(GLuint srcTex, unsigned srcW, unsigned srcH,
                                   unsigned dstW, unsigned dstH)
{
    if (!srcTex || srcW == 0 || srcH == 0 || dstW == 0 || dstH == 0)
        return false;

    if (dstW != m_texture.width() || dstH != m_texture.height())
        m_texture.init(dstW, dstH, m_linear);

    if (!m_texture.isValid())
        return false;

    return copyTextureRegion(srcTex, 0, 0, 0, 0,
                             std::min(srcW, dstW),
                             std::min(srcH, dstH),
                             dstW, dstH);
}

bool GameRenderer::copyTextureRegion(GLuint srcTex,
                                     unsigned srcX, unsigned srcY,
                                     unsigned dstX, unsigned dstY,
                                     unsigned copyW, unsigned copyH,
                                     unsigned dstW, unsigned dstH)
{
    if (!srcTex || copyW == 0 || copyH == 0 || dstW == 0 || dstH == 0)
        return false;

    if (dstW != m_texture.width() || dstH != m_texture.height())
        m_texture.init(dstW, dstH, m_linear);

    if (!m_texture.isValid())
        return false;

    return copyTextureRegionToTexture(m_texture.texId(), srcTex, srcX, srcY, dstX, dstY, copyW, copyH);
}

bool GameRenderer::beginTextureCopyFrame(unsigned dstW, unsigned dstH)
{
    if (dstW == 0 || dstH == 0)
        return false;

    auto& writeTexture = m_copyTextures[m_copyWriteIndex];
    if (writeTexture.width() != dstW || writeTexture.height() != dstH) {
        if (!writeTexture.init(dstW, dstH, m_linear))
            return false;
    }

    m_copyFrameActive = true;
    return writeTexture.isValid();
}

bool GameRenderer::copyTextureRegionToCopyFrame(GLuint srcTex,
                                                unsigned srcX, unsigned srcY,
                                                unsigned dstX, unsigned dstY,
                                                unsigned copyW, unsigned copyH)
{
    if (!m_copyFrameActive || !m_copyTextures[m_copyWriteIndex].isValid())
        return false;
    return copyTextureRegionToTexture(m_copyTextures[m_copyWriteIndex].texId(),
                                      srcTex, srcX, srcY, dstX, dstY, copyW, copyH);
}

void GameRenderer::commitTextureCopyFrame()
{
    if (!m_copyFrameActive || !m_copyTextures[m_copyWriteIndex].isValid())
        return;
    m_copyDisplayIndex = m_copyWriteIndex;
    m_copyWriteIndex ^= 1u;
    m_copyFrameActive = false;
    m_usingCopiedTexture = true;
}

void GameRenderer::abortTextureCopyFrame()
{
    m_copyFrameActive = false;
}

bool GameRenderer::ensureCopyDrawResources()
{
    if (m_copyDrawProgram && m_copyDrawVbo)
        return true;

#if defined(USE_GLES2)
    static const char* vertSrc =
        "#version 100\n"
        "attribute vec2 aPos;\n"
        "attribute vec2 aUV;\n"
        "varying vec2 vUV;\n"
        "void main(){ gl_Position = vec4(aPos, 0.0, 1.0); vUV = aUV; }\n";
    static const char* fragSrc =
        "#version 100\n"
        "precision mediump float;\n"
        "varying vec2 vUV;\n"
        "uniform sampler2D uTex;\n"
        "void main(){ gl_FragColor = texture2D(uTex, vUV); }\n";
#elif defined(USE_GL2)
    static const char* vertSrc =
        "#version 120\n"
        "attribute vec2 aPos;\n"
        "attribute vec2 aUV;\n"
        "varying vec2 vUV;\n"
        "void main(){ gl_Position = vec4(aPos, 0.0, 1.0); vUV = aUV; }\n";
    static const char* fragSrc =
        "#version 120\n"
        "varying vec2 vUV;\n"
        "uniform sampler2D uTex;\n"
        "void main(){ gl_FragColor = texture2D(uTex, vUV); }\n";
#elif defined(USE_GLES3)
    static const char* vertSrc =
        "#version 300 es\n"
        "in vec2 aPos;\n"
        "in vec2 aUV;\n"
        "out vec2 vUV;\n"
        "void main(){ gl_Position = vec4(aPos, 0.0, 1.0); vUV = aUV; }\n";
    static const char* fragSrc =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 vUV;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D uTex;\n"
        "void main(){ fragColor = texture(uTex, vUV); }\n";
#else
    static const char* vertSrc =
        "#version 150\n"
        "in vec2 aPos;\n"
        "in vec2 aUV;\n"
        "out vec2 vUV;\n"
        "void main(){ gl_Position = vec4(aPos, 0.0, 1.0); vUV = aUV; }\n";
    static const char* fragSrc =
        "#version 150\n"
        "in vec2 vUV;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D uTex;\n"
        "void main(){ fragColor = texture(uTex, vUV); }\n";
#endif

    auto compileShader = [](GLenum type, const char* source) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint ok = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (ok != GL_TRUE) {
            GLint logLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(static_cast<size_t>(std::max(1, logLen)), '\0');
            glGetShaderInfoLog(shader, logLen, nullptr, log.data());
            brls::Logger::warning("GameRenderer: copy shader compile failed: {}", log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    m_copyDrawProgram = glCreateProgram();
    glAttachShader(m_copyDrawProgram, vs);
    glAttachShader(m_copyDrawProgram, fs);
    glBindAttribLocation(m_copyDrawProgram, 0, "aPos");
    glBindAttribLocation(m_copyDrawProgram, 1, "aUV");
    glLinkProgram(m_copyDrawProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_copyDrawProgram, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLen = 0;
        glGetProgramiv(m_copyDrawProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(std::max(1, logLen)), '\0');
        glGetProgramInfoLog(m_copyDrawProgram, logLen, nullptr, log.data());
        brls::Logger::warning("GameRenderer: copy shader link failed: {}", log);
        glDeleteProgram(m_copyDrawProgram);
        m_copyDrawProgram = 0;
        return false;
    }

    glGenBuffers(1, &m_copyDrawVbo);
#if !defined(USE_GLES2)
    glGenVertexArrays(1, &m_copyDrawVao);
    glBindVertexArray(m_copyDrawVao);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, m_copyDrawVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(sizeof(float) * 2));
#if !defined(USE_GLES2)
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool GameRenderer::copyTextureRegionByDraw(GLuint dstTex,
                                           GLuint srcTex,
                                           unsigned srcX, unsigned srcY,
                                           unsigned dstX, unsigned dstY,
                                           unsigned copyW, unsigned copyH)
{
    if (!ensureCopyDrawResources())
        return false;

    unsigned dstW = 0;
    unsigned dstH = 0;
    if (m_copyFrameActive && m_copyTextures[m_copyWriteIndex].texId() == dstTex) {
        dstW = m_copyTextures[m_copyWriteIndex].width();
        dstH = m_copyTextures[m_copyWriteIndex].height();
    } else if (m_texture.texId() == dstTex) {
        dstW = m_texture.width();
        dstH = m_texture.height();
    }
    if (dstW == 0 || dstH == 0)
        return false;

    ScopedGLCopyState state;
    while (glGetError() != GL_NO_ERROR) {
    }

    if (!m_copyDrawFbo)
        glGenFramebuffers(1, &m_copyDrawFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, m_copyDrawFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;

    glViewport(static_cast<GLint>(dstX), static_cast<GLint>(dstY),
               static_cast<GLsizei>(copyW), static_cast<GLsizei>(copyH));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glUseProgram(m_copyDrawProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTex);
    const GLint uTex = glGetUniformLocation(m_copyDrawProgram, "uTex");
    if (uTex >= 0)
        glUniform1i(uTex, 0);

    constexpr float srcW = 256.0f;
    constexpr float srcH = 386.0f;
    const float u0 = static_cast<float>(srcX) / srcW;
    const float v0 = static_cast<float>(srcY) / srcH;
    const float u1 = static_cast<float>(srcX + copyW) / srcW;
    const float v1 = static_cast<float>(srcY + copyH) / srcH;
    const float verts[] = {
        -1.0f, -1.0f, u0, v0,
         1.0f, -1.0f, u1, v0,
        -1.0f,  1.0f, u0, v1,
        -1.0f,  1.0f, u0, v1,
         1.0f, -1.0f, u1, v0,
         1.0f,  1.0f, u1, v1,
    };

#if !defined(USE_GLES2)
    glBindVertexArray(m_copyDrawVao);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, m_copyDrawVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
#if defined(USE_GLES2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(sizeof(float) * 2));
#endif
    glDrawArrays(GL_TRIANGLES, 0, 6);
    const GLenum error = glGetError();
    return error == GL_NO_ERROR;
}

bool GameRenderer::copyTextureRegionToTexture(GLuint dstTex,
                                               GLuint srcTex,
                                               unsigned srcX, unsigned srcY,
                                              unsigned dstX, unsigned dstY,
                                              unsigned copyW, unsigned copyH)
{
    if (!srcTex || !dstTex || copyW == 0 || copyH == 0)
        return false;

#if defined(__SWITCH__)
    return copyTextureRegionByDraw(dstTex, srcTex, srcX, srcY, dstX, dstY, copyW, copyH);
#endif

    ScopedGLCopyState state;

    while (glGetError() != GL_NO_ERROR) {
    }

#if !defined(__SWITCH__)
    if (glCopyImageSubData)
    {
        glCopyImageSubData(srcTex, GL_TEXTURE_2D, 0,
                           static_cast<GLint>(srcX),
                           static_cast<GLint>(srcY),
                           0,
                           dstTex, GL_TEXTURE_2D, 0,
                           static_cast<GLint>(dstX),
                           static_cast<GLint>(dstY),
                           0,
                           static_cast<GLsizei>(copyW),
                           static_cast<GLsizei>(copyH),
                           1);
        const GLenum copyError = glGetError();
        if (copyError == GL_NO_ERROR)
            return true;
        brls::Logger::warning("GameRenderer: glCopyImageSubData failed glError=0x{:x}; falling back to FBO blit",
                              static_cast<unsigned>(copyError));
    }
#endif

    GLint previousReadFbo = 0;
    GLint previousDrawFbo = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFbo);
    if (!m_copyReadFbo)
        glGenFramebuffers(1, &m_copyReadFbo);
    if (!m_copyDrawFbo)
        glGenFramebuffers(1, &m_copyDrawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_copyReadFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srcTex, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_copyDrawFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    const bool complete =
        glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
        glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (complete)
    {
        glBlitFramebuffer(static_cast<GLint>(srcX),
                          static_cast<GLint>(srcY),
                          static_cast<GLint>(srcX + copyW),
                          static_cast<GLint>(srcY + copyH),
                          static_cast<GLint>(dstX),
                          static_cast<GLint>(dstY),
                          static_cast<GLint>(dstX + copyW),
                          static_cast<GLint>(dstY + copyH),
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    const GLenum error = glGetError();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFbo));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFbo));
    return complete && error == GL_NO_ERROR;
}

// ============================================================
// setFilter
// ============================================================
void GameRenderer::setFilter(bool linear)
{
    m_linear = linear;
    m_texture.setFilter(linear);
    for (auto& texture : m_copyTextures)
        texture.setFilter(linear);
}

// ============================================================
// setShader – 加载或切换着色器预设
// ============================================================
void GameRenderer::setShader(const std::string& shaderPath)
{
    m_renderChain.setShader(shaderPath);
}

// ============================================================
// drawToScreen – 通过渲染链将游戏帧绘制到屏幕指定矩形
// ============================================================
void GameRenderer::drawToScreen(float virtX, float virtY, float virtW, float virtH,
                                 float windowScale, int windowW, int windowH)
{
    if (!isReady()) return;

    // 计算视口物理尺寸（供着色器管线 viewport 缩放类型计算）
    // 使用 llround 四舍五入，避免 static_cast<unsigned> 的截断导致 1 像素精度丢失
    const auto viewW = static_cast<unsigned>(std::llround(static_cast<double>(virtW) * static_cast<double>(windowScale)));
    const auto viewH = static_cast<unsigned>(std::llround(static_cast<double>(virtH) * static_cast<double>(windowScale)));

    // 通过渲染链处理游戏帧纹理（着色器模式或直通模式）
    const auto& texture = displayTexture();
    GLuint finalTex = m_renderChain.run(texture.texId(),
                                        texture.width(), texture.height(),
                                        viewW, viewH);

    // 将最终纹理绘制到屏幕指定矩形
    m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                               windowScale, windowW, windowH);
}

void GameRenderer::drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                                       float virtX, float virtY, float virtW, float virtH,
                                       float windowScale, int windowW, int windowH)
{
    drawExternalTexture(tex, texW, texH, virtX, virtY, virtW, virtH,
                        windowScale, windowW, windowH, 0.0f, 0.0f, 1.0f, 1.0f);
}

void GameRenderer::drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                                       float virtX, float virtY, float virtW, float virtH,
                                       float windowScale, int windowW, int windowH,
                                       float u0, float v0, float u1, float v1,
                                       bool swizzleRB)
{
    if (!tex || texW == 0 || texH == 0 || !m_renderChain.isDirectRendererReady())
        return;

    GLuint finalTex = m_renderChain.run(tex, texW, texH,
        static_cast<unsigned>(std::llround(static_cast<double>(virtW) * static_cast<double>(windowScale))),
        static_cast<unsigned>(std::llround(static_cast<double>(virtH) * static_cast<double>(windowScale))));

    if (finalTex == tex)
        m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                                   windowScale, windowW, windowH, u0, v0, u1, v1, swizzleRB);
    else
        m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                                   windowScale, windowW, windowH);
}

} // namespace beiklive
