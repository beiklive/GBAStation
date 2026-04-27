#include "DynamicBackgroundBox.hpp"

#include <cmath>
#include <cstdlib>
#include <ctime>
namespace beiklive
{

    static const char *SYMBOLS[] = {
        "△",
        "○",
        "□",
        "×"};

    DynamicBackgroundBox::DynamicBackgroundBox()
    {
        std::srand((unsigned int)std::time(nullptr));
        this->setGrow(1);
        initIcons();
    }

    float DynamicBackgroundBox::randRange(float min, float max)
    {
        float t = (float)std::rand() / (float)RAND_MAX;
        return min + (max - min) * t;
    }

    void DynamicBackgroundBox::initIcons()
    {
        icons.clear();

        for (int i = 0; i < 24; i++)
        {
            FloatingIcon icon;

            icon.x = randRange(0.0f, 1280.0f);
            icon.y = randRange(0.0f, 720.0f);

            icon.speedX = randRange(-8.0f, 8.0f);
            icon.speedY = randRange(-22.0f, -8.0f);

            icon.size = randRange(24.0f, 60.0f);

            icon.rotation = randRange(0.0f, 6.28f);
            icon.rotateSpeed = randRange(-0.6f, 0.6f);

            icon.alpha = randRange(0.05f, 0.16f);

            icon.symbolIndex = std::rand() % 4;

            icons.push_back(icon);
        }
    }

    void DynamicBackgroundBox::shake(float strength, float duration)
    {
        shakeStrength = strength;
        shakeDuration = duration;
        shakeTimer = duration;
    }

    void DynamicBackgroundBox::update(float dt, float width, float height)
    {
        for (auto &icon : icons)
        {
            icon.x += icon.speedX * dt;
            icon.y += icon.speedY * dt;

            icon.rotation += icon.rotateSpeed * dt;

            if (icon.y < -80.0f)
            {
                icon.y = height + randRange(20.0f, 80.0f);
                icon.x = randRange(0.0f, width);
            }

            if (icon.x < -80.0f)
                icon.x = width + 40.0f;

            if (icon.x > width + 80.0f)
                icon.x = -40.0f;
        }

        if (shakeTimer > 0.0f)
        {
            shakeTimer -= dt;
            if (shakeTimer < 0.0f)
                shakeTimer = 0.0f;
        }
    }

    void DynamicBackgroundBox::drawGradient(NVGcontext *vg, float width, float height)
    {
        NVGpaint bg = nvgLinearGradient(
            vg,
            0, 0,
            0, height,
            nvgRGBA(20, 28, 60, 255),
            nvgRGBA(8, 10, 22, 255));

        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, width, height);
        nvgFillPaint(vg, bg);
        nvgFill(vg);
    }

    void DynamicBackgroundBox::drawIcons(NVGcontext *vg)
    {
        for (auto &icon : icons)
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
        float now = (float)brls::getCPUTimeUsec() / 1000000.0f;

        if (lastTime == 0.0f)
            lastTime = now;

        float dt = now - lastTime;
        lastTime = now;

        if (dt > 0.05f)
            dt = 0.05f;

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
