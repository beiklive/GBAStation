#pragma once

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
        Custom = 5,
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

private:
    struct ScreenDrawRect {
        bool sourceTop = true;
        RectF rect {};
    };

    RectF touchRect() const;
    std::vector<ScreenDrawRect> computeScreenRects() const;
    RectF firstRectForSource(bool sourceTop) const;

    GPU2D::DekoRenderer* m_renderer = nullptr;
    std::array<std::array<u32, 2>, 2> m_framebufferTextures {};
    bool m_waitForFramebufferReady = false;
    bool m_linearFiltering = false;
    bool m_screensSwapped = false;
    bool m_integerScale = false;
    ScreenLayout m_layout = ScreenLayout::Vertical;
};

} // namespace beiklive::nds_stub
