#pragma once
#include "common.hpp"
#include "Utils/ConfigManager.hpp"
#include <string>

namespace beiklive {

/// 游戏画面在显示区域中的缩放方式
enum class ScreenMode {
    Fit,           ///< 保持宽高比；上下/左右加黑边以适应视图（默认）
    Fill,          ///< 拉伸填满整个显示区域，不保持宽高比
    Original,      ///< 1:1 像素映射，不缩放
    IntegerScale,  ///< 整数倍率缩放（默认为视图内最大整数倍）
    Custom,        ///< 使用显式 customScale；xOffset/yOffset 相对中心偏移
};

/// 纹理过滤模式
enum class FilterMode {
    Nearest,  ///< 最近邻采样（像素风格，无模糊，默认）
    Linear,   ///< 双线性插值（平滑过渡）
};

/// 由 DisplayConfig::computeRect() 计算得出的屏幕空间绘制矩形
struct DisplayRect {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
};

/// 保存显示几何参数并计算绘制矩形。
///
/// 各参数通过 ConfigManager 以 "display.*" 键持久化：
///   display.mode               – "fit" | "fill" | "original" | "integer" | "custom"
///   display.filter             – "nearest" | "linear"
///   display.integer_scale_mult – 整数缩放倍率（0 = 自动最大值，>=1 = 固定倍率）
///   display.custom_scale       – 浮点缩放倍数（Custom 模式）
///   display.x_offset           – 浮点水平偏移（相对中心，所有模式有效）
///   display.y_offset           – 浮点垂直偏移（相对中心，所有模式有效）
class DisplayConfig {
public:
    ScreenMode mode             = ScreenMode::Original;
    FilterMode filterMode       = FilterMode::Nearest;  ///< 纹理过滤模式
    int        integerScaleMult = 0;     ///< IntegerScale 模式倍率；0 = 自动最大整数倍
    float      customScale      = 1.0f;  ///< Custom 模式下使用的缩放倍数
    float      xOffset          = 0.0f;  ///< 相对默认居中位置的水平像素偏移
    float      yOffset          = 0.0f;  ///< 相对默认居中位置的垂直像素偏移

    /// 从 @a cfg 加载显示设置。
    /// 不存在的键将使用默认值，首次加载后配置文件会自动
    /// 包含这些键，便于用户查看和编辑。
    void load(ConfigManager& cfg);

    /// 将当前设置写入 @a cfg 并刷新到磁盘。
    void save(ConfigManager& cfg) const;

    /// 计算游戏画面（@a gameW × @a gameH 像素）在视图区域
    /// （@a viewX, @a viewY），@a viewW × @a viewH 内的屏幕空间绘制矩形。
    DisplayRect computeRect(float viewX, float viewY,
                            float viewW, float viewH,
                            unsigned gameW, unsigned gameH) const;

    static const char* modeToString(ScreenMode mode);
    static ScreenMode  stringToMode(const std::string& s);
    static const char* filterModeToString(FilterMode f);
    static FilterMode  stringToFilterMode(const std::string& s);
};

} // namespace beiklive
