#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

namespace beiklive {

/// 每个通道的 FBO 缩放设置（ported from RetroArch video_shader_parse.h）
struct ShaderPassScale {
    enum Type { SOURCE = 0, SCALE_ABSOLUTE, VIEWPORT };
    Type  typeX    = SOURCE;
    Type  typeY    = SOURCE;
    float scaleX   = 1.0f;   ///< 当 type==SOURCE 或 VIEWPORT 时的乘数
    float scaleY   = 1.0f;
    int   absX     = 0;      ///< 当 type==SCALE_ABSOLUTE 时的固定像素尺寸
    int   absY     = 0;
};

/// LUT（查找）纹理描述符
struct ShaderLut {
    std::string id;          ///< uniform 名称（preset 中的 texture ID）
    GLuint      tex     = 0;
    GLint       uniform = -1;///< 在当前通道中的 uniform 位置
};

/// 后处理着色器链中的单个渲染通道
struct ShaderPass {
    GLuint program   = 0;
    GLuint fbo       = 0;       ///< 该通道的离屏 FBO
    GLuint outputTex = 0;       ///< FBO 的颜色附件纹理

    int    outW      = 0;       ///< outputTex / FBO 的宽高
    int    outH      = 0;

    // 内置 uniforms（可选，未找到时为 -1）
    GLint  texLoc    = -1;      ///< uniform sampler2D  tex
    GLint  dimsLoc   = -1;      ///< uniform vec2       dims
    GLint  insizeLoc = -1;      ///< uniform vec2       insize
    GLint  colorLoc  = -1;      ///< uniform vec4       color
    GLint  offsetLoc = -1;      ///< attribute          offset (vec2)

    // RetroArch 兼容 uniforms（可选，未找到时为 -1）
    GLint  sourceLoc      = -1; ///< uniform sampler2D  Source / Texture
    GLint  sourceSizeLoc  = -1; ///< uniform vec4       SourceSize / TextureSize
    GLint  outputSizeLoc  = -1; ///< uniform vec4       OutputSize
    GLint  frameCountLoc  = -1; ///< uniform int        FrameCount
    GLint  inputSizeLoc   = -1; ///< uniform vec4       InputSize
    GLint  finalVpSizeLoc = -1; ///< uniform vec4       FinalViewportSize

    // LUT uniform 位置（与 ShaderChain::m_luts 对应）
    std::vector<GLint> lutLocs;

    // 渲染配置
    GLenum         glFilter  = GL_NEAREST;
    GLenum         wrapMode  = GL_CLAMP_TO_EDGE;
    ShaderPassScale scale;
    int            fcMod     = 0;   ///< FrameCount 模数（0=不取模）
    std::string    alias;           ///< 通道别名（用于后续通道引用）
};

/// 管理有序的 OpenGL 着色器通道链。
///
/// 第 0 通道为内置通道：将源纹理的原始像素数据
/// 上传到经过颜色校正的 FBO 中。
/// 第 1..N 通道为可选的用户自定义后处理通道，按顺序串联执行。
///
/// 本类与 mGBA 及 NanoVG 完全解耦，独立拥有所有 GL 资源。
class ShaderChain {
public:
    ShaderChain()  = default;
    ~ShaderChain() = default;

    ShaderChain(const ShaderChain&)            = delete;
    ShaderChain& operator=(const ShaderChain&) = delete;

    /// 分配共享四边形几何体，并编译内置第 0 通道。
    /// 必须在有效的 GL 上下文中调用一次。
    bool initBuiltin();

    /// 使用预先拼接好的 GLSL 源码编译第 0 通道。
    bool init(const std::string& vertSrc, const std::string& fragSrc);

    /// 释放所有 GL 资源。
    void deinit();

    /// 追加一个用户自定义后处理通道（索引 >= 1）。
    /// @param vert      完整顶点着色器源码
    /// @param frag      完整片段着色器源码
    /// @param filter    GL_NEAREST / GL_LINEAR
    /// @param wrapMode  GL_CLAMP_TO_EDGE / GL_REPEAT / GL_MIRRORED_REPEAT
    /// @param scale     FBO 缩放设置
    /// @param fcMod     FrameCount 模数（0 = 不取模）
    /// @param alias     通道别名
    bool addPass(const std::string& vert, const std::string& frag,
                 GLenum filter   = GL_NEAREST,
                 GLenum wrapMode = GL_CLAMP_TO_EDGE,
                 const ShaderPassScale& scale = ShaderPassScale{},
                 int fcMod = 0,
                 const std::string& alias = {});

    /// 移除所有用户通道（索引 >= 1），保留第 0 通道。
    void clearPasses();

    /// 加载一个 LUT 纹理并存入 m_luts。
    /// @param id       uniform 名称（.glslp textures= 中的 ID）
    /// @param path     图像文件绝对路径
    /// @param linear   是否使用线性过滤
    /// @param mipmap   是否生成 mipmap
    /// @param wrap     GL_CLAMP_TO_EDGE / GL_REPEAT / ...
    /// @return 成功返回 true
    bool addLut(const std::string& id,
                const std::string& path,
                bool linear = false,
                bool mipmap = false,
                GLenum wrap = GL_CLAMP_TO_EDGE);

    /// 对 @a srcTex 执行完整的着色器链。
    GLuint run(GLuint srcTex, unsigned videoW, unsigned videoH);

    /// 当前最终输出的 GL 纹理 ID。
    GLuint outputTex() const;

    unsigned outputW() const { return m_lastVideoW; }
    unsigned outputH() const { return m_lastVideoH; }

    /// 设置所有 FBO 输出纹理的过滤模式。
    void setFilter(GLenum glFilter);

    /// 编译并链接 GLSL 程序。
    GLuint buildProgram(const char* vertSrc, const char* fragSrc);

private:
    std::vector<ShaderPass> m_passes;
    std::vector<ShaderLut>  m_luts;          ///< LUT 纹理列表

    GLuint   m_vao        = 0;
    GLuint   m_vbo        = 0;
    unsigned m_lastVideoW = 0;
    unsigned m_lastVideoH = 0;
    GLenum   m_glFilter   = GL_NEAREST;
    uint32_t m_frameCount = 0;

    bool _initPassFbo(ShaderPass& p, int srcW, int srcH,
                      unsigned viewW, unsigned viewH);
    void _bindPassAttrib(ShaderPass& p);
    void _lookupUniforms(ShaderPass& p);
    void _lookupLutUniforms(ShaderPass& p);
};

} // namespace beiklive
