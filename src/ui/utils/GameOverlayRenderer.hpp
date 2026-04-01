#pragma once

#include <borealis.hpp>
#include <nanovg.h>
#include <cstdio>

namespace beiklive
{

/// 游戏画面状态文字覆盖层绘制工具
///
/// 提供在游戏画面上方绘制半透明徽章文字的静态方法。
/// 每个方法接受 NVGcontext、绘制区域矩形及文字颜色参数。
class GameOverlayRenderer
{
public:
    /// 绘制通用标签徽章：半透明圆角背景 + 文字
    ///
    /// @param vg     NanoVG 上下文
    /// @param x,y    徽章左上角坐标
    /// @param w,h    徽章宽高
    /// @param text   显示文字
    /// @param bgColor  背景颜色（含透明度）
    /// @param textColor 文字颜色（含透明度）
    static void drawBadge(NVGcontext* vg, float x, float y, float w, float h,
                          const char* text, NVGcolor bgColor, NVGcolor textColor)
    {
        // 半透明背景
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 4.0f);
        nvgFillColor(vg, bgColor);
        nvgFill(vg);

        // 文字
        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, textColor);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, text, nullptr);
    }

    /// 绘制 FPS 覆盖层（左上角绿色）
    static void drawFps(NVGcontext* vg, float viewX, float viewY, float fps)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
        drawBadge(vg, viewX + 4.f, viewY + 4.f, 90.f, 22.f, buf,
                  nvgRGBA(0, 0, 0, 160), nvgRGBA(0, 255, 80, 230));
    }

    /// 绘制快进覆盖层（右上角蓝色 >>）
    static void drawFastForward(NVGcontext* vg, float viewX, float viewY, float viewW)
    {
        const char* text = ">>  快进";
        float w = 90.f, h = 22.f;
        drawBadge(vg, viewX + viewW - w - 4.f, viewY + 4.f, w, h, text,
                  nvgRGBA(0, 0, 0, 160), nvgRGBA(100, 220, 255, 230));
    }

    /// 绘制倒带覆盖层（顶部居中橙色 <<<）
    static void drawRewind(NVGcontext* vg, float viewX, float viewY, float viewW)
    {
        const char* text = "<<<  倒带";
        float w = 100.f, h = 22.f;
        drawBadge(vg, viewX + (viewW - w) * 0.5f, viewY + 4.f, w, h, text,
                  nvgRGBA(0, 0, 0, 160), nvgRGBA(255, 200, 0, 230));
    }

    /// 绘制暂停覆盖层（顶部居中黄色 PAUSED）
    static void drawPaused(NVGcontext* vg, float viewX, float viewY, float viewW)
    {
        const char* text = "  已暂停";
        float w = 90.f, h = 22.f;
        drawBadge(vg, viewX + (viewW - w) * 0.5f, viewY + 4.f, w, h, text,
                  nvgRGBA(0, 0, 0, 180), nvgRGBA(255, 220, 60, 230));
    }

    /// 绘制静音覆盖层（右下角红色 MUTE）
    static void drawMute(NVGcontext* vg, float viewX, float viewY, float viewW, float viewH)
    {
        const char* text = "静音";
        float w = 60.f, h = 22.f;
        drawBadge(vg, viewX + viewW - w - 4.f, viewY + viewH - h - 8.f, w, h, text,
                  nvgRGBA(180, 30, 30, 200), nvgRGBA(255, 255, 255, 230));
    }
};

} // namespace beiklive
