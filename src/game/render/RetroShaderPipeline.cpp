#include "game/render/RetroShaderPipeline.hpp"
#include "game/render/ShaderCompiler.hpp"
#include "game/render/GLSLPParser.hpp"

#include <borealis.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>

// stb_image：仅声明，实现已在 nanovg.c 中通过 STB_IMAGE_IMPLEMENTATION 编译
#include "stb_image.h"

namespace beiklive {

// ============================================================
// MVP 单位矩阵（列主序，RetroArch 顶点着色器所需）
// ============================================================
// MVP 单位矩阵（列主序）
static const float k_mvp4x4[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
};

static bool ensureCacheTexture(GLuint& tex, unsigned& texW, unsigned& texH,
                               unsigned w, unsigned h, bool filterLinear)
{
    if (w == 0 || h == 0) return false;
    if (tex && texW == w && texH == h) return true;

    if (tex) {
        glDeleteTextures(1, &tex);
        tex = 0;
    }

    glGenTextures(1, &tex);
    if (!tex) return false;

    glBindTexture(GL_TEXTURE_2D, tex);
    GLenum filter = filterLinear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    texW = w;
    texH = h;
    return true;
}

static bool copyTextureToCache(GLuint srcTex, unsigned srcW, unsigned srcH,
                               GLuint& dstTex, unsigned& dstW, unsigned& dstH,
                               bool filterLinear)
{
    if (!srcTex || srcW == 0 || srcH == 0) return false;
    if (!ensureCacheTexture(dstTex, dstW, dstH, srcW, srcH, filterLinear))
        return false;

    GLint prevReadFBO = 0;
    GLint prevTex     = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);

    GLuint copyFBO = 0;
    glGenFramebuffers(1, &copyFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, copyFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, srcTex, 0);

    bool ok = false;
    if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_2D, dstTex);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                            static_cast<GLsizei>(srcW),
                            static_cast<GLsizei>(srcH));
        ok = true;
    } else {
        brls::Logger::warning("RetroShaderPipeline: 无法复制纹理到缓存，源 FBO 不完整");
    }

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFBO));
    glDeleteFramebuffers(1, &copyFBO);

    return ok;
}

#if defined(USE_GLES2)
static unsigned nextPowerOfTwo(unsigned v)
{
    if (v <= 1) return 1;
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}
#endif

static float uvMax(unsigned image, unsigned texture)
{
    return texture > 0 ? static_cast<float>(image) / static_cast<float>(texture) : 1.0f;
}

static std::array<float, 8> makeUvCoords(float uMax, float vMax)
{
    uMax = std::max(0.0f, std::min(1.0f, uMax));
    vMax = std::max(0.0f, std::min(1.0f, vMax));
    return {0.0f, 0.0f, uMax, 0.0f, uMax, vMax, 0.0f, vMax};
}

static void addTexCoordAttribIfUsed(
    GLuint program,
    const std::string& attribName,
    unsigned imageW, unsigned imageH,
    unsigned textureW, unsigned textureH,
    std::vector<FullscreenQuad::ExtraTexCoordAttrib>& outAttribs)
{
    GLint location = glGetAttribLocation(program, attribName.c_str());
    if (location < 0) return;

    FullscreenQuad::ExtraTexCoordAttrib attrib;
    attrib.location = location;
    attrib.coords   = makeUvCoords(uvMax(imageW, textureW), uvMax(imageH, textureH));
    outAttribs.push_back(attrib);
}

static void addFrameSizeUniforms(
    GLuint program,
    const std::string& prefix,
    float inputW, float inputH,
    float texW, float texH)
{
    float invW = inputW > 0.0f ? 1.0f / inputW : 0.0f;
    float invH = inputH > 0.0f ? 1.0f / inputH : 0.0f;
    GLint loc  = glGetUniformLocation(program, (prefix + "TextureSize").c_str());
    if (loc >= 0) glUniform2f(loc, texW, texH);
    loc = glGetUniformLocation(program, (prefix + "InputSize").c_str());
    if (loc >= 0) glUniform2f(loc, inputW, inputH);
    loc = glGetUniformLocation(program, (prefix + "Size").c_str());
    if (loc >= 0) glUniform4f(loc, inputW, inputH, invW, invH);
}

// ============================================================
// init
// ============================================================
bool RetroShaderPipeline::init(const std::string& glslpPath)
{
    deinit();

    if (glslpPath.empty()) return false;

    if (!std::filesystem::exists(glslpPath)) {
        brls::Logger::warning("RetroShaderPipeline: 着色器文件不存在: {}", glslpPath);
        return false;
    }

    // 1. 根据文件扩展名选择解析方式
    std::vector<ShaderPassDesc>    descs;
    std::vector<GLSLPTextureDesc>  texDescs;
    std::vector<GLSLPParamOverride> paramDescs;
    std::string ext = std::filesystem::path(glslpPath).extension().string();
    // 将扩展名转为小写以兼容大小写差异
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (ext == ".glsl") {
        // 单个 .glsl 文件：自动构建单通道描述符，使用视口缩放填满屏幕
        ShaderPassDesc single;
        single.shaderPath  = glslpPath;
        single.filterLinear = true;
        single.scaleTypeX  = ShaderPassDesc::ScaleType::Viewport;
        single.scaleTypeY  = ShaderPassDesc::ScaleType::Viewport;
        single.scaleX      = 1.0f;
        single.scaleY      = 1.0f;
        descs.push_back(std::move(single));
        brls::Logger::info("RetroShaderPipeline: 单 .glsl 文件，构建单通道管线: {}", glslpPath);
    } else {
        // .glslp 预设文件：使用解析器读取多通道配置及外部纹理声明
        GLSLPPresetMeta meta;
        if (!GLSLPParser::parse(glslpPath, descs, &texDescs, &paramDescs, &meta) || descs.empty()) {
            brls::Logger::error("RetroShaderPipeline: 解析 .glslp 失败: {}", glslpPath);
            return false;
        }
        m_feedbackPass = meta.feedbackPass;
        m_historySize  = meta.historySize;
    }

    // 2. 初始化全屏四边形
    if (!m_quad.init()) {
        brls::Logger::error("RetroShaderPipeline: FullscreenQuad 初始化失败");
        return false;
    }

    // 3. 逐通道编译着色器（FBO 在 process 中按需分配）
    bool anyOk = false;
    for (const auto& desc : descs) {
        ShaderPass pass;
        pass.desc         = desc;
        pass.filterLinear = desc.filterLinear;
        pass.alias        = desc.alias;

        brls::Logger::debug("Pass alias='{}' shader='{}' filterLinear={}",
                        pass.alias, pass.desc.shaderPath, pass.filterLinear ? 1 : 0);

        pass.program = ShaderCompiler::compileRetroShader(desc.shaderPath);
        if (!pass.program) {
            brls::Logger::warning("RetroShaderPipeline: 跳过通道: {}", desc.shaderPath);
            // 仍然加入列表（会在 process 中被跳过），保持通道索引一致
        } else {
            anyOk = true;
            brls::Logger::info("RetroShaderPipeline: 编译通道: {}", desc.shaderPath);
        }
        m_passes.push_back(std::move(pass));
    }

    if (!anyOk) {
        brls::Logger::error("RetroShaderPipeline: 所有通道编译失败，管线未加载");
        deinit();
        return false;
    }

    // 4. 加载 .glslp 中声明的外部纹理
    for (const auto& td : texDescs) {
        ExternalTexture et;
        et.name     = td.name;
        et.wrapMode = td.wrapMode;
        et.texId    = loadTextureFromFile(td.path, td.filterLinear, td.wrapMode);
        if (et.texId) {
            m_textures.push_back(std::move(et));
        } else {
            brls::Logger::warning("RetroShaderPipeline: 外部纹理加载失败: {}", td.path);
        }
    }

    // 5. 从各 .glsl 通道源文件中解析 #pragma parameter 元数据，构建完整参数列表
    //    顺序：先扫描所有 pass 文件取得元数据（name/desc/min/max/step/default），
    //    再将 .glslp 中的覆盖值（paramDescs）应用到对应参数的 value 字段
    m_params.clear();
    for (const auto& pass : m_passes) {
        if (pass.desc.shaderPath.empty()) continue;
        std::vector<ShaderParamInfo> passMeta;
        GLSLPParser::parseParamMeta(pass.desc.shaderPath, passMeta);
        for (auto& pm : passMeta) {
            // 去重：若参数已存在则跳过（后续通道相同 uniform 名视为同一参数）
            bool dup = false;
            for (const auto& existing : m_params)
                if (existing.name == pm.name) { dup = true; break; }
            if (!dup) m_params.push_back(pm);
        }
    }
    // 将 .glslp 中的覆盖值应用到对应参数
    for (const auto& p : paramDescs) {
        bool applied = false;
        for (auto& pm : m_params) {
            if (pm.name == p.name) {
                pm.value = p.value;
                applied = true;
                brls::Logger::debug("RetroShaderPipeline: 参数覆盖 \"{}\" = {}", p.name, p.value);
                break;
            }
        }
        // 若参数名不在 .glsl #pragma parameter 列表中，仍添加（无元数据版本）
        if (!applied) {
            ShaderParamInfo pm;
            pm.name = p.name; pm.desc = p.name;
            pm.defaultValue = p.value; pm.minValue = 0.f; pm.maxValue = 1.f;
            pm.step = 0.f; pm.value = p.value;
            m_params.push_back(pm);
            brls::Logger::debug("RetroShaderPipeline: 无元数据参数 \"{}\" = {}", p.name, p.value);
        }
    }

    brls::Logger::info("RetroShaderPipeline: 加载完成，共 {} 个通道，{} 个外部纹理，{} 个参数",
                       m_passes.size(), m_textures.size(), m_params.size());
    return true;
}

// ============================================================
// deinit
// ============================================================
void RetroShaderPipeline::deinit()
{
    for (auto& pass : m_passes) {
        if (pass.fbo)     { glDeleteFramebuffers(1, &pass.fbo);  pass.fbo = 0; }
        if (pass.texture) { glDeleteTextures(1, &pass.texture);  pass.texture = 0; }
        if (pass.program) { glDeleteProgram(pass.program);       pass.program = 0; }
        pass.width = pass.height = 0;
        pass.imageWidth = pass.imageHeight = 0;
    }
    m_passes.clear();
    // 释放外部纹理 GL 对象
    for (auto& et : m_textures) {
        if (et.texId) { glDeleteTextures(1, &et.texId); et.texId = 0; }
    }
    m_textures.clear();
    m_params.clear();

    if (m_feedbackTex) { glDeleteTextures(1, &m_feedbackTex); m_feedbackTex = 0; }
    m_feedbackW = m_feedbackH = 0;
    m_feedbackImageW = m_feedbackImageH = 0;
    for (auto& t : m_historyTextures) {
        if (t) { glDeleteTextures(1, &t); t = 0; }
    }
    m_historyTextures.clear();
    m_historyWriteIdx = 0;
    m_historyW = m_historyH = 0;

    m_quad.deinit();
    m_lastOutW = m_lastOutH = 0;
    m_lastOutU = m_lastOutV = 1.0f;
}

// ============================================================
// allocateFBO – 为通道分配或调整 FBO + 颜色纹理
// ============================================================
bool RetroShaderPipeline::allocateFBO(ShaderPass& pass, int w, int h)
{
    if (w <= 0 || h <= 0) return false;
    unsigned texW = static_cast<unsigned>(w);
    unsigned texH = static_cast<unsigned>(h);
#if defined(USE_GLES2)
    // GLES2 旧设备对 NPOT 纹理/FBO 兼容性较弱，保留 POT 兜底。
    texW = nextPowerOfTwo(texW);
    texH = nextPowerOfTwo(texH);
#endif
    if (pass.fbo &&
        pass.imageWidth == w && pass.imageHeight == h &&
        pass.width == static_cast<int>(texW) &&
        pass.height == static_cast<int>(texH)) {
        return true;
    }

    // 释放旧 FBO / 纹理
    if (pass.fbo)     { glDeleteFramebuffers(1, &pass.fbo);  pass.fbo = 0; }
    if (pass.texture) { glDeleteTextures(1, &pass.texture);  pass.texture = 0; }

    // 创建颜色纹理
    glGenTextures(1, &pass.texture);
    glBindTexture(GL_TEXTURE_2D, pass.texture);
    GLenum glFilter = pass.filterLinear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLenum internalFormat = GL_RGBA;
    GLenum pixelType      = GL_UNSIGNED_BYTE;
#if !defined(USE_GLES2)
    if (pass.desc.floatFramebuffer) {
        internalFormat = GL_RGBA16F;
        pixelType      = GL_FLOAT;
    } else if (pass.desc.srgbFramebuffer) {
        internalFormat = GL_SRGB8_ALPHA8;
    }
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                 static_cast<GLsizei>(texW), static_cast<GLsizei>(texH),
                 0, GL_RGBA, pixelType, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 创建 FBO 并附加颜色纹理
    glGenFramebuffers(1, &pass.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, pass.texture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        brls::Logger::error("RetroShaderPipeline: FBO 不完整 (status={})", status);
        glDeleteFramebuffers(1, &pass.fbo);  pass.fbo = 0;
        glDeleteTextures(1, &pass.texture);  pass.texture = 0;
        return false;
    }

    pass.width       = static_cast<int>(texW);
    pass.height      = static_cast<int>(texH);
    pass.imageWidth  = w;
    pass.imageHeight = h;
    brls::Logger::debug("RetroShaderPipeline: 分配 FBO id={} image={}×{} texture={}×{}",
                        pass.fbo, w, h, texW, texH);
    return true;
}

// ============================================================
// computePassSize – 计算通道输出 FBO 尺寸
// ============================================================
void RetroShaderPipeline::computePassSize(const ShaderPassDesc& desc,
                                           unsigned videoW, unsigned videoH,
                                           unsigned viewW,  unsigned viewH,
                                           int& outW, int& outH)
{
    auto calcAxis = [](ShaderPassDesc::ScaleType type,
                       float scale, unsigned src, unsigned vp) -> int {
        const double dScale = static_cast<double>(scale);
        switch (type) {
            case ShaderPassDesc::ScaleType::Viewport:
                return std::max(1, static_cast<int>(std::llround(
                    static_cast<double>(vp) * dScale)));
            case ShaderPassDesc::ScaleType::Absolute:
                return std::max(1, static_cast<int>(std::llround(dScale)));
            case ShaderPassDesc::ScaleType::Source:
            default:
                return std::max(1, static_cast<int>(std::llround(
                    static_cast<double>(src) * dScale)));
        }
    };

    unsigned vpW = (viewW > 0) ? viewW : videoW;
    unsigned vpH = (viewH > 0) ? viewH : videoH;

    outW = calcAxis(desc.scaleTypeX, desc.scaleX, videoW, vpW);
    outH = calcAxis(desc.scaleTypeY, desc.scaleY, videoH, vpH);
}

// ============================================================
// setUniforms – 设置标准 RetroArch uniform 变量及额外纹理单元
// ============================================================
void RetroShaderPipeline::setUniforms(GLuint program,
                                       unsigned inputW, unsigned inputH,
                                       unsigned textureW, unsigned textureH,
                                       unsigned outW, unsigned outH,
                                       unsigned origW, unsigned origH,
                                       unsigned viewW, unsigned viewH,
                                       unsigned frameCount,
                                       const std::vector<std::pair<std::string,GLuint>>& extraTexUnits)
{
    auto setUniform1i = [&](const char* name, int v) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, v);
    };
    auto setUniform1f = [&](const char* name, float v) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1f(loc, v);
    };
    auto setUniform2f = [&](const char* name, float x, float y) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform2f(loc, x, y);
    };
    auto setUniform4f = [&](const char* name, float x, float y, float z, float w) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform4f(loc, x, y, z, w);
    };
    auto setUniformMat4 = [&](const char* name, const float* m) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, m);
    };
    auto setUniformSampler = [&](const char* name, GLuint v) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, static_cast<int>(v));
    };

    // MVP 投影矩阵（NDC -1..+1 → 标准 RetroArch 输出）
    setUniformMat4("MVPMatrix", k_mvp4x4);

    // 帧计数 / 方向
    setUniform1i("FrameCount",     static_cast<int>(frameCount));
    setUniform1i("FrameDirection", 1);

    // RetroArch 语义：InputSize 是有效画面尺寸，TextureSize 是实际纹理尺寸（可能含 padding）。
    setUniform2f("TextureSize", static_cast<float>(textureW), static_cast<float>(textureH));
    setUniform2f("InputSize",   static_cast<float>(inputW), static_cast<float>(inputH));
    setUniform4f("SourceSize",
                 static_cast<float>(inputW),  static_cast<float>(inputH),
                 1.f / static_cast<float>(inputW), 1.f / static_cast<float>(inputH));

    // 输出尺寸
    setUniform2f("OutputSize", static_cast<float>(outW), static_cast<float>(outH));
    setUniform4f("OutputSize4",
                 static_cast<float>(outW), static_cast<float>(outH),
                 1.f / static_cast<float>(outW), 1.f / static_cast<float>(outH));

    // 最终视口尺寸（RetroArch 标准 uniform）
    {
        float fvw = static_cast<float>(viewW > 0 ? viewW : outW);
        float fvh = static_cast<float>(viewH > 0 ? viewH : outH);
        setUniform2f("FinalViewportSize", fvw, fvh);
    }

    // 原始画面宽高比
    if (origW > 0 && origH > 0) {
        setUniform1f("OriginalAspect",
                     static_cast<float>(origW) / static_cast<float>(origH));
        setUniform1f("OriginalAspectRotAted",
                     static_cast<float>(origW) / static_cast<float>(origH));
    }

    // 帧时间增量（以微秒计；未精确追踪，填合理默认值 16666 ≈ 60fps）
    setUniform1i("FrameTimeDelta", 16666);

    // 原始视频输入尺寸（RetroArch 标准 OrigInputSize / OrigSize uniform）
    // 在多通道管线中，各通道的 OrigInputSize 始终指向第一个通道的输入（原始游戏帧）
    if (origW > 0 && origH > 0) {
        setUniform2f("OrigInputSize", static_cast<float>(origW), static_cast<float>(origH));
        setUniform4f("OrigSize",
                     static_cast<float>(origW), static_cast<float>(origH),
                     1.f / static_cast<float>(origW), 1.f / static_cast<float>(origH));
        // 部分着色器使用 OriginalSize 别名
        setUniform2f("OriginalSize", static_cast<float>(origW), static_cast<float>(origH));
        setUniform4f("OriginalSize4",
                     static_cast<float>(origW), static_cast<float>(origH),
                     1.f / static_cast<float>(origW), 1.f / static_cast<float>(origH));
    }

    // 主输入纹理采样器（unit 0）
    setUniformSampler("Texture",  0u);
    setUniformSampler("Source",   0u);
    setUniformSampler("tex",      0u);
    setUniformSampler("texture",  0u); // 部分着色器用小写

    // 额外纹理采样器（外部纹理和历史 Pass 输出）
    for (const auto& kv : extraTexUnits) {
        setUniformSampler(kv.first.c_str(), kv.second);
    }

    // .glslp parameters 参数覆盖值（作为 float uniform 传入，优先于 #pragma parameter 默认值）
    for (const auto& param : m_params) {
        GLint loc = glGetUniformLocation(program, param.name.c_str());
        if (loc >= 0) glUniform1f(loc, param.value);
    }
}

// ============================================================
// loadTextureFromFile – 从图像文件加载 GL 纹理
// ============================================================
GLuint RetroShaderPipeline::loadTextureFromFile(const std::string& path, bool filterLinear,
                                                 ShaderPassDesc::WrapMode wrapMode)
{
    if (path.empty()) return 0;

    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!data) {
        brls::Logger::warning("RetroShaderPipeline: 图像加载失败: {} (原因: {})",
                              path, stbi_failure_reason());
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum glFilter = filterLinear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);

    GLenum glWrap;
    switch (wrapMode) {
        case ShaderPassDesc::WrapMode::ClampToBorder:
#if !defined(USE_GLES2)
            glWrap = GL_CLAMP_TO_BORDER;
            {
                static const GLfloat borderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            }
#else
            glWrap = GL_CLAMP_TO_EDGE;
#endif
            break;
        case ShaderPassDesc::WrapMode::Repeat:
            glWrap = GL_REPEAT;
            break;
        case ShaderPassDesc::WrapMode::MirroredRepeat:
            glWrap = GL_MIRRORED_REPEAT;
            break;
        default:
            glWrap = GL_CLAMP_TO_EDGE;
            break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(glWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(glWrap));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return tex;
}

// ============================================================
// process – 执行多通道着色器管线
// ============================================================
GLuint RetroShaderPipeline::process(GLuint inputTex,
                                     unsigned videoW, unsigned videoH,
                                     unsigned viewW,  unsigned viewH,
                                     unsigned frameCount)
{
    if (m_passes.empty()) return inputTex;
    if (!m_quad.isInitialized()) {
        brls::Logger::warning("RetroShaderPipeline::process: FullscreenQuad 未初始化，直通返回");
        return inputTex;
    }

    // 保存 GL 状态，管线结束后恢复
    GLuint    prevFBO         = 0;
    GLint     prevViewport[4] = {};
    GLuint    prevProg        = 0;
    GLuint    prevVAO         = 0;
    GLuint    prevTex         = 0;
    // 混合、深度、模板、裁剪、面剔除等可能被外部（如 NanoVG 前一帧）设置的状态
    GLboolean prevBlendEn     = GL_FALSE;
    GLboolean prevDepthEn     = GL_FALSE;
    GLboolean prevStencilEn   = GL_FALSE;
    GLboolean prevScissorEn   = GL_FALSE;
    GLboolean prevCullEn      = GL_FALSE;
    GLint     prevBlendSrcRGB   = GL_ONE;
    GLint     prevBlendDstRGB   = GL_ZERO;
    GLint     prevBlendSrcAlpha = GL_ONE;
    GLint     prevBlendDstAlpha = GL_ZERO;
    GLboolean prevSRGBEn  = GL_FALSE;   // GL_FRAMEBUFFER_SRGB 状态
    {
        GLint tmp = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &tmp);
        prevFBO = static_cast<GLuint>(tmp);
        glGetIntegerv(GL_CURRENT_PROGRAM, &tmp);
        prevProg = static_cast<GLuint>(tmp);
#if !defined(USE_GLES2)
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &tmp);
        prevVAO = static_cast<GLuint>(tmp);
#endif
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &tmp);
        prevTex = static_cast<GLuint>(tmp);
    }
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // 保存并禁用可能干扰中间 FBO 渲染的 GL 状态。
    // NanoVG 在每帧渲染后会保持 GL_BLEND=enabled（混合模式 GL_ONE, GL_ONE_MINUS_SRC_ALPHA），
    // 若不在此处禁用，pass0 的点阵 alpha 通道会被混合污染（所有像素 alpha 变为 1），
    // 导致后续通道无法区分"亮点"与"暗点"，游戏画面变为不可见的暗色。
    prevBlendEn   = glIsEnabled(GL_BLEND);
    prevDepthEn   = glIsEnabled(GL_DEPTH_TEST);
    prevStencilEn = glIsEnabled(GL_STENCIL_TEST);
    prevScissorEn = glIsEnabled(GL_SCISSOR_TEST);
    prevCullEn    = glIsEnabled(GL_CULL_FACE);
    prevSRGBEn   = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    glGetIntegerv(GL_BLEND_SRC_RGB,   &prevBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB,   &prevBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDstAlpha);

    // 着色器中间 FBO 渲染：直接覆写，不做 alpha 混合
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);

    GLuint currentTex = inputTex;
    unsigned currentImageW = videoW;
    unsigned currentImageH = videoH;
    unsigned currentTexW = videoW;
    unsigned currentTexH = videoH;
    GLuint   maxTexUnit = 0;

    for (size_t idx = 0; idx < m_passes.size(); ++idx) {
        auto& pass = m_passes[idx];
        if (!pass.program) {
            brls::Logger::debug("RetroShaderPipeline: 跳过通道 {} (无程序)", idx);
            continue;
        }

        // frame_count_mod：仅每 N 帧执行一次
        if (pass.desc.frameCountMod > 1 && (frameCount % static_cast<unsigned>(pass.desc.frameCountMod)) != 0) {
            // 复用本 pass 上一次执行时的输出，并同步尺寸供后续 pass 的 Source 缩放和 uniform 使用。
            if (pass.texture && pass.width > 0 && pass.height > 0 &&
                pass.imageWidth > 0 && pass.imageHeight > 0) {
                currentTex = pass.texture;
                currentImageW = static_cast<unsigned>(pass.imageWidth);
                currentImageH = static_cast<unsigned>(pass.imageHeight);
                currentTexW = static_cast<unsigned>(pass.width);
                currentTexH = static_cast<unsigned>(pass.height);
            }
            continue;
        }
        int outW = static_cast<int>(currentImageW);
        int outH = static_cast<int>(currentImageH);
        computePassSize(pass.desc, currentImageW, currentImageH, viewW, viewH, outW, outH);

        // 确保 FBO 已分配且尺寸正确
        if (!allocateFBO(pass, outW, outH)) {
            brls::Logger::warning("RetroShaderPipeline: 跳过通道 {}（FBO 分配失败）", idx);
            continue;
        }

        // 绑定输出 FBO 和视口
        glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);
        if (pass.desc.srgbFramebuffer)
            glEnable(GL_FRAMEBUFFER_SRGB);
        else
            glDisable(GL_FRAMEBUFFER_SRGB);
        glViewport(0, 0, outW, outH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 激活着色器
        glUseProgram(pass.program);

        // 统一纹理采样状态设置：避免越界采样时边缘像素拉伸
        auto applyTextureSamplingState = [&](GLuint tex, GLenum wrap, bool updateFilter, GLenum filter) {
            if (!tex) return;
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrap));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrap));
            if (updateFilter) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filter));
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filter));
            }
#if !defined(USE_GLES2)
            if (wrap == GL_CLAMP_TO_BORDER) {
                static const GLfloat s_borderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, s_borderColor);
            }
#endif
            glBindTexture(GL_TEXTURE_2D, 0);
        };

        GLenum wrapGL = GL_CLAMP_TO_EDGE;
        switch (pass.desc.wrapMode) {
            case ShaderPassDesc::WrapMode::ClampToBorder:
#if !defined(USE_GLES2)
                wrapGL = GL_CLAMP_TO_BORDER;
#else
                // GLES2 不支持 GL_CLAMP_TO_BORDER，降级
                wrapGL = GL_CLAMP_TO_EDGE;
#endif
                break;
            case ShaderPassDesc::WrapMode::Repeat:
                wrapGL = GL_REPEAT;
                break;
            case ShaderPassDesc::WrapMode::MirroredRepeat:
                wrapGL = GL_MIRRORED_REPEAT;
                break;
            default:
                wrapGL = GL_CLAMP_TO_EDGE;
                break;
        }

#if !defined(USE_GLES2)
        // 自动防拉伸：当本 pass 是放大且仍为边缘钳制时，改为透明边框钳制
        // 仅应用在主输入纹理，避免影响 LUT/历史纹理语义
        if (wrapGL == GL_CLAMP_TO_EDGE &&
            (outW > static_cast<int>(currentImageW) || outH > static_cast<int>(currentImageH))) {
            wrapGL = GL_CLAMP_TO_BORDER;
        }
#endif

        // 仅主输入沿用当前 pass 的 filter_linear
        GLenum filterGL = pass.filterLinear ? GL_LINEAR : GL_NEAREST;
        applyTextureSamplingState(currentTex, wrapGL, true, filterGL);

        // mipmap_input：为输入纹理生成 mipmap 链（抗锯齿 / LOD 采样）
        if (pass.desc.mipmapInput && currentTex) {
            glBindTexture(GL_TEXTURE_2D, currentTex);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // 绑定主输入纹理到纹理单元 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex);

        // 构建额外纹理单元列表（外部纹理 + 已完成通道的输出）
        std::vector<std::pair<std::string, GLuint>> extraTexUnits;
        std::vector<FullscreenQuad::ExtraTexCoordAttrib> extraTexCoords;
        {
            GLuint unit = 1;
            // 外部纹理
            for (const auto& et : m_textures) {
                if (et.texId) {
                    // 兼容性修复：不再强制覆盖外部纹理采样状态
                    glActiveTexture(GL_TEXTURE0 + unit);
                    glBindTexture(GL_TEXTURE_2D, et.texId);
                    extraTexUnits.emplace_back(et.name, unit);
                    addTexCoordAttribIfUsed(pass.program, et.name + "TexCoord",
                                            1, 1, 1, 1, extraTexCoords);
                    if (et.name == "LUT")
                        addTexCoordAttribIfUsed(pass.program, "LUTTexCoord",
                                                1, 1, 1, 1, extraTexCoords);
                    ++unit;
                }
            }
            // 已完成通道的输出
            for (size_t pi = 0; pi < idx; ++pi) {
                const auto& prev = m_passes[pi];
                if (!prev.texture) continue;
                // 兼容性修复：不再强制覆盖历史 pass 纹理采样状态
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, prev.texture);
                // RetroArch 约定：PassN 使用 1-based 绝对索引（Pass1=第1个通道输出，Pass2=第2个，…）
                extraTexUnits.emplace_back("Pass" + std::to_string(pi + 1) + "Texture", unit);
                size_t prevN = idx - pi;
                extraTexUnits.emplace_back("PassPrev" + std::to_string(prevN) + "Texture", unit);
                if (!prev.alias.empty()) {
                    extraTexUnits.emplace_back(prev.alias + "Texture", unit);
                }

                addTexCoordAttribIfUsed(pass.program, "Pass" + std::to_string(pi + 1) + "TexCoord",
                                        static_cast<unsigned>(prev.imageWidth),
                                        static_cast<unsigned>(prev.imageHeight),
                                        static_cast<unsigned>(prev.width),
                                        static_cast<unsigned>(prev.height),
                                        extraTexCoords);
                addTexCoordAttribIfUsed(pass.program, "PassPrev" + std::to_string(prevN) + "TexCoord",
                                        static_cast<unsigned>(prev.imageWidth),
                                        static_cast<unsigned>(prev.imageHeight),
                                        static_cast<unsigned>(prev.width),
                                        static_cast<unsigned>(prev.height),
                                        extraTexCoords);
                if (prevN == 1) {
                    // 兼容部分社区 shader 使用的无编号别名 PassPrevTexture。
                    extraTexUnits.emplace_back("PassPrevTexture", unit);
                    addTexCoordAttribIfUsed(pass.program, "PassPrevTexCoord",
                                            static_cast<unsigned>(prev.imageWidth),
                                            static_cast<unsigned>(prev.imageHeight),
                                            static_cast<unsigned>(prev.width),
                                            static_cast<unsigned>(prev.height),
                                            extraTexCoords);
                }
                if (!prev.alias.empty()) {
                    addTexCoordAttribIfUsed(pass.program, prev.alias + "TexCoord",
                                            static_cast<unsigned>(prev.imageWidth),
                                            static_cast<unsigned>(prev.imageHeight),
                                            static_cast<unsigned>(prev.width),
                                            static_cast<unsigned>(prev.height),
                                            extraTexCoords);
                }
                ++unit;
            }

            // 原始输入纹理保持原采样状态，避免与主输入策略冲突
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, inputTex);
            extraTexUnits.emplace_back("OrigTexture", unit);
            extraTexUnits.emplace_back("PassPrev" + std::to_string(idx + 1) + "Texture", unit);
            addTexCoordAttribIfUsed(pass.program, "OrigTexCoord",
                                    videoW, videoH, videoW, videoH, extraTexCoords);
            addTexCoordAttribIfUsed(pass.program, "PassPrev" + std::to_string(idx + 1) + "TexCoord",
                                    videoW, videoH, videoW, videoH, extraTexCoords);
            ++unit;

            // 帧反馈纹理（上一帧的反馈 pass 输出，回退为当前原始输入）
            {
                GLuint fbTex = m_feedbackTex ? m_feedbackTex : inputTex;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, fbTex);
                extraTexUnits.emplace_back("FeedbackTexture", unit);
                if (m_feedbackTex) {
                    extraTexUnits.emplace_back("PassFeedbackTexture", unit);
                    extraTexUnits.emplace_back("PassFeedback" + std::to_string(m_feedbackPass + 1) + "Texture", unit);
                }
                addTexCoordAttribIfUsed(pass.program, "FeedbackTexCoord",
                                        m_feedbackTex ? m_feedbackImageW : videoW,
                                        m_feedbackTex ? m_feedbackImageH : videoH,
                                        m_feedbackTex ? m_feedbackW : videoW,
                                        m_feedbackTex ? m_feedbackH : videoH,
                                        extraTexCoords);
                addTexCoordAttribIfUsed(pass.program, "PassFeedbackTexCoord",
                                        m_feedbackTex ? m_feedbackImageW : videoW,
                                        m_feedbackTex ? m_feedbackImageH : videoH,
                                        m_feedbackTex ? m_feedbackW : videoW,
                                        m_feedbackTex ? m_feedbackH : videoH,
                                        extraTexCoords);
                if (m_feedbackTex) {
                    addTexCoordAttribIfUsed(pass.program,
                                            "PassFeedback" + std::to_string(m_feedbackPass + 1) + "TexCoord",
                                            m_feedbackImageW, m_feedbackImageH,
                                            m_feedbackW, m_feedbackH,
                                            extraTexCoords);
                }
                ++unit;
            }

            // 帧历史纹理（前 N 帧的原始输入）
            const unsigned historyCount = static_cast<unsigned>(m_historyTextures.size());
            if (historyCount == 0 && inputTex) {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, inputTex);
                extraTexUnits.emplace_back("PrevTexture", unit);
                extraTexUnits.emplace_back("OriginalHistory0", unit);
                extraTexUnits.emplace_back("OriginalHistory0Texture", unit);
                addTexCoordAttribIfUsed(pass.program, "PrevTexCoord",
                                        videoW, videoH, videoW, videoH, extraTexCoords);
                addTexCoordAttribIfUsed(pass.program, "OriginalHistory0TexCoord",
                                        videoW, videoH, videoW, videoH, extraTexCoords);
                ++unit;
            }
            for (unsigned hi = 0; hi < historyCount; ++hi) {
                unsigned hIdx = (m_historyWriteIdx + historyCount - 1 - hi) % historyCount;
                GLuint histTex = m_historyTextures[hIdx] ? m_historyTextures[hIdx] : inputTex;
                if (!histTex) continue;

                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, histTex);

                extraTexUnits.emplace_back("OriginalHistory" + std::to_string(hi), unit);
                extraTexUnits.emplace_back("OriginalHistory" + std::to_string(hi) + "Texture", unit);

                if (hi == 0) {
                    extraTexUnits.emplace_back("PrevTexture", unit);
                    addTexCoordAttribIfUsed(pass.program, "PrevTexCoord",
                                            videoW, videoH, videoW, videoH, extraTexCoords);
                } else {
                    extraTexUnits.emplace_back("Prev" + std::to_string(hi) + "Texture", unit);
                    addTexCoordAttribIfUsed(pass.program, "Prev" + std::to_string(hi) + "TexCoord",
                                            videoW, videoH, videoW, videoH, extraTexCoords);
                }

                addTexCoordAttribIfUsed(pass.program, "OriginalHistory" + std::to_string(hi) + "TexCoord",
                                        videoW, videoH, videoW, videoH, extraTexCoords);
                ++unit;
            }

            if (unit - 1 > maxTexUnit) maxTexUnit = unit - 1;
        }
        glActiveTexture(GL_TEXTURE0);

        // 设置 uniform（含 OrigInputSize / OrigSize 和参数覆盖值）
        setUniforms(pass.program,
                    currentImageW, currentImageH,
                    currentTexW, currentTexH,
                    static_cast<unsigned>(outW),
                    static_cast<unsigned>(outH),
                    videoW, videoH,
                    viewW, viewH,
                    frameCount,
                    extraTexUnits);

        // 设置历史通道的尺寸 uniform（PassNSize / PassPrevNTextureSize / PassPrevNInputSize）
        for (size_t pi = 0; pi < idx; ++pi) {
            const auto& prev = m_passes[pi];
            if (!prev.texture) continue;
            float inputW = static_cast<float>(prev.imageWidth);
            float inputH = static_cast<float>(prev.imageHeight);
            float texW = static_cast<float>(prev.width);
            float texH = static_cast<float>(prev.height);
            // 绝对索引尺寸
            std::string absPrefix = "Pass" + std::to_string(pi + 1);
            addFrameSizeUniforms(pass.program, absPrefix, inputW, inputH, texW, texH);
            // 相对索引尺寸
            size_t prevN = idx - pi;
            std::string prevPrefix = "PassPrev" + std::to_string(prevN);
            addFrameSizeUniforms(pass.program, prevPrefix, inputW, inputH, texW, texH);
            if (prevN == 1)
                addFrameSizeUniforms(pass.program, "PassPrev", inputW, inputH, texW, texH);
            // 别名尺寸：<alias>Size / <alias>TextureSize / <alias>InputSize
            if (!prev.alias.empty()) {
                addFrameSizeUniforms(pass.program, prev.alias, inputW, inputH, texW, texH);
            }
        }

        // 原始输入尺寸 uniform（OrigTextureSize / PassPrev{idx+1}TextureSize 等）
        // 对应上方绑定的 OrigTexture / PassPrev{idx+1}Texture
        {
            float fw    = static_cast<float>(videoW);
            float fh    = static_cast<float>(videoH);
            std::string origPrevPrefix = "PassPrev" + std::to_string(idx + 1);
            addFrameSizeUniforms(pass.program, "Orig", fw, fh, fw, fh);
            addFrameSizeUniforms(pass.program, origPrevPrefix, fw, fh, fw, fh);
        }

        // 反馈纹理尺寸 uniform。第一帧无反馈缓存时回退到原始输入纹理尺寸。
        {
            float inputW = static_cast<float>(m_feedbackTex ? m_feedbackImageW : videoW);
            float inputH = static_cast<float>(m_feedbackTex ? m_feedbackImageH : videoH);
            float texW = static_cast<float>(m_feedbackTex ? m_feedbackW : videoW);
            float texH = static_cast<float>(m_feedbackTex ? m_feedbackH : videoH);
            addFrameSizeUniforms(pass.program, "Feedback", inputW, inputH, texW, texH);
            addFrameSizeUniforms(pass.program, "PassFeedback", inputW, inputH, texW, texH);
            if (m_feedbackPass >= 0) {
                std::string feedbackPrefix = "PassFeedback" + std::to_string(m_feedbackPass + 1);
                addFrameSizeUniforms(pass.program, feedbackPrefix, inputW, inputH, texW, texH);
            }
        }

        // 历史帧尺寸 uniform：Prev / Prev1 / Prev2... 与 OriginalHistory0/1...
        {
            const unsigned historyCount = static_cast<unsigned>(m_historyTextures.size());
            if (historyCount == 0 && inputTex) {
                float fw = static_cast<float>(videoW);
                float fh = static_cast<float>(videoH);
                addFrameSizeUniforms(pass.program, "Prev", fw, fh, fw, fh);
                addFrameSizeUniforms(pass.program, "OriginalHistory0", fw, fh, fw, fh);
            }
            for (unsigned hi = 0; hi < historyCount; ++hi) {
                if (!m_historyTextures[(m_historyWriteIdx + historyCount - 1 - hi) % historyCount] && !inputTex)
                    continue;

                float fw = static_cast<float>(videoW);
                float fh = static_cast<float>(videoH);
                addFrameSizeUniforms(pass.program, "OriginalHistory" + std::to_string(hi), fw, fh, fw, fh);
                if (hi == 0)
                    addFrameSizeUniforms(pass.program, "Prev", fw, fh, fw, fh);
                else
                    addFrameSizeUniforms(pass.program, "Prev" + std::to_string(hi), fw, fh, fw, fh);
            }
        }

        // 绘制全屏四边形
        m_quad.draw(uvMax(currentImageW, currentTexW),
                    uvMax(currentImageH, currentTexH),
                    extraTexCoords);

        // 本通道输出成为下一通道输入
        currentTex = pass.texture;
        currentImageW = static_cast<unsigned>(outW);
        currentImageH = static_cast<unsigned>(outH);
        currentTexW = static_cast<unsigned>(pass.width);
        currentTexH = static_cast<unsigned>(pass.height);
    }

    m_lastOutW = currentImageW;
    m_lastOutH = currentImageH;
    m_lastOutU = uvMax(currentImageW, currentTexW);
    m_lastOutV = uvMax(currentImageH, currentTexH);

    // ---- 帧反馈纹理保存 ----
    if (m_feedbackPass >= 0 && static_cast<size_t>(m_feedbackPass) < m_passes.size()) {
        const auto& feedbackPass = m_passes[static_cast<size_t>(m_feedbackPass)];
        if (feedbackPass.texture && feedbackPass.width > 0 && feedbackPass.height > 0) {
            copyTextureToCache(feedbackPass.texture,
                               static_cast<unsigned>(feedbackPass.width),
                               static_cast<unsigned>(feedbackPass.height),
                               m_feedbackTex, m_feedbackW, m_feedbackH,
                               feedbackPass.filterLinear);
            m_feedbackImageW = static_cast<unsigned>(feedbackPass.imageWidth);
            m_feedbackImageH = static_cast<unsigned>(feedbackPass.imageHeight);
        }
    }

    // ---- 帧历史纹理管理 ----
    // RetroArch 的 Prev/Prev1/Prev2.../Prev6 纹理始终可用（从视频驱动的 prev_info 环形缓冲区）。
    // 此处始终维护至少 8 个历史帧（兼容 response-time 等需要多帧历史的着色器）。
    {
        const unsigned kMinHistory = 8u;
        const unsigned needHistory = std::max(static_cast<unsigned>(m_historySize), kMinHistory);
        if (inputTex && videoW > 0 && videoH > 0) {
            if (m_historyW != videoW || m_historyH != videoH) {
                for (auto& t : m_historyTextures) {
                    if (t) { glDeleteTextures(1, &t); t = 0; }
                }
                m_historyTextures.clear();
                m_historyWriteIdx = 0;
                m_historyW = videoW;
                m_historyH = videoH;
            }

            while (m_historyTextures.size() < needHistory) {
                GLuint t = 0;
                glGenTextures(1, &t);
                glBindTexture(GL_TEXTURE_2D, t);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             static_cast<GLsizei>(videoW), static_cast<GLsizei>(videoH),
                             0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                m_historyTextures.push_back(t);
            }
            // 将当前输入帧拷贝到环形缓冲区当前位置
            GLuint histTex = m_historyTextures[m_historyWriteIdx];
            unsigned histW = videoW;
            unsigned histH = videoH;
            copyTextureToCache(inputTex, videoW, videoH, histTex, histW, histH, false);
            m_historyWriteIdx = (m_historyWriteIdx + 1) % static_cast<unsigned>(m_historyTextures.size());
        }
    }

    // 恢复 GL 状态（精确解绑实际使用过的额外纹理单元）
    for (GLuint u = 1; u <= maxTexUnit; ++u) {
        glActiveTexture(GL_TEXTURE0 + u);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glUseProgram(prevProg);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, prevTex);
#if !defined(USE_GLES2)
    glBindVertexArray(prevVAO);
#endif

    // 恢复混合、深度、模板、裁剪、面剔除状态
    if (prevBlendEn)   glEnable(GL_BLEND);   else glDisable(GL_BLEND);
    if (prevDepthEn)   glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
    if (prevStencilEn) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
    if (prevScissorEn) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prevCullEn)    glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);
    if (prevSRGBEn)   glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
    glBlendFuncSeparate(static_cast<GLenum>(prevBlendSrcRGB),
                        static_cast<GLenum>(prevBlendDstRGB),
                        static_cast<GLenum>(prevBlendSrcAlpha),
                        static_cast<GLenum>(prevBlendDstAlpha));

    // 恢复原始输入纹理（游戏帧 m_texture）的采样参数为默认值，
    // 防止管线修改的 wrap_mode/filter_mode 影响 NanoVG 后续对该纹理的直通渲染。
    if (inputTex) {
        glBindTexture(GL_TEXTURE_2D, inputTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // 注：不重置 MIN/MAG_FILTER，游戏纹理过滤由 GameTexture::setFilter() 管理
        // 将 GL_TEXTURE0 恢复为调用前绑定的纹理（已在上方 state restore 中保存）
        glBindTexture(GL_TEXTURE_2D, prevTex);
    }

    return currentTex;
}

// ============================================================
// setParamValue – 更新指定参数的当前值
// ============================================================
void RetroShaderPipeline::setParamValue(const std::string& name, float value)
{
    for (auto& p : m_params) {
        if (p.name == name) {
            p.value = value;
            return;
        }
    }
    // 若参数不存在，忽略（可能是着色器未声明的参数）
}

} // namespace beiklive
