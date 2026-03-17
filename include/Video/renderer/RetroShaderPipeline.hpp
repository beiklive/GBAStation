#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

#include "Video/renderer/GLSLPParser.hpp"
#include "Video/renderer/FullscreenQuad.hpp"

namespace beiklive {

/// 单个着色器通道的运行时状态
struct ShaderPass {
    GLuint program     = 0; ///< 链接完成的 GL 程序 ID
    GLuint fbo         = 0; ///< 输出帧缓冲对象
    GLuint texture     = 0; ///< 输出颜色纹理
    int    width       = 0; ///< 输出纹理宽度（像素）
    int    height      = 0; ///< 输出纹理高度（像素）
    bool   filterLinear = false; ///< 纹理过滤：true=线性，false=最近邻
    std::string alias;           ///< 通道别名，供后续通道以 <alias>Texture 引用
    ShaderPassDesc desc;         ///< 来自 .glslp 的原始描述（用于尺寸重算）
};

/// .glslp 预设中声明的外部纹理（已加载到 GPU）
struct ExternalTexture {
    std::string name;         ///< uniform sampler2D 变量名
    GLuint      texId = 0;    ///< GL 纹理对象 ID（0 = 未加载）
};

/// RetroArch 多通道着色器管线
///
/// 实现以下渲染链：
///
/// inputTex → Pass0 FBO → Pass1 FBO → ... → PassN FBO → 返回最终纹理
///
/// 每个通道：
///   1. 绑定通道自身 FBO 为渲染目标；
///   2. 使用通道着色器程序；
///   3. 绑定前一通道的输出纹理（第一个通道使用原始游戏纹理）；
///   4. 绘制全屏四边形；
///   5. 将本通道输出纹理传给下一通道。
///
/// 兼容 RetroArch 着色器标准 uniform：
///   Texture, MVPMatrix, FrameCount, FrameDirection,
///   OutputSize, TextureSize, InputSize
///
/// 支持外部纹理（.glslp textures 字段）和历史 Pass 输出引用（PassNTexture）。
class RetroShaderPipeline {
public:
    RetroShaderPipeline()  = default;
    ~RetroShaderPipeline() { deinit(); }

    RetroShaderPipeline(const RetroShaderPipeline&)            = delete;
    RetroShaderPipeline& operator=(const RetroShaderPipeline&) = delete;

    /// 从 .glslp 文件加载并编译着色器管线。
    ///
    /// @param glslpPath  .glslp 着色器预设文件路径。
    /// @return true = 至少一个通道成功编译；false = 全部失败或文件不存在。
    bool init(const std::string& glslpPath);

    /// 释放所有 GL 资源（FBO、纹理、程序、VAO/VBO）。
    void deinit();

    /// 将输入纹理经过全部着色器通道处理，返回最终输出纹理。
    ///
    /// 若管线为空（未加载着色器），直接返回 @a inputTex。
    ///
    /// @param inputTex   输入纹理 ID（游戏原始帧）。
    /// @param videoW     输入纹理宽度（像素）。
    /// @param videoH     输入纹理高度（像素）。
    /// @param viewW      视口宽度（像素），用于 viewport 缩放类型计算。
    /// @param viewH      视口高度（像素）。
    /// @param frameCount 当前帧计数，传入着色器 FrameCount uniform。
    /// @return 最终输出纹理 ID。
    GLuint process(GLuint inputTex,
                   unsigned videoW, unsigned videoH,
                   unsigned viewW,  unsigned viewH,
                   unsigned frameCount = 0);

    bool     isLoaded() const { return !m_passes.empty(); }
    unsigned outputW()  const { return m_lastOutW; }
    unsigned outputH()  const { return m_lastOutH; }

private:
    std::vector<ShaderPass>     m_passes;
    std::vector<ExternalTexture> m_textures;  ///< 从 .glslp textures 字段加载的外部纹理
    FullscreenQuad              m_quad;
    unsigned                    m_lastOutW = 0;
    unsigned                    m_lastOutH = 0;

    /// 为通道分配或调整 FBO + 颜色纹理。
    bool allocateFBO(ShaderPass& pass, int w, int h);

    /// 根据 pass 描述和当前视频/视口尺寸，计算输出 FBO 像素尺寸。
    void computePassSize(const ShaderPassDesc& desc,
                         unsigned videoW, unsigned videoH,
                         unsigned viewW,  unsigned viewH,
                         int& outW, int& outH);

    /// 设置当前通道所需的 uniform 变量（单元0=主输入，单元1+= 外部/历史 pass 纹理）。
    ///
    /// @param program      当前通道着色器程序 ID
    /// @param inW/inH      输入纹理尺寸
    /// @param outW/outH    输出 FBO 尺寸
    /// @param frameCount   帧计数
    /// @param extraTextures 额外纹理绑定列表（name=uniform名称, texId=已绑定的 GL 纹理单元索引）
    void setUniforms(GLuint program,
                     unsigned inW, unsigned inH,
                     unsigned outW, unsigned outH,
                     unsigned frameCount,
                     const std::vector<std::pair<std::string,GLuint>>& extraTexUnits);

    /// 从图像文件加载纹理到 GPU。
    /// @return 创建的 GL 纹理 ID，失败返回 0。
    static GLuint loadTextureFromFile(const std::string& path, bool filterLinear);
};

} // namespace beiklive
