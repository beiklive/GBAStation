#include "DynamicBackgroundBox.hpp"
#include "core/common.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
namespace beiklive
{

    static const char *SYMBOLS[] = {
        "△",
        "○",
        "□",
        "×"};

    DynamicBackgroundBox::DynamicBackgroundBox()
    {
        this->setGrow(1);
    }

    float DynamicBackgroundBox::randRange(float min, float max)
    {
        float t = (float)std::rand() / (float)RAND_MAX;
        return min + (max - min) * t;
    }

    void DynamicBackgroundBox::shake(float strength, float duration)
    {
        shakeStrength = strength;
        shakeDuration = duration;
        shakeTimer = duration;
    }

    void DynamicBackgroundBox::setGradientTheme(GradientTheme theme)
    {
        beiklive::g_gradientTheme = theme;
    }

    void DynamicBackgroundBox::update(float dt, float width, float height)
    {
        beiklive::UpdateBackgroundIcons(dt, width, height);

        if (shakeTimer > 0.0f)
        {
            shakeTimer -= dt;
            if (shakeTimer < 0.0f)
                shakeTimer = 0.0f;
        }
    }

    void DynamicBackgroundBox::drawGradient(NVGcontext *vg, float width, float height)
    {
        NVGcolor topColor, bottomColor;
        beiklive::GetGradientColors(topColor, bottomColor);

        NVGpaint bg = nvgLinearGradient(
            vg,
            0, 0,
            0, height,
            topColor,
            bottomColor);

        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, width, height);
        nvgFillPaint(vg, bg);
        nvgFill(vg);
    }

    void DynamicBackgroundBox::drawIcons(NVGcontext *vg)
    {
        for (auto &icon : beiklive::g_backgroundIcons)
        {
            nvgSave(vg);

            nvgTranslate(vg, icon.x, icon.y);
            nvgRotate(vg, icon.rotation);

            nvgFontSize(vg, icon.size);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            nvgFillColor(vg, nvgRGBA(255, 255, 255, (unsigned char)(icon.alpha * 255)));
            nvgText(vg, 0, 0, SYMBOLS[icon.symbolIndex], nullptr);

            nvgRestore(vg);
        }
    }

    void DynamicBackgroundBox::draw(
        NVGcontext *vg,
        float x,
        float y,
        float width,
        float height,
        brls::Style style,
        brls::FrameContext *ctx)
    {
        // float now = std::chrono::duration<float>(
        //     std::chrono::steady_clock::now().time_since_epoch()
        // ).count();

        // if (beiklive::g_backgroundLastTime == 0.0f)
        //     beiklive::g_backgroundLastTime = now;

        // float dt = now - beiklive::g_backgroundLastTime;
        // beiklive::g_backgroundLastTime = now;

        // if (dt > 0.05f)
            float dt = 0.02f;

        update(dt, width, height);

        float offsetX = 0.0f;
        float offsetY = 0.0f;

        if (shakeTimer > 0.0f)
        {
            float t = shakeTimer / shakeDuration;
            float power = shakeStrength * t;

            offsetX = randRange(-power, power);
            offsetY = randRange(-power, power);
        }

        nvgSave(vg);
        nvgTranslate(vg, x + offsetX, y + offsetY);

        drawGradient(vg, width, height);
        drawIcons(vg);

        nvgRestore(vg);

        // 子控件正常绘制（菜单、列表等）
        brls::Box::draw(vg, x, y, width, height, style, ctx);
    }

}
