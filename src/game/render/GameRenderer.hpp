#pragma once

#include <glad/glad.h>
#include <array>
#include <string>
#include <vector>
#include <cstdint>

#include "game/render/GameTexture.hpp"
#include "game/render/FrameUploader.hpp"
#include "game/render/RenderChain.hpp"
#include "game/render/GLSLPParser.hpp"
#include "game/retro/LibretroLoader.hpp"

namespace beiklive {

/// 游戏帧渲染器
///
/// 负责将 libretro 核心输出的 CPU 帧数据上传到 GL 纹理，
/// 并通过渲染链（可选着色器管线）将纹理直接以 OpenGL 绘制到屏幕上的指定矩形区域。
///
/// 典型用法（须在 GL 上下文就绪后调用）：
/// @code
///   GameRenderer renderer;
///   renderer.init(gameWidth, gameHeight, false, shaderPath);
///   // 每帧：
///   renderer.uploadFrame(videoFrame);
///   renderer.drawToScreen(x, y, w, h, windowScale, winW, winH);
/// @endcode
class GameRenderer {
public:
    GameRenderer()  = default;
    ~GameRenderer() { deinit(); }

    GameRenderer(const GameRenderer&)            = delete;
    GameRenderer& operator=(const GameRenderer&) = delete;

    /// 在有效的 GL 上下文中初始化渲染器。
    ///
    /// @param width      游戏帧宽度（像素）。
    /// @param height     游戏帧高度（像素）。
    /// @param linear     是否使用线性过滤（false = 最近邻采样）。
    /// @param shaderPath 可选的 .glslp 或 .glsl 着色器预设路径；空字符串 = 直通模式。
    /// @return true = 初始化成功。
    bool init(unsigned width, unsigned height, bool linear = false,
              const std::string& shaderPath = "");

    /// 释放所有 GL 资源。
    void deinit();

    bool isReady() const;

    /// 将 libretro VideoFrame 数据上传至 GL 纹理。
    ///
    /// 须在 GL 上下文就绪的线程（UI 线程/draw 函数）中调用。
    /// @param frame  LibretroLoader 产出的视频帧。
    void uploadFrame(const LibretroLoader::VideoFrame& frame);

    /// 在 GPU 内部把外部纹理拷贝到当前游戏帧纹理。
    ///
    /// 源纹理必须与当前 GL 上下文共享资源。用于避免 GPU->CPU->GPU 回读路径。
    bool copyFromTexture(GLuint srcTex, unsigned srcW, unsigned srcH,
                         unsigned dstW, unsigned dstH);

    bool copyTextureRegion(GLuint srcTex,
                           unsigned srcX, unsigned srcY,
                           unsigned dstX, unsigned dstY,
                           unsigned copyW, unsigned copyH,
                           unsigned dstW, unsigned dstH);

    bool beginTextureCopyFrame(unsigned dstW, unsigned dstH);
    bool copyTextureRegionToCopyFrame(GLuint srcTex,
                                      unsigned srcX, unsigned srcY,
                                      unsigned dstX, unsigned dstY,
                                      unsigned copyW, unsigned copyH);
    void commitTextureCopyFrame();
    void abortTextureCopyFrame();

    /// 设置纹理过滤模式。
    /// @param linear  true = 线性过滤；false = 最近邻采样。
    void setFilter(bool linear);

    /// 加载或切换着色器预设（须在有效 GL 上下文中调用）。
    /// @param shaderPath  .glslp 或 .glsl 文件路径；空字符串 = 卸载并切换为直通模式。
    void setShader(const std::string& shaderPath);

    /// 返回当前是否已加载着色器管线。
    bool hasShader() const { return m_renderChain.hasShader(); }

    /// 返回当前着色器管线的参数列表（含完整元数据和当前值）。
    const std::vector<ShaderParamInfo>& getShaderParams() const { return m_renderChain.getShaderParams(); }

    /// 更新指定名称的着色器参数当前值。
    void setShaderParam(const std::string& name, float value) { m_renderChain.setShaderParam(name, value); }

    /// 将游戏帧通过渲染链（可选着色器）直接以 OpenGL 绘制到屏幕指定矩形（不经过 NanoVG）。
    ///
    /// 坐标系说明：@a virtX/@a virtY/@a virtW/@a virtH 使用 NanoVG 虚拟坐标（原点在左上角，
    /// Y 轴向下，单位为"虚拟像素" = 物理像素 / @a windowScale）。
    ///
    /// @param virtX       显示矩形左边（虚拟坐标）
    /// @param virtY       显示矩形上边（虚拟坐标）
    /// @param virtW       显示矩形宽度（虚拟坐标）
    /// @param virtH       显示矩形高度（虚拟坐标）
    /// @param windowScale NanoVG 窗口缩放比例
    /// @param windowW     窗口物理宽度（像素）
    /// @param windowH     窗口物理高度（像素）
    void drawToScreen(float virtX, float virtY, float virtW, float virtH,
                      float windowScale, int windowW, int windowH);

    void drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                             float virtX, float virtY, float virtW, float virtH,
                             float windowScale, int windowW, int windowH);

    void drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                             float virtX, float virtY, float virtW, float virtH,
                             float windowScale, int windowW, int windowH,
                             float u0, float v0, float u1, float v1,
                             bool swizzleRB = false);

    /// 返回当前帧纹理的宽度。
    unsigned texWidth() const;
    /// 返回当前帧纹理的高度。
    unsigned texHeight() const;

    /// 返回游戏帧纹理 ID（可传给 NanoVG 等）。
    GLuint texId() const;

private:
    const GameTexture& displayTexture() const;
    bool copyTextureRegionToTexture(GLuint dstTex,
                                    GLuint srcTex,
                                    unsigned srcX, unsigned srcY,
                                    unsigned dstX, unsigned dstY,
                                    unsigned copyW, unsigned copyH);
    bool copyTextureRegionByDraw(GLuint dstTex,
                                 GLuint srcTex,
                                 unsigned srcX, unsigned srcY,
                                 unsigned dstX, unsigned dstY,
                                 unsigned copyW, unsigned copyH);
    bool ensureCopyDrawResources();

    GameTexture  m_texture;     ///< 游戏帧 GL 纹理
    std::array<GameTexture, 2> m_copyTextures; ///< GPU copy 双缓冲目标纹理
    RenderChain  m_renderChain; ///< 渲染链（含着色器管线和直接渲染器）
    bool         m_linear = false; ///< 当前纹理过滤模式
    GLuint       m_copyReadFbo = 0;
    GLuint       m_copyDrawFbo = 0;
    GLuint       m_copyDrawProgram = 0;
    GLuint       m_copyDrawVao = 0;
    GLuint       m_copyDrawVbo = 0;
    unsigned     m_copyWriteIndex = 0;
    unsigned     m_copyDisplayIndex = 0;
    bool         m_copyFrameActive = false;
    bool         m_usingCopiedTexture = false;
};

} // namespace beiklive
