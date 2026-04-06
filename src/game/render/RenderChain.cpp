#include "game/render/RenderChain.hpp"

#include <borealis.hpp>

namespace beiklive {

// ============================================================
// init
// ============================================================
bool RenderChain::init(const std::string& shaderPath)
{
    // 初始化直接渲染器（直通 OpenGL 路径）
    if (!m_directRenderer.init()) {
        brls::Logger::warning("RenderChain: DirectQuadRenderer 初始化失败，drawToScreen 调用将被跳过");
    }

    if (!shaderPath.empty()) {
        if (!m_pipeline.init(shaderPath)) {
            brls::Logger::warning("RenderChain: 着色器管线加载失败: {}, 切换为直通模式",
                                  shaderPath);
            // 仍然返回 true：直通模式下仍然可用
        } else {
            brls::Logger::info("RenderChain: 着色器管线加载成功: {}", shaderPath);
        }
    }
    return true;
}

// ============================================================
// deinit
// ============================================================
void RenderChain::deinit()
{
    m_pipeline.deinit();
    m_directRenderer.deinit();
    m_frameCount = 0;
    m_lastW = m_lastH = 0;
}

// ============================================================
// run
// ============================================================
GLuint RenderChain::run(GLuint srcTex, unsigned videoW, unsigned videoH,
                        unsigned viewW, unsigned viewH)
{
    if (!m_pipeline.isLoaded()) {
        // 直通模式
        m_lastW = videoW;
        m_lastH = videoH;
        return srcTex;
    }

    GLuint out = m_pipeline.process(srcTex, videoW, videoH,
                                    viewW, viewH, m_frameCount);
    ++m_frameCount;

    m_lastW = m_pipeline.outputW() > 0 ? m_pipeline.outputW() : videoW;
    m_lastH = m_pipeline.outputH() > 0 ? m_pipeline.outputH() : videoH;

    return out;
}

// ============================================================
// setShader
// ============================================================
void RenderChain::setShader(const std::string& glslpPath)
{
    m_pipeline.deinit();
    m_frameCount = 0;
    m_lastW = m_lastH = 0;

    if (!glslpPath.empty()) {
        if (m_pipeline.init(glslpPath)) {
            brls::Logger::info("RenderChain: 着色器切换成功: {}", glslpPath);
        } else {
            brls::Logger::warning("RenderChain: 着色器切换失败: {}", glslpPath);
        }
    } else {
        brls::Logger::info("RenderChain: 着色器已卸载，切换为直通模式");
    }
}

// ============================================================
// drawToScreen
// ============================================================
void RenderChain::drawToScreen(GLuint tex,
                                float virtX, float virtY,
                                float virtW, float virtH,
                                float windowScale, int windowW, int windowH)
{
    if (!tex || virtW <= 0.0f || virtH <= 0.0f) return;
    if (!m_directRenderer.isInitialized()) return;
    if (windowW <= 0 || windowH <= 0) return;

    // 将 NanoVG 虚拟坐标转换为物理像素坐标
    float physicalX      = virtX * windowScale;
    float physicalY      = virtY * windowScale;
    float physicalWidth  = virtW * windowScale;
    float physicalHeight = virtH * windowScale;

    // 将物理像素坐标转换为 OpenGL NDC（归一化设备坐标）
    float ndcLeft   = (physicalX                      / static_cast<float>(windowW)) * 2.0f - 1.0f;
    float ndcRight  = ((physicalX + physicalWidth)     / static_cast<float>(windowW)) * 2.0f - 1.0f;
    float ndcTop    = 1.0f - (physicalY                / static_cast<float>(windowH)) * 2.0f;
    float ndcBottom = 1.0f - ((physicalY + physicalHeight) / static_cast<float>(windowH)) * 2.0f;

    m_directRenderer.render(tex, ndcLeft, ndcRight, ndcTop, ndcBottom);
}

} // namespace beiklive
