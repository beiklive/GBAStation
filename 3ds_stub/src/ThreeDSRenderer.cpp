#include "three_ds_stub/ThreeDSRenderer.hpp"

#include <array>
#include <cstdio>
#include <string_view>
#include <utility>

#include <EGL/eglext.h>
#include <glad/glad.h>

#include "core/frontend/emu_window.h"
#include "three_ds_stub/ThreeDSLog.hpp"

namespace beiklive::three_ds_stub {
namespace {

constexpr std::array<EGLint, 17> kFramebufferAttributes{
    EGL_SURFACE_TYPE,
    EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_DEPTH_SIZE,
    24,
    EGL_STENCIL_SIZE,
    8,
    EGL_NONE,
};

constexpr std::array<EGLint, 13> kFallbackFramebufferAttributes{
    EGL_SURFACE_TYPE,
    EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_NONE,
};

constexpr std::array<EGLint, 13> kSharedFramebufferAttributes{
    EGL_SURFACE_TYPE,
    EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_NONE,
};

constexpr std::array<EGLint, 7> kContextAttributes{
    EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
    EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
    EGL_CONTEXT_MAJOR_VERSION_KHR,
    4,
    EGL_CONTEXT_MINOR_VERSION_KHR,
    3,
    EGL_NONE,
};

constexpr std::array<EGLint, 5> kPbufferAttributes{
    EGL_WIDTH,
    1,
    EGL_HEIGHT,
    1,
    EGL_NONE,
};

constexpr std::array<std::uint8_t, 5> Glyph(char character) {
    switch (character) {
    case '0': return {0b111, 0b101, 0b101, 0b101, 0b111};
    case '1': return {0b010, 0b110, 0b010, 0b010, 0b111};
    case '2': return {0b111, 0b001, 0b111, 0b100, 0b111};
    case '3': return {0b111, 0b001, 0b111, 0b001, 0b111};
    case '4': return {0b101, 0b101, 0b111, 0b001, 0b001};
    case '5': return {0b111, 0b100, 0b111, 0b001, 0b111};
    case '6': return {0b111, 0b100, 0b111, 0b101, 0b111};
    case '7': return {0b111, 0b001, 0b010, 0b010, 0b010};
    case '8': return {0b111, 0b101, 0b111, 0b101, 0b111};
    case '9': return {0b111, 0b101, 0b111, 0b001, 0b111};
    case 'F': return {0b111, 0b100, 0b110, 0b100, 0b100};
    case 'P': return {0b110, 0b101, 0b110, 0b100, 0b100};
    case 'S': return {0b111, 0b100, 0b111, 0b001, 0b111};
    case ':': return {0b000, 0b010, 0b000, 0b010, 0b000};
    case '.': return {0b000, 0b000, 0b000, 0b000, 0b010};
    default: return {0, 0, 0, 0, 0};
    }
}

class SwitchSharedContext final : public Frontend::GraphicsContext {
public:
    SwitchSharedContext(EGLDisplay display, EGLConfig config, EGLContext shared_context)
        : display_{display} {
        surface_ = eglCreatePbufferSurface(display_, config, kPbufferAttributes.data());
        if (surface_ == EGL_NO_SURFACE) {
            logMessage(LogLevel::Error,
                       "GBAStation3DSStub: eglCreatePbufferSurface failed error=%#x",
                       eglGetError());
            return;
        }
        context_ = eglCreateContext(display_, config, shared_context, kContextAttributes.data());
        if (context_ == EGL_NO_CONTEXT) {
            logMessage(LogLevel::Error,
                       "GBAStation3DSStub: shared eglCreateContext failed error=%#x",
                       eglGetError());
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
    }

    ~SwitchSharedContext() override {
        if (eglGetCurrentContext() == context_) {
            DoneCurrent();
        }
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
        }
    }

    bool IsGLES() override {
        return false;
    }

    void MakeCurrent() override {
        if (context_ != EGL_NO_CONTEXT &&
            eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
            logMessage(LogLevel::Error,
                       "GBAStation3DSStub: shared eglMakeCurrent failed error=%#x",
                       eglGetError());
        }
    }

    void DoneCurrent() override {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    bool IsValid() const {
        return context_ != EGL_NO_CONTEXT && surface_ != EGL_NO_SURFACE;
    }

private:
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
};

} // namespace

bool ThreeDSRenderer::Init() {
    if (initialized_) {
        return true;
    }

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: eglGetDisplay failed error=%#x",
                   eglGetError());
        return false;
    }

    EGLint egl_major = 0;
    EGLint egl_minor = 0;
    if (eglInitialize(display_, &egl_major, &egl_minor) != EGL_TRUE) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: eglInitialize failed error=%#x",
                   eglGetError());
        Shutdown();
        return false;
    }
    if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: eglBindAPI(OpenGL) failed error=%#x",
                   eglGetError());
        Shutdown();
        return false;
    }

    EGLint config_count = 0;
    if (eglChooseConfig(display_, kFramebufferAttributes.data(), &config_, 1, &config_count) !=
            EGL_TRUE ||
        config_count < 1) {
        logMessage(LogLevel::Warning,
                   "GBAStation3DSStub: preferred EGL config unavailable; retrying without default depth/stencil count=%d error=%#x",
                   config_count, eglGetError());
        config_count = 0;
        if (eglChooseConfig(display_, kFallbackFramebufferAttributes.data(), &config_, 1,
                            &config_count) != EGL_TRUE ||
            config_count < 1) {
            logMessage(LogLevel::Error,
                       "GBAStation3DSStub: fallback eglChooseConfig failed count=%d error=%#x",
                       config_count, eglGetError());
            Shutdown();
            return false;
        }
    }

    EGLint shared_config_count = 0;
    if (eglChooseConfig(display_, kSharedFramebufferAttributes.data(), &shared_config_, 1,
                        &shared_config_count) != EGL_TRUE ||
        shared_config_count < 1) {
        shared_config_ = {};
        logMessage(LogLevel::Warning,
                   "GBAStation3DSStub: no EGL PBuffer config; background GL contexts disabled error=%#x",
                   eglGetError());
    }

    surface_ = eglCreateWindowSurface(display_, config_, nwindowGetDefault(), nullptr);
    if (surface_ == EGL_NO_SURFACE) {
        logMessage(LogLevel::Error,
                   "GBAStation3DSStub: eglCreateWindowSurface failed error=%#x", eglGetError());
        Shutdown();
        return false;
    }

    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, kContextAttributes.data());
    if (context_ == EGL_NO_CONTEXT) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: OpenGL 4.3 eglCreateContext failed error=%#x",
                   eglGetError());
        Shutdown();
        return false;
    }
    if (!MakeCurrent()) {
        Shutdown();
        return false;
    }
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress))) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: gladLoadGLLoader failed");
        Shutdown();
        return false;
    }
    if (!GLAD_GL_VERSION_4_3) {
        const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        logMessage(LogLevel::Error,
                   "GBAStation3DSStub: OpenGL 4.3 unavailable version=%s",
                   version ? version : "<null>");
        Shutdown();
        return false;
    }

    initialized_ = true;
    eglSwapInterval(display_, 1);
    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    logMessage(LogLevel::Info,
               "GBAStation3DSStub: EGL initialized version=%d.%d GL_VERSION=%s GL_VENDOR=%s GL_RENDERER=%s",
               egl_major, egl_minor, version ? version : "<null>", vendor ? vendor : "<null>",
               renderer ? renderer : "<null>");
    Clear(0.063f, 0.094f, 0.125f);
    SwapBuffers();
    return true;
}

void ThreeDSRenderer::Shutdown() {
    if (display_ != EGL_NO_DISPLAY && eglGetCurrentContext() == context_) {
        DoneCurrent();
    }
    if (display_ != EGL_NO_DISPLAY && context_ != EGL_NO_CONTEXT) {
        eglDestroyContext(display_, context_);
    }
    if (display_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(display_, surface_);
    }
    if (display_ != EGL_NO_DISPLAY) {
        eglTerminate(display_);
    }
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    config_ = {};
    shared_config_ = {};
    display_ = EGL_NO_DISPLAY;
    initialized_ = false;
}

bool ThreeDSRenderer::IsInitialized() const {
    return initialized_;
}

bool ThreeDSRenderer::MakeCurrent() {
    if (display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE ||
        context_ == EGL_NO_CONTEXT) {
        return false;
    }
    if (eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: eglMakeCurrent failed error=%#x",
                   eglGetError());
        return false;
    }
    return true;
}

void ThreeDSRenderer::DoneCurrent() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

void ThreeDSRenderer::PreparePresent() {
    if (initialized_) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
    }
}

void ThreeDSRenderer::DrawFps(double fps) {
    if (!initialized_) {
        return;
    }

    GLint previous_framebuffer = 0;
    GLint previous_draw_buffer = GL_BACK;
    GLint previous_scissor[4]{};
    GLfloat previous_clear_color[4]{};
    GLboolean previous_color_mask[4]{};
    const GLboolean scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_framebuffer);
    glGetIntegerv(GL_DRAW_BUFFER, &previous_draw_buffer);
    glGetIntegerv(GL_SCISSOR_BOX, previous_scissor);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previous_clear_color);
    glGetBooleanv(GL_COLOR_WRITEMASK, previous_color_mask);

    EGLint surface_height = 720;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &surface_height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    // Azahar's render FBO can leave a color-attachment draw target in the GL state cache. The
    // Switch window framebuffer needs GL_BACK explicitly, otherwise glClear-based glyphs vanish.
    glDrawBuffer(GL_BACK);
    glEnable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    const auto fill_rect = [surface_height](int x, int y, int width, int height) {
        glScissor(x, surface_height - y - height, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
    };

    char text[32]{};
    std::snprintf(text, sizeof(text), "FPS:%.1f", fps);
    const std::string_view label{text};
    constexpr int scale = 4;
    constexpr int glyph_width = 3 * scale;
    constexpr int glyph_height = 5 * scale;
    constexpr int advance = glyph_width + scale;
    constexpr int origin_x = 12;
    constexpr int origin_y = 12;
    constexpr int padding = 6;

    glClearColor(0.02f, 0.03f, 0.04f, 1.0f);
    fill_rect(origin_x - padding, origin_y - padding,
              static_cast<int>(label.size()) * advance - scale + padding * 2,
              glyph_height + padding * 2);

    glClearColor(0.35f, 1.0f, 0.35f, 1.0f);
    for (std::size_t character_index = 0; character_index < label.size(); ++character_index) {
        const auto glyph = Glyph(label[character_index]);
        for (int row = 0; row < 5; ++row) {
            for (int column = 0; column < 3; ++column) {
                if ((glyph[row] & (1U << (2 - column))) != 0) {
                    fill_rect(origin_x + static_cast<int>(character_index) * advance +
                                  column * scale,
                              origin_y + row * scale, scale, scale);
                }
            }
        }
    }

    glClearColor(previous_clear_color[0], previous_clear_color[1], previous_clear_color[2],
                 previous_clear_color[3]);
    glColorMask(previous_color_mask[0], previous_color_mask[1], previous_color_mask[2],
                previous_color_mask[3]);
    glScissor(previous_scissor[0], previous_scissor[1], previous_scissor[2],
              previous_scissor[3]);
    if (!scissor_was_enabled) {
        glDisable(GL_SCISSOR_TEST);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previous_framebuffer);
    glDrawBuffer(previous_draw_buffer);
    if (!fps_overlay_logged_) {
        const GLenum error = glGetError();
        logMessage(error == GL_NO_ERROR ? LogLevel::Info : LogLevel::Warning,
                   "GBAStation3DSStub: FPS overlay initialized surface_height=%d gl_error=%#x",
                   surface_height, error);
        fps_overlay_logged_ = true;
    }
}

void ThreeDSRenderer::SwapBuffers() {
    if (initialized_ && eglSwapBuffers(display_, surface_) != EGL_TRUE) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: eglSwapBuffers failed error=%#x",
                   eglGetError());
    }
}

std::unique_ptr<Frontend::GraphicsContext> ThreeDSRenderer::CreateSharedContext() const {
    if (display_ == EGL_NO_DISPLAY || shared_config_ == nullptr ||
        context_ == EGL_NO_CONTEXT) {
        return nullptr;
    }
    auto context = std::make_unique<SwitchSharedContext>(display_, shared_config_, context_);
    if (!context->IsValid()) {
        return nullptr;
    }
    return context;
}

void ThreeDSRenderer::Clear(float red, float green, float blue) {
    if (!initialized_) {
        return;
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ThreeDSRenderer::PresentStatus(const char* title, const char* detail) {
    logMessage(LogLevel::Error, "GBAStation3DSStub: status title=%s detail=%s",
               title ? title : "", detail ? detail : "");
    if (MakeCurrent()) {
        Clear(0.094f, 0.125f, 0.165f);
        SwapBuffers();
    }
}

} // namespace beiklive::three_ds_stub
