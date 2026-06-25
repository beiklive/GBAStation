#include "game/render/ShaderCompiler.hpp"

#include <borealis.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <filesystem>
#include <vector>

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
    return "precision mediump float;\n";
#else
    return "";
#endif
}

// ============================================================
// #include 预处理支持
// ============================================================

static std::string resolveIncludes(const std::string& filePath,
                                   std::vector<std::string>& visitedStack,
                                   int depth)
{
    const int kMaxIncludeDepth = 16;
    if (depth > kMaxIncludeDepth) {
        brls::Logger::error("ShaderCompiler: #include 嵌套深度超限 ({}) in: {}", depth, filePath);
        return {};
    }

    for (const auto& v : visitedStack) {
        if (v == filePath) {
            brls::Logger::error("ShaderCompiler: #include 循环引用检测: {}", filePath);
            return {};
        }
    }
    visitedStack.push_back(filePath);

    std::ifstream f(filePath);
    if (!f.is_open()) {
        brls::Logger::error("ShaderCompiler: 无法打开文件: {}", filePath);
        visitedStack.pop_back();
        return {};
    }

    std::string       fileName  = std::filesystem::path(filePath).filename().string();
    std::string       dir       = std::filesystem::path(filePath).parent_path().string();
    std::string       result;
    std::string       line;
    unsigned          lineNum   = 0;

    while (std::getline(f, line)) {
        ++lineNum;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            result += line + "\n";
            continue;
        }
        std::string trimmed = line.substr(start);

        bool isOptional = false;
        const char* includePrefix = nullptr;

        if (trimmed.rfind("#include ", 0) == 0) {
            includePrefix = "#include ";
        } else if (trimmed.rfind("#pragma include_optional ", 0) == 0) {
            includePrefix = "#pragma include_optional ";
            isOptional = true;
        }

        if (includePrefix) {
            std::string rest = trimmed.substr(std::strlen(includePrefix));
            size_t q1 = rest.find('"');
            if (q1 == std::string::npos) {
                result += line + "\n";
                continue;
            }
            size_t q2 = rest.find('"', q1 + 1);
            if (q2 == std::string::npos) {
                result += line + "\n";
                continue;
            }
            std::string includeFile = rest.substr(q1 + 1, q2 - q1 - 1);

            std::filesystem::path incPath;
            if (std::filesystem::path(includeFile).is_absolute()) {
                incPath = includeFile;
            } else {
                incPath = std::filesystem::path(dir) / includeFile;
            }
            incPath = incPath.lexically_normal();
            std::string resolvedPath = incPath.string();

            if (!std::filesystem::exists(resolvedPath)) {
                if (!isOptional) {
                    brls::Logger::error("ShaderCompiler: #include 文件不存在: {}", resolvedPath);
                    visitedStack.pop_back();
                    return {};
                }
                brls::Logger::debug("ShaderCompiler: 可选的 #include 未找到: {}", resolvedPath);
                result += line + "\n";
                continue;
            }

            // 插入 #line 1 "included_file" 用于错误行号追踪
            result += "#line 1 \"" + resolvedPath + "\"\n";
            std::string included = resolveIncludes(resolvedPath, visitedStack, depth + 1);
            if (included.empty()) {
                visitedStack.pop_back();
                return {};
            }
            result += included;
            // 恢复行号指向当前文件的下一条指令
            result += "#line " + std::to_string(lineNum + 1) + " \"" + fileName + "\"\n";
        } else {
            result += line + "\n";
        }
    }

    visitedStack.pop_back();
    return result;
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

    // PassPrevNTexCoord 属性：绑定到与 TexCoord 相同的位置（2）
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
    // 1. #include 预处理：递归解析 #include 和 #pragma include_optional 指令
    std::vector<std::string> visitedStack;
    std::string body = resolveIncludes(glslPath, visitedStack, 0);
    if (body.empty()) {
        brls::Logger::error("ShaderCompiler: #include 预处理失败: {}", glslPath);
        return 0;
    }

    // 2. 从预处理后的内容中提取 #version 声明
    std::string cleanBody;
    std::string extractedVersionLine;
    {
        std::istringstream iss(body);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::string trimLine = line;
            size_t firstNonSpace = trimLine.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos &&
                trimLine.substr(firstNonSpace, 8) == "#version") {
                if (extractedVersionLine.empty()) {
                    extractedVersionLine = trimLine.substr(firstNonSpace);
                } else {
                    brls::Logger::warning("ShaderCompiler: 着色器含多个 #version 声明，忽略重复行: {}",
                                          trimLine.substr(firstNonSpace));
                }
                continue;
            }
            cleanBody += line + "\n";
        }
    }

    std::string versionLine;
    if (!extractedVersionLine.empty()) {
        versionLine = extractedVersionLine + "\n";
    } else {
        versionLine = glslVersionString();
    }
    const char* fragPrec = fragPrecisionPrefix();

    // 顶点着色器：注入版本 + VERTEX 宏 + PARAMETER_UNIFORM 宏
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
