#pragma once

#include "Game/ShaderChain.hpp"

#include <string>

namespace beiklive {

/// RetroArch GLSL / GLSLP 着色器加载器。
///
/// 支持：
///  - 单文件 .glsl（用 `#pragma stage vertex` / `#pragma stage fragment`
///    或 `#ifdef VERTEX` / `#ifdef FRAGMENT` 分隔顶点/片段着色器）
///  - 多通道预设文件 .glslp（RetroArch 格式，`shaders=N`, `shader0=...` 等）
///
/// 加载时会自动注入兼容性头部，使大多数 RetroArch legacy GLSL 着色器
///（使用 attribute / varying / gl_FragColor / texture2D）可直接运行于
/// GL3/GLES3 上下文，无需修改着色器文件。
class GlslpLoader {
public:
    /// 解析单个 .glsl 文件，将其作为一个用户通道追加到 @a chain 中。
    /// @param chain  目标 ShaderChain（pass 0 应已初始化）。
    /// @param path   .glsl 文件的绝对路径。
    /// @return 成功返回 true。
    static bool loadGlslIntoChain(ShaderChain& chain, const std::string& path);

    /// 解析 .glslp 预设文件，将所有着色器通道追加到 @a chain。
    /// 追加前会先清除已有的用户通道（pass >= 1）。
    /// @param chain  目标 ShaderChain（pass 0 应已初始化）。
    /// @param path   .glslp 文件的绝对路径。
    /// @return 成功加载至少一个通道返回 true。
    static bool loadGlslpIntoChain(ShaderChain& chain, const std::string& path);

    /// 将 .glsl 文件的顶点/片段阶段解析为独立的 GLSL 源字符串。
    /// @param path      .glsl 文件路径。
    /// @param outVert   解析出的顶点着色器完整源码（含兼容头）。
    /// @param outFrag   解析出的片段着色器完整源码（含兼容头）。
    /// @return 成功返回 true。
    static bool parseGlslFile(const std::string& path,
                               std::string& outVert,
                               std::string& outFrag);

private:
    /// 以指定目录为基准解析相对路径，返回绝对路径。
    static std::string resolvePath(const std::string& base,
                                   const std::string& rel);

    /// 用兼容头包装顶点着色器主体，返回完整可编译源码。
    static std::string wrapVertSource(const std::string& body);

    /// 用兼容头包装片段着色器主体，返回完整可编译源码。
    static std::string wrapFragSource(const std::string& body);
};

} // namespace beiklive
