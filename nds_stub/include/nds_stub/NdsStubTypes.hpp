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

} // namespace beiklive::nds_stub
