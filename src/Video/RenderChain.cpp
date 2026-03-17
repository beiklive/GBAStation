#include "Video/RenderChain.hpp"

#include <borealis.hpp>

namespace beiklive {

// ============================================================
// init
// ============================================================
bool RenderChain::init(const std::string& shaderPath)
{
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

    // brls::Logger::debug("RenderChain::run: 着色器管线处理 srcTex={} {}×{}", srcTex, videoW, videoH);

    GLuint out = m_pipeline.process(srcTex, videoW, videoH,
                                    viewW, viewH, m_frameCount);
    ++m_frameCount;

    m_lastW = m_pipeline.outputW() > 0 ? m_pipeline.outputW() : videoW;
    m_lastH = m_pipeline.outputH() > 0 ? m_pipeline.outputH() : videoH;

    // brls::Logger::debug("RenderChain::run: 输出 tex={} {}×{}", out, m_lastW, m_lastH);

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

} // namespace beiklive

