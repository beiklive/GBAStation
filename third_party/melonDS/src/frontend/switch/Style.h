#ifndef STYLE_H
#define STYLE_H

#include "Gfx.h"

const float TextLineHeight = 20.f;
const float UIRowHeight = TextLineHeight * 4.f;
const float BackButtonHeight = TextLineHeight * 3.f;

// https://coolors.co/e03548-4ca12c-292f36-909797-f7fff7
const Gfx::Color WallpaperColor = {(u32)0xE03548FF};
const Gfx::Color DarkColor = {(u32)0x292F36FF};
const Gfx::Color DarkColorTransparent = {(u32)0x292F36CC};
const Gfx::Color WidgetColorBright = {(u32)0xF7FFF7FF};
const Gfx::Color WidgetColorVibrant = {(u32)0x4CA12CFF};
const Gfx::Color SeparatorColor = {(u32)0x909797FF};

#endif