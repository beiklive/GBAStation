#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <switch.h>

#include "nds_stub/NdsStubTypes.hpp"

namespace GPU2D {
class DekoRenderer;
}
namespace Gfx {
struct NdsFilterPass;
}

namespace beiklive::nds_stub {

class NdsGameLayer {
public:
    enum class ScreenLayout {
        Vertical = 0,
        Horizontal = 1,
        TopPriority = 2,
        BottomPriority = 3,
        HybridHorizontal = 4,
        SingleTop = 5,
        SingleBottom = 6,
        Custom = 7,
    };

    void init(GPU2D::DekoRenderer* renderer);
    void deinit();

    RectF topRect() const;
    RectF bottomRect() const;

    bool readTouch(u16& outX, u16& outY) const;
    bool ndsPointToScreen(bool sourceTop, float ndsX, float ndsY, float& outX, float& outY) const;
    void drawScreens() const;
    bool captureCurrentFrameRgba(std::vector<std::uint8_t>& outRgba,
                                 int& outWidth,
                                 int& outHeight) const;
    bool copyCachedFrameRgba(std::vector<std::uint8_t>& outRgba,
                             int& outWidth,
                             int& outHeight) const;
    bool refreshCaptureCache() const;
    void requestDeferredCapture() const;
    bool refreshMenuFreezeTexture();
    void setMenuFreezeEnabled(bool enabled) { m_menuFreezeEnabled = enabled && m_menuFreezeTexture != 0; }
    bool menuFreezeEnabled() const { return m_menuFreezeEnabled; }
    void setWaitForFramebufferReady(bool enabled) { m_waitForFramebufferReady = enabled; }
    void setLinearFiltering(bool enabled) { m_linearFiltering = enabled; }
    bool linearFiltering() const { return m_linearFiltering; }
    void setScreensSwapped(bool enabled) { m_screensSwapped = enabled; }
    bool screensSwapped() const { return m_screensSwapped; }
    void setScreenLayout(int layout);
    int screenLayout() const { return static_cast<int>(m_layout); }
    void setIntegerScale(bool enabled) { m_integerScale = enabled; }
    bool integerScale() const { return m_integerScale; }
    void setOrientation(int orientation) { m_orientation = std::clamp(orientation, 0, 3); }
    int orientation() const { return m_orientation; }
    void setScreenGap(float gap) { m_screenGap = std::clamp(gap, -256.0f, 256.0f); }
    float screenGap() const { return m_screenGap; }
    void setCustomLayoutSettings(const NdsCustomLayoutSettings& settings) { m_customLayout = settings; }
    const NdsCustomLayoutSettings& customLayoutSettings() const { return m_customLayout; }
    void setOverlayEnabled(bool enabled) { m_overlayEnabled = enabled; }
    bool overlayEnabled() const { return m_overlayEnabled; }
    void setOverlayTexture(std::uint32_t texture, int width, int height);
    void clearOverlayTexture();
    void setShaderEnabled(bool enabled) { m_shaderEnabled = enabled; }
    bool shaderEnabled() const { return m_shaderEnabled; }
    void setShaderType(const std::string& type);
    const std::string& shaderType() const { return m_shaderType; }
    void setShaderParams(const std::array<float, 8>& params) { m_shaderParams = params; }

private:
    struct ScreenDrawRect {
        bool sourceTop = true;
        RectF rect {};
        RectF layoutRect {};
    };

    RectF touchRect() const;
    std::vector<ScreenDrawRect> computeScreenRects() const;
    RectF layoutBounds() const;
    RectF rotateScreenRect(const RectF& rect, const RectF& layoutRect) const;
    bool mapPointToUnrotated(float x, float y, const ScreenDrawRect& item, float& outX, float& outY) const;
    bool mapNdsPointToScreen(float ndsX, float ndsY, const ScreenDrawRect& item, float& outX, float& outY) const;
    RectF firstRectForSource(bool sourceTop) const;
    void drawScreenTexture(const ScreenDrawRect& item,
                           std::uint32_t texture,
                           const RectF& sourceRect) const;
    void drawScreenTextureMultiPass(const ScreenDrawRect& item,
                                    std::uint32_t texture,
                                    const RectF& sourceRect,
                                    const Gfx::NdsFilterPass* passes,
                                    int passCount,
                                    std::uint32_t tempTextureA,
                                    std::uint32_t tempTextureB,
                                    int tempWidth,
                                    int tempHeight) const;
    void clearShaderPassTextures() const;
    bool ensureShaderPassTextures(int width, int height) const;

    GPU2D::DekoRenderer* m_renderer = nullptr;
    std::array<std::array<u32, 2>, 2> m_framebufferTextures {};
    bool m_waitForFramebufferReady = false;
    bool m_linearFiltering = false;
    bool m_screensSwapped = false;
    bool m_integerScale = true;
    ScreenLayout m_layout = ScreenLayout::Vertical;
    int m_orientation = 0;
    float m_screenGap = 0.0f;
    NdsCustomLayoutSettings m_customLayout {};
    bool m_overlayEnabled = false;
    std::uint32_t m_overlayTexture = 0;
    int m_overlayWidth = 0;
    int m_overlayHeight = 0;
    bool m_menuFreezeEnabled = false;
    std::uint32_t m_menuFreezeTexture = 0;
    int m_menuFreezeWidth = 0;
    int m_menuFreezeHeight = 0;
    bool m_shaderEnabled = false;
    std::string m_shaderType = "RetroArch_dot";
    std::array<float, 8> m_shaderParams {2.4f, 0.05f, 0.65f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    mutable std::array<std::uint32_t, 2> m_shaderPassTextures {};
    mutable int m_shaderPassTextureWidth = 0;
    mutable int m_shaderPassTextureHeight = 0;
    mutable std::vector<std::uint8_t> m_lastCaptureRgba;
    mutable int m_lastCaptureWidth = 0;
    mutable int m_lastCaptureHeight = 0;
};

} // namespace beiklive::nds_stub
