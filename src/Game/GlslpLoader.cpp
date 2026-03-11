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
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

/// 对 GLSL 源码进行行级预处理：
///  - 删除 #version 声明（由兼容头统一提供）
///  - 删除已知 RetroArch 属性/变量声明（由兼容头替代）
///  - 对现代语法进行必要转换
/// 判断某行是否为 RetroArch 属性/uniform 声明（需要剥除）
static bool isRaAttributeDecl(const std::string& t)
{
    // 匹配: [attribute|in|COMPAT_ATTRIBUTE] [qualifiers] vecN Name;
    // 目标: VertexCoord, TexCoord, COLOR
    auto startsWithKw = [&](const char* kw) {
        size_t n = std::strlen(kw);
        if (t.size() < n) return false;
        if (t.substr(0, n) != kw) return false;
        // 紧跟空白或换行
        return t.size() == n || t[n] == ' ' || t[n] == '\t';
    };
    // 也处理 COMPAT_ATTRIBUTE（RetroArch 着色器用宏定义的属性关键字）
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

    // MVPMatrix
    if (t.find("MVPMatrix") != std::string::npos) return true;

    // sampler2D Texture; / sampler2D Source;
    if (t.find("sampler2D") != std::string::npos) {
        // 使用词边界检查：Texture 或 Source 后跟 ; 或空白或数组 [
        auto hasToken = [&](const char* tok) {
            size_t pos = t.find(tok);
            if (pos == std::string::npos) return false;
            size_t end = pos + std::strlen(tok);
            if (end >= t.size()) return true;
            char next = t[end];
            return next == ';' || next == ' ' || next == '\t'
                || next == '[' || next == ')';
        };
        if (hasToken("Texture") || hasToken("Source")) return true;
    }
    return false;
}

static std::string preprocessVertSource(const std::string& src)
{
    // 字符串全局替换辅助函数（在 GL3 与 GL2 路径中共用）
    auto replaceAll = [](std::string s,
                         const std::string& from,
                         const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
        return s;
    };

    std::istringstream iss(src);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);

        // 删除版本声明
        if (t.rfind("#version", 0) == 0) continue;

        // 删除 VertexCoord / TexCoord / COLOR 的 attribute/in/COMPAT_ATTRIBUTE 声明
        if (isRaAttributeDecl(t)) continue;

        // 删除 MVPMatrix / Texture / Source uniform 声明
        if (isRaUniformDecl(t)) continue;

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
        // 在 #version 330 / 300 es 下：
        // attribute → in，varying → out，texture2D → texture
        line = replaceAll(line, "attribute ", "in ");
        line = replaceAll(line, "varying ",   "out ");
        line = replaceAll(line, "texture2D(", "texture(");
        line = replaceAll(line, "texture2DLod(", "textureLod(");
#endif
        // RetroArch 着色器常用 'MVPMatrix * VertexCoord'，
        // 其中 VertexCoord 被视为 vec4。但我们的兼容块将其定义为 vec2，
        // 需要将其提升为 vec4 以避免 mat4*vec2 的类型错误。
        line = replaceAll(line, "MVPMatrix * VertexCoord",
                          "MVPMatrix * vec4(VertexCoord, 0.0, 1.0)");

        oss << line << "\n";
    }
    return oss.str();
}

static std::string preprocessFragSource(const std::string& src)
{
    std::istringstream iss(src);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);

        if (t.rfind("#version", 0) == 0) continue;

        // 删除 Texture / Source 的 sampler uniform 声明
        if (isRaUniformDecl(t)) continue;

        // 删除 RetroArch 片段着色器输出声明 'out ... vec4 FragColor;'
        // 兼容块已经提供了 'out vec4 fragColor;' 以及 '#define FragColor fragColor'
        if (t.rfind("out ", 0) == 0 && t.find("FragColor") != std::string::npos
            && t.find("#define") == std::string::npos) {
            continue;
        }

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)
        {
            auto replaceAll = [](std::string s,
                                 const std::string& from,
                                 const std::string& to) {
                size_t pos = 0;
                while ((pos = s.find(from, pos)) != std::string::npos) {
                    s.replace(pos, from.size(), to);
                    pos += to.size();
                }
                return s;
            };
            line = replaceAll(line, "varying ",      "in ");
            line = replaceAll(line, "gl_FragColor",  "fragColor");
            line = replaceAll(line, "texture2D(",    "texture(");
            line = replaceAll(line, "texture2DLod(", "textureLod(");
        }
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
    // 现代 GLSL：用 in/out 和 layout qualifier
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块 ==\n"
        "layout(location = 0) in vec2 offset;\n"
        "uniform vec2 dims;\n"
        "uniform vec2 insize;\n"
        "// RetroArch 标准属性的别名函数\n"
        "vec2 _bkVertexCoord() { return vec2(offset.x * 2.0 - 1.0, offset.y * 2.0 - 1.0); }\n"
        "vec2 _bkTexCoord()    { return offset * insize; }\n"
        "#define VertexCoord _bkVertexCoord()\n"
        "#define TexCoord    _bkTexCoord()\n"
        "#define MVPMatrix   mat4(1.0)\n";
#else
    // 旧版 GLSL：attribute / varying 语法
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块 ==\n"
        "attribute vec2 offset;\n"
        "uniform vec2 dims;\n"
        "uniform vec2 insize;\n"
        "vec2 _bkVertexCoord() { return vec2(offset.x * 2.0 - 1.0, offset.y * 2.0 - 1.0); }\n"
        "vec2 _bkTexCoord()    { return offset * insize; }\n"
        "#define VertexCoord _bkVertexCoord()\n"
        "#define TexCoord    _bkTexCoord()\n"
        "#define MVPMatrix   mat4(1.0)\n";
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
        "// == BeikLive RetroArch 兼容块 ==\n"
        "uniform sampler2D tex;\n"
        "out vec4 fragColor;\n"
        "#define Texture tex\n"
        "#define Source  tex\n"
        // RetroArch 着色器在 GL3 路径中将输出定义为 FragColor（大写 F），
        // 映射到我们的 fragColor 输出变量，避免重复声明冲突。
        "#define FragColor fragColor\n"
        "// gl_FragColor → fragColor（已在 preprocessFragSource 中替换）\n";
#else
    static const char* kCompat =
        "// == BeikLive RetroArch 兼容块 ==\n"
        "uniform sampler2D tex;\n"
        "#define Texture tex\n"
        "#define Source  tex\n"
        // GL2/GLES2 模式：FragColor 保持使用 gl_FragColor 内置变量
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
        std::ostringstream vss, fss;
        std::string line;

        bool inStage   = false;   // 是否在某个已识别的 VERTEX/FRAGMENT 块中
        bool inVert    = false;
        bool inFrag    = false;
        int  nestedDepth = 0;     // 当前阶段内部嵌套 #if 的深度

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
                    continue;
                }
                if (isFragMarker(t)) {
                    inStage = true; inFrag = true; inVert = false; nestedDepth = 0;
                    continue;
                }
                // 阶段外的行（前导注释、#pragma parameter 等）直接忽略
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

        vertBody = vss.str();
        fragBody = fss.str();
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
            "    gl_Position = MVPMatrix * vec4(VertexCoord, 0.0, 1.0);\n"
            "    vTexCoord = TexCoord;\n"
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
    std::string vert, frag;
    if (!parseGlslFile(path, vert, frag)) return false;

    if (!chain.addPass(vert, frag)) {
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

    // 解析 key=value 行
    std::unordered_map<std::string, std::string> kv;
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

    // 读取着色器数量
    int shaderCount = 0;
    {
        auto it = kv.find("shaders");
        if (it == kv.end()) {
            brls::Logger::error("[GlslpLoader] Preset missing 'shaders' key: {}", path);
            return false;
        }
        try {
            shaderCount = std::stoi(it->second);
        } catch (const std::exception& e) {
            brls::Logger::error("[GlslpLoader] Preset 'shaders' value '{}' is invalid: {}",
                                it->second, e.what());
            return false;
        }
    }
    if (shaderCount <= 0) {
        brls::Logger::error("[GlslpLoader] Preset shaders={} is not positive: {}",
                            shaderCount, path);
        return false;
    }

    // 清除现有用户通道，重新加载
    chain.clearPasses();

    int loaded = 0;
    for (int i = 0; i < shaderCount; ++i) {
        std::string shaderKey = "shader" + std::to_string(i);
        auto it = kv.find(shaderKey);
        if (it == kv.end() || it->second.empty()) {
            brls::Logger::warning("[GlslpLoader] Missing {} in preset", shaderKey);
            continue;
        }
        std::string shaderPath = resolvePath(baseDir, it->second);

        // 读取该通道的过滤设置（可选）
        std::string filterKey = "filter_linear" + std::to_string(i);
        auto fit = kv.find(filterKey);
        if (fit != kv.end()) {
            bool linear = (fit->second == "true" || fit->second == "1");
            chain.setFilter(linear ? GL_LINEAR : GL_NEAREST);
        }

        if (loadGlslIntoChain(chain, shaderPath))
            ++loaded;
        else
            brls::Logger::warning("[GlslpLoader] Failed to load {}", shaderPath);
    }

    if (loaded > 0) {
        brls::Logger::info("[GlslpLoader] Loaded {}/{} passes from: {}",
                           loaded, shaderCount, path);
        return true;
    }
    return false;
}

} // namespace beiklive
