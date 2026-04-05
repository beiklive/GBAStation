#include "Video/DisplayConfig.hpp"

#include <algorithm>
#include <cmath>

namespace beiklive {

// ---- ConfigManager 键名 ----
static constexpr const char* KEY_MODE      = "display.mode";
static constexpr const char* KEY_FILTER    = "display.filter";
static constexpr const char* KEY_INT_SCALE = "display.integer_scale_mult";
static constexpr const char* KEY_SCALE     = "display.custom_scale";
static constexpr const char* KEY_XOFF      = "display.x_offset";
static constexpr const char* KEY_YOFF      = "display.y_offset";

// ============================================================
// 字符串 ↔ ScreenMode 枚举转换
// ============================================================
const char* DisplayConfig::modeToString(ScreenMode m)
{
    switch (m) {
        case ScreenMode::Fit:          return "fit";
        case ScreenMode::Fill:         return "fill";
        case ScreenMode::Original:     return "original";
        case ScreenMode::IntegerScale: return "integer";
        case ScreenMode::Custom:       return "custom";
    }
    return "fit";
}

ScreenMode DisplayConfig::stringToMode(const std::string& s)
{
    if (s == "fill")     return ScreenMode::Fill;
    if (s == "original") return ScreenMode::Original;
    if (s == "integer")  return ScreenMode::IntegerScale;
    if (s == "custom")   return ScreenMode::Custom;
    return ScreenMode::Fit;     // 默认 / 未知
}

// ============================================================
// 字符串 ↔ FilterMode 枚举转换
// ============================================================
const char* DisplayConfig::filterModeToString(FilterMode f)
{
    switch (f) {
        case FilterMode::Nearest: return "nearest";
        case FilterMode::Linear:  return "linear";
    }
    return "nearest";
}

FilterMode DisplayConfig::stringToFilterMode(const std::string& s)
{
    if (s == "linear") return FilterMode::Linear;
    return FilterMode::Nearest;   // 默认 / 未知
}

// ============================================================
// 从 ConfigManager 加载配置
// ============================================================
void DisplayConfig::load(ConfigManager& cfg)
{
    // 注册默认值，确保首次保存时这些键出现在配置文件中
    cfg.SetDefault(KEY_MODE,      std::string(modeToString(ScreenMode::Fit)));
    cfg.SetDefault(KEY_FILTER,    std::string(filterModeToString(FilterMode::Nearest)));
    cfg.SetDefault(KEY_INT_SCALE, 0);
    cfg.SetDefault(KEY_SCALE,     1.0f);
    cfg.SetDefault(KEY_XOFF,      0.0f);
    cfg.SetDefault(KEY_YOFF,      0.0f);

    if (auto v = cfg.Get(KEY_MODE);      v)
        mode             = stringToMode(v->AsString().value_or("fit"));
    if (auto v = cfg.Get(KEY_FILTER);    v)
        filterMode       = stringToFilterMode(v->AsString().value_or("nearest"));
    if (auto v = cfg.Get(KEY_INT_SCALE); v)
        integerScaleMult = v->AsInt().value_or(0);
    if (auto v = cfg.Get(KEY_SCALE);     v)
        customScale      = v->AsFloat().value_or(1.0f);
    if (auto v = cfg.Get(KEY_XOFF);      v)
        xOffset          = v->AsFloat().value_or(0.0f);
    if (auto v = cfg.Get(KEY_YOFF);      v)
        yOffset          = v->AsFloat().value_or(0.0f);
}

// ============================================================
// 将配置写入 ConfigManager 并刷新到磁盘
// ============================================================
void DisplayConfig::save(ConfigManager& cfg) const
{
    cfg.Set(KEY_MODE,      std::string(modeToString(mode)));
    cfg.Set(KEY_FILTER,    std::string(filterModeToString(filterMode)));
    cfg.Set(KEY_INT_SCALE, integerScaleMult);
    cfg.Set(KEY_SCALE,     customScale);
    cfg.Set(KEY_XOFF,      xOffset);
    cfg.Set(KEY_YOFF,      yOffset);
    cfg.Save();
}

// ============================================================
// 计算绘制矩形
// ============================================================
DisplayRect DisplayConfig::computeRect(float viewX, float viewY,
                                        float viewW, float viewH,
                                        unsigned gameW, unsigned gameH) const
{
    DisplayRect r;
    const float gw = static_cast<float>(gameW);
    const float gh = static_cast<float>(gameH);
    if (gw <= 0.f || gh <= 0.f) return r;

    float scale = 1.0f;
    switch (mode) {
        case ScreenMode::Fit: {
            scale = std::min(viewW / gw, viewH / gh);
            break;
        }
        case ScreenMode::Fill: {
            r.w = viewW;
            r.h = viewH;
            r.x = viewX + xOffset;
            r.y = viewY + yOffset;
            return r;
        }
        case ScreenMode::Original: {
            scale = 1.0f;
            break;
        }
        case ScreenMode::IntegerScale: {
            float s = std::min(viewW / gw, viewH / gh);
            float maxScale = std::max(1.0f, std::floor(s));
            if (integerScaleMult <= 0) {
                scale = maxScale;
            } else {
                scale = std::max(1.0f, std::min(static_cast<float>(integerScaleMult), maxScale));
            }
            break;
        }
        case ScreenMode::Custom: {
            scale = customScale;
            break;
        }
    }

    r.w = gw * scale;
    r.h = gh * scale;
    r.x = viewX + (viewW - r.w) * 0.5f + xOffset;
    r.y = viewY + (viewH - r.h) * 0.5f + yOffset;
    return r;
}

} // namespace beiklive
