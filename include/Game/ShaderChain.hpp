#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

namespace beiklive {

/// 后处理着色器链中的单个渲染通道
struct ShaderPass {
    GLuint program   = 0;
    GLuint fbo       = 0;       ///< 该通道的离屏 FBO
    GLuint outputTex = 0;       ///< FBO 的颜色附件纹理

    int    outW      = 0;       ///< outputTex / FBO 的宽高
    int    outH      = 0;

    GLint  texLoc    = -1;      ///< uniform sampler2D  tex
    GLint  dimsLoc   = -1;      ///< uniform vec2       dims
    GLint  insizeLoc = -1;      ///< uniform vec2       insize
    GLint  colorLoc  = -1;      ///< uniform vec4       color
    GLint  offsetLoc = -1;      ///< attribute          offset (vec2)
};

/// 管理有序的 OpenGL 着色器通道链。
///
/// 第 0 通道为内置通道：将源纹理的原始像素数据
/// 上传到经过颜色校正的 FBO 中。
/// 第 1..N 通道为可选的用户自定义后处理通道，按顺序串联执行。
///
/// 本类与 mGBA 及 NanoVG 完全解耦，独立拥有所有 GL 资源：
/// 共享的全屏四边形 VAO/VBO、着色器程序，以及每个通道的
/// FBO 和颜色附件纹理。
class ShaderChain {
public:
    ShaderChain()  = default;
    ~ShaderChain() = default;

    ShaderChain(const ShaderChain&)            = delete;
    ShaderChain& operator=(const ShaderChain&) = delete;

    /// 分配共享四边形几何体，并编译内置第 0 通道
    /// （在编译期根据宏选择对应版本的 GLSL）。
    /// 必须在有效的 GL 上下文中调用一次。
    /// @return 成功返回 true。
    bool initBuiltin();

    /// 分配共享四边形几何体，并使用预先拼接好的 GLSL 源码
    /// （头部前缀 + 着色器体已合并）编译第 0 通道。
    /// 必须在有效的 GL 上下文中调用一次。
    /// @return 成功返回 true。
    bool init(const std::string& vertSrc, const std::string& fragSrc);

    /// 释放所有 GL 资源。即使未调用 init() 也可安全调用。
    void deinit();

    /// 追加一个用户自定义后处理通道（索引 >= 1）。
    /// @param vert  完整顶点着色器源码（含版本前缀）。
    /// @param frag  完整片段着色器源码。
    /// @return 编译链接成功返回 true。
    bool addPass(const std::string& vert, const std::string& frag);

    /// 移除所有用户通道（索引 >= 1），保留第 0 通道。
    void clearPasses();

    /// 对 @a srcTex 执行完整的着色器链。
    /// 首次使用或 videoW/videoH 发生变化时自动（重新）创建 FBO。
    /// 执行前后会完整保存并恢复所有修改过的 GL 状态，
    /// 确保外部的 NVG/Borealis 上下文不受影响。
    /// @return 最终通道的输出 GL 纹理 ID；失败时返回 @a srcTex。
    GLuint run(GLuint srcTex, unsigned videoW, unsigned videoH);

    /// 当前最终输出的 GL 纹理 ID（与最近一次 run() 的返回值相同）。
    GLuint outputTex() const;

    /// 最近一次渲染输出的尺寸。
    unsigned outputW() const { return m_lastVideoW; }
    unsigned outputH() const { return m_lastVideoH; }

    /// 设置所有 FBO 输出纹理的过滤模式（GL_NEAREST 或 GL_LINEAR）。
    /// 会立即更新已存在的纹理参数，并对下次创建的 FBO 生效。
    void setFilter(GLenum glFilter);
    /// 将 "offset" 顶点属性固定到位置 0，
    /// 使所有通道可共享同一 VAO 绑定。
    /// @return 程序句柄；失败返回 0。
    GLuint buildProgram(const char* vertSrc, const char* fragSrc);

private:
    std::vector<ShaderPass> m_passes;
    GLuint   m_vao        = 0;
    GLuint   m_vbo        = 0;
    unsigned m_lastVideoW = 0;
    unsigned m_lastVideoH = 0;
    GLenum   m_glFilter   = GL_NEAREST;  ///< FBO 输出纹理的过滤模式

    bool _initPassFbo(ShaderPass& p, int w, int h);
    void _bindPassAttrib(ShaderPass& p);
};

} // namespace beiklive
