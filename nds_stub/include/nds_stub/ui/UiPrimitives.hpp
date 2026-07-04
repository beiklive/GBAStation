#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <switch.h>

#include "../../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub::ui {

using Gfx::Color;
using Gfx::Vector2f;

constexpr float kGradientFocusFlowCycleMs = 1800.0f;
constexpr float kGradientFocusBrightness = 1.0f;

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
// 绘制选中项的外发光渐变边框
// 包含四边流动的彩虹渐变色段 + 左侧光柱
void drawGradientBorder(Vector2f pos, Vector2f size, float width);

} // namespace beiklive::nds_stub::ui
