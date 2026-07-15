#pragma once

#include <cstdint>

#include "three_ds_stub/ThreeDSSwitch.hpp"

namespace SwRenderer {
struct ScreenInfo;
}

namespace beiklive::three_ds_stub {

class ThreeDSRenderer {
public:
    bool Init();
    void Shutdown();
    void Present(const SwRenderer::ScreenInfo& top, const SwRenderer::ScreenInfo& bottom);
    void PresentStatus(const char* title, const char* detail);

private:
    void Clear(std::uint32_t color);

    Framebuffer framebuffer_{};
    bool initialized_ = false;
};

} // namespace beiklive::three_ds_stub
