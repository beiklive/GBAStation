#pragma once

#include "Game/ShaderChain.hpp"

#include <string>

namespace beiklive {

/// RetroArch GLSL / GLSLP 着色器加载器。
///
/// 支持：
///  - 单文件 .glsl（#pragma stage vertex/fragment 格式）
///  - 单文件 .glsl（RetroArch 标准 #if defined(VERTEX)/#elif defined(FRAGMENT) 格式）
///  - 多通道预设 .glslp（RetroArch 格式：shaders=N, shader0=..., textures=..., #reference）
///
/// 编译策略（移植自 RetroArch shader_glsl.c / video_shader_parse.c）：
///  - RetroArch 格式：整个文件编译两次，前置 #define VERTEX/#define FRAGMENT
///    着色器自身的 #if __VERSION__ >= 130 + COMPAT_* 宏处理新旧 GLSL 语法
///  - #pragma stage 格式：按阶段拆分后注入兼容声明（VertexCoord/TexCoord/Texture 等）
///  - 自动去除 #pragma parameter 行（含引号，非法 GLSL）
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
};

} // namespace beiklive
