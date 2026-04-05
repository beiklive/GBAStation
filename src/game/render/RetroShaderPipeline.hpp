#pragma once

#include <glad/glad.h>
#include <string>
#include <utility>
#include <vector>

#include "game/render/GLSLPParser.hpp"
#include "game/render/FullscreenQuad.hpp"

namespace beiklive {

/// 单个着色器通道的运行时状态
struct ShaderPass {
    GLuint program      = 0;     ///< 链接完成的 GL 程序 ID
    GLuint fbo          = 0;     ///< 输出帧缓冲对象
    GLuint texture      = 0;     ///< 输出颜色纹理
    int    width        = 0;     ///< 输出纹理宽度（像素）
    int    height       = 0;     ///< 输出纹理高度（像素）
    bool   filterLinear = false; ///< 纹理过滤：true=线性，false=最近邻
    std::string alias;           ///< 通道别名，供后续通道以 <alias>Texture 引用
    ShaderPassDesc desc;         ///< 来自 .glslp 的原始描述（用于尺寸重算）
};

/// .glslp 预设中声明的外部纹理（已加载到 GPU）
struct ExternalTexture {
    std::string name;      ///< uniform sampler2D 变量名
    GLuint      texId = 0; ///< GL 纹理对象 ID（0 = 未加载）
};

/// RetroArch 多通道着色器管线
///
/// 实现以下渲染链：
///   inputTex → Pass0 FBO → Pass1 FBO → ... → PassN FBO → 返回最终纹理
///
/// 兼容 RetroArch 着色器标准 uniform：
///   Texture, MVPMatrix, FrameCount, FrameDirection,
///   OutputSize, TextureSize, InputSize
class RetroShaderPipeline {
public:
    RetroShaderPipeline()  = default;
    ~RetroShaderPipeline() { deinit(); }

    RetroShaderPipeline(const RetroShaderPipeline&)            = delete;
    RetroShaderPipeline& operator=(const RetroShaderPipeline&) = delete;

    /// 从 .glslp 或单个 .glsl 文件加载并编译着色器管线。
    ///
    /// @param glslpPath  .glslp 或 .glsl 文件路径。
    /// @return true = 至少一个通道成功编译。
    bool init(const std::string& glslpPath);

    /// 释放所有 GL 资源（FBO、纹理、程序、VAO/VBO）。
    void deinit();

    /// 将输入纹理经过全部着色器通道处理，返回最终输出纹理。
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

    /// 返回当前管线中所有参数的完整元数据（含当前值）。
    const std::vector<ShaderParamInfo>& getParams() const { return m_params; }

    /// 更新指定名称参数的当前值（区分大小写）。
    void setParamValue(const std::string& name, float value);

private:
    std::vector<ShaderPass>      m_passes;
    std::vector<ExternalTexture> m_textures; ///< 外部纹理
    std::vector<ShaderParamInfo> m_params;   ///< 着色器参数完整元数据
    FullscreenQuad               m_quad;
    unsigned                     m_lastOutW = 0;
    unsigned                     m_lastOutH = 0;

    /// 为通道分配或调整 FBO + 颜色纹理。
    bool allocateFBO(ShaderPass& pass, int w, int h);

    /// 根据 pass 描述和当前视频/视口尺寸，计算输出 FBO 像素尺寸。
    void computePassSize(const ShaderPassDesc& desc,
                         unsigned videoW, unsigned videoH,
                         unsigned viewW,  unsigned viewH,
                         int& outW, int& outH);

    /// 设置当前通道所需的 uniform 变量。
    void setUniforms(GLuint program,
                     unsigned inW, unsigned inH,
                     unsigned outW, unsigned outH,
                     unsigned origW, unsigned origH,
                     unsigned frameCount,
                     const std::vector<std::pair<std::string,GLuint>>& extraTexUnits);

    /// 从图像文件加载纹理到 GPU。
    static GLuint loadTextureFromFile(const std::string& path, bool filterLinear);
};

} // namespace beiklive
