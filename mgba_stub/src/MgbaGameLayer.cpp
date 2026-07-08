#include "mgba_stub/MgbaGameLayer.hpp"

#include "mgba_stub/MgbaShaderCatalog.hpp"

#include <algorithm>
#include <cmath>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::mgba_stub {
namespace {

Gfx::ShaderMode shaderModeFromType(const std::string& type)
{
    const std::string shader = normalizeMgbaShaderType(type);
    const std::string key = MgbaShaderMatchKey(shader);
    if (shader == "RetroArch_dot-clear")
        return Gfx::shaderMode_NdsDotClear;
    if (shader == "RetroArch_lcd-grid-v2-nds-color" ||
        key.find("lcd-grid") != std::string::npos)
        return Gfx::shaderMode_NdsLcdGridNdsColor;
    if (shader == "RetroArch_xbrz-freescale" ||
        key.find("xbr") != std::string::npos ||
        key.find("sabr") != std::string::npos ||
        key.find("hq") != std::string::npos ||
        key.find("scale2x") != std::string::npos)
        return Gfx::shaderMode_NdsXbrzFreescale;
    return Gfx::shaderMode_NdsDot;
}

float paramValue(const std::vector<MgbaShaderParam>& params, const char* name, float fallback)
{
    for (const auto& param : params)
    {
        if (param.name == name)
            return param.value;
    }
    return fallback;
}

} // namespace

MgbaGameLayer::~MgbaGameLayer()
{
    releaseGraphicsResources();
}

void MgbaGameLayer::releaseGraphicsResources()
{
    if (m_gameTexture != 0)
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(m_gameTexture);
        m_gameTexture = 0;
    }
    m_width = 0;
    m_height = 0;
    clearOverlay();
}

bool MgbaGameLayer::createTexture(unsigned width, unsigned height)
{
    if (width == 0 || height == 0)
        return false;
    if (m_gameTexture != 0 && m_width == width && m_height == height)
        return true;
    if (m_gameTexture != 0)
        Gfx::TextureDelete(m_gameTexture);
    m_gameTexture = Gfx::TextureCreate(width, height, DkImageFormat_RGBA8_Unorm);
    m_width = width;
    m_height = height;
    return m_gameTexture != 0;
}

void MgbaGameLayer::uploadFrame(const void* rgba, unsigned width, unsigned height, unsigned strideBytes)
{
    if (!rgba || !createTexture(width, height))
        return;
    Gfx::TextureUpload(m_gameTexture, 0, 0, width, height, const_cast<void*>(rgba), strideBytes);
}

void MgbaGameLayer::setOverlay(std::uint32_t texture, int width, int height)
{
    m_overlayTexture = texture;
    m_overlayWidth = width;
    m_overlayHeight = height;
}

void MgbaGameLayer::clearOverlay()
{
    m_overlayTexture = 0;
    m_overlayWidth = 0;
    m_overlayHeight = 0;
}

void MgbaGameLayer::setDisplaySettings(const MgbaDisplaySettings& display)
{
    m_display = display;
    m_display.mgbaShaderType = normalizeMgbaShaderType(m_display.mgbaShaderType);
}

MgbaGameLayer::DrawRect MgbaGameLayer::computeDrawRect() const
{
    DrawRect rect;
    if (m_width == 0 || m_height == 0)
        return rect;

    const float screenW = static_cast<float>(kScreenWidth);
    const float screenH = static_cast<float>(kScreenHeight);
    const float gameW = static_cast<float>(m_width);
    const float gameH = static_cast<float>(m_height);

    switch (m_display.layout)
    {
    case 0:
    {
        const float scale = std::min(screenW / gameW, screenH / gameH);
        rect.w = gameW * scale;
        rect.h = gameH * scale;
        break;
    }
    case 1:
        return {0.0f, 0.0f, screenW, screenH};
    case 2:
        rect.w = gameW;
        rect.h = gameH;
        break;
    case 3:
        rect.h = screenH;
        rect.w = screenH * (4.0f / 3.0f);
        break;
    case 5:
    case 7:
    {
        const float scale = std::clamp(m_display.customLayout.topScale, 1.0f, 15.0f);
        rect.w = gameW * scale;
        rect.h = gameH * scale;
        rect.x = (screenW - rect.w) * 0.5f + m_display.customLayout.topOffsetX;
        rect.y = (screenH - rect.h) * 0.5f + m_display.customLayout.topOffsetY;
        return rect;
    }
    case 4:
    {
        const float base = std::min(screenW / gameW, screenH / gameH);
        const float scale = m_display.integerScaleMultiplier > 0
                                ? static_cast<float>(m_display.integerScaleMultiplier)
                                : std::max(1.0f, std::floor(base));
        rect.w = gameW * scale;
        rect.h = gameH * scale;
        break;
    }
    default:
    {
        const float scale = std::min(screenW / gameW, screenH / gameH);
        rect.w = gameW * scale;
        rect.h = gameH * scale;
        break;
    }
    }

    rect.x = (screenW - rect.w) * 0.5f;
    rect.y = (screenH - rect.h) * 0.5f;
    return rect;
}

std::array<float, 8> MgbaGameLayer::shaderUniforms(const std::string& type,
                                                   const std::vector<MgbaShaderParam>& params)
{
    std::array<float, 8> values {};
    const std::string shader = normalizeMgbaShaderType(type);
    if (shader == "RetroArch_dot")
    {
        values[0] = paramValue(params, "gamma", 2.4f);
        values[1] = paramValue(params, "shine", 0.05f);
        values[2] = paramValue(params, "blend", 0.65f);
    }
    else if (shader == "RetroArch_dot-clear")
    {
        values[0] = paramValue(params, "screen_gamma", 2.2f);
        values[1] = paramValue(params, "dot_gamma", 2.2f);
        values[2] = paramValue(params, "dot_scale_x", 1.1f);
        values[3] = paramValue(params, "dot_scale_y", 1.1f);
        values[4] = paramValue(params, "dot_opacity", 0.7f);
        values[5] = paramValue(params, "halftone_strength", 0.7f);
    }
    else if (shader == "RetroArch_lcd-grid-v2-nds-color")
    {
        values[0] = paramValue(params, "gain", 1.5f);
        values[1] = paramValue(params, "gamma", 2.2f);
        values[2] = paramValue(params, "blacklevel", 0.0f);
        values[3] = paramValue(params, "ambient", 0.0f);
        values[4] = paramValue(params, "bgr", 1.0f);
        values[5] = paramValue(params, "nds_color", 1.0f);
    }
    return values;
}

void MgbaGameLayer::draw() const
{
    if (m_gameTexture == 0 || m_width == 0 || m_height == 0)
        return;

    const DrawRect rect = computeDrawRect();
    const bool useShader = m_display.shaderEnabled;
    Gfx::SetSampler((useShader ? Gfx::sampler_Nearest :
                    (m_display.linearFiltering ? Gfx::sampler_Linear : Gfx::sampler_Nearest)) |
                    Gfx::sampler_ClampToEdge);
    if (useShader)
    {
        Gfx::SetNdsShaderParams(shaderUniforms(m_display.mgbaShaderType, m_display.shaderParams));
        Gfx::SetShaderMode(shaderModeFromType(m_display.mgbaShaderType));
    }

    Gfx::DrawRectangle(m_gameTexture,
                       {rect.x, rect.y},
                       {rect.w, rect.h},
                       {0.0f, 0.0f},
                       {static_cast<float>(m_width), static_cast<float>(m_height)},
                       {1.0f, 1.0f, 1.0f, 1.0f});
    Gfx::SetShaderMode(Gfx::shaderMode_Default);

    if (m_display.overlayEnabled && m_overlayTexture != 0 && m_overlayWidth > 0 && m_overlayHeight > 0)
    {
        Gfx::SetSampler(Gfx::sampler_Linear | Gfx::sampler_ClampToEdge);
        Gfx::DrawRectangle(m_overlayTexture,
                           {0.0f, 0.0f},
                           {static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)},
                           {0.0f, 0.0f},
                           {static_cast<float>(m_overlayWidth), static_cast<float>(m_overlayHeight)},
                           {1.0f, 1.0f, 1.0f, 1.0f});
    }
    Gfx::SetSampler(Gfx::sampler_Nearest | Gfx::sampler_ClampToEdge);
}

} // namespace beiklive::mgba_stub
