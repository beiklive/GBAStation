#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <switch.h>

#include "../../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub::ui {

using Gfx::Color;
using Gfx::Vector2f;

constexpr float kGradientFocusFlowCycleMs = 3600.0f;
constexpr float kGradientFocusBrightness = 1.0f;

#define NDS_STUB_KEYICON_A "\uE0E0"
#define NDS_STUB_KEYICON_B "\uE0E1"
#define NDS_STUB_KEYICON_X "\uE0E2"
#define NDS_STUB_KEYICON_Y "\uE0E3"
#define NDS_STUB_KEYICON_LSB "\uE104"
#define NDS_STUB_KEYICON_RSB "\uE105"
#define NDS_STUB_KEYICON_LT "\uE0E6"
#define NDS_STUB_KEYICON_RT "\uE0E7"
#define NDS_STUB_KEYICON_LB "\uE0E4"
#define NDS_STUB_KEYICON_RB "\uE0E5"
#define NDS_STUB_KEYICON_START "\uE0EF"
#define NDS_STUB_KEYICON_BACK "\uE0F0"
#define NDS_STUB_KEYICON_LEFT "\uE0ED"
#define NDS_STUB_KEYICON_UP "\uE0EB"
#define NDS_STUB_KEYICON_RIGHT "\uE0EE"
#define NDS_STUB_KEYICON_DOWN "\uE0EC"
#define NDS_STUB_KEYICON_UNKNOWN "\uE152"

float clamp01(float value);
float easeOutCubic(float t);
float easeOutQuart(float t);
float lerp(float a, float b, float t);
float animationProgress(std::uint64_t startTick, float durationMs);
float gradientFocusAnimationOffset();
Color mixColor(Color a, Color b, float t);
Color gradientFocusColor(float offset, float alpha);

// 绘制填充矩形（cool=true 时启用圆角/高斯模糊效果）
void drawRect(Vector2f pos, Vector2f size, Color color, bool cool = false);
// 绘制水平/垂直细线（本质上是极细的矩形）
void drawLine(Vector2f pos, Vector2f size, Color color);
// 绘制矩形边框（用4条细线拼出上/下/左/右四条边）
void drawBorder(Vector2f pos, Vector2f size, float width, Color color);
// 绘制选中项的外发光渐变边框，风格近似 Borealis View::drawHighlight：
// 外侧阴影 + 环形渐变流动边框，内部保持透明。
void drawGradientBorder(Vector2f pos, Vector2f size, float width);

} // namespace beiklive::nds_stub::ui
