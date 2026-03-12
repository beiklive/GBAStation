#pragma once

#include <glad/glad.h>
#include <string>

namespace beiklive {

/// 视频渲染链接口（占位符）
///
/// 当前实现为直通（pass-through）：run() 原样返回输入纹理。
/// 渲染链的具体实现由调用方自行扩展。
///
/// 预留接口供未来实现后处理通道（着色器、滤镜等）。
class RenderChain {
public:
    RenderChain()  = default;
    ~RenderChain() = default;

    RenderChain(const RenderChain&)            = delete;
    RenderChain& operator=(const RenderChain&) = delete;

    /// 在有效的 GL 上下文中初始化渲染链资源。
    /// 当前实现为空操作，返回 true。
    bool init() { return true; }

    /// 释放渲染链占用的 GL 资源。
    /// 当前实现为空操作。
    void deinit() {}

    /// 执行渲染链：对输入纹理 @a srcTex 进行处理并返回输出纹理 ID。
    ///
    /// 当前实现为直通（pass-through），直接返回 @a srcTex，
    /// 不做任何额外处理。调用方可替换此函数实现自定义渲染逻辑。
    ///
    /// @param srcTex  源纹理（游戏画面 GL 纹理 ID）
    /// @param videoW  源纹理宽度（像素）；当前实现未使用，保留供未来扩展
    /// @param videoH  源纹理高度（像素）；当前实现未使用，保留供未来扩展
    /// @return        经过渲染链处理后的输出纹理 ID
    GLuint run(GLuint srcTex, unsigned videoW, unsigned videoH) {
        setLastSize(videoW, videoH);
        return srcTex;
    }

    /// 返回最终输出纹理的宽度。直通模式下等于最后一次 run() 的 videoW。
    unsigned outputW() const { return m_lastW; }

    /// 返回最终输出纹理的高度。直通模式下等于最后一次 run() 的 videoH。
    unsigned outputH() const { return m_lastH; }

    /// 通知渲染链当前帧的视频尺寸（供 outputW/outputH 返回）。
    void setLastSize(unsigned w, unsigned h) { m_lastW = w; m_lastH = h; }

private:
    unsigned m_lastW = 0;
    unsigned m_lastH = 0;
};

} // namespace beiklive
