#pragma once

#include "Game/ShaderChain.hpp"

#include <string>

namespace beiklive {

/// RetroArch GLSL / GLSLP 着色器加载器。
///
/// 支持：
///  - 单文件 .glsl（#pragma stage vertex/fragment 或 #ifdef VERTEX/FRAGMENT 格式）
///  - 多通道预设 .glslp（RetroArch 格式：shaders=N, shader0=..., textures=..., #reference）
///
/// 兼容性特性（移植自 RetroArch shader_glsl.c / video_shader_parse.c）：
///  - 自动剥除 #pragma parameter 行（含 " 字符，非法 GLSL，但 RetroArch C 预处理器合法）
///  - 注入 COMPAT_* 宏（COMPAT_ATTRIBUTE, COMPAT_VARYING, COMPAT_TEXTURE,
///    COMPAT_PRECISION, PARAMETER_UNIFORM, VERTEX/FRAGMENT）
///  - legacy 语法转换（attribute/varying/texture2D → in/out/texture）
///  - #reference 指令链式跟随
///  - 每通道 scale 因子、wrap 模式、LUT 纹理加载
class GlslpLoader {
public:
    /// 解析单个 .glsl 文件，将其作为一个用户通道追加到 @a chain 中。
    static bool loadGlslIntoChain(ShaderChain& chain, const std::string& path);

    /// 解析 .glslp 预设文件，将所有着色器通道追加到 @a chain。
    /// 追加前会先清除已有的用户通道（pass >= 1）。
    static bool loadGlslpIntoChain(ShaderChain& chain, const std::string& path);

    /// 将 .glsl 文件的顶点/片段阶段解析为独立的 GLSL 源字符串。
    static bool parseGlslFile(const std::string& path,
                               std::string& outVert,
                               std::string& outFrag);

private:
    static std::string resolvePath(const std::string& base, const std::string& rel);
    static std::string wrapVertSource(const std::string& body);
    static std::string wrapFragSource(const std::string& body);
};

} // namespace beiklive
