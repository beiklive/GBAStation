#include "Game/GlslpLoader.hpp"
#include "Game/ShaderChain.hpp"

// ---- NanoVG 后端检测 ----
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

#include <borealis.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace beiklive {

// ============================================================
// 工具函数
// ============================================================

static std::string readFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string trimStr(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string GlslpLoader::resolvePath(const std::string& baseDir,
                                      const std::string& rel)
{
    try {
        std::filesystem::path p =
            std::filesystem::path(baseDir) / std::filesystem::path(rel);
        return p.lexically_normal().string();
    } catch (...) {
        return rel;
    }
}

// ============================================================
// #pragma parameter 行提取（用于 ShaderParamDef 列表）
// ============================================================

static std::vector<ShaderParamDef> extractParamDefs(const std::string& src)
{
    std::vector<ShaderParamDef> defs;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trimStr(line);
        if (t.rfind("#pragma parameter", 0) != 0) continue;
        // 格式：#pragma parameter NAME "DESC" DEFAULT MIN MAX STEP
        std::istringstream ps(t.substr(17)); // skip "#pragma parameter"
        ShaderParamDef d;
        ps >> d.name;
        if (d.name.empty()) continue;
        // 读取描述字符串（可能带引号）
        std::string word;
        ps >> std::ws;
        if (ps.peek() == '"') {
            std::getline(ps, d.desc, '"'); // skip leading "
            std::getline(ps, d.desc, '"');
        } else {
            ps >> d.desc;
        }
        // 读取数值参数
        float def = 0.f, mn = 0.f, mx = 1.f, step = 0.1f;
        ps >> def >> mn >> mx >> step;
        d.defaultVal = def;
        d.minVal     = mn;
        d.maxVal     = mx;
        d.step       = step;
        d.currentVal = def;
        defs.push_back(d);
    }
    return defs;
}

// ============================================================
// #pragma parameter 行去除（仅用空格替换，保留行号）
// 参考 RetroArch shader_glsl.c: gl_glsl_strip_parameter_pragmas()
// ============================================================

static void stripPragmaParameters(std::string& src)
{
    const char* marker = "#pragma parameter";
    size_t pos = 0;
    while ((pos = src.find(marker, pos)) != std::string::npos) {
        // 找到行尾，将整行替换为空格（保留换行符，以保留行号）
        size_t end = src.find('\n', pos);
        size_t replEnd = (end == std::string::npos) ? src.size() : end;
        for (size_t i = pos; i < replEnd; ++i)
            src[i] = ' ';
        pos = replEnd;
    }
}

// ============================================================
// 计算目标 GLSL 版本字符串
// 参考 RetroArch shader_glsl.c: gl_glsl_compile_shader()
//
// vno = 0 表示着色器中无 #version 行
// ============================================================

static std::string computeVersionStr(unsigned vno)
{
#if defined(NANOVG_GLES3) || defined(NANOVG_GLES2)
    // GLES：旧着色器（< 300）使用 GLSL ES 1.00，支持 attribute/varying
    // 与 RetroArch 对 GLES3 的处理一致：version < 130 → #version 100
    if (vno >= 300)
        return "#version 300 es\nprecision mediump float;\n";
    return "#version 100\nprecision mediump float;\n";

#elif defined(NANOVG_GL3)
    // 桌面 GL3：统一使用 #version 330 core
    // 着色器通过 #if __VERSION__ >= 130 的 COMPAT_* 宏自动适配新旧语法
    (void)vno;
    return "#version 330 core\n";

#else  // GL2
    if (vno == 0 || vno < 120) return "#version 120\n";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "#version %u\n", vno < 120 ? 120u : vno);
    return buf;
#endif
}

// ============================================================
// 检测 GLSL 版本字符串是否使用现代语法（in/out，无 attribute/varying）
// ============================================================

static bool isModernGlsl()
{
#if defined(NANOVG_GL3)
    return true;   // #version 330 core：必须使用 in/out
#elif defined(NANOVG_GLES3) || defined(NANOVG_GLES2)
    return false;  // #version 100：attribute/varying 完全支持
#else
    return false;  // GL2：attribute/varying 完全支持
#endif
}

// ============================================================
// 文本替换辅助
// ============================================================

static void replaceAll(std::string& s,
                       const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

// ============================================================
// #pragma stage vertex/fragment 格式（自定义简化格式）
// 为这类着色器构建顶点/片段源码。
//
// 此格式依赖宿主提供 VertexCoord/TexCoord/COLOR/MVPMatrix 语义，
// 因此需要注入兼容头部声明。
// ============================================================

static std::string buildPragmaStageSrc(const std::string& rawBody, bool isVertex)
{
    // Step 1：去除 #pragma parameter 行
    std::string body = rawBody;
    stripPragmaParameters(body);

    std::string versionStr = computeVersionStr(0);  // 使用平台默认版本
    bool modern = isModernGlsl();

    std::string result = versionStr;

    if (isVertex) {
        // 顶点阶段：注入 VertexCoord/TexCoord/COLOR/MVPMatrix 声明
        if (modern) {
            // GL3：in/out 语法
            result +=
                "#define PARAMETER_UNIFORM\n"
                "in vec4 VertexCoord;\n"
                "in vec2 TexCoord;\n"
                "in vec4 COLOR;\n"
                "#define MVPMatrix mat4(1.0)\n";
            // 将 body 中的 varying → out（顶点输出）
            replaceAll(body, "varying ", "out ");
        } else {
            // GLES2/GL2：attribute/varying 语法
            result +=
                "#define PARAMETER_UNIFORM\n"
                "attribute vec4 VertexCoord;\n"
                "attribute vec2 TexCoord;\n"
                "attribute vec4 COLOR;\n"
                "#define MVPMatrix mat4(1.0)\n";
        }
        result += body;
    } else {
        // 片段阶段：注入 Texture 采样器及输出变量声明
        if (modern) {
            // GL3：out vec4 fragColor，映射 gl_FragColor/texture2D
            result +=
                "#define PARAMETER_UNIFORM\n"
                "uniform sampler2D Texture;\n"
                "out vec4 fragColor;\n";
            // body 中的 varying → in（片段输入），gl_FragColor → fragColor
            replaceAll(body, "varying ",     "in ");
            replaceAll(body, "gl_FragColor", "fragColor");
            replaceAll(body, "texture2D(",   "texture(");
            replaceAll(body, "texture2DLod(","textureLod(");
        } else {
            // GLES2/GL2：保留 varying/gl_FragColor/texture2D
            result +=
                "#define PARAMETER_UNIFORM\n"
                "uniform sampler2D Texture;\n";
        }
        result += body;
    }

    return result;
}

// ============================================================
// RetroArch 格式（#if defined(VERTEX)/#elif defined(FRAGMENT)）
// 移植自 RetroArch shader_glsl.c: gl_glsl_compile_shader()
//
// 策略：
//  1. 提取并剥除 #version 行（使用 strtoul 跳过）
//  2. 计算目标版本字符串
//  3. 组装：[version] + [stage_define + PARAMETER_UNIFORM] + [source_after_version]
//
// 不做任何 attribute/varying 文本转换——由着色器自身的
// "#if __VERSION__ >= 130" + COMPAT_* 宏完成兼容性处理。
// ============================================================

static std::string buildRetroArchSrc(const std::string& source, bool isVertex)
{
    const char* src = source.c_str();

    // 找到 #version 行，提取版本号，并跳过该行
    const char* versionPtr = std::strstr(src, "#version");
    unsigned    vno        = 0;
    const char* afterVer   = src;

    if (versionPtr) {
        char* endPtr = nullptr;
        vno = static_cast<unsigned>(std::strtoul(versionPtr + 8, &endPtr, 10));
        // 跳到 #version 行尾的换行符之后
        afterVer = endPtr ? endPtr : versionPtr + 8;
        while (*afterVer && *afterVer != '\n') ++afterVer;
        if (*afterVer == '\n') ++afterVer;
    }

    std::string versionStr = computeVersionStr(vno);

    // 阶段定义块（与 RetroArch 完全一致）
    // 参考 shader_glsl.c::gl_glsl_compile_program():
    //   "#define VERTEX\n#define PARAMETER_UNIFORM\n
    //    #define _HAS_ORIGINALASPECT_UNIFORMS\n#define _HAS_FRAMETIME_UNIFORMS\n"
    std::string defineStr = isVertex
        ? "#define VERTEX\n#define PARAMETER_UNIFORM\n"
          "#define _HAS_ORIGINALASPECT_UNIFORMS\n#define _HAS_FRAMETIME_UNIFORMS\n"
        : "#define FRAGMENT\n#define PARAMETER_UNIFORM\n"
          "#define _HAS_ORIGINALASPECT_UNIFORMS\n#define _HAS_FRAMETIME_UNIFORMS\n";

    return versionStr + defineStr + afterVer;
}

// ============================================================
// GlslpLoader::parseGlslFile
// 将 .glsl 文件解析为独立的顶点/片段 GLSL 源码字符串。
//
// 支持两种格式：
//   A. RetroArch 标准格式：#if defined(VERTEX) / #elif defined(FRAGMENT)
//      → 使用 buildRetroArchSrc（仅前置 #define VERTEX/#define FRAGMENT）
//   B. 自定义简化格式：#pragma stage vertex / #pragma stage fragment
//      → 使用 buildPragmaStageSrc（注入声明兼容层）
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

    // ---- 格式 A：RetroArch #if defined(VERTEX)/#elif defined(FRAGMENT) ----
    // 检测是否有 VERTEX 阶段标记
    bool hasIfVertex    = (src.find("#if defined(VERTEX)")  != std::string::npos ||
                           src.find("#ifdef VERTEX")        != std::string::npos ||
                           src.find("#elif defined(VERTEX)")!= std::string::npos);
    bool hasIfFragment  = (src.find("#if defined(FRAGMENT)")  != std::string::npos ||
                           src.find("#ifdef FRAGMENT")         != std::string::npos ||
                           src.find("#elif defined(FRAGMENT)") != std::string::npos);

    if (hasIfVertex && hasIfFragment) {
        // 纯 RetroArch 格式：整个文件作为源，各编译一次
        // #pragma parameter 行由 stripPragmaParameters 去除（保留行号）
        std::string stripped = src;
        stripPragmaParameters(stripped);
        outVert = buildRetroArchSrc(stripped, true);
        outFrag = buildRetroArchSrc(stripped, false);
        return true;
    }

    // ---- 格式 B：#pragma stage vertex/fragment ----
    {
        const std::string kStage = "#pragma stage";
        size_t vStart = std::string::npos;
        size_t fStart = std::string::npos;

        size_t pos = 0;
        while ((pos = src.find(kStage, pos)) != std::string::npos) {
            size_t eol = src.find('\n', pos);
            std::string tag = (eol == std::string::npos)
                              ? src.substr(pos)
                              : src.substr(pos, eol - pos);
            if (tag.find("vertex") != std::string::npos)
                vStart = (eol == std::string::npos) ? src.size() : eol + 1;
            else if (tag.find("fragment") != std::string::npos)
                fStart = (eol == std::string::npos) ? src.size() : eol + 1;
            pos += kStage.size();
        }

        if (vStart != std::string::npos && fStart != std::string::npos) {
            std::string vertBody, fragBody;
            if (vStart < fStart) {
                size_t fragPragmaLine = src.rfind(kStage, fStart - 1);
                if (fragPragmaLine == std::string::npos) fragPragmaLine = fStart;
                vertBody = src.substr(vStart, fragPragmaLine - vStart);
                fragBody = src.substr(fStart);
            } else {
                size_t vertPragmaLine = src.rfind(kStage, vStart - 1);
                if (vertPragmaLine == std::string::npos) vertPragmaLine = vStart;
                fragBody = src.substr(fStart, vertPragmaLine - fStart);
                vertBody = src.substr(vStart);
            }
            outVert = buildPragmaStageSrc(vertBody, true);
            outFrag = buildPragmaStageSrc(fragBody, false);
            return true;
        }
    }

    // ---- 格式 C：无阶段标记，视为纯片段着色器 ----
    brls::Logger::warning("[GlslpLoader] No stage markers in {}, "
                          "treating as fragment-only shader", path);
    {
        // 生成默认直通顶点着色器
        bool modern = isModernGlsl();
        std::string versionStr = computeVersionStr(0);
        std::string defaultVert = versionStr;
        if (modern) {
            defaultVert +=
                "in vec4 VertexCoord;\n"
                "in vec2 TexCoord;\n"
                "in vec4 COLOR;\n"
                "out vec2 vTexCoord;\n"
                "void main() {\n"
                "    gl_Position = VertexCoord;\n"
                "    vTexCoord = TexCoord;\n"
                "}\n";
        } else {
            defaultVert +=
                "attribute vec4 VertexCoord;\n"
                "attribute vec2 TexCoord;\n"
                "attribute vec4 COLOR;\n"
                "varying vec2 vTexCoord;\n"
                "void main() {\n"
                "    gl_Position = VertexCoord;\n"
                "    vTexCoord = TexCoord;\n"
                "}\n";
        }
        outVert = defaultVert;

        // 片段：检测是否有 RetroArch 结构
        std::string stripped = src;
        stripPragmaParameters(stripped);
        outFrag = buildRetroArchSrc(stripped, false);
        return true;
    }
}

// ============================================================
// GlslpLoader::loadGlslIntoChain
// ============================================================

bool GlslpLoader::loadGlslIntoChain(ShaderChain& chain, const std::string& path)
{
    // 读取原始源码以提取 #pragma parameter 定义
    std::string rawSrc = readFile(path);
    auto paramDefs = extractParamDefs(rawSrc);

    std::unordered_map<std::string, float> paramDefaults;
    for (const auto& d : paramDefs)
        paramDefaults[d.name] = d.currentVal;

    std::string vert, frag;
    if (!parseGlslFile(path, vert, frag)) return false;

    if (!chain.addPass(vert, frag,
                       GL_NEAREST, GL_CLAMP_TO_EDGE, ShaderPassScale{}, 0, {},
                       paramDefaults)) {
        brls::Logger::error("[GlslpLoader] Failed to compile pass from: {}", path);
        return false;
    }

    chain.setParamDefs(paramDefs);
    brls::Logger::info("[GlslpLoader] Loaded GLSL pass: {}", path);
    return true;
}

// ============================================================
// GlslpLoader::loadGlslpIntoChain
// 移植自 RetroArch video_shader_parse.c: video_shader_parse()
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

    // ---- 检查 #reference 指令 ----
    {
        std::istringstream refiss(src);
        std::string refLine;
        while (std::getline(refiss, refLine)) {
            std::string t = trimStr(refLine);
            if (t.rfind("#reference", 0) == 0) {
                size_t q1 = t.find('"');
                size_t q2 = (q1 != std::string::npos) ? t.find('"', q1 + 1) : std::string::npos;
                std::string refPath;
                if (q1 != std::string::npos && q2 != std::string::npos)
                    refPath = t.substr(q1 + 1, q2 - q1 - 1);
                else {
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

    // 解析 key=value 行
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
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
                v = v.substr(1, v.size() - 2);
            kv[k] = v;
        }
    }

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

    int shaderCount = kvInt("shaders", -1);
    if (shaderCount <= 0) {
        brls::Logger::error("[GlslpLoader] Preset missing/invalid 'shaders': {}", path);
        return false;
    }

    chain.clearPasses();

    // ---- 解析全局参数覆盖值 ----
    std::unordered_map<std::string, float> globalParamOverrides;
    {
        auto paramIt = kv.find("parameters");
        if (paramIt != kv.end() && !paramIt->second.empty()) {
            std::string paramList = paramIt->second;
            if (paramList.size() >= 2 && paramList.front() == '"' && paramList.back() == '"')
                paramList = paramList.substr(1, paramList.size() - 2);
            std::replace(paramList.begin(), paramList.end(), ';', ' ');
            std::istringstream piss(paramList);
            std::string pname;
            while (piss >> pname) {
                auto pit = kv.find(pname);
                if (pit != kv.end()) {
                    try { globalParamOverrides[pname] = std::stof(pit->second); }
                    catch (...) {}
                }
            }
        }
    }

    // ---- 解析 LUT 纹理 ----
    {
        auto texIt = kv.find("textures");
        if (texIt != kv.end() && !texIt->second.empty()) {
            std::string texList = texIt->second;
            std::replace(texList.begin(), texList.end(), ';', ' ');
            std::replace(texList.begin(), texList.end(), ',', ' ');
            std::istringstream tiss(texList);
            std::string texId;
            while (tiss >> texId) {
                auto pathIt = kv.find(texId + "_path");
                if (pathIt == kv.end() || pathIt->second.empty()) continue;
                std::string lutPath = resolvePath(baseDir, pathIt->second);
                bool linear  = kvBool(texId + "_linear",  false);
                bool mipmap  = kvBool(texId + "_mipmap",  false);
                std::string wrapStr = kv.count(texId + "_wrap_mode")
                                      ? kv[texId + "_wrap_mode"] : "clamp_to_edge";
                GLenum wrap = GL_CLAMP_TO_EDGE;
                if (wrapStr == "repeat")              wrap = GL_REPEAT;
                else if (wrapStr == "mirrored_repeat") wrap = GL_MIRRORED_REPEAT;
                chain.addLut(texId, lutPath, linear, mipmap, wrap);
            }
        }
    }

    // ---- 第一轮：收集所有通道的 #pragma parameter 定义 ----
    std::vector<ShaderParamDef> allParamDefs;
    std::unordered_set<std::string> seenParams;
    std::unordered_map<std::string, float> allParamDefaults;
    for (int i = 0; i < shaderCount; ++i) {
        auto it = kv.find("shader" + std::to_string(i));
        if (it == kv.end() || it->second.empty()) continue;
        std::string shaderPath = resolvePath(baseDir, it->second);
        std::string rawSrc = readFile(shaderPath);
        if (rawSrc.empty()) continue;
        for (const auto& d : extractParamDefs(rawSrc)) {
            allParamDefaults[d.name] = d.defaultVal;
            if (seenParams.insert(d.name).second)
                allParamDefs.push_back(d);
        }
    }
    // 应用全局预设覆盖值
    for (const auto& ov : globalParamOverrides) {
        allParamDefaults[ov.first] = ov.second;
        for (auto& def : allParamDefs)
            if (def.name == ov.first) { def.currentVal = ov.second; break; }
    }
    for (const auto& def : allParamDefs)
        allParamDefaults[def.name] = def.currentVal;

    // ---- 第二轮：加载每个着色器通道 ----
    int loaded = 0;
    for (int i = 0; i < shaderCount; ++i) {
        std::string siStr = std::to_string(i);
        auto it = kv.find("shader" + siStr);
        if (it == kv.end() || it->second.empty()) {
            brls::Logger::warning("[GlslpLoader] Missing shader{} in preset", i);
            continue;
        }
        std::string shaderPath = resolvePath(baseDir, it->second);

        bool linear = kvBool("filter_linear" + siStr, false);
        GLenum glFilter = linear ? GL_LINEAR : GL_NEAREST;

        std::string wrapStr = kv.count("wrap_mode" + siStr)
                              ? kv["wrap_mode" + siStr] : "clamp_to_edge";
        GLenum wrapMode = GL_CLAMP_TO_EDGE;
        if (wrapStr == "repeat")               wrapMode = GL_REPEAT;
        else if (wrapStr == "mirrored_repeat") wrapMode = GL_MIRRORED_REPEAT;

        // Scale 参数
        std::string scaleTypeStr  = kv.count("scale_type"   + siStr) ? kv["scale_type"   + siStr] : "";
        std::string scaleTypeXStr = kv.count("scale_type_x" + siStr) ? kv["scale_type_x" + siStr] : scaleTypeStr;
        std::string scaleTypeYStr = kv.count("scale_type_y" + siStr) ? kv["scale_type_y" + siStr] : scaleTypeStr;
        float scaleXf = kvFloat("scale_x" + siStr, kvFloat("scale" + siStr, 1.0f));
        float scaleYf = kvFloat("scale_y" + siStr, kvFloat("scale" + siStr, 1.0f));
        int   absX    = kvInt("scale_x" + siStr, kvInt("scale" + siStr, 0));
        int   absY    = kvInt("scale_y" + siStr, kvInt("scale" + siStr, 0));

        ShaderPassScale scale;
        scale.scaleX = scaleXf;
        scale.scaleY = scaleYf;
        scale.absX   = absX;
        scale.absY   = absY;

        auto parseScaleType = [](const std::string& s) {
            if (s == "absolute") return ShaderPassScale::SCALE_ABSOLUTE;
            if (s == "viewport") return ShaderPassScale::VIEWPORT;
            return ShaderPassScale::SOURCE;
        };
        scale.typeX = scaleTypeXStr.empty() ? ShaderPassScale::SOURCE : parseScaleType(scaleTypeXStr);
        scale.typeY = scaleTypeYStr.empty() ? ShaderPassScale::SOURCE : parseScaleType(scaleTypeYStr);

        int fcMod = kvInt("frame_count_mod" + siStr, 0);
        std::string alias = kv.count("alias" + siStr) ? kv["alias" + siStr] : "";

        std::string vert, frag;
        if (!parseGlslFile(shaderPath, vert, frag)) {
            brls::Logger::warning("[GlslpLoader] Failed to parse {}", shaderPath);
            continue;
        }
        if (!chain.addPass(vert, frag, glFilter, wrapMode, scale, fcMod, alias,
                           allParamDefaults)) {
            brls::Logger::warning("[GlslpLoader] Failed to compile {}", shaderPath);
            continue;
        }
        brls::Logger::info("[GlslpLoader] Loaded pass {}: {}", i, shaderPath);
        ++loaded;
    }

    if (loaded > 0) {
        brls::Logger::info("[GlslpLoader] Loaded {}/{} passes from: {}",
                           loaded, shaderCount, path);
        chain.setParamDefs(allParamDefs);
        return true;
    }
    return false;
}

} // namespace beiklive
