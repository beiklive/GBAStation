#include "Game/ShaderChain.hpp"

// ---- NanoVG 后端检测（必须在 nanovg_gl.h 之前引入）----
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

#include <borealis.hpp>  // brls::Logger
#include <cstring>

// stb_image 声明
#include <borealis/extern/nanovg/stb_image.h>

// OrigTexture 固定纹理单元偏移量
static constexpr int k_origTexUnitOffset = 32;
static constexpr int k_maxFrameHistory   = 6;
static constexpr GLint k_defaultUniformNameLen = 64;

// ============================================================
// 着色器调试日志开关（独立于其他功能调试，默认关闭）
//
// 用途：帮助排查着色器相关问题（如全白画面、坐标错误等）。
// 开关：通过 ShaderChain::setDebugLog(true) 或配置项
//       render.shaderDebugLog=true 开启。
// 节流：run() 中的逐帧日志仅在前 3 帧 + 每 300 帧输出一次，
//       避免海量日志影响其他功能调试。
// ============================================================
static bool s_shaderDebugLog = false;

/// 条件调试日志宏：只在 s_shaderDebugLog 为 true 时触发，
/// 开关关闭时零开销（不求值参数）。
#define SC_DLOG(...) do { if (s_shaderDebugLog) brls::Logger::debug(__VA_ARGS__); } while(0)

/// 将 C 字符串截断为最多 maxLen 个字符，超出部分追加省略提示。
static std::string sc_truncate(const char* s, size_t maxLen = 800)
{
    if (!s) return "(null)";
    size_t n = std::strlen(s);
    if (n <= maxLen) return std::string(s, n);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "...[%zu more chars]", n - maxLen);
    std::string result;
    result.reserve(maxLen + 64);
    result.append(s, maxLen);
    result.append(buf);
    return result;
}

// RetroArch 兼容 MVP 单位矩阵（列主序）
// 顶点坐标已为 NDC，MVPMatrix 设为单位矩阵使 gl_Position = VertexCoord 直通
static constexpr GLfloat k_mvpIdentity[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
};

// ============================================================
// 顶点缓冲区布局（RetroArch 兼容）
// 每顶点 10 个 float：VertexCoord(4) + TexCoord(2) + COLOR(4)
// 全屏四边形，TRIANGLE_FAN，逆时针绕序
// ============================================================
static constexpr int k_vertStride = 10;  // floats per vertex

static const GLfloat k_quadVerts[] = {
    // VertexCoord (vec4 NDC)      TexCoord (vec2 UV)  COLOR (vec4)
    -1.0f, -1.0f, 0.0f, 1.0f,   0.0f, 0.0f,         1.0f, 1.0f, 1.0f, 1.0f,  // BL
     1.0f, -1.0f, 0.0f, 1.0f,   1.0f, 0.0f,         1.0f, 1.0f, 1.0f, 1.0f,  // BR
     1.0f,  1.0f, 0.0f, 1.0f,   1.0f, 1.0f,         1.0f, 1.0f, 1.0f, 1.0f,  // TR
    -1.0f,  1.0f, 0.0f, 1.0f,   0.0f, 1.0f,         1.0f, 1.0f, 1.0f, 1.0f,  // TL
};

// ============================================================
// 内置直通着色器（RetroArch 兼容顶点格式）
// 使用 VertexCoord / TexCoord / Texture 标准命名
// ============================================================

#if defined(NANOVG_GLES3) || defined(NANOVG_GL3)

static const char* const k_builtinVert =
#  if defined(NANOVG_GLES3)
    "#version 300 es\n"
    "precision mediump float;\n"
#  else
    "#version 330 core\n"
#  endif
    "in vec4 VertexCoord;\n"
    "in vec2 TexCoord;\n"
    "in vec4 COLOR;\n"
    "out vec2 vTexCoord;\n"
    "void main() {\n"
    "    gl_Position = VertexCoord;\n"
    "    vTexCoord   = TexCoord;\n"
    "}\n";

static const char* const k_builtinFrag =
#  if defined(NANOVG_GLES3)
    "#version 300 es\n"
    "precision mediump float;\n"
#  else
    "#version 330 core\n"
#  endif
    "uniform sampler2D Texture;\n"
    "in  vec2 vTexCoord;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = vec4(texture(Texture, vTexCoord).rgb, 1.0);\n"
    "}\n";

#else  // GL2 / GLES2

static const char* const k_builtinVert =
#  if defined(NANOVG_GLES2)
    "#version 100\n"
    "precision mediump float;\n"
#  else
    "#version 120\n"
#  endif
    "attribute vec4 VertexCoord;\n"
    "attribute vec2 TexCoord;\n"
    "attribute vec4 COLOR;\n"
    "varying vec2 vTexCoord;\n"
    "void main() {\n"
    "    gl_Position = VertexCoord;\n"
    "    vTexCoord   = TexCoord;\n"
    "}\n";

static const char* const k_builtinFrag =
#  if defined(NANOVG_GLES2)
    "#version 100\n"
    "precision mediump float;\n"
#  else
    "#version 120\n"
#  endif
    "uniform sampler2D Texture;\n"
    "varying vec2 vTexCoord;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(texture2D(Texture, vTexCoord).rgb, 1.0);\n"
    "}\n";

#endif

namespace beiklive {

static GLenum lookupUniformType(GLuint program, const char* name)
{
    GLint numUniforms = 0;
    GLint maxNameLen  = k_defaultUniformNameLen;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS,           &numUniforms);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLen);
    if (numUniforms <= 0 || maxNameLen <= 0)
        return 0;

    std::vector<char> unameBuf(static_cast<size_t>(maxNameLen + 1), '\0');
    for (GLint i = 0; i < numUniforms; ++i) {
        GLint  usize = 0;
        GLenum utype = 0;
        std::fill(unameBuf.begin(), unameBuf.end(), '\0');
        glGetActiveUniform(program, (GLuint)i,
                           (GLsizei)unameBuf.size() - 1,
                           nullptr, &usize, &utype, unameBuf.data());
        if (std::strcmp(unameBuf.data(), name) == 0)
            return utype;
    }
    return 0;
}

static void lookupSizeUniform(GLuint program, const char* name,
                              GLint& loc, bool& isVec2)
{
    loc = glGetUniformLocation(program, name);
    isVec2 = false;
    if (loc >= 0)
        isVec2 = (lookupUniformType(program, name) == GL_FLOAT_VEC2);
}

// ============================================================
// ShaderChain：查询单个通道的所有 uniform 位置
// ============================================================
void ShaderChain::_lookupUniforms(ShaderPass& p)
{
    // Source / Texture 采样器
    p.sourceLoc = glGetUniformLocation(p.program, "Source");
    if (p.sourceLoc < 0)
        p.sourceLoc = glGetUniformLocation(p.program, "Texture");

    // SourceSize（vec4 或 vec2 TextureSize）
    lookupSizeUniform(p.program, "SourceSize", p.sourceSizeLoc, p.sourceSizeIsVec2);
    if (p.sourceSizeLoc < 0) {
        lookupSizeUniform(p.program, "TextureSize", p.sourceSizeLoc, p.sourceSizeIsVec2);
    }

    // OutputSize
    lookupSizeUniform(p.program, "OutputSize", p.outputSizeLoc, p.outputSizeIsVec2);

    // InputSize
    lookupSizeUniform(p.program, "InputSize", p.inputSizeLoc, p.inputSizeIsVec2);

    p.frameCountLoc  = glGetUniformLocation(p.program, "FrameCount");
    p.frameDirectionLoc = glGetUniformLocation(p.program, "FrameDirection");
    p.finalVpSizeLoc = glGetUniformLocation(p.program, "FinalViewportSize");
    p.origTexLoc     = glGetUniformLocation(p.program, "OrigTexture");
    p.origInputSizeLoc = glGetUniformLocation(p.program, "OrigInputSize");
    p.mvpMatrixLoc   = glGetUniformLocation(p.program, "MVPMatrix");

    SC_DLOG("[ShaderChain::_lookupUniforms] prog={}: "
            "source={} sourceSize={}(vec2={}) outputSize={}(vec2={}) "
            "inputSize={}(vec2={}) frameCount={} frameDir={} mvpMatrix={} "
            "finalVpSize={} origTex={} origInputSize={}",
            p.program,
            p.sourceLoc, p.sourceSizeLoc, p.sourceSizeIsVec2,
            p.outputSizeLoc, p.outputSizeIsVec2,
            p.inputSizeLoc, p.inputSizeIsVec2,
            p.frameCountLoc, p.frameDirectionLoc, p.mvpMatrixLoc,
            p.finalVpSizeLoc, p.origTexLoc, p.origInputSizeLoc);
}

// ============================================================
// ShaderChain：查询 LUT uniform 位置
// ============================================================
void ShaderChain::_lookupLutUniforms(ShaderPass& p)
{
    p.lutLocs.resize(m_luts.size(), -1);
    for (size_t i = 0; i < m_luts.size(); ++i)
        p.lutLocs[i] = glGetUniformLocation(p.program, m_luts[i].id.c_str());
}

// ============================================================
// ShaderChain：查询前序通道纹理 uniform 位置
// ============================================================
void ShaderChain::_lookupPassTextures(ShaderPass& p, int passIdx)
{
    int unitOffset = 0;
    auto addBinding = [&](const std::string& textureName,
                          ShaderPass::PassTexBinding::Kind kind,
                          int srcPassIdx,
                          int historyIndex) {
        GLint loc = glGetUniformLocation(p.program, textureName.c_str());
        if (loc < 0) return;

        int reusedOffset = -1;
        for (const auto& b : p.passTexBindings) {
            if (b.kind == kind &&
                b.srcPassIdx == srcPassIdx &&
                b.historyIndex == historyIndex &&
                b.texUnitOffset >= 0) {
                reusedOffset = b.texUnitOffset;
                break;
            }
        }

        ShaderPass::PassTexBinding binding;
        binding.kind         = kind;
        binding.srcPassIdx   = srcPassIdx;
        binding.historyIndex = historyIndex;
        binding.loc          = loc;
        binding.texUnitOffset = (reusedOffset >= 0) ? reusedOffset : unitOffset++;

        constexpr const char* kTextureSuffix = "Texture";
        constexpr size_t kTextureSuffixLen = sizeof("Texture") - 1;
        if (textureName.size() >= kTextureSuffixLen &&
            textureName.compare(textureName.size() - kTextureSuffixLen,
                                kTextureSuffixLen, kTextureSuffix) == 0) {
            const std::string stem =
                textureName.substr(0, textureName.size() - kTextureSuffixLen);
            const std::string textureSizeName = stem + "TextureSize";
            const std::string inputSizeName   = stem + "InputSize";
            lookupSizeUniform(p.program, textureSizeName.c_str(),
                              binding.textureSizeLoc, binding.textureSizeIsVec2);
            lookupSizeUniform(p.program, inputSizeName.c_str(),
                              binding.inputSizeLoc, binding.inputSizeIsVec2);
        }
        p.passTexBindings.push_back(binding);
    };

    // PassNTexture：N 从 0 到 passIdx-2
    for (int n = 0; n < passIdx - 1; ++n) {
        addBinding("Pass" + std::to_string(n) + "Texture",
                   ShaderPass::PassTexBinding::Kind::PassOutput,
                   n + 1, -1);
    }

    // PassPrevNTexture：相对当前通道回溯到更早的通道输出
    for (int prev = 1; prev < passIdx; ++prev) {
        const int srcPassIdx = passIdx - prev;
        if (prev == 1) {
            addBinding("PassPrevTexture",
                       ShaderPass::PassTexBinding::Kind::PassOutput,
                       srcPassIdx, -1);
        }
        addBinding("PassPrev" + std::to_string(prev) + "Texture",
                   ShaderPass::PassTexBinding::Kind::PassOutput,
                   srcPassIdx, -1);
    }

    // {alias}Texture
    for (int n = 1; n < passIdx; ++n) {
        if (m_passes[n].alias.empty()) continue;
        addBinding(m_passes[n].alias + "Texture",
                   ShaderPass::PassTexBinding::Kind::PassOutput,
                   n, -1);
    }

    // PrevNTexture：上一帧/更早帧的原始输入（由 pass0 输出快照维护）
    addBinding("PrevTexture",
               ShaderPass::PassTexBinding::Kind::FrameHistory,
               -1, 0);
    for (int n = 1; n <= k_maxFrameHistory; ++n) {
        addBinding("Prev" + std::to_string(n) + "Texture",
                   ShaderPass::PassTexBinding::Kind::FrameHistory,
                   -1, n - 1);
    }
}

// ============================================================
// ShaderChain：构建第 0 通道及四边形几何体
// ============================================================
bool ShaderChain::init(const std::string& vertSrc, const std::string& fragSrc)
{
    // ---- 全屏四边形 VAO / VBO（RetroArch 兼容顶点格式）----
    glGenBuffers(1, &m_vbo);
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quadVerts), k_quadVerts, GL_STATIC_DRAW);

    // 属性绑定（固定位置：0=VertexCoord, 1=TexCoord, 2=COLOR）
    static constexpr GLsizei kStride = k_vertStride * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, kStride, (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kStride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, kStride, (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ---- 内置着色器程序（第 0 通道）----
    ShaderPass pass0;
    pass0.program = buildProgram(vertSrc.c_str(), fragSrc.c_str());
    if (!pass0.program) {
        brls::Logger::error("[ShaderChain] Built-in shader compilation FAILED");
        return false;
    }
    _lookupUniforms(pass0);

    // 初始化 Source/Texture 采样器到纹理单元 0
    glUseProgram(pass0.program);
    if (pass0.sourceLoc >= 0) glUniform1i(pass0.sourceLoc, 0);
    if (pass0.mvpMatrixLoc >= 0)
        glUniformMatrix4fv(pass0.mvpMatrixLoc, 1, GL_FALSE, k_mvpIdentity);
    glUseProgram(0);

    m_passes.push_back(std::move(pass0));
    brls::Logger::debug("[ShaderChain] Initialised, built-in pass 0 ready");
    return true;
}

// ============================================================
// ShaderChain：默认内置初始化
// ============================================================
bool ShaderChain::initBuiltin()
{
    return init(std::string(k_builtinVert), std::string(k_builtinFrag));
}

// ============================================================
// ShaderChain：释放所有 GL 资源
// ============================================================
void ShaderChain::deinit()
{
    for (auto& p : m_passes) {
        if (p.fbo)       glDeleteFramebuffers(1, &p.fbo);
        if (p.outputTex) glDeleteTextures(1,      &p.outputTex);
        if (p.program)   glDeleteProgram(p.program);
    }
    m_passes.clear();

    for (auto& lut : m_luts) {
        if (lut.tex) glDeleteTextures(1, &lut.tex);
    }
    m_luts.clear();
    _clearFrameHistory();

    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { glDeleteBuffers(1,      &m_vbo); m_vbo = 0; }

    m_lastVideoW = 0;
    m_lastVideoH = 0;
    m_frameCount = 0;
    brls::Logger::debug("[ShaderChain] All GL resources released");
}

// ============================================================
// ShaderChain：追加用户自定义通道
// ============================================================
bool ShaderChain::addPass(const std::string& vert, const std::string& frag,
                          GLenum filter, GLenum wrapMode,
                          const ShaderPassScale& scale, int fcMod,
                          const std::string& alias,
                          const std::unordered_map<std::string, float>& paramDefaults)
{
    ShaderPass p;
    p.program = buildProgram(vert.c_str(), frag.c_str());
    if (!p.program) return false;

    p.glFilter = filter;
    p.wrapMode = wrapMode;
    p.scale    = scale;
    p.fcMod    = fcMod;
    p.alias    = alias;

    _lookupUniforms(p);
    _lookupLutUniforms(p);
    _lookupPassTextures(p, (int)m_passes.size());

    // 初始化所有采样器 uniform 及常量 uniform，确保程序创建后即处于完全就绪状态。
    // 这样即使 run() 尚未被调用，着色器程序也能正确工作。
    glUseProgram(p.program);

    // Source/Texture 采样器 → 纹理单元 0
    if (p.sourceLoc >= 0) glUniform1i(p.sourceLoc, 0);

    // LUT 采样器 → 纹理单元 1, 2, ...
    for (size_t li = 0; li < m_luts.size() && li < p.lutLocs.size(); ++li) {
        if (p.lutLocs[li] >= 0)
            glUniform1i(p.lutLocs[li], (GLint)(1 + li));
    }

    // 前序通道纹理采样器 → 单元 1 + lutCount + texUnitOffset
    {
        int lutCount = (int)m_luts.size();
        for (const auto& binding : p.passTexBindings) {
            if (binding.texUnitOffset < 0)
                glUniform1i(binding.loc, 0);  // 无帧历史时回退到源纹理（单元 0）
            else
                glUniform1i(binding.loc, 1 + lutCount + binding.texUnitOffset);
        }
    }

    // OrigTexture 采样器 → 固定纹理单元 k_origTexUnitOffset
    if (p.origTexLoc >= 0)
        glUniform1i(p.origTexLoc, k_origTexUnitOffset);

    // MVPMatrix：设为单位矩阵（RetroArch 着色器将顶点坐标已为 NDC，直通即可）
    if (p.mvpMatrixLoc >= 0)
        glUniformMatrix4fv(p.mvpMatrixLoc, 1, GL_FALSE, k_mvpIdentity);

    // 初始化 #pragma parameter 参数默认值（避免除零→全白画面）
    for (const auto& kv : paramDefaults) {
        GLint loc = glGetUniformLocation(p.program, kv.first.c_str());
        if (loc >= 0) glUniform1f(loc, kv.second);
    }
    glUseProgram(0);

    m_lastVideoW = 0;
    m_lastVideoH = 0;

    m_passes.push_back(std::move(p));
    brls::Logger::info("[ShaderChain] User pass {} added (alias='{}')",
                       m_passes.size() - 1, alias);
    {
        const auto& added = m_passes.back();
        SC_DLOG("[ShaderChain::addPass] passIdx={} alias='{}' program={} "
                "filter={:#x} wrap={:#x} fcMod={} paramDefaults={}",
                m_passes.size() - 1, alias, added.program,
                filter, wrapMode, fcMod, paramDefaults.size());
        SC_DLOG("[ShaderChain::addPass]   lutLocs={} passTexBindings={}",
                added.lutLocs.size(), added.passTexBindings.size());
    }
    return true;
}

// ============================================================
// ShaderChain：清除用户通道（保留第 0 通道）
// ============================================================
void ShaderChain::clearPasses()
{
    while (m_passes.size() > 1) {
        auto& p = m_passes.back();
        if (p.fbo)       glDeleteFramebuffers(1, &p.fbo);
        if (p.outputTex) glDeleteTextures(1,      &p.outputTex);
        if (p.program)   glDeleteProgram(p.program);
        m_passes.pop_back();
    }
    for (auto& lut : m_luts) {
        if (lut.tex) glDeleteTextures(1, &lut.tex);
    }
    m_luts.clear();
    _clearFrameHistory();

    m_lastVideoW = 0;
    m_lastVideoH = 0;
    brls::Logger::info("[ShaderChain] User passes cleared");
}

// ============================================================
// ShaderChain：加载 LUT 纹理
// ============================================================
bool ShaderChain::addLut(const std::string& id, const std::string& path,
                          bool linear, bool mipmap, GLenum wrap)
{
    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) {
        brls::Logger::error("[ShaderChain] Failed to load LUT: {}", path);
        return false;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)wrap);
    GLenum filter = linear ? GL_LINEAR : GL_NEAREST;
    GLenum minFilter = (mipmap && linear) ? GL_LINEAR_MIPMAP_LINEAR
                     : (mipmap)           ? GL_NEAREST_MIPMAP_NEAREST
                     : filter;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)filter);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    ShaderLut lut;
    lut.id  = id;
    lut.tex = tex;
    m_luts.push_back(lut);

    for (size_t pi = 1; pi < m_passes.size(); ++pi) {
        m_passes[pi].lutLocs.resize(m_luts.size(), -1);
        size_t idx = m_luts.size() - 1;
        m_passes[pi].lutLocs[idx] =
            glGetUniformLocation(m_passes[pi].program, id.c_str());
    }

    brls::Logger::info("[ShaderChain] LUT loaded: id='{}', {}x{}, path={}", id, w, h, path);
    return true;
}

// ============================================================
// ShaderChain：获取输出纹理
// ============================================================
GLuint ShaderChain::outputTex() const
{
    if (m_passes.empty()) return 0;
    return m_passes.back().outputTex;
}

// ============================================================
// ShaderChain：执行着色器链
// ============================================================
GLuint ShaderChain::run(GLuint srcTex, unsigned videoW, unsigned videoH)
{
    if (m_passes.empty()) return srcTex;

    bool dimsChanged = (videoW != m_lastVideoW || videoH != m_lastVideoH);
    if (dimsChanged) {
        m_lastVideoW = videoW;
        m_lastVideoH = videoH;
        _clearFrameHistory();
    }
    for (auto& p : m_passes) {
        if (!p.fbo || dimsChanged) {
            if (!_initPassFbo(p, (int)videoW, (int)videoH, videoW, videoH))
                return srcTex;
        }
    }

    // ---- 保存 GL 状态 ----
    GLint     prevFbo           = 0;
    GLint     prevViewport[4]   = {};
    GLint     prevProgram       = 0;
    GLint     prevVAO           = 0;
    GLint     prevActiveTexture = GL_TEXTURE0;
    GLint     prevTex0          = 0;
    GLboolean prevScissor       = GL_FALSE;
    GLboolean prevDepth         = GL_FALSE;
    GLboolean prevStencil       = GL_FALSE;
    GLboolean prevBlend         = GL_FALSE;
    GLint     prevBlendSrcRGB   = GL_ONE,  prevBlendDstRGB   = GL_ZERO;
    GLint     prevBlendSrcAlpha = GL_ONE,  prevBlendDstAlpha = GL_ZERO;
    GLfloat   prevClearColor[4] = {};
    GLboolean prevColorMask[4]  = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};

    glGetIntegerv(GL_FRAMEBUFFER_BINDING,  &prevFbo);
    glGetIntegerv(GL_VIEWPORT,              prevViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM,       &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,  &prevVAO);
    glGetIntegerv(GL_ACTIVE_TEXTURE,        &prevActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,    &prevTex0);
    prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    prevDepth   = glIsEnabled(GL_DEPTH_TEST);
    prevStencil = glIsEnabled(GL_STENCIL_TEST);
    prevBlend   = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_RGB,         &prevBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB,         &prevBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,       &prevBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,       &prevBlendDstAlpha);
    glGetFloatv(GL_COLOR_CLEAR_VALUE,       prevClearColor);
    glGetBooleanv(GL_COLOR_WRITEMASK,       prevColorMask);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    // 禁止着色器写入 alpha 通道：许多 RetroArch 着色器（如 gba-color）的矩阵变换
    // 会将 FragColor.a 置为 0。若让该 0 写入 FBO，NanoVG 的预乘 Alpha 混合
    // (GL_ONE, 1-srcAlpha) 会把白色背景叠加进来，导致全白画面。
    // 保持 FBO alpha = glClearColor 设定的 1.0，确保最终输出对 NanoVG 完全不透明。
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    // ---- 辅助函数：构建 RetroArch vec4 尺寸 ----
    auto makeSizeVec4 = [](int w, int h, float& invW, float& invH) {
        invW = (w > 0) ? 1.f / (float)w : 0.f;
        invH = (h > 0) ? 1.f / (float)h : 0.f;
    };
    auto setSizeUniform = [&](GLint loc, bool isVec2, int w, int h) {
        if (loc < 0) return;
        float invW = 0.f, invH = 0.f;
        makeSizeVec4(w, h, invW, invH);
        if (isVec2)
            glUniform2f(loc, (float)w, (float)h);
        else
            glUniform4f(loc, (float)w, (float)h, invW, invH);
    };
    auto resolvePassInputSize = [](const ShaderPass& refPass, int& w, int& h) {
        w = (refPass.inputW > 0) ? refPass.inputW : refPass.outW;
        h = (refPass.inputH > 0) ? refPass.inputH : refPass.outH;
    };

    // ---- 节流调试日志（前 3 帧 + 每 300 帧 + 尺寸变化时输出）----
    const bool logThisFrame = s_shaderDebugLog &&
        (m_frameCount < 3 || m_frameCount % 300 == 0 || dimsChanged);
    if (logThisFrame) {
        brls::Logger::debug("[ShaderChain::run] frame={} srcTex={} videoW={} videoH={} "
                            "passes={} luts={} dimsChanged={}",
                            m_frameCount, srcTex, videoW, videoH,
                            m_passes.size(), m_luts.size(), dimsChanged);
        brls::Logger::debug("[ShaderChain::run]   savedGL: fbo={} vp=[{},{},{},{}] "
                            "prog={} blend={} scissor={}",
                            prevFbo, prevViewport[0], prevViewport[1],
                            prevViewport[2], prevViewport[3],
                            prevProgram, (int)prevBlend, (int)prevScissor);
    }

    GLuint inputTex = srcTex;
    int inputW = (int)videoW;
    int inputH = (int)videoH;

    for (size_t i = 0; i < m_passes.size(); ++i) {
        ShaderPass& p = m_passes[i];
        if (!p.program || !p.fbo) continue;

        p.inputW = inputW;
        p.inputH = inputH;

        glBindFramebuffer(GL_FRAMEBUFFER, p.fbo);
        glViewport(0, 0, p.outW, p.outH);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 绑定输入纹理到单元 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTex);
        glUseProgram(p.program);

        // Source/Texture 采样器（纹理单元 0）
        if (p.sourceLoc >= 0) glUniform1i(p.sourceLoc, 0);

        // RetroArch 兼容 uniforms
        setSizeUniform(p.sourceSizeLoc, p.sourceSizeIsVec2, inputW, inputH);
        setSizeUniform(p.inputSizeLoc, p.inputSizeIsVec2, inputW, inputH);
        setSizeUniform(p.outputSizeLoc, p.outputSizeIsVec2, p.outW, p.outH);
        if (p.finalVpSizeLoc >= 0) {
            float invW, invH;
            makeSizeVec4((int)videoW, (int)videoH, invW, invH);
            glUniform4f(p.finalVpSizeLoc,
                        (float)videoW, (float)videoH, invW, invH);
        }
        if (p.frameCountLoc >= 0) {
            uint32_t fc = (p.fcMod > 0) ? (m_frameCount % (uint32_t)p.fcMod) : m_frameCount;
            glUniform1i(p.frameCountLoc, (GLint)fc);
        }
        // FrameDirection: +1 正向播放，-1 倒带（此处始终为正向）
        if (p.frameDirectionLoc >= 0)
            glUniform1i(p.frameDirectionLoc, 1);

        // LUT 纹理（从单元 1 开始）
        for (size_t li = 0; li < m_luts.size() && li < p.lutLocs.size(); ++li) {
            if (p.lutLocs[li] < 0 || !m_luts[li].tex) continue;
            glActiveTexture((GLenum)(GL_TEXTURE1 + li));
            glBindTexture(GL_TEXTURE_2D, m_luts[li].tex);
            glUniform1i(p.lutLocs[li], (GLint)(1 + li));
        }

        // 前序通道纹理
        int lutCount = (int)m_luts.size();
        for (const auto& binding : p.passTexBindings) {
            int unit = 1 + lutCount + binding.texUnitOffset;
            GLuint passTex = srcTex;
            int texW = (int)videoW;
            int texH = (int)videoH;
            int inputRefW = texW;
            int inputRefH = texH;

            if (binding.kind == ShaderPass::PassTexBinding::Kind::PassOutput) {
                if (binding.srcPassIdx >= 0 && binding.srcPassIdx < (int)m_passes.size()) {
                    const ShaderPass& refPass = m_passes[binding.srcPassIdx];
                    if (refPass.outputTex) {
                        passTex = refPass.outputTex;
                        texW = refPass.outW;
                        texH = refPass.outH;
                        resolvePassInputSize(refPass, inputRefW, inputRefH);
                    }
                }
            } else if (binding.historyIndex >= 0 &&
                       binding.historyIndex < (int)m_frameHistory.size() &&
                       m_frameHistory[binding.historyIndex].tex) {
                const auto& history = m_frameHistory[binding.historyIndex];
                passTex = history.tex;
                texW = history.w;
                texH = history.h;
                inputRefW = history.w;
                inputRefH = history.h;
            }

            glActiveTexture((GLenum)(GL_TEXTURE0 + unit));
            glBindTexture(GL_TEXTURE_2D, passTex);
            glUniform1i(binding.loc, unit);
            setSizeUniform(binding.textureSizeLoc, binding.textureSizeIsVec2, texW, texH);
            setSizeUniform(binding.inputSizeLoc, binding.inputSizeIsVec2, inputRefW, inputRefH);
        }

        // OrigTexture（固定单元 k_origTexUnitOffset）
        if (p.origTexLoc >= 0) {
            glActiveTexture((GLenum)(GL_TEXTURE0 + k_origTexUnitOffset));
            glBindTexture(GL_TEXTURE_2D, srcTex);
            glUniform1i(p.origTexLoc, k_origTexUnitOffset);
        }

        // OrigInputSize
        if (p.origInputSizeLoc >= 0)
            glUniform2f(p.origInputSizeLoc, (float)videoW, (float)videoH);

        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (logThisFrame) {
            brls::Logger::debug("[ShaderChain::run]   pass[{}]: prog={} fbo={} "
                                "inputTex={} in={}x{} -> out={}x{} outputTex={}",
                                i, p.program, p.fbo,
                                inputTex, inputW, inputH,
                                p.outW, p.outH, p.outputTex);
            brls::Logger::debug("[ShaderChain::run]   pass[{}] uniforms: "
                                "source={} sourceSize={} outputSize={} frameCount={} frameDir={} "
                                "origTex={} origInputSize={} mvp={}",
                                i, p.sourceLoc, p.sourceSizeLoc, p.outputSizeLoc,
                                p.frameCountLoc, p.frameDirectionLoc,
                                p.origTexLoc, p.origInputSizeLoc,
                                p.mvpMatrixLoc);
        }

        inputTex = p.outputTex;
        inputW   = p.outW;
        inputH   = p.outH;
    }

    if (!m_passes.empty())
        _captureFrameHistory(m_passes.front());

    if (logThisFrame) {
        brls::Logger::debug("[ShaderChain::run] -> finalTex={} ({}x{})",
                            inputTex,
                            m_passes.empty() ? 0 : m_passes.back().outW,
                            m_passes.empty() ? 0 : m_passes.back().outH);
    }

    ++m_frameCount;

    // ---- 恢复 GL 状态 ----
    int maxPassTexOffset = 0;
    for (const auto& pass : m_passes) {
        for (const auto& b : pass.passTexBindings) {
            if (b.texUnitOffset + 1 > maxPassTexOffset)
                maxPassTexOffset = b.texUnitOffset + 1;
        }
        if (pass.origTexLoc >= 0) {
            if (k_origTexUnitOffset + 1 > maxPassTexOffset)
                maxPassTexOffset = k_origTexUnitOffset + 1;
        }
    }
    int totalExtraUnits = (int)m_luts.size() + maxPassTexOffset;
    for (int u = 1; u <= totalExtraUnits; ++u) {
        glActiveTexture((GLenum)(GL_TEXTURE0 + u));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glUseProgram((GLuint)prevProgram);
    glBindVertexArray((GLuint)prevVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex0);
    glActiveTexture((GLenum)prevActiveTexture);

    if (prevScissor) glEnable(GL_SCISSOR_TEST);  else glDisable(GL_SCISSOR_TEST);
    if (prevDepth)   glEnable(GL_DEPTH_TEST);    else glDisable(GL_DEPTH_TEST);
    if (prevStencil) glEnable(GL_STENCIL_TEST);  else glDisable(GL_STENCIL_TEST);
    if (prevBlend)   glEnable(GL_BLEND);         else glDisable(GL_BLEND);

    glBlendFuncSeparate((GLenum)prevBlendSrcRGB,   (GLenum)prevBlendDstRGB,
                        (GLenum)prevBlendSrcAlpha, (GLenum)prevBlendDstAlpha);
    glClearColor(prevClearColor[0], prevClearColor[1],
                 prevClearColor[2], prevClearColor[3]);
    glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);

    return inputTex;
}

// ============================================================
// ShaderChain：编译并链接 GLSL 程序
// ============================================================
GLuint ShaderChain::buildProgram(const char* vertSrc, const char* fragSrc)
{
    // 调试日志：记录着色器源码（截断至 800 字符）及编译结果
    if (s_shaderDebugLog) {
        brls::Logger::debug("[ShaderChain::buildProgram] vertSrc (len={}):\n{}",
                            std::strlen(vertSrc), sc_truncate(vertSrc));
        brls::Logger::debug("[ShaderChain::buildProgram] fragSrc (len={}):\n{}",
                            std::strlen(fragSrc), sc_truncate(fragSrc));
    }

    auto compileShader = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint logLen = 0;
            glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(logLen > 0 ? logLen : 512, '\0');
            glGetShaderInfoLog(s, (GLsizei)log.size(), nullptr, &log[0]);
            const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
            brls::Logger::error("[ShaderChain] {} shader compile error:\n{}", typeName, log);
            brls::Logger::debug("[ShaderChain] Source:\n{}", src);
            glDeleteShader(s);
            return 0u;
        }
        return s;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertSrc);
    if (vs) SC_DLOG("[ShaderChain::buildProgram] vertex shader compiled OK (id={})", vs);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (fs) SC_DLOG("[ShaderChain::buildProgram] fragment shader compiled OK (id={})", fs);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);

    // 固定顶点属性位置（必须在 link 之前）
    glBindAttribLocation(prog, 0, "VertexCoord");
    glBindAttribLocation(prog, 1, "TexCoord");
    glBindAttribLocation(prog, 2, "COLOR");

    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen > 0 ? logLen : 512, '\0');
        glGetProgramInfoLog(prog, (GLsizei)log.size(), nullptr, &log[0]);
        brls::Logger::error("[ShaderChain] Program link error: {}", log);
        glDeleteProgram(prog);
        return 0;
    }
    SC_DLOG("[ShaderChain::buildProgram] program linked OK (id={})", prog);
    return prog;
}

// ============================================================
// ShaderChain：设置 FBO 输出纹理的过滤模式
// ============================================================
void ShaderChain::setFilter(GLenum glFilter)
{
    if (m_glFilter == glFilter) return;
    m_glFilter = glFilter;
    if (!m_passes.empty() && m_passes[0].outputTex) {
        glBindTexture(GL_TEXTURE_2D, m_passes[0].outputTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_glFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    for (auto& slot : m_frameHistory) {
        if (!slot.tex) continue;
        glBindTexture(GL_TEXTURE_2D, slot.tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_glFilter);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
// ShaderChain：为单个通道（重新）创建 FBO 及颜色纹理
// ============================================================
bool ShaderChain::_initPassFbo(ShaderPass& p, int srcW, int srcH,
                                unsigned viewW, unsigned viewH)
{
    if (p.fbo)       { glDeleteFramebuffers(1, &p.fbo);       p.fbo       = 0; }
    if (p.outputTex) { glDeleteTextures(1,      &p.outputTex); p.outputTex = 0; }

    int w = srcW, h = srcH;
    switch (p.scale.typeX) {
        case ShaderPassScale::SCALE_ABSOLUTE:
            w = (p.scale.absX > 0) ? p.scale.absX : srcW;
            break;
        case ShaderPassScale::VIEWPORT:
            w = (int)((float)viewW * p.scale.scaleX);
            break;
        default:
            w = (int)((float)srcW * p.scale.scaleX);
            break;
    }
    switch (p.scale.typeY) {
        case ShaderPassScale::SCALE_ABSOLUTE:
            h = (p.scale.absY > 0) ? p.scale.absY : srcH;
            break;
        case ShaderPassScale::VIEWPORT:
            h = (int)((float)viewH * p.scale.scaleY);
            break;
        default:
            h = (int)((float)srcH * p.scale.scaleY);
            break;
    }
    if (w <= 0) w = srcW;
    if (h <= 0) h = srcH;

    GLenum filter   = p.glFilter;
    GLenum wrapMode = p.wrapMode;

    glGenTextures(1, &p.outputTex);
    glBindTexture(GL_TEXTURE_2D, p.outputTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)filter);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &p.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, p.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, p.outputTex, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        brls::Logger::error("[ShaderChain] FBO incomplete: {:#x}", (unsigned)status);
        glDeleteFramebuffers(1, &p.fbo);       p.fbo       = 0;
        glDeleteTextures(1,     &p.outputTex); p.outputTex = 0;
        return false;
    }

    p.outW = w;
    p.outH = h;
    brls::Logger::debug("[ShaderChain] FBO created: {}x{}", w, h);
    SC_DLOG("[ShaderChain::_initPassFbo] src={}x{} view={}x{} "
            "scaleX(type={} val={:.3f}) scaleY(type={} val={:.3f}) "
            "-> outW={} outH={} fbo={} outputTex={} filter={:#x} wrap={:#x}",
            srcW, srcH, viewW, viewH,
            (int)p.scale.typeX,
            (p.scale.typeX == ShaderPassScale::SCALE_ABSOLUTE
                ? static_cast<float>(p.scale.absX) : p.scale.scaleX),
            (int)p.scale.typeY,
            (p.scale.typeY == ShaderPassScale::SCALE_ABSOLUTE
                ? static_cast<float>(p.scale.absY) : p.scale.scaleY),
            w, h, p.fbo, p.outputTex, filter, wrapMode);
    return true;
}

// ============================================================
// ShaderChain：帧历史纹理管理
// ============================================================
void ShaderChain::_clearFrameHistory()
{
    for (auto& slot : m_frameHistory) {
        if (slot.tex) glDeleteTextures(1, &slot.tex);
        slot.tex = 0;
        slot.w   = 0;
        slot.h   = 0;
    }
    m_frameHistory.clear();
}

bool ShaderChain::_ensureHistoryTexture(FrameHistorySlot& slot, int w, int h)
{
    if (w <= 0 || h <= 0)
        return false;

    if (slot.tex && slot.w == w && slot.h == h)
        return true;

    if (slot.tex) {
        glDeleteTextures(1, &slot.tex);
        slot.tex = 0;
    }

    glGenTextures(1, &slot.tex);
    glBindTexture(GL_TEXTURE_2D, slot.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)m_glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)m_glFilter);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    slot.w = w;
    slot.h = h;
    return true;
}

void ShaderChain::_captureFrameHistory(const ShaderPass& sourcePass)
{
    if (!sourcePass.fbo || sourcePass.outW <= 0 || sourcePass.outH <= 0)
        return;

    FrameHistorySlot slot;
    if (m_frameHistory.size() >= k_maxFrameHistory) {
        slot = m_frameHistory.back(); // Reuse the oldest slot to avoid reallocating GL texture objects.
        m_frameHistory.pop_back();
    }

    if (!_ensureHistoryTexture(slot, sourcePass.outW, sourcePass.outH))
        return;

    // Use glCopyTexSubImage2D here because it works across the current
    // GL2/GLES2/GL3 compatibility layer with the existing FBO/texture setup.
    // If frame-history copies ever become a bottleneck, this is the obvious
    // place to switch to a platform-specific blit/copy-image fast path.
    glBindFramebuffer(GL_FRAMEBUFFER, sourcePass.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, slot.tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sourcePass.outW, sourcePass.outH);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_frameHistory.push_front(slot);
    if (m_frameHistory.size() > k_maxFrameHistory)
        m_frameHistory.pop_back();
}

// ============================================================
// ShaderChain：_bindPassAttrib（已内联到 init()，此处保留为空）
// ============================================================
void ShaderChain::_bindPassAttrib(ShaderPass& /*p*/)
{
    // 顶点属性绑定已在 init() 的 VAO 初始化中完成（所有通道共享同一 VAO）
}

// ============================================================
// ShaderChain：设置单个参数的运行时值
// ============================================================
bool ShaderChain::setParam(const std::string& name, float val)
{
    bool found = false;
    for (auto& d : m_params) {
        if (d.name == name) {
            d.currentVal = val;
            found = true;
            break;
        }
    }

    GLint prevProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    int applied = 0;
    for (const auto& pass : m_passes) {
        if (!pass.program) continue;
        GLint loc = glGetUniformLocation(pass.program, name.c_str());
        if (loc < 0) continue;
        glUseProgram(pass.program);
        glUniform1f(loc, val);
        ++applied;
    }
    glUseProgram((GLuint)prevProg);
    return found || (applied > 0);
}

// ============================================================
// ShaderChain：调试日志开关（独立控制，默认关闭）
// ============================================================
void ShaderChain::setDebugLog(bool enable)
{
    s_shaderDebugLog = enable;
}

bool ShaderChain::debugLogEnabled()
{
    return s_shaderDebugLog;
}

} // namespace beiklive
