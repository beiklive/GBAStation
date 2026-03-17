#include "Video/renderer/RetroShaderPipeline.hpp"
#include "Video/renderer/ShaderCompiler.hpp"
#include "Video/renderer/GLSLPParser.hpp"

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
static const float k_identity4x4[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
};

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
        if (!GLSLPParser::parse(glslpPath, descs, &texDescs, &paramDescs) || descs.empty()) {
            brls::Logger::error("RetroShaderPipeline: 解析 .glslp 失败: {}", glslpPath);
            return false;
        }
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
        et.name  = td.name;
        et.texId = loadTextureFromFile(td.path, td.filterLinear);
        if (et.texId) {
            //brls::Logger::info("RetroShaderPipeline: 加载外部纹理 \"{}\" id={} 来自 {}",
                            //    et.name, et.texId, td.path);
            m_textures.push_back(std::move(et));
        } else {
            brls::Logger::warning("RetroShaderPipeline: 外部纹理加载失败: {}", td.path);
        }
    }

    // 5. 存储 .glslp 参数默认值（用于渲染时作为 uniform float 传入各通道）
    m_params.clear();
    for (const auto& p : paramDescs) {
        m_params.emplace_back(p.name, p.value);
        brls::Logger::debug("RetroShaderPipeline: 参数 \"{}\" = {}", p.name, p.value);
    }

    brls::Logger::info("RetroShaderPipeline: 加载完成，共 {} 个通道，{} 个外部纹理，{} 个参数覆盖",
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
        pass.width  = 0;
        pass.height = 0;
    }
    m_passes.clear();
    // 释放外部纹理 GL 对象
    for (auto& et : m_textures) {
        if (et.texId) { glDeleteTextures(1, &et.texId); et.texId = 0; }
    }
    m_textures.clear();
    m_params.clear();
    m_quad.deinit();
    m_lastOutW = m_lastOutH = 0;
}

// ============================================================
// allocateFBO – 为通道分配或调整 FBO + 颜色纹理
// ============================================================
bool RetroShaderPipeline::allocateFBO(ShaderPass& pass, int w, int h)
{
    if (w <= 0 || h <= 0) return false;
    if (pass.fbo && pass.width == w && pass.height == h) return true;

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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

    pass.width  = w;
    pass.height = h;
    //brls::Logger::debug("RetroShaderPipeline: 分配 FBO id={} {}×{}", pass.fbo, w, h);
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
        switch (type) {
            case ShaderPassDesc::ScaleType::Viewport:
                return std::max(1, static_cast<int>(std::round(
                    static_cast<float>(vp) * scale)));
            case ShaderPassDesc::ScaleType::Absolute:
                return std::max(1, static_cast<int>(std::round(scale)));
            case ShaderPassDesc::ScaleType::Source:
            default:
                return std::max(1, static_cast<int>(std::round(
                    static_cast<float>(src) * scale)));
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
                                       unsigned inW, unsigned inH,
                                       unsigned outW, unsigned outH,
                                       unsigned origW, unsigned origH,
                                       unsigned frameCount,
                                       const std::vector<std::pair<std::string,GLuint>>& extraTexUnits)
{
    auto setUniform1i = [&](const char* name, int v) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, v);
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

    // MVP 单位矩阵
    setUniformMat4("MVPMatrix", k_identity4x4);

    // 帧计数 / 方向
    setUniform1i("FrameCount",     static_cast<int>(frameCount));
    setUniform1i("FrameDirection", 1);

    // 输入纹理尺寸（TextureSize / InputSize 均指输入）
    setUniform2f("TextureSize", static_cast<float>(inW), static_cast<float>(inH));
    setUniform2f("InputSize",   static_cast<float>(inW), static_cast<float>(inH));
    setUniform4f("SourceSize",
                 static_cast<float>(inW),  static_cast<float>(inH),
                 1.f / static_cast<float>(inW), 1.f / static_cast<float>(inH));

    // 输出尺寸
    setUniform2f("OutputSize", static_cast<float>(outW), static_cast<float>(outH));
    setUniform4f("OutputSize4",
                 static_cast<float>(outW), static_cast<float>(outH),
                 1.f / static_cast<float>(outW), 1.f / static_cast<float>(outH));

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
        GLint loc = glGetUniformLocation(program, param.first.c_str());
        if (loc >= 0) glUniform1f(loc, param.second);
    }
}

// ============================================================
// loadTextureFromFile – 从图像文件加载 GL 纹理
// ============================================================
GLuint RetroShaderPipeline::loadTextureFromFile(const std::string& path, bool filterLinear)
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
        //brls::Logger::warning("RetroShaderPipeline::process: FullscreenQuad 未初始化，直通返回");
        return inputTex;
    }

    // //brls::Logger::debug("RetroShaderPipeline::process: 输入 tex={} {}×{} 视口 {}×{} 帧={}",
    //                     inputTex, videoW, videoH, viewW, viewH, frameCount);

    // 保存 GL 状态，管线结束后恢复
    GLuint prevFBO      = 0;
    GLint  prevViewport[4] = {};
    GLuint prevProg     = 0;
    GLuint prevVAO      = 0;
    GLuint prevTex      = 0;
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

    GLuint currentTex = inputTex;
    unsigned currentW = videoW;
    unsigned currentH = videoH;
    GLuint   maxTexUnit = 0; // 记录实际使用的最大纹理单元编号，用于恢复时精确解绑

    for (size_t idx = 0; idx < m_passes.size(); ++idx) {
        auto& pass = m_passes[idx];
        if (!pass.program) {
            // 跳过编译失败的通道，直接传入输出
            brls::Logger::debug("RetroShaderPipeline: 跳过通道 {} (无程序)", idx);
            continue;
        }

        // 计算本通道输出尺寸
        int outW = static_cast<int>(currentW);
        int outH = static_cast<int>(currentH);
        computePassSize(pass.desc, currentW, currentH, viewW, viewH, outW, outH);

        //brls::Logger::debug("RetroShaderPipeline: 通道 {}: 输入 tex={} {}×{} → 输出 {}×{}",
                            // idx, currentTex, currentW, currentH, outW, outH);

        // 确保 FBO 已分配且尺寸正确
        if (!allocateFBO(pass, outW, outH)) {
            brls::Logger::warning("RetroShaderPipeline: 跳过通道 {}（FBO 分配失败）", idx);
            continue;
        }

        // 绑定输出 FBO 和视口
        glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);
        glViewport(0, 0, outW, outH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 激活着色器
        glUseProgram(pass.program);

        // 绑定主输入纹理到纹理单元 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex);

        // 构建额外纹理单元列表（外部纹理 + 已完成通道的输出）
        // 纹理单元 1..N：外部纹理（COLOR_PALETTE、BACKGROUND 等）
        // 纹理单元 N+1..：历史 Pass 输出（Pass0Texture / PassPrevNTexture / <alias>Texture）
        std::vector<std::pair<std::string, GLuint>> extraTexUnits;
        {
            GLuint unit = 1;
            // 外部纹理
            for (const auto& et : m_textures) {
                if (et.texId) {
                    glActiveTexture(GL_TEXTURE0 + unit);
                    glBindTexture(GL_TEXTURE_2D, et.texId);
                    extraTexUnits.emplace_back(et.name, unit);
                    ++unit;
                }
            }
            // 已完成通道的输出
            // PassNTexture（绝对索引）+ PassPrevMTexture（相对索引，M = idx-pi）
            // + <alias>Texture（通道别名）
            for (size_t pi = 0; pi < idx; ++pi) {
                const auto& prev = m_passes[pi];
                if (!prev.texture) continue;
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, prev.texture);
                // 绝对索引：PassNTexture
                extraTexUnits.emplace_back("Pass" + std::to_string(pi) + "Texture", unit);
                // 相对索引：PassPrevNTexture（N = 当前通道索引 - 历史通道索引）
                size_t prevN = idx - pi;
                extraTexUnits.emplace_back("PassPrev" + std::to_string(prevN) + "Texture", unit);
                // 若该通道定义了 alias，也以 <alias>Texture 绑定
                if (!prev.alias.empty()) {
                    extraTexUnits.emplace_back(prev.alias + "Texture", unit);
                }
                ++unit;
            }
            if (unit - 1 > maxTexUnit) maxTexUnit = unit - 1;
        }
        glActiveTexture(GL_TEXTURE0); // 恢复活动纹理单元到 0

        // 设置 uniform（含 OrigInputSize / OrigSize 和参数覆盖值）
        setUniforms(pass.program,
                    currentW, currentH,
                    static_cast<unsigned>(outW),
                    static_cast<unsigned>(outH),
                    videoW, videoH,
                    frameCount,
                    extraTexUnits);

        // 设置历史通道的尺寸 uniform（PassNSize / PassPrevNTextureSize / PassPrevNInputSize）
        for (size_t pi = 0; pi < idx; ++pi) {
            const auto& prev = m_passes[pi];
            if (!prev.texture) continue;
            float fw = static_cast<float>(prev.width);
            float fh = static_cast<float>(prev.height);
            float inv_w = (prev.width  > 0) ? 1.f / fw : 0.f;
            float inv_h = (prev.height > 0) ? 1.f / fh : 0.f;
            // 绝对索引尺寸：PassNTextureSize / PassNInputSize
            std::string absPrefix = "Pass" + std::to_string(pi);
            {
                GLint loc;
                loc = glGetUniformLocation(pass.program, (absPrefix + "TextureSize").c_str());
                if (loc >= 0) glUniform2f(loc, fw, fh);
                loc = glGetUniformLocation(pass.program, (absPrefix + "InputSize").c_str());
                if (loc >= 0) glUniform2f(loc, fw, fh);
                loc = glGetUniformLocation(pass.program, (absPrefix + "Size").c_str());
                if (loc >= 0) glUniform4f(loc, fw, fh, inv_w, inv_h);
            }
            // 相对索引尺寸：PassPrevNTextureSize / PassPrevNInputSize
            size_t prevN = idx - pi;
            std::string prevPrefix = "PassPrev" + std::to_string(prevN);
            {
                GLint loc;
                loc = glGetUniformLocation(pass.program, (prevPrefix + "TextureSize").c_str());
                if (loc >= 0) glUniform2f(loc, fw, fh);
                loc = glGetUniformLocation(pass.program, (prevPrefix + "InputSize").c_str());
                if (loc >= 0) glUniform2f(loc, fw, fh);
                loc = glGetUniformLocation(pass.program, (prevPrefix + "Size").c_str());
                if (loc >= 0) glUniform4f(loc, fw, fh, inv_w, inv_h);
            }
            // 别名尺寸：<alias>Size / <alias>TextureSize / <alias>InputSize
            if (!prev.alias.empty()) {
                GLint loc;
                loc = glGetUniformLocation(pass.program, (prev.alias + "TextureSize").c_str());
                if (loc >= 0) glUniform2f(loc, fw, fh);
                loc = glGetUniformLocation(pass.program, (prev.alias + "InputSize").c_str());
                if (loc >= 0) glUniform2f(loc, fw, fh);
                loc = glGetUniformLocation(pass.program, (prev.alias + "Size").c_str());
                if (loc >= 0) glUniform4f(loc, fw, fh, inv_w, inv_h);
            }
        }

        // 绘制全屏四边形
        m_quad.draw();

        // //brls::Logger::debug("RetroShaderPipeline: 通道 {} 完成，输出 tex={}", idx, pass.texture);

        // 本通道输出成为下一通道输入
        currentTex = pass.texture;
        currentW   = static_cast<unsigned>(outW);
        currentH   = static_cast<unsigned>(outH);
    }

    m_lastOutW = currentW;
    m_lastOutH = currentH;

    // //brls::Logger::debug("RetroShaderPipeline::process: 最终输出 tex={} {}×{}",
    //                     currentTex, m_lastOutW, m_lastOutH);

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

    return currentTex;
}

} // namespace beiklive
