#pragma once

#include <string>
#include <vector>

namespace beiklive {

/// 单个着色器通道的描述（从 .glslp 文件解析而来）
struct ShaderPassDesc {
    /// .glsl 着色器文件的绝对路径（已解析为绝对路径）
    std::string shaderPath;

    /// 输出纹理是否使用线性过滤（true = GL_LINEAR，false = GL_NEAREST）
    bool filterLinear = false;

    /// 输出 FBO 尺寸缩放类型
    enum class ScaleType {
        Source,   ///< 相对于输入源纹理尺寸缩放（默认）
        Viewport, ///< 相对于视口（屏幕）尺寸缩放
        Absolute, ///< 固定像素数
    };

    ScaleType scaleTypeX = ScaleType::Source; ///< X 轴缩放类型
    ScaleType scaleTypeY = ScaleType::Source; ///< Y 轴缩放类型

    float scaleX = 1.0f; ///< X 轴缩放系数（ScaleType::Source/Viewport）或像素数（Absolute）
    float scaleY = 1.0f; ///< Y 轴缩放系数

    bool floatFramebuffer = false; ///< 是否使用浮点 FBO（暂未使用，保留兼容性）
    bool srgbFramebuffer  = false; ///< 是否使用 sRGB FBO（暂未使用，保留兼容性）
};

/// RetroArch .glslp 着色器预设解析器
///
/// 支持的 .glslp 键：
///   shaders = N
///   shader0 = path/to/shader.glsl
///   filter_linear0 = true|false
///   scale_type_x0 = source|viewport|absolute
///   scale_type_y0 = source|viewport|absolute
///   scale_x0 = 1.0
///   scale_y0 = 1.0
///   scale_type0 = source|viewport|absolute  （同时设置 X 和 Y）
///   scale0 = 1.0                           （同时设置 X 和 Y）
class GLSLPParser {
public:
    /// 解析 .glslp 文件，将结果填入 @a outPasses。
    ///
    /// @param glslpPath  .glslp 文件的绝对或相对路径。
    ///                   shader 路径将相对于此文件的目录解析为绝对路径。
    /// @param outPasses  输出：每个通道的描述列表（按顺序）。
    /// @return true = 解析成功；false = 文件无法打开或 shaders 键缺失。
    static bool parse(const std::string& glslpPath,
                      std::vector<ShaderPassDesc>& outPasses);
};

} // namespace beiklive
