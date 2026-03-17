#pragma once

#include <glad/glad.h>
#include <string>

#include "Video/renderer/RetroShaderPipeline.hpp"

namespace beiklive {

/// 视频渲染链
///
/// 将游戏原始帧纹理经过可选的 RetroArch 多通道着色器管线处理后返回最终纹理。
///
/// 着色器管线通过 init(shaderPath) 或 setShader(path) 加载；
/// 若未加载着色器则直通（pass-through）返回原始纹理。
class RenderChain {
public:
    RenderChain()  = default;
    ~RenderChain() { deinit(); }

    RenderChain(const RenderChain&)            = delete;
    RenderChain& operator=(const RenderChain&) = delete;

    /// 在有效的 GL 上下文中初始化渲染链资源。
    ///
    /// @param shaderPath  .glslp 着色器预设文件路径；
    ///                    空字符串 = 不加载着色器（直通模式）。
    /// @return true = 初始化成功（若 shaderPath 为空或着色器加载失败，
    ///         仍返回 true 并以直通模式运行）。
    bool init(const std::string& shaderPath = "");

    /// 释放渲染链占用的所有 GL 资源（包括着色器管线）。
    void deinit();

    /// 执行渲染链：对输入纹理 @a srcTex 进行处理并返回输出纹理 ID。
    ///
    /// 若着色器管线已加载，将 @a srcTex 经过多通道着色器处理后返回结果纹理；
    /// 否则直接返回 @a srcTex（直通模式）。
    ///
    /// @param srcTex  源纹理（游戏画面 GL 纹理 ID）
    /// @param videoW  源纹理宽度（像素）
    /// @param videoH  源纹理高度（像素）
    /// @param viewW   显示视口宽度（像素），供 viewport 缩放类型计算；0 = 使用 videoW
    /// @param viewH   显示视口高度（像素）；0 = 使用 videoH
    /// @return        经过渲染链处理后的输出纹理 ID
    GLuint run(GLuint srcTex, unsigned videoW, unsigned videoH,
               unsigned viewW = 0, unsigned viewH = 0);

    /// 返回最终输出纹理的宽度。
    /// 直通模式下等于最后一次 run() 的 videoW；
    /// 着色器模式下为着色器管线的最终输出宽度。
    unsigned outputW() const { return m_lastW; }

    /// 返回最终输出纹理的高度。
    unsigned outputH() const { return m_lastH; }

    /// 加载新的着色器预设，替换当前管线（若有）。
    /// 须在有效的 GL 上下文中调用。
    ///
    /// @param glslpPath  .glslp 文件路径；空字符串 = 卸载当前着色器并切换为直通模式。
    void setShader(const std::string& glslpPath);

    /// 返回当前是否已加载着色器管线。
    bool hasShader() const { return m_pipeline.isLoaded(); }

    /// 返回当前着色器管线的参数列表（含完整元数据和当前值）。
    /// 若无着色器管线，返回空列表。
    const std::vector<ShaderParamInfo>& getShaderParams() const { return m_pipeline.getParams(); }

    /// 更新指定名称的着色器参数当前值。
    /// 若无着色器或参数不存在，则忽略。
    void setShaderParam(const std::string& name, float value) { m_pipeline.setParamValue(name, value); }

private:
    RetroShaderPipeline m_pipeline;
    unsigned            m_frameCount = 0; ///< 累计帧计数，传入着色器 FrameCount uniform
    unsigned            m_lastW      = 0;
    unsigned            m_lastH      = 0;
};

} // namespace beiklive
