#pragma once

#include <glad/glad.h>

namespace beiklive {

/// 直接 GL 纹理四边形渲染器
///
/// 在不依赖 NanoVG 的情况下，将 GL 2D 纹理直接渲染到屏幕（或当前帧缓冲）上的指定矩形区域。
///
/// 适用于在 NanoVG 帧内（nvgBeginFrame/nvgEndFrame 之间）直接绘制游戏帧，
/// 以避免 NanoVG 批量渲染的额外开销。调用 render() 时会自动保存并恢复所有修改的
/// GL 状态，确保 NanoVG 后续绘制（UI 叠加层等）不受影响。
class DirectQuadRenderer {
public:
    DirectQuadRenderer()  = default;
    ~DirectQuadRenderer() { deinit(); }

    DirectQuadRenderer(const DirectQuadRenderer&)            = delete;
    DirectQuadRenderer& operator=(const DirectQuadRenderer&) = delete;

    /// 在有效的 GL 上下文中编译直通着色器并创建 VAO/VBO/EBO 资源。
    /// @return true = 成功；false = GL 资源创建失败。
    bool init();

    /// 释放所有 GL 资源。
    void deinit();

    bool isInitialized() const { return m_prog != 0; }

    /// 将纹理 @a tex 直接渲染到由 NDC 坐标指定的矩形区域。
    ///
    /// 渲染前后自动保存/恢复关键 GL 状态（混合、深度测试、模板测试、着色器程序等），
    /// 使该函数在 NanoVG 帧内可安全重复调用。
    ///
    /// @param tex       GL 2D 纹理 ID
    /// @param ndcLeft   矩形左边（NDC，-1 = 屏幕左端，+1 = 屏幕右端）
    /// @param ndcRight  矩形右边
    /// @param ndcTop    矩形上边（NDC，+1 = 屏幕顶端）
    /// @param ndcBottom 矩形下边
    void render(GLuint tex,
                float ndcLeft, float ndcRight,
                float ndcTop,  float ndcBottom) const;

private:
    GLuint m_prog = 0;  ///< GL 着色器程序（直通顶点 + 片段）
    GLuint m_vao  = 0;  ///< Vertex Array Object（GL3/GLES3）
    GLuint m_vbo  = 0;  ///< Vertex Buffer Object（动态，按帧更新顶点坐标）
    GLuint m_ebo  = 0;  ///< Element Buffer Object（静态索引）
    GLint  m_uTex = -1; ///< sampler2D uniform 位置
};

} // namespace beiklive
