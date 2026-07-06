#pragma once

#include <string>
#include <vector>

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

struct NdsShaderParam {
    std::string name;
    std::string label;
    float value = 0.0f;
    float defaultValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.1f;
    int decimals = 2;
};

} // namespace beiklive::nds_stub
