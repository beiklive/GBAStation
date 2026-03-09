#pragma once

#ifdef __SWITCH__

#include "common.hpp"
#include "ShaderChain.hpp"
#include "DisplayConfig.hpp"
#include <borealis.hpp>
#include <string>

namespace beiklive {

class GameRuntime {
public:
    GameRuntime() = default;
    explicit GameRuntime(const std::string& gamePath);
    ~GameRuntime();

    /// 加载并启动构造函数中指定路径的 ROM。
    bool loadGame();

    /// 初始化 OpenGL 资源（源纹理 + 着色器链）。
    /// 必须在有效的 GL 上下文中调用。
    void glInit();

    /// 释放所有 OpenGL 资源。
    void glDeinit();

    /// 在内置第 0 通道之后追加一个用户自定义着色器通道。
    /// @param vert  完整顶点着色器源码（含版本前缀）。
    /// @param frag  完整片段着色器源码。
    /// @return 编译链接成功返回 true。
    bool addShaderPass(const std::string& vert, const std::string& frag);

    /// 移除所有用户追加的通道（索引 >= 1），保留内置第 0 通道。
    void clearShaderPasses();

    /// 从 settingConfig 重新读取显示设置并立即生效。
    void reloadDisplayConfig();

    /// 每帧由 Borealis 调用。以约 60 fps 推进模拟器，
    /// 将像素上传至着色器链，并通过 NVG 渲染画面。
    void draw(NVGcontext* vg, float x, float y, float width, float height,
              brls::Style style, brls::FrameContext* ctx);

    /// 启用/禁用帧率限制器（正常速度 / 快进模式）。
    /// 从快进切回正常时，会先排空音频缓冲区以避免杂音
    /// （与 main.c 的 _setFrameLimiter 逻辑一致）。
    void setFrameLimiter(bool limit);

    /// 用户按下退出键后返回 true，GameView 应立即弹出页面。
    bool wantsExit() const { return m_wantsExit; }

private:
    std::string m_gamePath;
    bool        m_gameLoaded = false; ///< loadGame() 完成后置为 true

    // ---- GL 资源 -----------------------------------------------
    GLuint       m_srcTex = 0;       ///< 256×256 RGBA 源纹理（原始像素数据）

    // ---- 着色器管线 --------------------------------------------
    ShaderChain  m_chain;            ///< 多通道着色器链（拥有 FBO + VAO）

    // ---- 显示几何参数 ------------------------------------------
    DisplayConfig m_display;         ///< 屏幕模式 + 偏移量；从 settingConfig 加载

    // ---- NVG 集成 ---------------------------------------------
    int          m_nvgImage      = -1;              ///< 包装链输出的 NVG 图像句柄
    NVGcontext*  m_nvgCtx        = nullptr;
    GLuint       m_nvgTex        = 0;               ///< 当前被 m_nvgImage 包装的 GL 纹理
    FilterMode   m_activeFilter  = FilterMode::Nearest; ///< 当前已应用到纹理和 NVG 图像的过滤模式

    // ---- 软件渲染器像素缓冲区 ----------------------------------
    color_t*     m_softBuffer = nullptr;

    // ---- 帧率控制器（与 main.c 逻辑对应）---------------------
    bool         m_frameLimiter = true;   ///< true = 正常速度，false = 快进
    unsigned     m_framecount   = 0;      ///< 快进模式下已模拟的帧数计数
    unsigned     m_framecap     = 3;     ///< 每显示帧最多模拟的帧数（快进上限）
    bool         m_wantsExit    = false;  ///< 用户请求退出后置为 true，通知 GameView 弹出

    static color_t* frameBuffer;

    // 使包装着色器链输出的 NVG 图像失效（下一帧重新创建）
    void _invalidateNvgImage();

    void _clearScreen();
    void _drawFrame();

#ifdef __SWITCH__
    PadState m_pad;   ///< libnx 手柄状态，用于游戏输入轮询
#endif
};

} // namespace beiklive

#endif // __SWITCH__