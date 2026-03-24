#pragma once

#include <glad/glad.h>

namespace beiklive {

/// 全屏四边形渲染辅助类
///
/// 在 OpenGL NDC 空间（-1 到 +1）中创建一个覆盖整个视口的四边形，
/// 供 RetroArch 着色器通道使用。
///
/// 顶点格式（每顶点 12 个 float，共 48 字节）：
///   layout=0  VertexCoord (vec4 xyzw)
///   layout=1  COLOR       (vec4 rgba)
///   layout=2  TexCoord    (vec4 uvst)
///
/// 纹理坐标约定：(0,0) = 左下，(1,1) = 右上（OpenGL 标准）。
class FullscreenQuad {
public:
    FullscreenQuad()  = default;
    ~FullscreenQuad() { deinit(); }

    FullscreenQuad(const FullscreenQuad&)            = delete;
    FullscreenQuad& operator=(const FullscreenQuad&) = delete;

    /// 在有效 GL 上下文中初始化 VAO / VBO / EBO 资源。
    /// @return true = 成功；false = GL 资源创建失败。
    bool init();

    /// 释放所有 GL 资源。
    void deinit();

    /// 绘制全屏四边形（以 2 个三角形拼成的矩形）。
    /// 调用前须先绑定目标 FBO 和着色器程序。
    void draw() const;

    bool isInitialized() const { return m_vbo != 0; }

private:
    GLuint m_vao = 0; ///< Vertex Array Object（GL3/GLES3）
    GLuint m_vbo = 0; ///< Vertex Buffer Object
    GLuint m_ebo = 0; ///< Element Buffer Object（索引缓冲）
};

} // namespace beiklive
