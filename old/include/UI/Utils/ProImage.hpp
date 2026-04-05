#pragma once

#include <borealis/views/image.hpp>
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#ifdef BOREALIS_USE_OPENGL
#  include <glad/glad.h>
#endif

namespace beiklive::UI
{

/// ProImage 支持的着色器动画类型
enum class ShaderAnimationType
{
    NONE,           ///< 无动画
    PSP_XMB_RIPPLE, ///< PSP XMB 风格波纹背景（GL 着色器）
};

/**
 * ProImage – 增强版 brls::Image 组件。
 *
 * 功能：
 *  - Kawase 模糊：通过 NanoVG 实现的多遍盒式模糊。
 *  - 着色器动画：通过 OpenGL GLSL 渲染 PSP XMB 波纹效果。
 *  - 异步 PNG 图片加载，支持文件字节缓存。
 */
class ProImage : public brls::Image
{
  public:
    ProImage();
    ~ProImage() override;

    // ── Kawase 模糊 ──────────────────────────────────────────────────────────

    /// 启用/禁用图片上的 Kawase 模糊效果。
    void setBlurEnabled(bool enabled);
    bool isBlurEnabled() const;

    /// 模糊半径（像素），值越大效果越强，默认 8。
    void setBlurRadius(float radius);
    float getBlurRadius() const;

    // ── 异步图片加载 ─────────────────────────────────────────────────────────

    /**
     * 异步加载图片文件（仅 PNG）。
     * - 优先检查 ImageFileCache（主线程字节缓存）。
     * - 未命中时在后台线程读取文件，存入缓存，再在主线程创建 NVG 纹理。
     * - 加载期间 draw() 显示"加载中..."占位文字。
     * - 在上一次加载完成前再次调用会取消上次加载（通过生成计数器丢弃结果）。
     */
    void setImageFromFileAsync(const std::string& path);

    // ── 着色器动画 ───────────────────────────────────────────────────────────

    void setShaderAnimation(ShaderAnimationType type);
    ShaderAnimationType getShaderAnimation() const;

    /// 设置 PSP XMB 波纹着色器的背景基色。
    /// r、g、b 范围 [0, 1]，默认深海军蓝。
    void setXmbBgColor(float r, float g, float b);

    // ── 覆写 ─────────────────────────────────────────────────────────────────

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

  private:
    // 模糊
    bool  m_blurEnabled = false;
    float m_blurRadius  = 8.0f;

    // 着色器动画
    ShaderAnimationType m_shaderAnimation = ShaderAnimationType::NONE;
    float m_animTime = 0.0f; ///< 已流逝秒数（按真实帧间隔累加）
    /// 上次着色器时间更新时间戳。
    std::chrono::steady_clock::time_point m_shaderLastTime;
    bool m_shaderTimerStarted = false;

    // PSP XMB 背景色（RGB，[0,1]）
    float m_xmbBgR = 0.05f;
    float m_xmbBgG = 0.10f;
    float m_xmbBgB = 0.25f;

#ifdef BOREALIS_USE_OPENGL
    // ── PSP XMB GL 着色器资源 ────────────────────────────────────────────────
    GLuint m_xmbProgram  = 0;  ///< 已编译的 GLSL 着色器程序
    GLuint m_xmbVAO      = 0;  ///< 全屏四边形顶点数组对象
    GLuint m_xmbVBO      = 0;  ///< 全屏四边形顶点缓冲
    GLuint m_xmbFbo      = 0;  ///< 离屏渲染帧缓冲对象
    GLuint m_xmbFboTex   = 0;  ///< 颜色附件纹理
    int    m_xmbFboW     = 0;  ///< 当前 FBO 纹理宽度
    int    m_xmbFboH     = 0;  ///< 当前 FBO 纹理高度
    int    m_xmbNvgImage = -1; ///< m_xmbFboTex 的 NanoVG 图像句柄
    bool   m_xmbInited   = false;

    GLint  m_xmbUTime       = -1;
    GLint  m_xmbUResolution = -1;
    GLint  m_xmbUBgColor    = -1;

    void   initXmbShader();
    void   resizeXmbFbo(int w, int h, NVGcontext* vg);
    void   drawPspXmbShader(NVGcontext* vg, float x, float y, float w, float h);
    void   freeXmbResources(NVGcontext* vg);
#endif

    void drawBlur(NVGcontext* vg, float x, float y, float w, float h, NVGpaint basePaint);

    // ── 异步加载状态 ──────────────────────────────────────────────────────────
    bool               m_asyncLoading = false; ///< 异步加载进行中时为 true
    std::atomic<int>   m_asyncGen{0};           ///< 每次新异步请求时递增
};

} // namespace beiklive::UI
