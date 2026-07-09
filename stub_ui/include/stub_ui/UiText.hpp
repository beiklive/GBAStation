#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "../../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::stub_ui {

std::uint32_t materialFont();
void releaseTextGraphicsResources();

std::size_t utf8SafePrefix(const std::string& text, std::size_t bytes);
std::string ellipsizeText(const std::string& source, float maxTextW, float fontSize = 16.0f);
std::string formatBytes(std::uint64_t bytes);
bool endsWithNoCase(const std::string& value, const char* suffix);
float focusedMarqueeOffset(float textW, float boxW);

} // namespace beiklive::stub_ui
