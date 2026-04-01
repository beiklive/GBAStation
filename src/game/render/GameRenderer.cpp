#include "game/render/GameRenderer.hpp"

#include <borealis.hpp>
#include <cstring>
#include <algorithm>

namespace beiklive {

// ============================================================
// init
// ============================================================
bool GameRenderer::init(unsigned width, unsigned height, bool linear)
{
    // 初始化 GL 纹理
    if (!m_texture.init(width, height, linear)) {
        brls::Logger::error("GameRenderer: 游戏帧纹理初始化失败 ({}x{})", width, height);
        return false;
    }

    // 初始化直接渲染器（编译直通着色器）
    if (!m_quadRenderer.init()) {
        brls::Logger::error("GameRenderer: DirectQuadRenderer 初始化失败");
        m_texture.deinit();
        return false;
    }

    brls::Logger::info("GameRenderer: 初始化完成 ({}x{} linear={})", width, height, linear);
    return true;
}

// ============================================================
// deinit
// ============================================================
void GameRenderer::deinit()
{
    m_quadRenderer.deinit();
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
// drawToScreen – 将纹理绘制到屏幕指定矩形
// ============================================================
void GameRenderer::drawToScreen(float virtX, float virtY, float virtW, float virtH,
                                 float windowScale, int windowW, int windowH)
{
    if (!isReady()) return;

    // 将虚拟坐标转换为物理像素坐标
    const float physX = virtX * windowScale;
    const float physY = virtY * windowScale;
    const float physW = virtW * windowScale;
    const float physH = virtH * windowScale;

    // 将物理坐标转换为 NDC（归一化设备坐标）
    // NDC 范围：x ∈ [-1, 1]（左 → 右），y ∈ [-1, 1]（下 → 上）
    const float ndcLeft   =  (physX / windowW) * 2.0f - 1.0f;
    const float ndcRight  =  ((physX + physW) / windowW) * 2.0f - 1.0f;
    const float ndcTop    =  1.0f - (physY / windowH) * 2.0f;
    const float ndcBottom =  1.0f - ((physY + physH) / windowH) * 2.0f;

    m_quadRenderer.render(m_texture.texId(),
                          ndcLeft, ndcRight,
                          ndcTop, ndcBottom);
}

} // namespace beiklive
