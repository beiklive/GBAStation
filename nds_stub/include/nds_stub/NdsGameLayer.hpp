#pragma once

#include <array>

#include <switch.h>

#include "nds_stub/NdsStubTypes.hpp"

namespace GPU2D {
class DekoRenderer;
}

namespace beiklive::nds_stub {

class NdsGameLayer {
public:
    void init(GPU2D::DekoRenderer* renderer);
    void deinit();

    RectF topRect() const;
    RectF bottomRect() const;

    bool readTouch(u16& outX, u16& outY) const;
    void drawScreens() const;
    void setWaitForFramebufferReady(bool enabled) { m_waitForFramebufferReady = enabled; }

private:
    GPU2D::DekoRenderer* m_renderer = nullptr;
    std::array<std::array<u32, 2>, 2> m_framebufferTextures {};
    bool m_waitForFramebufferReady = false;
};

} // namespace beiklive::nds_stub
