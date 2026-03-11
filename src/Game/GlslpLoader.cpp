#include "Game/GlslpLoader.hpp"

// NanoVG 后端检测（与 ShaderChain.cpp / game_view.cpp 一致）
#ifdef BOREALIS_USE_OPENGL
#  ifdef USE_GLES3
#    define NANOVG_GLES3
#  elif defined(USE_GLES2)
#    define NANOVG_GLES2
#  elif defined(USE_GL2)
#    define NANOVG_GL2
#  else
#    define NANOVG_GL3
#  endif
#endif

#include <borealis.hpp>   // brls::Logger

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// stb_image for LUT texture loading (implementation provided by borealis/nanovg.c)
#include <borealis/extern/nanovg/stb_image.h>

namespace beiklive {

// ============================================================
// 内部工具函数
// ============================================================

/// 读取整个文件为字符串，失败返回空串
static std::string readFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open()) return {};
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

/// 裁剪字符串首尾空白
static std::string trimStr(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

/// 字符串全局替换
static std::string replaceAll(std::string s,
                               const std::string& from,
                               const std::string& to)
{
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

/// 从 RetroArch GLSL 源码中剥除 #pragma parameter 行。
/// 这些行含有 " 字符（合法 C 预处理器，但非法 GLSL），
/// 必须在编译前移除（参考 RetroArch shader_glsl.c::gl_glsl_strip_parameter_pragmas）。
static std::string stripPragmaParameters(const std::string& src)
{
    const std::string kPragma = "#pragma parameter";
    if (src.find(kPragma) == std::string::npos) return src;

    std::string result;
    result.reserve(src.size());
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);
        if (t.rfind(kPragma, 0) == 0) {
            // 保留换行以维持行号不变（用空格替换整行内容）
            result += '\n';
        } else {
            result += line;
            result += '\n';
        }
    }
    return result;
}

/// 从 GLSL 源码中解析 #pragma parameter 行，提取参数名及其默认值。
///
/// #pragma parameter 格式（RetroArch 标准）：
///   #pragma parameter NAME "描述" 默认值 最小值 最大值 步进
///
/// 本函数在着色器被编译前调用（剥除 #pragma 行之前），
/// 返回值用于在链接后将 uniform 初始化为默认值，
/// 避免 uniform 默认为 0 导致除零（1/0 = INF）进而全白画面。
static std::unordered_map<std::string, float> extractParamDefaults(const std::string& src)
{
    std::unordered_map<std::string, float> defaults;
    const std::string kPragma = "#pragma parameter";
    if (src.find(kPragma) == std::string::npos) return defaults;

    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);
        if (t.rfind(kPragma, 0) != 0) continue;

        // 跳过 "#pragma parameter" 前缀，读取参数名
        std::istringstream ls(t.substr(kPragma.size()));
        std::string name;
        if (!(ls >> name) || name.empty()) continue;

        // 跳过带引号的描述字符串（可能含空格）
        std::string rest;
        std::getline(ls, rest);
        size_t q1 = rest.find('"');
        if (q1 != std::string::npos) {
            size_t q2 = rest.find('"', q1 + 1);
            if (q2 != std::string::npos)
                rest = rest.substr(q2 + 1);
            else
                rest = rest.substr(q1 + 1);
        }

        // 解析第一个浮点数（默认值）
        std::istringstream vs(rest);
        float defVal = 0.f;
        if (vs >> defVal) {
            defaults[name] = defVal;
        } else {
            brls::Logger::warning("[GlslpLoader] 无法解析 #pragma parameter '{}' 的默认值，"
                                  "将跳过（uniform 保持 GL 默认值 0.0）", name);
        }
    }
    return defaults;
}

/// 剥除 #pragma stage 标记行（已被分割逻辑消费，不应进入着色器源码）
static std::string stripStagePragmas(const std::string& src)
{
    if (src.find("#pragma stage") == std::string::npos) return src;
    std::string result;
    result.reserve(src.size());
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);
        if (t.rfind("#pragma stage", 0) == 0)
            result += '\n';
        else {
            result += line;
            result += '\n';
        }
    }
    return result;
}

// ============================================================
// GlslpLoader::resolvePath
// ============================================================
std::string GlslpLoader::resolvePath(const std::string& baseDir,
                                      const std::string& rel)
{
    if (rel.empty()) return rel;
    // 如果已是绝对路径，直接返回
    if (rel[0] == '/'
#ifdef _WIN32
        || (rel.size() >= 2 && rel[1] == ':')
#endif
    ) return rel;

    try {
        std::filesystem::path p =
            std::filesystem::path(baseDir) / std::filesystem::path(rel);
        return p.lexically_normal().string();
    } catch (...) {
        return rel;
    }
}

// ============================================================
// GLSL 兼容性包装
// ============================================================

/// 判断某行是否为 RetroArch 属性/uniform 声明（需要剥除）
static bool isRaAttributeDecl(const std::string& t)
{
    auto startsWithKw = [&](const char* kw) {
        size_t n = std::strlen(kw);
        if (t.size() < n) return false;
        if (t.substr(0, n) != kw) return false;
        return t.size() == n || t[n] == ' ' || t[n] == '\t';
    };
    bool isAttrib = startsWithKw("attribute") || startsWithKw("in")
                 || startsWithKw("COMPAT_ATTRIBUTE");
    if (!isAttrib) return false;
    return t.find("VertexCoord") != std::string::npos
        || t.find("TexCoord")    != std::string::npos
        || t.find("COLOR")       != std::string::npos;
}

/// 判断某行是否为需要剥除的 uniform 声明
static bool isRaUniformDecl(const std::string& t)
{
    if (t.find("uniform") == std::string::npos) return false;

    if (t.find("MVPMatrix") != std::string::npos) return true;

    if (t.find("sampler2D") != std::string::npos) {
        // 仅剥除 "Texture" 和 "Source" 作为独立标识符（不是其他名称的后缀/前缀）
        // 例如：剥除 "uniform sampler2D Texture;"，但保留 "uniform sampler2D PrevTexture;"
        auto hasExactToken = [&](const char* tok) {
            size_t pos = t.find(tok);
            if (pos == std::string::npos) return false;
            // 左边界：必须是空白字符（非字母数字/下划线）
            if (pos > 0) {
                char prev = t[pos - 1];
                if (std::isalnum((unsigned char)prev) || prev == '_') return false;
            }
            size_t end = pos + std::strlen(tok);
            if (end >= t.size()) return true;
            // 右边界：必须是非标识符字符
            char next = t[end];
            return !std::isalnum((unsigned char)next) && next != '_';
        };
        if (hasExactToken("Texture") || hasExactToken("Source")) return true;
    }
    return false;
}

/// 判断某行是否为包装器已提供的宏重定义（避免 "Macro X redefined" 编译错误）。
/// 包装器（wrapVertSource / wrapFragSource）已注入这些宏，
/// 若着色器源码中再次 #define 相同名称则会触发编译错误。
static bool isRaWrapperMacroRedef(const std::string& t,
                                   const char* const macros[],
                                   size_t count)
{
    // 行必须含 #define
    size_t definePos = t.find("#define");
    if (definePos == std::string::npos) return false;

    // 跳过 "#define" 及其后的空白，定位宏名起始位置
    size_t nameStart = definePos + 7;
    while (nameStart < t.size() && (t[nameStart] == ' ' || t[nameStart] == '\t'))
        ++nameStart;

    for (size_t i = 0; i < count; ++i) {
        size_t mlen = std::strlen(macros[i]);
        if (t.size() < nameStart + mlen) continue;
        if (t.substr(nameStart, mlen) != macros[i]) continue;
        // 宏名之后必须是空白、'('（函数式宏）或行尾，否则可能是另一个宏名的前缀
        size_t afterName = nameStart + mlen;
        if (afterName >= t.size()
                || t[afterName] == ' ' || t[afterName] == '\t'
                || t[afterName] == '(')
            return true;
    }
    return false;
}

static std::string preprocessVertSource(const std::string& rawSrc)
{
    // 先剥除 #pragma parameter 和 #pragma stage 行
    std::string src = stripPragmaParameters(stripStagePragmas(rawSrc));

    // 包装器已注入的顶点着色器宏（着色器源码中的同名 #define 会引发重定义错误）
    static const char* kVertWrapperMacros[] = {
        "COMPAT_ATTRIBUTE", "COMPAT_VARYING", "COMPAT_TEXTURE",
        "COMPAT_TEXTURE_2D", "COMPAT_PRECISION", "VertexCoord",
        "TexCoord", "MVPMatrix", "COLOR"
    };
    static const size_t kVertWrapperMacroCount =
        sizeof(kVertWrapperMacros) / sizeof(kVertWrapperMacros[0]);

    std::istringstream iss(src);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);

        if (t.rfind("#version", 0) == 0) continue;
        if (isRaAttributeDecl(t)) continue;
        if (isRaUniformDecl(t)) continue;
        // 剥除包装器已提供的宏重定义（避免 "Macro X redefined" 编译错误）
        if (isRaWrapperMacroRedef(t, kVertWrapperMacros, kVertWrapperMacroCount)) {
            oss << "\n"; // 保留行号
            continue;
        }

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
        line = replaceAll(line, "attribute ", "in ");
        line = replaceAll(line, "varying ",   "out ");
        line = replaceAll(line, "texture2D(", "texture(");
        line = replaceAll(line, "texture2DLod(", "textureLod(");
        line = replaceAll(line, "COMPAT_ATTRIBUTE ", "in ");
        line = replaceAll(line, "COMPAT_VARYING ",   "out ");
        line = replaceAll(line, "COMPAT_TEXTURE_2D(", "texture(");
        line = replaceAll(line, "COMPAT_TEXTURE(",    "texture(");
#else
        line = replaceAll(line, "COMPAT_ATTRIBUTE ",   "attribute ");
        line = replaceAll(line, "COMPAT_VARYING ",     "varying ");
        line = replaceAll(line, "COMPAT_TEXTURE_2D(", "texture2D(");
        line = replaceAll(line, "COMPAT_TEXTURE(",    "texture2D(");
#endif
        oss << line << "\n";
    }
    return oss.str();
}

static std::string preprocessFragSource(const std::string& rawSrc)
{
    std::string src = stripPragmaParameters(stripStagePragmas(rawSrc));

    // 包装器已注入的片段着色器宏（着色器源码中的同名 #define 会引发重定义错误）
    static const char* kFragWrapperMacros[] = {
        "COMPAT_VARYING", "COMPAT_TEXTURE", "COMPAT_TEXTURE_2D",
        "COMPAT_PRECISION", "Texture", "Source", "FragColor", "texture"
    };
    static const size_t kFragWrapperMacroCount =
        sizeof(kFragWrapperMacros) / sizeof(kFragWrapperMacros[0]);

    std::istringstream iss(src);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);

        if (t.rfind("#version", 0) == 0) continue;
        if (isRaUniformDecl(t)) continue;

        // 剥除 RetroArch 片段输出声明（兼容块已提供 out vec4 fragColor）
        if (t.rfind("out ", 0) == 0 && t.find("FragColor") != std::string::npos
            && t.find("#define") == std::string::npos) {
            continue;
        }

        // 剥除包装器已提供的宏重定义（避免 "Macro X redefined" 编译错误）
        if (isRaWrapperMacroRedef(t, kFragWrapperMacros, kFragWrapperMacroCount)) {
            oss << "\n"; // 保留行号
            continue;
        }

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
        line = replaceAll(line, "varying ",      "in ");
        line = replaceAll(line, "gl_FragColor",  "fragColor");
        line = replaceAll(line, "texture2D(",    "texture(");
        line = replaceAll(line, "texture2DLod(", "textureLod(");
        line = replaceAll(line, "COMPAT_VARYING ",    "in ");
        line = replaceAll(line, "COMPAT_TEXTURE_2D(", "texture(");
        line = replaceAll(line, "COMPAT_TEXTURE(",    "texture(");
#else
        line = replaceAll(line, "COMPAT_VARYING ",     "varying ");
        line = replaceAll(line, "COMPAT_TEXTURE_2D(", "texture2D(");
        line = replaceAll(line, "COMPAT_TEXTURE(",    "texture2D(");
#endif
        oss << line << "\n";
    }
    return oss.str();
}

// ============================================================
// 顶点着色器兼容头
// ============================================================
std::string GlslpLoader::wrapVertSource(const std::string& body)
{
#if defined(NANOVG_GLES3)
    static const char* kHeader =
        "#version 300 es\n"
        "precision mediump float;\n";
#elif defined(NANOVG_GL3)
    static const char* kHeader =
        "#version 330 core\n";
#elif defined(NANOVG_GLES2)
    static const char* kHeader =
        "#version 100\n"
        "precision mediump float;\n";
#else  // GL2 / fallback
    static const char* kHeader =
        "#version 120\n";
#endif

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
    // 现代 GLSL：in/out + 完整 RetroArch COMPAT_* 宏
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块（GL3/GLES3）==\n"
        "#define PARAMETER_UNIFORM\n"
        "#define VERTEX\n"
        "#define COMPAT_ATTRIBUTE in\n"
        "#define COMPAT_VARYING out\n"
        "#define COMPAT_PRECISION\n"
        "#define COMPAT_TEXTURE texture\n"
        "#define COMPAT_TEXTURE_2D texture\n"
        "layout(location = 0) in vec2 offset;\n"
        "uniform vec2 dims;\n"
        "uniform vec2 insize;\n"
        // VertexCoord / TexCoord must be vec4 so shaders can swizzle .z/.w
        "vec4 _bkVertexCoord() { return vec4(offset.x * 2.0 - 1.0, offset.y * 2.0 - 1.0, 0.0, 1.0); }\n"
        "vec4 _bkTexCoord()    { return vec4(offset * insize, 0.0, 0.0); }\n"
        "#define VertexCoord _bkVertexCoord()\n"
        "#define TexCoord    _bkTexCoord()\n"
        "#define MVPMatrix   mat4(1.0)\n"
        // COLOR is a per-vertex attribute in RetroArch (typically white); provide constant fallback
        "#define COLOR       vec4(1.0, 1.0, 1.0, 1.0)\n";
#else
    // 旧版 GLSL：attribute/varying 语法
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块（GL2/GLES2）==\n"
        "#define PARAMETER_UNIFORM\n"
        "#define VERTEX\n"
        "#define COMPAT_ATTRIBUTE attribute\n"
        "#define COMPAT_VARYING varying\n"
#  if defined(NANOVG_GLES2)
        "#define COMPAT_PRECISION mediump\n"
#  else
        "#define COMPAT_PRECISION\n"
#  endif
        "#define COMPAT_TEXTURE texture2D\n"
        "#define COMPAT_TEXTURE_2D texture2D\n"
        "attribute vec2 offset;\n"
        "uniform vec2 dims;\n"
        "uniform vec2 insize;\n"
        // VertexCoord / TexCoord must be vec4 so shaders can swizzle .z/.w
        "vec4 _bkVertexCoord() { return vec4(offset.x * 2.0 - 1.0, offset.y * 2.0 - 1.0, 0.0, 1.0); }\n"
        "vec4 _bkTexCoord()    { return vec4(offset * insize, 0.0, 0.0); }\n"
        "#define VertexCoord _bkVertexCoord()\n"
        "#define TexCoord    _bkTexCoord()\n"
        "#define MVPMatrix   mat4(1.0)\n"
        // COLOR is a per-vertex attribute in RetroArch (typically white); provide constant fallback
        "#define COLOR       vec4(1.0, 1.0, 1.0, 1.0)\n";
#endif

    return std::string(kHeader) + kCompat + preprocessVertSource(body);
}

// ============================================================
// 片段着色器兼容头
// ============================================================
std::string GlslpLoader::wrapFragSource(const std::string& body)
{
#if defined(NANOVG_GLES3)
    static const char* kHeader =
        "#version 300 es\n"
        "precision mediump float;\n";
#elif defined(NANOVG_GL3)
    static const char* kHeader =
        "#version 330 core\n";
#elif defined(NANOVG_GLES2)
    static const char* kHeader =
        "#version 100\n"
        "precision mediump float;\n";
#else
    static const char* kHeader =
        "#version 120\n";
#endif

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块（GL3/GLES3）==\n"
        "#define PARAMETER_UNIFORM\n"
        "#define FRAGMENT\n"
        "#define COMPAT_VARYING in\n"
        "#define COMPAT_PRECISION\n"
        "#define COMPAT_TEXTURE texture\n"
        "#define COMPAT_TEXTURE_2D texture\n"
        "uniform sampler2D tex;\n"
        "out vec4 fragColor;\n"
        "#define Texture tex\n"
        "#define Source  tex\n"
        "#define FragColor fragColor\n";
#else
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块（GL2/GLES2）==\n"
        "#define PARAMETER_UNIFORM\n"
        "#define FRAGMENT\n"
        "#define COMPAT_VARYING varying\n"
#  if defined(NANOVG_GLES2)
        "#define COMPAT_PRECISION mediump\n"
#  else
        "#define COMPAT_PRECISION\n"
#  endif
        "#define COMPAT_TEXTURE texture2D\n"
        "#define COMPAT_TEXTURE_2D texture2D\n"
        "uniform sampler2D tex;\n"
        "#define Texture tex\n"
        "#define Source  tex\n"
        "#define FragColor gl_FragColor\n";
#endif

    return std::string(kHeader) + kCompat + preprocessFragSource(body);
}

// ============================================================
// GlslpLoader::parseGlslFile
// ============================================================
bool GlslpLoader::parseGlslFile(const std::string& path,
                                  std::string& outVert,
                                  std::string& outFrag)
{
    std::string src = readFile(path);
    if (src.empty()) {
        brls::Logger::error("[GlslpLoader] Cannot read: {}", path);
        return false;
    }

    // ---- 尝试 #pragma stage 分割 ----
    const std::string kStagePragma = "#pragma stage";
    size_t vertPos = src.find(kStagePragma);
    if (vertPos != std::string::npos) {
        // 查找 vertex 和 fragment 两个阶段标记
        size_t vStart = std::string::npos, fStart = std::string::npos;

        // 遍历所有 #pragma stage 标记
        size_t pos = 0;
        while ((pos = src.find(kStagePragma, pos)) != std::string::npos) {
            size_t eol = src.find('\n', pos);
            std::string tag = (eol == std::string::npos)
                              ? src.substr(pos)
                              : src.substr(pos, eol - pos);
            if (tag.find("vertex") != std::string::npos)
                vStart = (eol == std::string::npos) ? src.size() : eol + 1;
            else if (tag.find("fragment") != std::string::npos)
                fStart = (eol == std::string::npos) ? src.size() : eol + 1;
            pos += kStagePragma.size();
        }

        if (vStart != std::string::npos && fStart != std::string::npos) {
            std::string vertBody, fragBody;
            if (vStart < fStart) {
                // vertex 段在前：提取 vStart 到 fragment pragma 行之间的内容
                // 用 rfind 找 fragment pragma 行开始位置（从 fStart 回溯到行首）
                size_t fragPragmaLine = src.rfind(kStagePragma, fStart - 1);
                if (fragPragmaLine == std::string::npos) fragPragmaLine = fStart;
                vertBody = src.substr(vStart, fragPragmaLine - vStart);
                fragBody = src.substr(fStart);
            } else {
                // fragment 段在前：提取 fStart 到 vertex pragma 行之间的内容
                size_t vertPragmaLine = src.rfind(kStagePragma, vStart - 1);
                if (vertPragmaLine == std::string::npos) vertPragmaLine = vStart;
                fragBody = src.substr(fStart, vertPragmaLine - fStart);
                vertBody = src.substr(vStart);
            }
            outVert = wrapVertSource(vertBody);
            outFrag = wrapFragSource(fragBody);
            return true;
        }
    }

    // ---- 尝试 #ifdef VERTEX / FRAGMENT 分割（支持 #elif 和嵌套 #if）----
    // 正确处理 RetroArch 典型格式：
    //   #if defined(VERTEX) ... #elif defined(FRAGMENT) ... #endif
    // 以及经典格式：
    //   #ifdef VERTEX ... #ifdef FRAGMENT ... 或 #else
    {
        std::string vertBody, fragBody;
        std::ostringstream vss, fss, preStageSS;
        std::string line;

        bool inStage    = false;   // 是否在某个已识别的 VERTEX/FRAGMENT 块中
        bool inVert     = false;
        bool inFrag     = false;
        bool foundStage = false;   // 是否已找到至少一个阶段标记
        int  nestedDepth = 0;      // 当前阶段内部嵌套 #if 的深度

        // 检测是否为 VERTEX 阶段标记（#if defined(VERTEX) / #ifdef VERTEX / #elif defined(VERTEX)）
        auto isVertMarker = [](const std::string& t) {
            return t == "#ifdef VERTEX"
                || t == "#if defined(VERTEX)"
                || t == "#elif defined(VERTEX)";
        };
        // 检测是否为 FRAGMENT 阶段标记
        auto isFragMarker = [](const std::string& t) {
            return t == "#ifdef FRAGMENT"
                || t == "#if defined(FRAGMENT)"
                || t == "#elif defined(FRAGMENT)";
        };

        std::istringstream iss(src);
        while (std::getline(iss, line)) {
            std::string t = trimStr(line);

            if (!inStage) {
                // 尚未进入任何阶段：寻找起始标记
                if (isVertMarker(t)) {
                    inStage = true; inVert = true; inFrag = false; nestedDepth = 0;
                    foundStage = true;
                    continue;
                }
                if (isFragMarker(t)) {
                    inStage = true; inFrag = true; inVert = false; nestedDepth = 0;
                    foundStage = true;
                    continue;
                }
                // 阶段外的行（前导注释、#pragma parameter、全局 #define 等）
                // 收集到 preStageSS，以便稍后同时前置到 vert 和 frag body
                preStageSS << line << "\n";
                continue;
            }

            // 已在某个阶段内部
            // 首先在深度为 0 时处理阶段级控制指令
            if (nestedDepth == 0) {
                if (isFragMarker(t)) {
                    // 切换到 FRAGMENT 阶段（例如 #elif defined(FRAGMENT)）
                    inFrag = true; inVert = false;
                    continue;
                }
                if (isVertMarker(t)) {
                    // 切换到 VERTEX 阶段
                    inVert = true; inFrag = false;
                    continue;
                }
                if (t == "#endif") {
                    // 外层 #if defined(VERTEX/FRAGMENT) 的终止
                    inStage = false; inVert = false; inFrag = false;
                    continue;
                }
                if (t == "#else") {
                    // 深度 0 的 #else：通常意味着 #ifdef VERTEX / #else (fragment) / #endif 格式
                    if (inVert) { inFrag = true; inVert = false; }
                    else        { inStage = false; inVert = false; inFrag = false; }
                    continue;
                }
                // 其他深度 0 的行：属于当前阶段内容，继续执行深度跟踪
            }

            // 跟踪嵌套 #if 深度
            if (t.rfind("#ifdef", 0) == 0 || t.rfind("#ifndef", 0) == 0
                || t.rfind("#if ", 0) == 0 || t == "#if") {
                nestedDepth++;
            } else if (t == "#endif") {
                // nestedDepth > 0（否则上面的 depth==0 分支已处理）
                nestedDepth--;
            }
            // 深度 > 0 内的 #elif / #else 直接输出，不做阶段切换

            if (inVert) vss << line << "\n";
            else if (inFrag) fss << line << "\n";
        }

        if (foundStage) {
            // 将阶段外的全局声明（#define target_gamma 等）前置到两个着色器 body
            std::string preStage = preStageSS.str();
            vertBody = preStage + vss.str();
            fragBody = preStage + fss.str();
        } else {
            vertBody = vss.str();
            fragBody = fss.str();
        }
        if (!vertBody.empty() && !fragBody.empty()) {
            outVert = wrapVertSource(vertBody);
            outFrag = wrapFragSource(fragBody);
            return true;
        }
    }

    // ---- 没有找到阶段分隔符：尝试视整个文件为片段着色器，
    //      并生成一个标准直通顶点着色器 ----
    brls::Logger::warning("[GlslpLoader] No stage markers in {}, "
                          "treating as fragment-only shader", path);
    {
        // 默认直通顶点着色器体（使用 RetroArch 风格变量名）
        static const char* kDefaultVertBody =
            "// 自动生成的直通顶点着色器\n"
#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
            "out vec2 vTexCoord;\n"
#else
            "varying vec2 vTexCoord;\n"
#endif
            "void main() {\n"
            "    gl_Position = MVPMatrix * VertexCoord;\n"
            "    vTexCoord = TexCoord.xy;\n"
            "}\n";

        outVert = wrapVertSource(kDefaultVertBody);
        outFrag = wrapFragSource(src);
        return true;
    }
}

// ============================================================
// GlslpLoader::loadGlslIntoChain
// ============================================================
bool GlslpLoader::loadGlslIntoChain(ShaderChain& chain, const std::string& path)
{
    // 先读取原始源码以提取 #pragma parameter 默认值（编译前）
    std::string rawSrc = readFile(path);
    auto paramDefaults = extractParamDefaults(rawSrc);

    std::string vert, frag;
    if (!parseGlslFile(path, vert, frag)) return false;

    if (!chain.addPass(vert, frag,
                       GL_NEAREST, GL_CLAMP_TO_EDGE, ShaderPassScale{}, 0, {},
                       paramDefaults)) {
        brls::Logger::error("[GlslpLoader] Failed to compile pass from: {}", path);
        return false;
    }
    brls::Logger::info("[GlslpLoader] Loaded GLSL pass: {}", path);
    return true;
}

// ============================================================
// GlslpLoader::loadGlslpIntoChain
// ============================================================
bool GlslpLoader::loadGlslpIntoChain(ShaderChain& chain, const std::string& path)
{
    std::string src = readFile(path);
    if (src.empty()) {
        brls::Logger::error("[GlslpLoader] Cannot read preset: {}", path);
        return false;
    }

    std::string baseDir;
    try {
        baseDir = std::filesystem::path(path).parent_path().string();
    } catch (...) {
        baseDir = ".";
    }

    // ---- 检查 #reference 指令（RetroArch 简单预设格式）----
    // #reference 行直接引用另一个预设文件，本文件中的其他设置覆盖引用预设
    {
        std::istringstream refiss(src);
        std::string refLine;
        while (std::getline(refiss, refLine)) {
            std::string t = trimStr(refLine);
            if (t.rfind("#reference", 0) == 0) {
                // 格式: #reference "path/to/preset.glslp"
                size_t q1 = t.find('"');
                size_t q2 = (q1 != std::string::npos) ? t.find('"', q1 + 1) : std::string::npos;
                std::string refPath;
                if (q1 != std::string::npos && q2 != std::string::npos)
                    refPath = t.substr(q1 + 1, q2 - q1 - 1);
                else {
                    // 无引号格式
                    size_t sp = t.find(' ');
                    if (sp != std::string::npos)
                        refPath = trimStr(t.substr(sp + 1));
                }
                if (!refPath.empty()) {
                    refPath = resolvePath(baseDir, refPath);
                    brls::Logger::info("[GlslpLoader] Following #reference → {}", refPath);
                    return loadGlslpIntoChain(chain, refPath);
                }
            }
        }
    }

    // 解析 key=value 行（忽略注释行，但保留 #reference 已单独处理）
    std::unordered_map<std::string, std::string> kv;
    {
        std::istringstream iss(src);
        std::string line;
        while (std::getline(iss, line)) {
            std::string t = trimStr(line);
            if (t.empty() || t[0] == '#' || t[0] == ';') continue;
            size_t eq = t.find('=');
            if (eq == std::string::npos) continue;
            std::string k = trimStr(t.substr(0, eq));
            std::string v = trimStr(t.substr(eq + 1));
            // 去掉引号
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
                v = v.substr(1, v.size() - 2);
            kv[k] = v;
        }
    }

    // ---- 辅助函数：安全读取 bool / float / int ----
    auto kvBool = [&](const std::string& key, bool def) -> bool {
        auto it = kv.find(key);
        if (it == kv.end()) return def;
        return it->second == "true" || it->second == "1";
    };
    auto kvFloat = [&](const std::string& key, float def) -> float {
        auto it = kv.find(key);
        if (it == kv.end()) return def;
        try { return std::stof(it->second); } catch (...) { return def; }
    };
    auto kvInt = [&](const std::string& key, int def) -> int {
        auto it = kv.find(key);
        if (it == kv.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    };

    // ---- 读取着色器数量 ----
    int shaderCount = kvInt("shaders", -1);
    if (shaderCount <= 0) {
        brls::Logger::error("[GlslpLoader] Preset missing/invalid 'shaders' key: {}", path);
        return false;
    }

    // 清除现有用户通道，重新加载
    chain.clearPasses();

    // ---- 解析预设文件中的全局参数覆盖值 ----
    // 格式: parameters = "PARAM1;PARAM2" 及对应的 PARAM1 = value 行
    std::unordered_map<std::string, float> globalParamOverrides;
    {
        auto paramIt = kv.find("parameters");
        if (paramIt != kv.end() && !paramIt->second.empty()) {
            std::string paramList = paramIt->second;
            // 去掉可能存在的引号
            if (paramList.size() >= 2 && paramList.front() == '"' && paramList.back() == '"')
                paramList = paramList.substr(1, paramList.size() - 2);
            // 支持分号或空格分隔
            std::replace(paramList.begin(), paramList.end(), ';', ' ');
            std::istringstream piss(paramList);
            std::string pname;
            while (piss >> pname) {
                auto pit = kv.find(pname);
                if (pit != kv.end()) {
                    try {
                        globalParamOverrides[pname] = std::stof(pit->second);
                    } catch (...) {
                        brls::Logger::warning("[GlslpLoader] 预设全局参数 '{}' 的值 '{}' 无法解析为浮点数，已跳过",
                                              pname, pit->second);
                    }
                }
            }
        }
    }

    // ---- 解析并加载 LUT 纹理 ----
    {
        auto texIt = kv.find("textures");
        if (texIt != kv.end() && !texIt->second.empty()) {
            // textures=id1;id2;id3  （分号或逗号分隔）
            std::string texList = texIt->second;
            std::replace(texList.begin(), texList.end(), ';', ' ');
            std::replace(texList.begin(), texList.end(), ',', ' ');
            std::istringstream tiss(texList);
            std::string texId;
            while (tiss >> texId) {
                std::string pathKey  = texId + "_path";
                std::string linKey   = texId + "_linear";
                std::string mipKey   = texId + "_mipmap";
                std::string wrapKey  = texId + "_wrap_mode";

                auto pathIt = kv.find(pathKey);
                if (pathIt == kv.end() || pathIt->second.empty()) continue;

                std::string lutPath = resolvePath(baseDir, pathIt->second);
                bool linear  = kvBool(linKey,  false);
                bool mipmap  = kvBool(mipKey,  false);
                std::string wrapStr = (kv.count(wrapKey)) ? kv[wrapKey] : "clamp_to_edge";

                GLenum wrap = GL_CLAMP_TO_EDGE;
                if (wrapStr == "repeat")           wrap = GL_REPEAT;
                else if (wrapStr == "mirrored_repeat") wrap = GL_MIRRORED_REPEAT;
                else if (wrapStr == "clamp_to_border") wrap = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_BORDER not available in GLES; fallback to GL_CLAMP_TO_EDGE

                chain.addLut(texId, lutPath, linear, mipmap, wrap);
            }
        }
    }

    // ---- 加载每个着色器通道 ----
    int loaded = 0;
    for (int i = 0; i < shaderCount; ++i) {
        std::string siStr   = std::to_string(i);
        std::string shaderKey = "shader" + siStr;
        auto it = kv.find(shaderKey);
        if (it == kv.end() || it->second.empty()) {
            brls::Logger::warning("[GlslpLoader] Missing {} in preset", shaderKey);
            continue;
        }
        std::string shaderPath = resolvePath(baseDir, it->second);

        // 过滤模式
        bool linear = kvBool("filter_linear" + siStr, false);
        GLenum glFilter = linear ? GL_LINEAR : GL_NEAREST;

        // Wrap 模式（用于 FBO 输出纹理）
        std::string wrapStr = kv.count("wrap_mode" + siStr) ? kv["wrap_mode" + siStr] : "clamp_to_edge";
        GLenum wrapMode = GL_CLAMP_TO_EDGE;
        if (wrapStr == "repeat")               wrapMode = GL_REPEAT;
        else if (wrapStr == "mirrored_repeat") wrapMode = GL_MIRRORED_REPEAT;

        // Scale 类型和因子（ported from video_shader_parse.c::video_shader_parse_pass）
        std::string scaleTypeStr  = kv.count("scale_type"   + siStr) ? kv["scale_type"   + siStr] : "";
        std::string scaleTypeXStr = kv.count("scale_type_x" + siStr) ? kv["scale_type_x" + siStr] : scaleTypeStr;
        std::string scaleTypeYStr = kv.count("scale_type_y" + siStr) ? kv["scale_type_y" + siStr] : scaleTypeStr;

        float scaleXf = kvFloat("scale_x" + siStr, kvFloat("scale" + siStr, 1.0f));
        float scaleYf = kvFloat("scale_y" + siStr, kvFloat("scale" + siStr, 1.0f));
        int   absX    = kvInt("scale_x"   + siStr, kvInt("scale" + siStr, 0));
        int   absY    = kvInt("scale_y"   + siStr, kvInt("scale" + siStr, 0));

        ShaderPassScale scale;
        scale.typeX = ShaderPassScale::SOURCE;
        scale.typeY = ShaderPassScale::SOURCE;
        scale.scaleX = scaleXf;
        scale.scaleY = scaleYf;
        scale.absX   = absX;
        scale.absY   = absY;

        auto parseScaleType = [](const std::string& s) {
            if (s == "absolute") return ShaderPassScale::SCALE_ABSOLUTE;
            if (s == "viewport") return ShaderPassScale::VIEWPORT;
            return ShaderPassScale::SOURCE;
        };
        if (!scaleTypeXStr.empty()) scale.typeX = parseScaleType(scaleTypeXStr);
        if (!scaleTypeYStr.empty()) scale.typeY = parseScaleType(scaleTypeYStr);

        // frame_count_mod
        int fcMod = kvInt("frame_count_mod" + siStr, 0);

        // Pass alias
        std::string alias;
        if (kv.count("alias" + siStr)) alias = kv["alias" + siStr];

        // 编译并添加通道
        // 先读取原始源码以提取 #pragma parameter 默认值（在编译前）
        std::string rawShaderSrc = readFile(shaderPath);
        auto paramDefaults = extractParamDefaults(rawShaderSrc);
        // 将预设文件中的全局参数覆盖值合并到该通道的默认值映射
        // （仅覆盖该通道着色器中实际声明的参数，避免向无关通道传递多余值）
        for (const auto& ov : globalParamOverrides)
            paramDefaults[ov.first] = ov.second;

        std::string vert, frag;
        if (!parseGlslFile(shaderPath, vert, frag)) {
            brls::Logger::warning("[GlslpLoader] Failed to parse {}", shaderPath);
            continue;
        }
        if (!chain.addPass(vert, frag, glFilter, wrapMode, scale, fcMod, alias,
                           paramDefaults)) {
            brls::Logger::warning("[GlslpLoader] Failed to compile {}", shaderPath);
            continue;
        }
        brls::Logger::info("[GlslpLoader] Loaded pass {}: {}", i, shaderPath);
        ++loaded;
    }

    if (loaded > 0) {
        brls::Logger::info("[GlslpLoader] Loaded {}/{} passes from: {}",
                           loaded, shaderCount, path);
        return true;
    }
    return false;
}

} // namespace beiklive
