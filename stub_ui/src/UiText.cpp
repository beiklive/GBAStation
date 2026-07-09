#include "stub_ui/UiText.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#include <switch.h>

namespace beiklive::stub_ui {

namespace {

std::vector<std::uint8_t> gMaterialFontData;
std::uint32_t gMaterialFont = 0;
bool gMaterialFontAttempted = false;

} // namespace

std::uint32_t materialFont()
{
    if (gMaterialFontAttempted)
        return gMaterialFont;
    gMaterialFontAttempted = true;

    constexpr const char* paths[] = {
        "romfs:/material/MaterialIcons-Regular.ttf",
        "sdmc:/GBAStation/resources/material/MaterialIcons-Regular.ttf",
        "/GBAStation/resources/material/MaterialIcons-Regular.ttf",
    };
    for (const char* path : paths)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            continue;
        in.seekg(0, std::ios::end);
        const std::streamoff size = in.tellg();
        if (size <= 0)
            continue;
        in.seekg(0, std::ios::beg);
        gMaterialFontData.resize(static_cast<std::size_t>(size));
        in.read(reinterpret_cast<char*>(gMaterialFontData.data()), size);
        if (!in)
        {
            gMaterialFontData.clear();
            continue;
        }
        gMaterialFont = Gfx::FontLoad(gMaterialFontData.data());
        break;
    }
    return gMaterialFont;
}

void releaseTextGraphicsResources()
{
    if (gMaterialFont != 0)
    {
        Gfx::FontDelete(gMaterialFont);
        gMaterialFont = 0;
    }
    gMaterialFontData.clear();
    gMaterialFontData.shrink_to_fit();
    gMaterialFontAttempted = false;
}

std::size_t utf8SafePrefix(const std::string& text, std::size_t bytes)
{
    bytes = std::min(bytes, text.size());
    while (bytes > 0 && bytes < text.size() &&
           (static_cast<unsigned char>(text[bytes]) & 0xC0u) == 0x80u)
        --bytes;
    return bytes;
}

std::string ellipsizeText(const std::string& source, float maxTextW, float fontSize)
{
    if (source.empty() || maxTextW <= 18.0f)
        return source.empty() ? source : "...";
    if (Gfx::MeasureText(Gfx::SystemFontChinese, fontSize, source.c_str()).X <= maxTextW)
        return source;

    const std::string suffix = "...";
    std::size_t lo = 0;
    std::size_t hi = source.size();
    std::size_t best = 0;
    while (lo <= hi)
    {
        const std::size_t mid = lo + (hi - lo) / 2;
        const std::size_t cut = utf8SafePrefix(source, mid);
        const std::string candidate = source.substr(0, cut) + suffix;
        if (Gfx::MeasureText(Gfx::SystemFontChinese, fontSize, candidate.c_str()).X <= maxTextW)
        {
            best = cut;
            lo = mid + 1;
        }
        else
        {
            if (mid == 0)
                break;
            hi = mid - 1;
        }
    }

    std::string result = best == 0 ? suffix : source.substr(0, best) + suffix;
    while (result.size() > suffix.size() &&
           Gfx::MeasureText(Gfx::SystemFontChinese, fontSize, result.c_str()).X > maxTextW)
    {
        const std::size_t trimmed = utf8SafePrefix(result, result.size() - suffix.size() - 1);
        result = source.substr(0, trimmed) + suffix;
    }
    return result;
}

std::string formatBytes(std::uint64_t bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 3)
    {
        value /= 1024.0;
        ++unit;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(unit == 0 ? 0 : 1) << value << ' ' << units[unit];
    return out.str();
}

bool endsWithNoCase(const std::string& value, const char* suffix)
{
    const std::size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;

    const std::size_t offset = value.size() - suffixLen;
    for (std::size_t i = 0; i < suffixLen; ++i)
    {
        if (std::tolower(static_cast<unsigned char>(value[offset + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i])))
            return false;
    }
    return true;
}

float focusedMarqueeOffset(float textW, float boxW)
{
    const float overflow = std::max(0.0f, textW - boxW);
    if (overflow < 1.0f)
        return 0.0f;

    constexpr float holdMs = 620.0f;
    constexpr float speedPxPerMs = 0.038f;
    const float travel = overflow + 28.0f;
    const float travelMs = travel / speedPxPerMs;
    const float cycleMs = holdMs * 2.0f + travelMs * 2.0f;
    const float nowMs = static_cast<float>(armTicksToNs(armGetSystemTick()) / 1000000ULL);
    float t = std::fmod(nowMs, cycleMs);
    if (t < holdMs)
        return 0.0f;
    t -= holdMs;
    if (t < travelMs)
        return std::min(overflow, t * speedPxPerMs);
    t -= travelMs;
    if (t < holdMs)
        return overflow;
    t -= holdMs;
    return std::max(0.0f, overflow - t * speedPxPerMs);
}

} // namespace beiklive::stub_ui
