//
// Created by cpasjuste on 21/11/16.
//

#include "cross2d/c2d.h"

using namespace c2d;

static SDL_Window *window = nullptr;
static SDL_GLContext context = nullptr;

SDL2Renderer::SDL2Renderer(const Vector2f &s) : GLRenderer(s) {
    // disable mouse cursor
    SDL_ShowCursor(SDL_DISABLE);

#ifdef __PS4__
    //SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "fast");
#ifndef GL_SHADERS_BINARY
    SDL_SetHint(SDL_HINT_PS4_PIGLET_MODULES_PATH, "/data/self/system/common/lib");
#endif
#endif

    if ((SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE)) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't init sdl: %s\n", SDL_GetError());
        return;
    }

    Vector2i size = {(int) Renderer::getSize().x, (int) Renderer::getSize().y};
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
#ifndef ANDROID
    if (size.x <= 0 || size.y <= 0) {
        // force fullscreen if window size == 0
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
#else
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif

#ifdef __GLES2__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif __GLES3__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __GL2__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#elif __GL1__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
#endif
#ifdef __SDL2_RGB565__ // for 16 bits displays (retropico)
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

    window = SDL_CreateWindow(
        "CROSS2D_SDL2_GL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        size.x, size.y, flags);
    if (!window) {
        printf("Couldn't SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s\n", SDL_GetError());
        return;
    }

    SDL_GetWindowSize(window, &size.x, &size.y);
    if (size.x != (int) Renderer::getSize().x || size.y != (int) Renderer::getSize().y) {
        SDL2Renderer::setSize((float) size.x, (float) size.y);
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        printf("Couldn't SDL_GL_CreateContext: %s\n", SDL_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't SDL_GL_CreateContext: %s\n",
                     SDL_GetError());
        return;
    }

    SDL_GL_SetSwapInterval(1);

#if __GLAD__
    // amdgpu proprietary driver 19.30 and SDL2 "getproc" bug
    // it seems safer to also use glad on linux (nintendo switch also use this)
#if defined(__GLES2__) || defined(__GLES3__)
    gladLoadGLES2(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
#else
    gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
#endif
#elif __GLEW__
    glewInit();
#endif

    initGL();

    // we need to delete gl context after all resources are freed by C2DObject destructor
    std::atexit(exitCallback);

    available = true;

    auto display = C2DDevice::getDisplaySize();
    printf("SDL2Renderer(%p): platform: %s, resolution: %i x %i, display: %i x %i\n",
           this, C2DDevice::getName().c_str(),
           (int) m_size.x, (int) m_size.y, display.x, display.y);
}

void SDL2Renderer::setFullscreen(const bool value) {
    if (!value) return;

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

    // update render size
    int w, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    SDL2Renderer::setSize((float) w, (float) h);
}

void SDL2Renderer::flip(bool draw, bool inputs) {
    // call base class (draw childs)
    GLRenderer::flip(draw, inputs);

    // flip
    SDL_GL_SwapWindow(window);
}

void SDL2Renderer::delay(unsigned int ms) {
    SDL_Delay(ms);
}

SDL_Window *SDL2Renderer::getWindow() {
    return window;
}

SDL_GLContext SDL2Renderer::getContext() {
    return context;
}

void SDL2Renderer::exitCallback() {
    if (context != nullptr) {
        SDL_GL_DeleteContext(context);
    }

    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();
}
