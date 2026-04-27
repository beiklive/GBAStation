#pragma once

#include <borealis.hpp>
#include <vector>

namespace beiklive
{
    


class DynamicBackgroundBox : public brls::Box
{
public:
    DynamicBackgroundBox();
    virtual ~DynamicBackgroundBox() = default;

public:
    // 外部主动调用：模拟器震动反馈 / 操作反馈
    void shake(float strength = 12.0f, float duration = 0.25f);

protected:
    void draw(
        NVGcontext* vg,
        float x,
        float y,
        float width,
        float height,
        brls::Style style,
        brls::FrameContext* ctx) override;

private:
    struct FloatingIcon
    {
        float x;
        float y;

        float speedX;
        float speedY;

        float size;

        float rotation;
        float rotateSpeed;

        float alpha;

        int symbolIndex;
    };

private:
    std::vector<FloatingIcon> icons;

    float lastTime = 0.0f;

    // shake
    float shakeStrength = 0.0f;
    float shakeTimer = 0.0f;
    float shakeDuration = 0.0f;

private:
    void initIcons();
    void update(float dt, float width, float height);

    void drawGradient(NVGcontext* vg, float width, float height);
    void drawIcons(NVGcontext* vg);

    float randRange(float min, float max);
};


} // namespace beiklive
