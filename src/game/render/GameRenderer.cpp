#include "game/render/GameRenderer.hpp"

#include <borealis.hpp>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <array>

namespace beiklive {

static std::array<float, 8> scaleUv(const std::array<float, 8>& uv, float uMax, float vMax)
{
    return {
        uv[0] * uMax, uv[1] * vMax,
        uv[2] * uMax, uv[3] * vMax,
        uv[4] * uMax, uv[5] * vMax,
        uv[6] * uMax, uv[7] * vMax,
    };
}

// ============================================================
// init
// ============================================================
bool GameRenderer::init(unsigned width, unsigned height, bool linear,
                         const std::string& shaderPath)
{
    m_linear = linear;

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
        m_texture.init(frame.width, frame.height, m_linear);
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
    m_linear = linear;
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
    // 使用 llround 四舍五入，避免 static_cast<unsigned> 的截断导致 1 像素精度丢失
    const auto viewW = static_cast<unsigned>(std::llround(static_cast<double>(virtW) * static_cast<double>(windowScale)));
    const auto viewH = static_cast<unsigned>(std::llround(static_cast<double>(virtH) * static_cast<double>(windowScale)));

    // 通过渲染链处理游戏帧纹理（着色器模式或直通模式）
    GLuint finalTex = m_renderChain.run(m_texture.texId(),
                                        m_texture.width(), m_texture.height(),
                                        viewW, viewH);

    // 将最终纹理绘制到屏幕指定矩形
    m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                               windowScale, windowW, windowH);
}

void GameRenderer::drawToScreen(float virtX, float virtY, float virtW, float virtH,
                                 float windowScale, int windowW, int windowH,
                                 const std::array<float, 8>& uv)
{
    if (!isReady()) return;

    const auto viewW = static_cast<unsigned>(std::llround(static_cast<double>(virtW) * static_cast<double>(windowScale)));
    const auto viewH = static_cast<unsigned>(std::llround(static_cast<double>(virtH) * static_cast<double>(windowScale)));
    GLuint finalTex = m_renderChain.run(m_texture.texId(),
                                        m_texture.width(), m_texture.height(),
                                        viewW, viewH);
    const auto finalUv = scaleUv(uv, m_renderChain.outputU(), m_renderChain.outputV());
    m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                               windowScale, windowW, windowH, finalUv);
}

void GameRenderer::drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                                       float virtX, float virtY, float virtW, float virtH,
                                       float windowScale, int windowW, int windowH)
{
    drawExternalTexture(tex, texW, texH, virtX, virtY, virtW, virtH,
                        windowScale, windowW, windowH, 0.0f, 0.0f, 1.0f, 1.0f);
}

void GameRenderer::drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                                       float virtX, float virtY, float virtW, float virtH,
                                       float windowScale, int windowW, int windowH,
                                       float u0, float v0, float u1, float v1,
                                       bool swizzleRB)
{
    drawExternalTexture(tex, texW, texH, virtX, virtY, virtW, virtH,
                        windowScale, windowW, windowH,
                        std::array<float, 8>{u0, v0, u1, v0, u1, v1, u0, v1},
                        swizzleRB);
}

void GameRenderer::drawExternalTexture(GLuint tex, unsigned texW, unsigned texH,
                                       float virtX, float virtY, float virtW, float virtH,
                                       float windowScale, int windowW, int windowH,
                                       const std::array<float, 8>& uv,
                                       bool swizzleRB)
{
    if (!tex || texW == 0 || texH == 0 || !m_renderChain.isDirectRendererReady())
        return;

    GLuint finalTex = m_renderChain.run(tex, texW, texH,
        static_cast<unsigned>(std::llround(static_cast<double>(virtW) * static_cast<double>(windowScale))),
        static_cast<unsigned>(std::llround(static_cast<double>(virtH) * static_cast<double>(windowScale))));

    const auto finalUv = scaleUv(uv, m_renderChain.outputU(), m_renderChain.outputV());
    m_renderChain.drawToScreen(finalTex, virtX, virtY, virtW, virtH,
                               windowScale, windowW, windowH, finalUv,
                               finalTex == tex ? swizzleRB : false);
}

} // namespace beiklive
