#include "Video/renderer/FullscreenQuad.hpp"

#include <borealis.hpp>

namespace beiklive {

// ============================================================
// 顶点数据布局（每顶点 12 个 float = 48 字节）
//
//   layout=0  VertexCoord  vec4 (x, y, z, w)
//   layout=1  COLOR        vec4 (r, g, b, a)
//   layout=2  TexCoord     vec4 (u, v, 0, 0)
//
// 四顶点构成覆盖全 NDC 的矩形（-1..+1 × -1..+1）：
//   底左 (−1,−1)  TexCoord (0,0)
//   底右 (+1,−1)  TexCoord (1,0)
//   顶右 (+1,+1)  TexCoord (1,1)
//   顶左 (−1,+1)  TexCoord (0,1)
// ============================================================
static const float k_quadVerts[] = {
    //  x     y    z    w      r    g    b    a      u    v    s    t
    -1.f, -1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   0.f, 0.f, 0.f, 0.f,
     1.f, -1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   1.f, 0.f, 0.f, 0.f,
     1.f,  1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   1.f, 1.f, 0.f, 0.f,
    -1.f,  1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   0.f, 1.f, 0.f, 0.f,
};
static const GLuint k_quadIndices[] = { 0, 1, 2, 0, 2, 3 };

// 每顶点字节数
static constexpr GLsizei k_stride = 12 * sizeof(float);
// 各属性在顶点内的字节偏移
static constexpr GLsizei k_offVertex  = 0;                  // VertexCoord
static constexpr GLsizei k_offColor   = 4 * sizeof(float);  // COLOR
static constexpr GLsizei k_offTexCoord = 8 * sizeof(float); // TexCoord

// ============================================================
// init
// ============================================================
bool FullscreenQuad::init()
{
    if (m_vbo) return true; // 已初始化

#if !defined(USE_GLES2)
    // GL3 / GLES3：使用 VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
#endif

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quadVerts),
                 k_quadVerts, GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(k_quadIndices),
                 k_quadIndices, GL_STATIC_DRAW);

    // 配置顶点属性
    // VertexCoord (location=0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offVertex));
    // COLOR (location=1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offColor));
    // TexCoord (location=2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offTexCoord));

#if !defined(USE_GLES2)
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    brls::Logger::debug("FullscreenQuad: 初始化完成 VAO={} VBO={} EBO={}",
                        m_vao, m_vbo, m_ebo);
    return true;
}

// ============================================================
// deinit
// ============================================================
void FullscreenQuad::deinit()
{
    if (m_ebo) { glDeleteBuffers(1, &m_ebo);         m_ebo = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo);         m_vbo = 0; }
#if !defined(USE_GLES2)
    if (m_vao) { glDeleteVertexArrays(1, &m_vao);    m_vao = 0; }
#endif
}

// ============================================================
// draw
// ============================================================
void FullscreenQuad::draw() const
{
    if (!m_vbo) return;

#if !defined(USE_GLES2)
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
#else
    // GLES2 无 VAO，手动绑定缓冲和属性
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offColor));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, k_stride,
                          reinterpret_cast<const void*>(k_offTexCoord));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

} // namespace beiklive
