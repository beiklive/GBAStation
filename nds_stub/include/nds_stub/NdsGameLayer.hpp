#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <switch.h>

#include "nds_stub/NdsStubTypes.hpp"

namespace GPU2D {
class DekoRenderer;
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
    void drawScreens() const;
    bool captureCurrentFrameRgba(std::vector<std::uint8_t>& outRgba,
                                 int& outWidth,
                                 int& outHeight) const;
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
    RectF firstRectForSource(bool sourceTop) const;

    GPU2D::DekoRenderer* m_renderer = nullptr;
    std::array<std::array<u32, 2>, 2> m_framebufferTextures {};
    bool m_waitForFramebufferReady = false;
    bool m_linearFiltering = false;
    bool m_screensSwapped = false;
    bool m_integerScale = false;
    ScreenLayout m_layout = ScreenLayout::Vertical;
    int m_orientation = 0;
    float m_screenGap = 0.0f;
    NdsCustomLayoutSettings m_customLayout {};
    bool m_overlayEnabled = false;
    std::uint32_t m_overlayTexture = 0;
    int m_overlayWidth = 0;
    int m_overlayHeight = 0;
};

} // namespace beiklive::nds_stub
