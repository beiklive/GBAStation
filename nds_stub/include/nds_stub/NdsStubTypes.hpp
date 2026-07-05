#pragma once

namespace beiklive::nds_stub {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kDsWidth = 256;
constexpr int kDsHeight = 192;

struct RectF {
    float x;
    float y;
    float w;
    float h;
};

struct NdsCustomLayoutSettings {
    float topScale = 1.0f;
    float topOffsetX = 0.0f;
    float topOffsetY = 0.0f;
    float bottomScale = 1.0f;
    float bottomOffsetX = 0.0f;
    float bottomOffsetY = 0.0f;
};

} // namespace beiklive::nds_stub
