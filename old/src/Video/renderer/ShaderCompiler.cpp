#include "Video/renderer/ShaderCompiler.hpp"

#include <borealis.hpp>
#include <fstream>
#include <sstream>
#include <string>

namespace beiklive {

// ============================================================
// 平台版本字符串
// ============================================================

const char* ShaderCompiler::glslVersionString()
{
#if defined(USE_GLES2)
    return "#version 100\n";
#elif defined(USE_GLES3)
    return "#version 300 es\n";
#elif defined(USE_GL2)
    return "#version 120\n";
#else
    return "#version 130\n";
#endif
}

const char* ShaderCompiler::fragPrecisionPrefix()
{
#if defined(USE_GLES2) || defined(USE_GLES3)
    // GLES 片段着色器默认无 float 精度，必须显式声明。
    // RetroArch 着色器内部会用 #ifdef GL_ES 自行声明，但为保险起见补一个默认。
    return "precision mediump float;\n";
#else
    return "";
#endif
}

// ============================================================
// 着色器对象编译
// ============================================================

GLuint ShaderCompiler::compileShader(GLenum type, const std::string& src)
{
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;

    const char* ptr = src.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
        brls::Logger::error("ShaderCompiler: 着色器编译失败 ({}):\n{}",
                            (type == GL_VERTEX_SHADER ? "vertex" : "fragment"),
                            log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// ============================================================
// 程序链接
// ============================================================

GLuint ShaderCompiler::compileProgram(const std::string& vertSrc,
                                       const std::string& fragSrc)
{
    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return 0;
    }

    GLuint prog = glCreateProgram();
    if (!prog) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    glAttachShader(prog, vert);
    glAttachShader(prog, frag);

    // 绑定固定属性位置（与 FullscreenQuad 的布局一致）
    glBindAttribLocation(prog, 0, "VertexCoord");
    glBindAttribLocation(prog, 1, "COLOR");
    glBindAttribLocation(prog, 2, "TexCoord");

    // PassPrevNTexCoord 属性（RetroArch 规范：第 N 个历史通道的纹理坐标）
    // 在本实现中所有 FBO 纹理的 UV 均为 [0,1]，与 TexCoord 完全一致，
    // 因此将这些属性绑定到与 TexCoord 相同的顶点位置（2），
    // 使着色器能正确采样历史通道纹理（如 diffusion-sampler/mixer 等）。
    for (int n = 1; n <= 8; ++n) {
        std::string attrName = "PassPrev" + std::to_string(n) + "TexCoord";
        glBindAttribLocation(prog, 2, attrName.c_str());
    }

    glLinkProgram(prog);

    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint status = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
        brls::Logger::error("ShaderCompiler: 着色器链接失败:\n{}", log);
        glDeleteProgram(prog);
        return 0;
    }

    return prog;
}

// ============================================================
// RetroArch 合并 .glsl 文件编译
// ============================================================

GLuint ShaderCompiler::compileRetroShader(const std::string& glslPath)
{
    std::ifstream f(glslPath);
    if (!f.is_open()) {
        brls::Logger::error("ShaderCompiler: 无法打开着色器文件: {}", glslPath);
        return 0;
    }

    std::ostringstream oss;
    oss << f.rdbuf();
    const std::string body = oss.str();

    // 从文件中提取 #version 声明，并构建去除 #version 的主体。
    // 策略：保留着色器声明的版本（如 #version 330），若无声明则回退到平台默认值。
    // 这样可兼容 nnedi3（需要 #version 330）、ScaleFX（#version 130）等不同着色器。
    std::string cleanBody;
    std::string extractedVersionLine; // 来自着色器的 "#version XXX" 行（去掉换行符）
    {
        std::istringstream iss(body);
        std::string line;
        while (std::getline(iss, line)) {
            // 去行尾 \r
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // 识别并提取 #version 行
            std::string trimLine = line;
            size_t firstNonSpace = trimLine.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos &&
                trimLine.substr(firstNonSpace, 8) == "#version") {
                // 保留着色器声明的第一个 #version 行（后续重复行发出警告）
                if (extractedVersionLine.empty()) {
                    extractedVersionLine = trimLine.substr(firstNonSpace);
                } else {
                    brls::Logger::warning("ShaderCompiler: 着色器含多个 #version 声明，忽略重复行: {}",
                                          trimLine.substr(firstNonSpace));
                }
                continue; // 从 cleanBody 中移除，稍后统一注入到最前面
            }
            cleanBody += line + "\n";
        }
    }

    // 确定最终使用的 #version 行：
    //   - 若着色器声明了版本，使用该版本（如 "#version 330"）
    //   - 否则使用平台默认版本（如 "#version 130"）
    std::string versionLine;
    if (!extractedVersionLine.empty()) {
        versionLine = extractedVersionLine + "\n";
    } else {
        versionLine = glslVersionString();
    }
    const char* fragPrec = fragPrecisionPrefix();

    // 顶点着色器：注入版本 + VERTEX 宏 + PARAMETER_UNIFORM 宏
    // PARAMETER_UNIFORM 宏告知着色器使用 uniform float 而非 #define 常量，
    // 使运行时通过 glUniform1f 设置的参数值能正确生效。
    std::string vertSrc =
        versionLine +
        "#define VERTEX\n"
        "#define PARAMETER_UNIFORM\n" +
        cleanBody;

    // 片段着色器：注入版本 + 精度 + FRAGMENT 宏 + PARAMETER_UNIFORM 宏
    std::string fragSrc =
        versionLine +
        std::string(fragPrec) +
        "#define FRAGMENT\n"
        "#define PARAMETER_UNIFORM\n" +
        cleanBody;

    GLuint prog = compileProgram(vertSrc, fragSrc);
    if (!prog) {
        brls::Logger::error("ShaderCompiler: 编译失败: {}", glslPath);
    }
    return prog;
}

} // namespace beiklive
