#pragma once

#include <glad/glad.h>
#include <vector>
#include <cstdint>

#include "game/render/GameTexture.hpp"
#include "game/render/FrameUploader.hpp"
#include "game/render/DirectQuadRenderer.hpp"
#include "game/retro/LibretroLoader.hpp"

namespace beiklive {

/// 游戏帧渲染器
///
/// 负责将 libretro 核心输出的 CPU 帧数据上传到 GL 纹理，
/// 并将纹理直接以 OpenGL 绘制到屏幕上的指定矩形区域。
///
/// 典型用法（须在 GL 上下文就绪后调用）：
/// @code
///   GameRenderer renderer;
///   renderer.init(gameWidth, gameHeight);
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
    /// @param width   游戏帧宽度（像素）。
    /// @param height  游戏帧高度（像素）。
    /// @param linear  是否使用线性过滤（false = 最近邻采样）。
    /// @return true = 初始化成功。
    bool init(unsigned width, unsigned height, bool linear = false);

    /// 释放所有 GL 资源。
    void deinit();

    bool isReady() const { return m_texture.isValid() && m_quadRenderer.isInitialized(); }

    /// 将 libretro VideoFrame 数据上传至 GL 纹理。
    ///
    /// 须在 GL 上下文就绪的线程（UI 线程/draw 函数）中调用。
    /// @param frame  LibretroLoader 产出的视频帧。
    void uploadFrame(const LibretroLoader::VideoFrame& frame);

    /// 设置纹理过滤模式。
    /// @param linear  true = 线性过滤；false = 最近邻采样。
    void setFilter(bool linear);

    /// 将游戏帧纹理直接以 OpenGL 绘制到屏幕指定矩形（不经过 NanoVG）。
    ///
    /// 坐标系说明：@a virtX/@a virtY/@a virtW/@a virtH 使用 NanoVG 虚拟坐标（原点在左上角，
    /// Y 轴向下，单位为"虚拟像素" = 物理像素 / @a windowScale）。
    /// 方法内部将其转换为 OpenGL NDC 坐标后调用 DirectQuadRenderer 完成绘制。
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

    /// 返回当前帧纹理的宽度。
    unsigned texWidth()  const { return m_texture.width();  }
    /// 返回当前帧纹理的高度。
    unsigned texHeight() const { return m_texture.height(); }

    /// 返回游戏帧纹理 ID（可传给 NanoVG 等）。
    GLuint texId() const { return m_texture.texId(); }

private:
    GameTexture        m_texture;       ///< 游戏帧 GL 纹理
    DirectQuadRenderer m_quadRenderer;  ///< 直接 GL 四边形渲染器
};

} // namespace beiklive
