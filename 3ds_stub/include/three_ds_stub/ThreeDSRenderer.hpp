#pragma once

#include <cstdint>
#include <memory>

#include <EGL/egl.h>

#include "three_ds_stub/ThreeDSSwitch.hpp"

namespace Frontend {
class GraphicsContext;
}

namespace beiklive::three_ds_stub {

class ThreeDSRenderer {
public:
    bool Init();
    void Shutdown();
    bool IsInitialized() const;
    bool MakeCurrent();
    void DoneCurrent();
    void PreparePresent();
    void DrawFps(double fps);
    void SwapBuffers();
    std::unique_ptr<Frontend::GraphicsContext> CreateSharedContext() const;
    void PresentStatus(const char* title, const char* detail);

private:
    void Clear(float red, float green, float blue);

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_{};
    EGLConfig shared_config_{};
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
    bool initialized_ = false;
};

} // namespace beiklive::three_ds_stub
