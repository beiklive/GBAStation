#pragma once

#include <string>
#include <vector>

namespace beiklive {

/// 单个着色器通道的描述（从 .glslp 文件解析而来）
struct ShaderPassDesc {
    /// .glsl 着色器文件的绝对路径（已解析为绝对路径）
    std::string shaderPath;

    /// 通道别名（alias），供后续通道以 <alias>Texture 形式引用本通道输出
    std::string alias;

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

/// .glslp 预设中声明的外部纹理描述
struct GLSLPTextureDesc {
    std::string name;          ///< 纹理名称（对应 GLSL uniform sampler2D 的变量名）
    std::string path;          ///< 纹理图片文件的绝对路径
    bool        filterLinear = false; ///< 采样过滤模式：true = 线性，false = 最近邻
};

/// .glslp 预设中声明的参数默认值覆盖
/// 用于向各通道着色器传递 #pragma parameter 对应的 uniform float 值
struct GLSLPParamOverride {
    std::string name;   ///< 参数名（对应 GLSL uniform float 变量名，大小写与 GLSL 一致）
    float       value;  ///< 覆盖默认值（来自 .glslp 文件的 NAME = value 键值对）
};

/// 着色器参数完整元数据
/// 来源：从 .glsl 源文件中解析 #pragma parameter 指令，并结合 .glslp 中的覆盖值
struct ShaderParamInfo {
    std::string name;         ///< GLSL uniform 变量名（区分大小写）
    std::string desc;         ///< 参数显示名称（来自 #pragma parameter 中的引号字符串）
    float       defaultValue; ///< 默认值（来自 #pragma parameter）
    float       minValue;     ///< 最小值（来自 #pragma parameter）
    float       maxValue;     ///< 最大值（来自 #pragma parameter）
    float       step;         ///< 步进值（来自 #pragma parameter；0 表示连续）
    float       value;        ///< 当前值（初始为 defaultValue，可被 .glslp 覆盖）
};

/// RetroArch .glslp 着色器预设解析器
///
/// 支持的 .glslp 键：
///   shaders = N
///   shader0 = path/to/shader.glsl
///   alias0 = "NAME"
///   filter_linear0 = true|false
///   scale_type_x0 = source|viewport|absolute
///   scale_type_y0 = source|viewport|absolute
///   scale_x0 = 1.0
///   scale_y0 = 1.0
///   scale_type0 = source|viewport|absolute  （同时设置 X 和 Y）
///   scale0 = 1.0                           （同时设置 X 和 Y）
///   textures = NAME1;NAME2
///   NAME1 = path/to/image.png
///   NAME1_linear = true|false
///   parameters = PARAM1;PARAM2           （参数名列表，分号分隔）
///   PARAM1 = 0.5                          （参数默认值覆盖）
class GLSLPParser {
public:
    /// 解析 .glslp 文件，将结果填入 @a outPasses、@a outTextures 和 @a outParams。
    ///
    /// @param glslpPath   .glslp 文件的绝对或相对路径。
    ///                    shader 和纹理路径均相对于此文件目录解析为绝对路径。
    /// @param outPasses   输出：每个通道的描述列表（按顺序）。
    /// @param outTextures 输出：预设中声明的外部纹理列表（可为 nullptr 时忽略）。
    /// @param outParams   输出：预设中声明的参数默认值覆盖列表（可为 nullptr 时忽略）。
    /// @return true = 解析成功；false = 文件无法打开或 shaders 键缺失。
    static bool parse(const std::string& glslpPath,
                      std::vector<ShaderPassDesc>& outPasses,
                      std::vector<GLSLPTextureDesc>* outTextures = nullptr,
                      std::vector<GLSLPParamOverride>* outParams = nullptr);

    /// 从单个 .glsl 着色器源文件中解析 #pragma parameter 指令并输出参数元数据。
    ///
    /// #pragma parameter 格式：
    ///   #pragma parameter NAME "Display Name" DEFAULT MIN MAX STEP
    ///
    /// @param shaderPath  .glsl 文件路径。
    /// @param outMeta     输出：解析到的参数元数据列表（已去重，后出现的覆盖先出现的）。
    static void parseParamMeta(const std::string& shaderPath,
                               std::vector<ShaderParamInfo>& outMeta);
};

} // namespace beiklive
