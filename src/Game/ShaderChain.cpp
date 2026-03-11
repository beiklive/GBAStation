#include "Game/ShaderChain.hpp"

// ---- NanoVG 后端检测（必须在 nanovg_gl.h 之前引入）----
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
#endif

#include <borealis.hpp>  // brls::Logger
#include <cstring>

// ============================================================
// GLSL 着色器源码
// 版本/精度前缀在运行时拼接，使同一段着色器体
// 可在 GLES2、GLES3 及桌面 GL3/GL4 下正常编译。
// ============================================================
#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)

static const char* const k_glslHeader =
#  if defined(NANOVG_GLES3)
    "#version 300 es\nprecision mediump float;\n";
#  else
    "#version 330 core\n";
#  endif

// 现代 GLSL（使用 in/out、texture()）
static const char* const k_vertBody =
    "in  vec2 offset;\n"
    "uniform vec2 dims;\n"
    "uniform vec2 insize;\n"
    "out vec2 texCoord;\n"
    "void main() {\n"
    "    vec2 scaledOffset = offset * dims;\n"
    // x: 将 [0,1] 映射到 [-1,+1]（从左到右）
    // y: 将 [0,1] 映射到 [-1,+1]（从上到下，使 NVG 读取时 y=0 为图像顶部）
    "    gl_Position = vec4(scaledOffset.x * 2.0 - dims.x,\n"
    "                       scaledOffset.y * 2.0 - dims.y,\n"
    "                       0.0, 1.0);\n"
    "    texCoord = offset * insize;\n"
    "}\n";

static const char* const k_fragBody =
    "in  vec2 texCoord;\n"
    "uniform sampler2D tex;\n"
    "uniform vec4 color;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    // 强制 alpha=1，丢弃 XBGR 像素格式中的零 alpha 字节
    "    vec4 c = vec4(texture(tex, texCoord).rgb, 1.0);\n"
    "    fragColor = c * color;\n"
    "}\n";

#else  // GLES2 / GL2

static const char* const k_glslHeader =
#  if defined(NANOVG_GLES2)
    "#version 100\nprecision mediump float;\n";
#  else
    "#version 120\n";
#  endif

// 旧版 GLSL（使用 attribute/varying、texture2D()、gl_FragColor）
static const char* const k_vertBody =
    "attribute vec2 offset;\n"
    "uniform vec2 dims;\n"
    "uniform vec2 insize;\n"
    "varying vec2 texCoord;\n"
    "void main() {\n"
    "    vec2 scaledOffset = offset * dims;\n"
    "    gl_Position = vec4(scaledOffset.x * 2.0 - dims.x,\n"
    "                       scaledOffset.y * 2.0 - dims.y,\n"
    "                       0.0, 1.0);\n"
    "    texCoord = offset * insize;\n"
    "}\n";

static const char* const k_fragBody =
    "varying vec2 texCoord;\n"
    "uniform sampler2D tex;\n"
    "uniform vec4 color;\n"
    "void main() {\n"
    "    vec4 c = vec4(texture2D(tex, texCoord).rgb, 1.0);\n"
    "    gl_FragColor = c * color;\n"
    "}\n";

#endif // 着色器版本选择

// 全屏四边形角点偏移量（两个三角形，通过 TRIANGLE_FAN 绘制）。
// 与原始 Switch main.c 中的布局一致。
static const GLfloat k_quadOffsets[] = {
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f,
    0.f, 1.f,
};

namespace beiklive {

// ============================================================
// ShaderChain：查询单个通道的所有 uniform / attrib 位置
// ============================================================
void ShaderChain::_lookupUniforms(ShaderPass& p)
{
    p.texLoc    = glGetUniformLocation(p.program, "tex");
    p.dimsLoc   = glGetUniformLocation(p.program, "dims");
    p.insizeLoc = glGetUniformLocation(p.program, "insize");
    p.colorLoc  = glGetUniformLocation(p.program, "color");
    p.offsetLoc = glGetAttribLocation (p.program, "offset");

    // RetroArch 兼容 uniforms（不存在时 glGetUniformLocation 返回 -1，安全忽略）
    p.sourceLoc     = glGetUniformLocation(p.program, "Source");
    if (p.sourceLoc < 0)
        p.sourceLoc = glGetUniformLocation(p.program, "Texture");
    p.sourceSizeLoc = glGetUniformLocation(p.program, "SourceSize");
    if (p.sourceSizeLoc < 0)
        p.sourceSizeLoc = glGetUniformLocation(p.program, "TextureSize");
    p.outputSizeLoc = glGetUniformLocation(p.program, "OutputSize");
    p.frameCountLoc = glGetUniformLocation(p.program, "FrameCount");
}

// ============================================================
// ShaderChain：构建第 0 通道及四边形几何体
// ============================================================
bool ShaderChain::init(const std::string& vertSrc, const std::string& fragSrc)
{
    // ---- 全屏四边形 VAO / VBO ----
    glGenBuffers(1, &m_vbo);
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quadOffsets), k_quadOffsets, GL_STATIC_DRAW);
    // 属性 0（"offset"）的绑定推迟到第 0 通道就绪后进行
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ---- 内置着色器程序（第 0 通道）----
    ShaderPass pass0;
    pass0.program = buildProgram(vertSrc.c_str(), fragSrc.c_str());
    if (!pass0.program) {
        brls::Logger::error("[ShaderChain] Built-in shader compilation FAILED");
        return false;
    }
    _lookupUniforms(pass0);
    _bindPassAttrib(pass0);

    // FBO 在首次调用 run() 时延迟创建（需要视频尺寸）
    m_passes.push_back(std::move(pass0));
    brls::Logger::debug("[ShaderChain] Initialised, built-in pass 0 ready");
    return true;
}

// ============================================================
// ShaderChain：默认内置初始化（使用内置 GLSL 源码）
// ============================================================
bool ShaderChain::initBuiltin()
{
    std::string vertSrc = std::string(k_glslHeader) + k_vertBody;
    std::string fragSrc = std::string(k_glslHeader) + k_fragBody;
    return init(vertSrc, fragSrc);
}

// ============================================================
// ShaderChain：释放所有 GL 资源
// ============================================================
void ShaderChain::deinit()
{
    for (auto& p : m_passes) {
        if (p.fbo)       glDeleteFramebuffers(1, &p.fbo);
        if (p.outputTex) glDeleteTextures(1,      &p.outputTex);
        if (p.program)   glDeleteProgram(p.program);
    }
    m_passes.clear();

    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { glDeleteBuffers(1,      &m_vbo); m_vbo = 0; }

    m_lastVideoW = 0;
    m_lastVideoH = 0;
    m_frameCount = 0;
    brls::Logger::debug("[ShaderChain] All GL resources released");
}

// ============================================================
// ShaderChain：追加用户自定义通道
// ============================================================
bool ShaderChain::addPass(const std::string& vert, const std::string& frag)
{
    ShaderPass p;
    p.program = buildProgram(vert.c_str(), frag.c_str());
    if (!p.program) return false;

    _lookupUniforms(p);
    _bindPassAttrib(p);

    // 强制下次 run() 时重建 FBO——最终输出纹理将发生变化
    m_lastVideoW = 0;
    m_lastVideoH = 0;

    m_passes.push_back(std::move(p));
    brls::Logger::info("[ShaderChain] User pass {} added", m_passes.size() - 1);
    return true;
}

// ============================================================
// ShaderChain：清除用户通道（保留第 0 通道）
// ============================================================
void ShaderChain::clearPasses()
{
    while (m_passes.size() > 1) {
        auto& p = m_passes.back();
        if (p.fbo)       glDeleteFramebuffers(1, &p.fbo);
        if (p.outputTex) glDeleteTextures(1,      &p.outputTex);
        if (p.program)   glDeleteProgram(p.program);
        m_passes.pop_back();
    }
    // 强制重建 FBO，使第 0 通道的输出纹理按正确尺寸重新分配
    m_lastVideoW = 0;
    m_lastVideoH = 0;
    brls::Logger::info("[ShaderChain] User passes cleared");
}

// ============================================================
// ShaderChain：获取输出纹理
// ============================================================
GLuint ShaderChain::outputTex() const
{
    if (m_passes.empty()) return 0;
    return m_passes.back().outputTex;
}

// ============================================================
// ShaderChain：执行着色器链
// ============================================================
GLuint ShaderChain::run(GLuint srcTex, unsigned videoW, unsigned videoH)
{
    if (m_passes.empty()) return srcTex;

    // 当分辨率变化时（重新）创建 FBO
    bool dimsChanged = (videoW != m_lastVideoW || videoH != m_lastVideoH);
    if (dimsChanged) {
        m_lastVideoW = videoW;
        m_lastVideoH = videoH;
    }
    for (auto& p : m_passes) {
        if (!p.fbo || dimsChanged) {
            if (!_initPassFbo(p, (int)videoW, (int)videoH))
                return srcTex;
        }
    }

    // ----------------------------------------------------------------
    // 保存 Borealis/NVG 依赖的所有 GL 状态。
    // NVG 使用了模板（裁剪路径）、裁剪测试、混合、自有着色器程序、
    // VAO、活动纹理单元及纹理绑定。
    // 必须在退出时将上下文恢复原状，
    // 以确保 nvgEndFrame() 的延迟刷新正常工作。
    // ----------------------------------------------------------------
    GLint     prevFbo           = 0;
    GLint     prevViewport[4]   = {};
    GLint     prevProgram       = 0;
    GLint     prevVAO           = 0;
    GLint     prevActiveTexture = GL_TEXTURE0;
    GLint     prevTex0          = 0;
    GLboolean prevScissor       = GL_FALSE;
    GLboolean prevDepth         = GL_FALSE;
    GLboolean prevStencil       = GL_FALSE;
    GLboolean prevBlend         = GL_FALSE;
    GLint     prevBlendSrcRGB   = GL_ONE,  prevBlendDstRGB   = GL_ZERO;
    GLint     prevBlendSrcAlpha = GL_ONE,  prevBlendDstAlpha = GL_ZERO;
    GLfloat   prevClearColor[4] = {};

    glGetIntegerv(GL_FRAMEBUFFER_BINDING,  &prevFbo);
    glGetIntegerv(GL_VIEWPORT,              prevViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM,       &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,  &prevVAO);
    glGetIntegerv(GL_ACTIVE_TEXTURE,        &prevActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,    &prevTex0);
    prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    prevDepth   = glIsEnabled(GL_DEPTH_TEST);
    prevStencil = glIsEnabled(GL_STENCIL_TEST);
    prevBlend   = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_RGB,         &prevBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB,         &prevBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,       &prevBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,       &prevBlendDstAlpha);
    glGetFloatv(GL_COLOR_CLEAR_VALUE,       prevClearColor);

    // 为我们的渲染通道设置干净的 GL 状态
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    // ---- 依次执行每个通道 ----
    GLuint inputTex = srcTex;
    for (size_t i = 0; i < m_passes.size(); ++i) {
        const ShaderPass& p = m_passes[i];
        if (!p.program || !p.fbo) continue;

        glBindFramebuffer(GL_FRAMEBUFFER, p.fbo);
        glViewport(0, 0, p.outW, p.outH);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTex);
        glUseProgram(p.program);

        if (p.texLoc    >= 0) glUniform1i(p.texLoc,   0);
        if (p.sourceLoc >= 0) glUniform1i(p.sourceLoc, 0);
        if (p.colorLoc  >= 0) glUniform4f(p.colorLoc,  1.f, 1.f, 1.f, 1.f);
        if (p.dimsLoc   >= 0) glUniform2f(p.dimsLoc,   1.f, 1.f);
        // insize: UV 覆盖整张纹理（源纹理已是 videoW×videoH，无填充）
        if (p.insizeLoc >= 0) glUniform2f(p.insizeLoc, 1.f, 1.f);

        // RetroArch 兼容 uniforms
        if (p.sourceSizeLoc >= 0) {
            float invW = (videoW > 0) ? 1.f / (float)videoW : 0.f;
            float invH = (videoH > 0) ? 1.f / (float)videoH : 0.f;
            glUniform4f(p.sourceSizeLoc,
                        (float)videoW, (float)videoH, invW, invH);
        }
        if (p.outputSizeLoc >= 0) {
            float invW = (p.outW > 0) ? 1.f / (float)p.outW : 0.f;
            float invH = (p.outH > 0) ? 1.f / (float)p.outH : 0.f;
            glUniform4f(p.outputSizeLoc,
                        (float)p.outW, (float)p.outH, invW, invH);
        }
        if (p.frameCountLoc >= 0) {
            // 大多数 RetroArch 着色器将 FrameCount 声明为 int，
            // 使用 glUniform1i 可同时兼容 int 和 uint 类型（正数范围内比特模式相同）
            glUniform1i(p.frameCountLoc, (GLint)m_frameCount);
        }

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        inputTex = p.outputTex;
    }

    ++m_frameCount;

    // ----------------------------------------------------------------
    // 恢复所有已保存的 GL 状态
    // ----------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glUseProgram((GLuint)prevProgram);
    glBindVertexArray((GLuint)prevVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex0);
    glActiveTexture((GLenum)prevActiveTexture);

    if (prevScissor) glEnable(GL_SCISSOR_TEST);  else glDisable(GL_SCISSOR_TEST);
    if (prevDepth)   glEnable(GL_DEPTH_TEST);    else glDisable(GL_DEPTH_TEST);
    if (prevStencil) glEnable(GL_STENCIL_TEST);  else glDisable(GL_STENCIL_TEST);
    if (prevBlend)   glEnable(GL_BLEND);         else glDisable(GL_BLEND);

    glBlendFuncSeparate((GLenum)prevBlendSrcRGB,   (GLenum)prevBlendDstRGB,
                        (GLenum)prevBlendSrcAlpha, (GLenum)prevBlendDstAlpha);
    glClearColor(prevClearColor[0], prevClearColor[1],
                 prevClearColor[2], prevClearColor[3]);

    return inputTex;
}

// ============================================================
// ShaderChain：编译并链接 GLSL 程序
// ============================================================
GLuint ShaderChain::buildProgram(const char* vertSrc, const char* fragSrc)
{
    auto compileShader = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            brls::Logger::error("[ShaderChain] Shader compile error: {}", log);
            glDeleteShader(s);
            return 0u;
        }
        return s;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    // 将 "offset" 固定到位置 0，使所有通道共享同一 VAO 绑定
    glBindAttribLocation(prog, 0, "offset");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        brls::Logger::error("[ShaderChain] Program link error: {}", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// ============================================================
// ShaderChain：设置 FBO 输出纹理的过滤模式
// ============================================================
void ShaderChain::setFilter(GLenum glFilter)
{
    if (m_glFilter == glFilter) return;
    m_glFilter = glFilter;
    for (auto& p : m_passes) {
        if (!p.outputTex) continue;
        glBindTexture(GL_TEXTURE_2D, p.outputTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_glFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// ============================================================
// ShaderChain：为单个通道（重新）创建 FBO 及颜色纹理
// ============================================================
bool ShaderChain::_initPassFbo(ShaderPass& p, int w, int h)
{
    if (p.fbo)       { glDeleteFramebuffers(1, &p.fbo);       p.fbo       = 0; }
    if (p.outputTex) { glDeleteTextures(1,      &p.outputTex); p.outputTex = 0; }

    glGenTextures(1, &p.outputTex);
    glBindTexture(GL_TEXTURE_2D, p.outputTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_glFilter);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &p.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, p.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, p.outputTex, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        brls::Logger::error("[ShaderChain] FBO incomplete: {:#x}", (unsigned)status);
        glDeleteFramebuffers(1, &p.fbo);       p.fbo       = 0;
        glDeleteTextures(1,     &p.outputTex); p.outputTex = 0;
        return false;
    }

    p.outW = w;
    p.outH = h;
    brls::Logger::debug("[ShaderChain] FBO created: {}×{}, tex={}, fbo={}", w, h,
                        p.outputTex, p.fbo);
    return true;
}

// ============================================================
// ShaderChain：在共享 VAO 中绑定 "offset" 顶点属性
// ============================================================
void ShaderChain::_bindPassAttrib(ShaderPass& p)
{
    if (p.offsetLoc < 0 || !m_vao || !m_vbo) return;
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer((GLuint)p.offsetLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray((GLuint)p.offsetLoc);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace beiklive
