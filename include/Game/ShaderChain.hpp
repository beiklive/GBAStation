#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace beiklive {

/// 着色器参数定义（对应 #pragma parameter 行）
/// 用于运行时着色器参数面板的实时调整
struct ShaderParamDef {
    std::string name;        ///< 参数名称（对应 GLSL uniform 名）
    std::string desc;        ///< 用户可读描述字符串
    float       defaultVal;  ///< 着色器文件中声明的默认值
    float       minVal;      ///< 最小允许值
    float       maxVal;      ///< 最大允许值
    float       step;        ///< UI 滑块步进值
    float       currentVal;  ///< 当前运行时值（可通过 setParam 修改）
};

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

    // RetroArch 兼容 uniforms（可选，未找到时为 -1）
    GLint  sourceLoc         = -1; ///< uniform sampler2D  Source / Texture
    GLint  sourceSizeLoc     = -1; ///< uniform vec4/vec2  SourceSize / TextureSize
    bool   sourceSizeIsVec2  = false; ///< true 表示 sourceSizeLoc 指向 vec2（旧式 TextureSize）
    GLint  outputSizeLoc     = -1; ///< uniform vec4/vec2  OutputSize
    bool   outputSizeIsVec2  = false; ///< true 表示 outputSizeLoc 指向 vec2（旧式 OutputSize）
    GLint  frameCountLoc     = -1; ///< uniform int        FrameCount
    GLint  frameDirectionLoc = -1; ///< uniform int        FrameDirection（+1=正向 -1=倒带）
    GLint  inputSizeLoc      = -1; ///< uniform vec4/vec2  InputSize
    bool   inputSizeIsVec2   = false; ///< true 表示 inputSizeLoc 指向 vec2（旧式 InputSize）
    GLint  finalVpSizeLoc    = -1; ///< uniform vec4       FinalViewportSize
    GLint  origTexLoc        = -1; ///< uniform sampler2D  OrigTexture（原始源纹理引用）
    GLint  origInputSizeLoc  = -1; ///< uniform vec2       OrigInputSize（原始视频分辨率）
    GLint  mvpMatrixLoc      = -1; ///< uniform mat4       MVPMatrix（RetroArch 兼容 MVP 矩阵，初始化为单位矩阵）

    // LUT uniform 位置（与 ShaderChain::m_luts 对应）
    std::vector<GLint> lutLocs;

    /// 前序通道纹理绑定（PassNTexture / {alias}Texture / PrevNTexture）
    struct PassTexBinding {
        int   srcPassIdx;    ///< m_passes 中源通道的索引（-1 = 使用原始源纹理）
        GLint loc;           ///< 该 uniform 在本通道程序中的位置
        int   texUnitOffset; ///< 从 (1 + luts.size()) 起的纹理单元偏移量
    };
    std::vector<PassTexBinding> passTexBindings;

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
    /// @param vert         完整顶点着色器源码
    /// @param frag         完整片段着色器源码
    /// @param filter       GL_NEAREST / GL_LINEAR
    /// @param wrapMode     GL_CLAMP_TO_EDGE / GL_REPEAT / GL_MIRRORED_REPEAT
    /// @param scale        FBO 缩放设置
    /// @param fcMod        FrameCount 模数（0 = 不取模）
    /// @param alias        通道别名
    /// @param paramDefaults #pragma parameter 行解析得到的参数默认值映射（参数名 → 默认值）。
    ///                      链接后会将每个参数 uniform 初始化为对应的默认值，
    ///                      避免未设置时默认为 0 导致除零等错误（如全白画面）。
    bool addPass(const std::string& vert, const std::string& frag,
                 GLenum filter   = GL_NEAREST,
                 GLenum wrapMode = GL_CLAMP_TO_EDGE,
                 const ShaderPassScale& scale = ShaderPassScale{},
                 int fcMod = 0,
                 const std::string& alias = {},
                 const std::unordered_map<std::string, float>& paramDefaults = {});

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

    unsigned outputW() const {
        if (!m_passes.empty() && m_passes.back().outW > 0)
            return static_cast<unsigned>(m_passes.back().outW);
        return m_lastVideoW;
    }
    unsigned outputH() const {
        if (!m_passes.empty() && m_passes.back().outH > 0)
            return static_cast<unsigned>(m_passes.back().outH);
        return m_lastVideoH;
    }

    /// 设置所有 FBO 输出纹理的过滤模式。
    void setFilter(GLenum glFilter);

    /// 编译并链接 GLSL 程序。
    GLuint buildProgram(const char* vertSrc, const char* fragSrc);

    // ---- 着色器调试日志开关 ----------------------------------------
    /// 启用/禁用着色器专项调试日志（独立开关，默认关闭）。
    /// 启用后会在编译、链接、FBO 创建及逐帧渲染的关键路径
    /// 输出 debug 级别日志，以辅助排查着色器问题（如全白画面）。
    /// run() 中的逐帧日志已做节流处理（前 3 帧 + 每 300 帧一次），
    /// 不会淹没其他功能的调试输出。
    /// 可通过配置项 render.shaderDebugLog=true 在启动时自动开启。
    static void setDebugLog(bool enable);

    /// 返回着色器调试日志当前是否已启用。
    static bool debugLogEnabled();

    // ---- 着色器参数 API（用于运行时参数面板）----

    /// 返回所有通道合并后的参数定义列表（只读）。
    const std::vector<ShaderParamDef>& params() const { return m_params; }

    /// 批量设置参数定义（由 GlslpLoader 在加载完所有通道后调用）。
    void setParamDefs(const std::vector<ShaderParamDef>& defs) { m_params = defs; }

    /// 设置单个参数的当前值，并立即更新所有包含该 uniform 的通道。
    /// 调用此函数需要有效的 OpenGL 上下文。
    /// @param name  参数名（对应 GLSL uniform 名）
    /// @param val   新值
    /// @return 如果参数定义存在或至少更新了一个通道，返回 true
    bool setParam(const std::string& name, float val);

private:
    std::vector<ShaderPass>     m_passes;
    std::vector<ShaderLut>      m_luts;    ///< LUT 纹理列表
    std::vector<ShaderParamDef> m_params;  ///< 所有通道合并的参数定义（含当前值）

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
    /// 查询前序通道纹理 uniform 位置（PassNTexture / aliasTexture / PrevNTexture）
    void _lookupPassTextures(ShaderPass& p, int passIdx);
};

} // namespace beiklive
