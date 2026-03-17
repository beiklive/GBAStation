#pragma once

#include <glad/glad.h>
#include <string>

namespace beiklive {

/// RetroArch 风格 GLSL 着色器编译器
///
/// RetroArch .glsl 文件将顶点和片段着色器合并在一个文件中，
/// 通过预处理器条件 `#if defined(VERTEX)` / `#elif defined(FRAGMENT)` 区分。
///
/// 本编译器：
///   1. 读取合并 .glsl 文件；
///   2. 自动注入正确的 #version 字符串及 VERTEX/FRAGMENT 宏；
///   3. 在链接前绑定固定属性位置（VertexCoord=0, COLOR=1, TexCoord=2）；
///   4. 支持 GLES2 / GLES3 / GL2 / GL3 四种后端。
class ShaderCompiler {
public:
    /// 编译 RetroArch 合并 .glsl 文件。
    ///
    /// @param glslPath  .glsl 文件的绝对路径。
    /// @return 链接完成的 GL 程序 ID；失败时返回 0。
    static GLuint compileRetroShader(const std::string& glslPath);

    /// 从独立的顶点/片段着色器源码字符串编译并链接程序。
    ///
    /// @param vertSrc  顶点着色器完整源码（含 #version）。
    /// @param fragSrc  片段着色器完整源码（含 #version）。
    /// @return 链接完成的 GL 程序 ID；失败时返回 0。
    static GLuint compileProgram(const std::string& vertSrc,
                                 const std::string& fragSrc);

private:
    /// 编译单个着色器阶段。
    static GLuint compileShader(GLenum type, const std::string& src);

    /// 返回当前平台的 GLSL 版本字符串（含换行符）。
    static const char* glslVersionString();

    /// 返回片段着色器额外需要的精度前缀（GLES 需要）。
    static const char* fragPrecisionPrefix();
};

} // namespace beiklive
