#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <switch.h>

#include "../../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::stub_ui {

using Gfx::Color;
using Gfx::Vector2f;

constexpr float kGradientFocusFlowCycleMs = 3600.0f;
constexpr float kGradientFocusBrightness = 1.0f;

#define STUB_UI_KEYICON_A "\uE0E0"
#define STUB_UI_KEYICON_B "\uE0E1"
#define STUB_UI_KEYICON_X "\uE0E2"
#define STUB_UI_KEYICON_Y "\uE0E3"
#define STUB_UI_KEYICON_LSB "\uE104"
#define STUB_UI_KEYICON_RSB "\uE105"
#define STUB_UI_KEYICON_LT "\uE0E6"
#define STUB_UI_KEYICON_RT "\uE0E7"
#define STUB_UI_KEYICON_LB "\uE0E4"
#define STUB_UI_KEYICON_RB "\uE0E5"
#define STUB_UI_KEYICON_START "\uE0EF"
#define STUB_UI_KEYICON_BACK "\uE0F0"
#define STUB_UI_KEYICON_LEFT "\uE0ED"
#define STUB_UI_KEYICON_UP "\uE0EB"
#define STUB_UI_KEYICON_RIGHT "\uE0EE"
#define STUB_UI_KEYICON_DOWN "\uE0EC"
#define STUB_UI_KEYICON_UNKNOWN "\uE152"

float clamp01(float value);
float easeOutCubic(float t);
float easeOutQuart(float t);
float lerp(float a, float b, float t);
float animationProgress(std::uint64_t startTick, float durationMs);
float gradientFocusAnimationOffset();
Color mixColor(Color a, Color b, float t);
Color gradientFocusColor(float offset, float alpha);

void drawRect(Vector2f pos, Vector2f size, Color color, bool cool = false);
void drawLine(Vector2f pos, Vector2f size, Color color);
void drawBorder(Vector2f pos, Vector2f size, float width, Color color);
void drawGradientBorder(Vector2f pos, Vector2f size, float width);
void releasePrimitiveGraphicsResources();

} // namespace beiklive::stub_ui
