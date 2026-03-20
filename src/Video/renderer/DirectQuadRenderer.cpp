#include "Video/renderer/DirectQuadRenderer.hpp"

#include <borealis.hpp>

namespace beiklive {

// ============================================================
// 直通顶点着色器
//
// 输入：
//   aPos (location=0)  NDC 二维坐标 (x, y)
//   aUV  (location=1)  纹理坐标 (u, v)
// ============================================================

#if defined(USE_GLES2)
static const char* k_vertSrc =
    "#version 100\n"
    "attribute vec2 aPos;\n"
    "attribute vec2 aUV;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "}\n";

static const char* k_fragSrc =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 vUV;\n"
    "uniform sampler2D uTex;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(uTex, vUV);\n"
    "}\n";

#elif defined(USE_GL2)
static const char* k_vertSrc =
    "#version 120\n"
    "attribute vec2 aPos;\n"
    "attribute vec2 aUV;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "}\n";

static const char* k_fragSrc =
    "#version 120\n"
    "varying vec2 vUV;\n"
    "uniform sampler2D uTex;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(uTex, vUV);\n"
    "}\n";

#elif defined(USE_GLES3)
static const char* k_vertSrc =
    "#version 300 es\n"
    "in vec2 aPos;\n"
    "in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "}\n";

static const char* k_fragSrc =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 vUV;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D uTex;\n"
    "void main() {\n"
    "    fragColor = texture(uTex, vUV);\n"
    "}\n";

#else // GL3（默认）
static const char* k_vertSrc =
    "#version 130\n"
    "in vec2 aPos;\n"
    "in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "}\n";

static const char* k_fragSrc =
    "#version 130\n"
    "in vec2 vUV;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D uTex;\n"
    "void main() {\n"
    "    fragColor = texture(uTex, vUV);\n"
    "}\n";
#endif

// 顶点格式：每顶点 4 个 float（x, y, u, v）
// 顶点排列：
//   0: 左上  (ndcLeft,  ndcTop,    0, 0)
//   1: 右上  (ndcRight, ndcTop,    1, 0)
//   2: 右下  (ndcRight, ndcBottom, 1, 1)
//   3: 左下  (ndcLeft,  ndcBottom, 0, 1)
// GLES2 仅支持 GL_UNSIGNED_SHORT 索引，GL3/GLES3 支持 GL_UNSIGNED_INT。
#if defined(USE_GLES2)
static const GLushort k_indices[] = { 0, 1, 2, 0, 2, 3 };
static constexpr GLenum k_indexType = GL_UNSIGNED_SHORT;
#else
static const GLuint k_indices[] = { 0, 1, 2, 0, 2, 3 };
static constexpr GLenum k_indexType = GL_UNSIGNED_INT;
#endif
static constexpr GLsizei k_stride    = 4 * sizeof(float);
static constexpr GLsizei k_offPos    = 0;
static constexpr GLsizei k_offUV     = 2 * sizeof(float);
static constexpr int     k_vertCount = 4;

// ============================================================
// init
// ============================================================
bool DirectQuadRenderer::init()
{
    if (m_prog) return true; // 已初始化

    // ---- 编译并链接直通着色器程序 ----
    auto compileShader = [](GLenum type, const char* src) -> GLuint {
        GLuint shader = glCreateShader(type);
        if (!shader) return 0;
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint ok = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (ok == GL_FALSE) {
            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (len > 0) {
                std::string log(static_cast<size_t>(len), '\0');
                glGetShaderInfoLog(shader, len, nullptr, &log[0]);
                brls::Logger::error("DirectQuadRenderer: 着色器编译失败:\n{}", log);
            }
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    };

    GLuint vert = compileShader(GL_VERTEX_SHADER,   k_vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, k_fragSrc);
    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return false;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glBindAttribLocation(prog, 0, "aPos");
    glBindAttribLocation(prog, 1, "aUV");
    glLinkProgram(prog);
    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        if (len > 0) {
            std::string log(static_cast<size_t>(len), '\0');
            glGetProgramInfoLog(prog, len, nullptr, &log[0]);
            brls::Logger::error("DirectQuadRenderer: 着色器链接失败:\n{}", log);
        }
        glDeleteProgram(prog);
        return false;
    }

    m_uTex = glGetUniformLocation(prog, "uTex");

    // ---- 创建 VAO/VBO/EBO ----
#if !defined(USE_GLES2)
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
#endif

    // VBO（动态，内容在 render() 中更新）
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(k_vertCount * k_stride),
                 nullptr, GL_DYNAMIC_DRAW);

    // EBO（静态索引）
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(k_indices), k_indices, GL_STATIC_DRAW);

    // 顶点属性：aPos (location=0), aUV (location=1)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offPos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offUV));

#if !defined(USE_GLES2)
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_prog = prog;
    brls::Logger::debug("DirectQuadRenderer: 初始化完成 prog={} vao={} vbo={}",
                        m_prog, m_vao, m_vbo);
    return true;
}

// ============================================================
// deinit
// ============================================================
void DirectQuadRenderer::deinit()
{
    if (m_ebo)  { glDeleteBuffers(1, &m_ebo);         m_ebo  = 0; }
    if (m_vbo)  { glDeleteBuffers(1, &m_vbo);         m_vbo  = 0; }
#if !defined(USE_GLES2)
    if (m_vao)  { glDeleteVertexArrays(1, &m_vao);    m_vao  = 0; }
#endif
    if (m_prog) { glDeleteProgram(m_prog);             m_prog = 0; }
    m_uTex = -1;
}

// ============================================================
// render
// ============================================================
void DirectQuadRenderer::render(GLuint tex,
                                 float ndcLeft, float ndcRight,
                                 float ndcTop,  float ndcBottom) const
{
    if (!m_prog || !tex) return;

    // ── 顶点数据（每帧按 NDC 坐标动态生成）──
    // UV 约定：v=0 对应 OpenGL 纹理底部（即游戏帧第一行，视觉上的顶部），
    // v=1 对应 OpenGL 纹理顶部（游戏帧最后一行，视觉上的底部）。
    // 因此：屏幕顶端 (ndcTop) → UV v=0，屏幕底端 (ndcBottom) → UV v=1，图像正向显示。
    const float verts[] = {
        ndcLeft,  ndcTop,    0.0f, 0.0f,  // 左上：UV (0,0) = 游戏帧顶部
        ndcRight, ndcTop,    1.0f, 0.0f,  // 右上：UV (1,0)
        ndcRight, ndcBottom, 1.0f, 1.0f,  // 右下：UV (1,1) = 游戏帧底部
        ndcLeft,  ndcBottom, 0.0f, 1.0f,  // 左下：UV (0,1)
    };

    // ── 保存关键 GL 状态 ──
    GLint  prevProg    = 0;
    GLint  prevVAO     = 0;
    GLint  prevVBO     = 0;
    GLint  prevEBO     = 0;
    GLint  prevTex     = 0;
    GLint  prevFBO     = 0;
    GLint  prevActive  = 0;
    GLboolean blendEn  = GL_FALSE;
    GLboolean depthEn  = GL_FALSE;
    GLboolean stencilEn = GL_FALSE;
    GLboolean cullEn   = GL_FALSE;
    GLboolean scissorEn = GL_FALSE;
    GLint  prevSrcRGB  = 0, prevDstRGB  = 0;
    GLint  prevSrcAlpha = 0, prevDstAlpha = 0;

    glGetIntegerv(GL_CURRENT_PROGRAM,        &prevProg);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,   &prevVBO);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevEBO);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,     &prevTex);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,    &prevFBO);
    glGetIntegerv(GL_ACTIVE_TEXTURE,         &prevActive);
#if !defined(USE_GLES2)
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,   &prevVAO);
#endif
    blendEn   = glIsEnabled(GL_BLEND);
    depthEn   = glIsEnabled(GL_DEPTH_TEST);
    stencilEn = glIsEnabled(GL_STENCIL_TEST);
    cullEn    = glIsEnabled(GL_CULL_FACE);
    scissorEn = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_BLEND_SRC_RGB,   &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB,   &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);

    // ── 设置直通渲染所需 GL 状态 ──
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    // 游戏帧为不透明，关闭混合以直接写入像素
    glDisable(GL_BLEND);

    // 渲染到当前帧缓冲（borealis 已绑定 FBO 0 = 屏幕）
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFBO));

    glUseProgram(m_prog);

    // ── 绑定纹理到 unit 0 ──
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    if (m_uTex >= 0) glUniform1i(m_uTex, 0);

    // ── 更新顶点缓冲并绘制 ──
#if !defined(USE_GLES2)
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(sizeof(verts)), verts);
    glDrawElements(GL_TRIANGLES, 6, k_indexType, nullptr);
    glBindVertexArray(0);
#else
    // GLES2：无 VAO，手动绑定属性
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(sizeof(verts)), verts);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offPos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offUV));
    glDrawElements(GL_TRIANGLES, 6, k_indexType, nullptr);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif

    // ── 恢复 GL 状态 ──
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));
    glActiveTexture(static_cast<GLenum>(prevActive));

    if (blendEn)   glEnable(GL_BLEND);   else glDisable(GL_BLEND);
    if (depthEn)   glEnable(GL_DEPTH_TEST);
    if (stencilEn) glEnable(GL_STENCIL_TEST);
    if (cullEn)    glEnable(GL_CULL_FACE);
    if (scissorEn) glEnable(GL_SCISSOR_TEST);
    glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB),
                        static_cast<GLenum>(prevDstRGB),
                        static_cast<GLenum>(prevSrcAlpha),
                        static_cast<GLenum>(prevDstAlpha));

    glUseProgram(static_cast<GLuint>(prevProg));
    glBindBuffer(GL_ARRAY_BUFFER,
                 static_cast<GLuint>(prevVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLuint>(prevEBO));
#if !defined(USE_GLES2)
    glBindVertexArray(static_cast<GLuint>(prevVAO));
#endif
}

} // namespace beiklive
