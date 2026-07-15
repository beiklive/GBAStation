#include "three_ds_stub/ThreeDSRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "three_ds_stub/ThreeDSLog.hpp"
#include "video_core/gpu.h"
#include "video_core/renderer_software/renderer_software.h"

namespace beiklive::three_ds_stub {
namespace {

constexpr int kOutputWidth = 1280;
constexpr int kOutputHeight = 720;

struct Rect {
    int x;
    int y;
    int width;
    int height;
};

constexpr Rect kTopRect{240, 0, 800, 480};
constexpr Rect kBottomRect{480, 480, 320, 240};

std::uint32_t PackRgba(const std::uint8_t* rgba) {
    return static_cast<std::uint32_t>(rgba[0]) |
           (static_cast<std::uint32_t>(rgba[1]) << 8) |
           (static_cast<std::uint32_t>(rgba[2]) << 16) |
           (0xFFu << 24);
}

void BlitScreen(std::uint32_t* output, int stride_pixels, const SwRenderer::ScreenInfo& info,
                const Rect& rect) {
    if (!output || info.pixels.empty() || info.width == 0 || info.height == 0) {
        return;
    }

    const std::uint32_t native_width = info.height;
    const std::uint32_t native_height = info.width;
    for (int oy = 0; oy < rect.height; ++oy) {
        const std::uint32_t source_y = static_cast<std::uint32_t>(oy) * native_height / rect.height;
        std::uint32_t* row = output + (rect.y + oy) * stride_pixels + rect.x;
        for (int ox = 0; ox < rect.width; ++ox) {
            const std::uint32_t source_x = static_cast<std::uint32_t>(ox) * native_width / rect.width;
            const std::size_t offset =
                (static_cast<std::size_t>(source_y) * info.height + source_x) * 4;
            if (offset + 3 < info.pixels.size()) {
                row[ox] = PackRgba(info.pixels.data() + offset);
            }
        }
    }
}

} // namespace

bool ThreeDSRenderer::Init() {
    if (initialized_) {
        return true;
    }

    libnx_Result rc = framebufferCreate(&framebuffer_, nwindowGetDefault(), kOutputWidth,
                                        kOutputHeight, PIXEL_FORMAT_RGBA_8888, 2);
    if (R_FAILED(rc)) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: framebufferCreate failed rc=%#x", rc);
        return false;
    }

    rc = framebufferMakeLinear(&framebuffer_);
    if (R_FAILED(rc)) {
        logMessage(LogLevel::Error,
                   "GBAStation3DSStub: framebufferMakeLinear failed rc=%#x", rc);
        framebufferClose(&framebuffer_);
        return false;
    }

    initialized_ = true;
    Clear(0xFF101820u);
    return true;
}

void ThreeDSRenderer::Shutdown() {
    if (!initialized_) {
        return;
    }
    framebufferClose(&framebuffer_);
    framebuffer_ = {};
    initialized_ = false;
}

void ThreeDSRenderer::Clear(std::uint32_t color) {
    if (!initialized_) {
        return;
    }
    std::uint32_t stride = 0;
    auto* pixels = static_cast<std::uint32_t*>(framebufferBegin(&framebuffer_, &stride));
    if (pixels) {
        const int stride_pixels = static_cast<int>(stride / sizeof(std::uint32_t));
        for (int y = 0; y < kOutputHeight; ++y) {
            std::fill_n(pixels + y * stride_pixels, kOutputWidth, color);
        }
    }
    framebufferEnd(&framebuffer_);
}

void ThreeDSRenderer::Present(const SwRenderer::ScreenInfo& top,
                              const SwRenderer::ScreenInfo& bottom) {
    if (!initialized_) {
        return;
    }

    std::uint32_t stride = 0;
    auto* pixels = static_cast<std::uint32_t*>(framebufferBegin(&framebuffer_, &stride));
    if (pixels) {
        const int stride_pixels = static_cast<int>(stride / sizeof(std::uint32_t));
        for (int y = 0; y < kOutputHeight; ++y) {
            std::fill_n(pixels + y * stride_pixels, kOutputWidth, 0xFF101820u);
        }
        BlitScreen(pixels, stride_pixels, top, kTopRect);
        BlitScreen(pixels, stride_pixels, bottom, kBottomRect);
    }
    framebufferEnd(&framebuffer_);
}

void ThreeDSRenderer::PresentStatus(const char* title, const char* detail) {
    logMessage(LogLevel::Error, "GBAStation3DSStub: status title=%s detail=%s",
               title ? title : "", detail ? detail : "");
    Clear(0xFF18202Au);
}

} // namespace beiklive::three_ds_stub
