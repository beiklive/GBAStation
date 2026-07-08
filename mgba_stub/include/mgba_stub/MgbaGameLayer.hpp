#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "mgba_stub/MgbaMenuLayer.hpp"

namespace beiklive::mgba_stub {

class MgbaGameLayer {
public:
    ~MgbaGameLayer();

    bool createTexture(unsigned width, unsigned height);
    void releaseGraphicsResources();
    void uploadFrame(const void* rgba, unsigned width, unsigned height, unsigned strideBytes);
    void setOverlay(std::uint32_t texture, int width, int height);
    void clearOverlay();
    void setDisplaySettings(const MgbaDisplaySettings& display);
    const MgbaDisplaySettings& displaySettings() const { return m_display; }
    void draw() const;

private:
    struct DrawRect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    DrawRect computeDrawRect() const;
    static std::array<float, 8> shaderUniforms(const std::string& type,
                                               const std::vector<MgbaShaderParam>& params);

    std::uint32_t m_gameTexture = 0;
    unsigned m_width = 0;
    unsigned m_height = 0;
    std::uint32_t m_overlayTexture = 0;
    int m_overlayWidth = 0;
    int m_overlayHeight = 0;
    MgbaDisplaySettings m_display {};
};

} // namespace beiklive::mgba_stub
