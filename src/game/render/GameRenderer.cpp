#include "game/render/GameRenderer.hpp"

#include <borealis.hpp>
#include <cstring>
#include <algorithm>

namespace beiklive {

// ============================================================
// init
// ============================================================
bool GameRenderer::init(unsigned width, unsigned height, bool linear,
                         const std::string& shaderPath)
{
    // 初始化 GL 纹理
    if (!m_texture.init(width, height, linear)) {
        brls::Logger::error("GameRenderer: 游戏帧纹理初始化失败 ({}x{})", width, height);
        return false;
    }

    // 初始化渲染链（含直接渲染器和可选的着色器管线）
    if (!m_renderChain.init(shaderPath)) {
        brls::Logger::error("GameRenderer: RenderChain 初始化失败");
        m_texture.deinit();
        return false;
    }

    brls::Logger::info("GameRenderer: 初始化完成 ({}x{} linear={} shader={})",
                       width, height, linear,
                       shaderPath.empty() ? "无" : shaderPath);
    return true;
}

// ============================================================
// deinit
// ============================================================
void GameRenderer::deinit()
{
    m_renderChain.deinit();
    m_texture.deinit();
}

// ============================================================
// uploadFrame – 将 libretro VideoFrame 上传至 GL 纹理
// ============================================================
void GameRenderer::uploadFrame(const LibretroLoader::VideoFrame& frame)
{
    if (frame.pixels.empty() || frame.width == 0 || frame.height == 0)
        return;

    // 若尺寸发生变化，重新初始化纹理
    if (frame.width != m_texture.width() || frame.height != m_texture.height()) {
        bool linear = false; // 保持当前过滤模式
        m_texture.init(frame.width, frame.height, linear);
    }

    // 上传 RGBA8888 数据（LibretroLoader 已将帧数据转换为 RGBA8888）
    FrameUploader::upload(m_texture.texId(),
                          frame.width, frame.height,
                          frame.pixels.data(),
                          m_texture.width(), m_texture.height());
}

// ============================================================
// setFilter
// ============================================================
void GameRenderer::setFilter(bool linear)
{
    m_texture.setFilter(linear);
}

// ============================================================
// setShader – 加载或切换着色器预设
// ============================================================
void GameRenderer::setShader(const std::string& shaderPath)
{
    m_renderChain.setShader(shaderPath);
}

// ============================================================
// drawToScreen – 通过渲染链将游戏帧绘制到屏幕指定矩形
// ============================================================
void GameRenderer::drawToScreen(float virtX, float virtY, float virtW, float virtH,
                                 float windowScale, int windowW, int windowH)
{
    if (!isReady()) return;

    // 计算视口物理尺寸（供着色器管线 viewport 缩放类型计算）
    const auto viewW = static_cast<unsigned>(virtW * windowScale);
    const auto viewH = static_cast<unsigned>(virtH * windowScale);

    // 通过渲染链处理游戏帧纹理（着色器模式或直通模式）
    GLuint finalTex = m_renderChain.run(m_texture.texId(),
                                        m_texture.width(), m_texture.height(),
                                        viewW, viewH);

    // 将最终纹理绘制到屏幕指定矩形
    m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                               windowScale, windowW, windowH);
}

} // namespace beiklive
