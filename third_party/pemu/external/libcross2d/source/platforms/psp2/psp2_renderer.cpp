//
// Created by cpasjuste on 21/11/16.
//

#include <SDL2/SDL.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>

#include "cross2d/platforms/psp2/psp2_renderer.h"
#include "cross2d/platforms/psp2/psp2_texture.h"
#include "cross2d/platforms/psp2/psp2_shaders.h"

#include <psp2/kernel/clib.h>

#define printf sceClibPrintf

using namespace c2d;

int _newlib_heap_size_user = 256 * 1024 * 1024;

extern "C" {
extern float _vita2d_ortho_matrix[4 * 4];
extern SceGxmVertexProgram *_vita2d_colorVertexProgram;
extern SceGxmFragmentProgram *_vita2d_colorFragmentProgram;
extern SceGxmVertexProgram *_vita2d_textureVertexProgram;
extern SceGxmFragmentProgram *_vita2d_textureTintFragmentProgram;
extern const SceGxmProgramParameter *_vita2d_colorWvpParam;
extern const SceGxmProgramParameter *_vita2d_textureWvpParam;
extern SceGxmProgramParameter *_vita2d_textureTintColorParam;
}

PSP2Renderer::PSP2Renderer(const Vector2f &size) : Renderer(size) {
    if (size.x == 0 || size.y == 0) {
        Renderer::setSize(960, 544);
    }

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    if ((SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE)) < 0) {
        printf("Couldn't init sdl: %s\n", SDL_GetError());
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't init sdl: %s\n", SDL_GetError());
        return;
    }

    vita2d_init();
    vita2d_set_vblank_wait(1);

    m_shaderList = (ShaderList *) new PSP2ShaderList();
}

void PSP2Renderer::draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) {
    Vertex *vertices;
    size_t vertexCount;

    if (!vertexArray || !vertexArray->getVertexCount()) {
        //printf("PSP2Renderer::draw: no vertices\n");
        return;
    }

    vertices = vertexArray->getVertices()->data();
    vertexCount = vertexArray->getVertexCount();
    auto v2d_indices = (uint16_t *) vita2d_pool_memalign(vertexCount * sizeof(uint16_t), sizeof(uint16_t));
    if (!v2d_indices) {
        printf("PSP2Render::draw: vita2d_pool_memalign failed (v2d_indices)\n");
        return;
    }

    SceGxmPrimitiveType type;
    switch (vertexArray->getPrimitiveType()) {
        case PrimitiveType::Triangles:
            type = SCE_GXM_PRIMITIVE_TRIANGLES;
            break;
        case PrimitiveType::Lines:
            type = SCE_GXM_PRIMITIVE_LINES;
            break;
        case PrimitiveType::TriangleStrip:
            type = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
            break;
        case PrimitiveType::TriangleFan:
            type = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
            break;
        default:
            printf("PSP2Render::draw: unsupported primitive type\n");
            return;
    }

    auto tex = (PSP2Texture *) texture;
    if (tex && tex->available) {
        if (vertices[0].color == Color::White) {
            // we only apply custom shader to "white" texture
            tex->applyShader();
        } else {
            sceGxmSetVertexProgram(vita2d_get_context(), _vita2d_textureVertexProgram);
            sceGxmSetFragmentProgram(vita2d_get_context(), _vita2d_textureTintFragmentProgram);
            void *color_buffer;
            sceGxmReserveFragmentDefaultUniformBuffer(vita2d_get_context(), &color_buffer);
            auto tint_color = (float *) vita2d_pool_memalign(4 * sizeof(float), sizeof(float));
            tint_color[0] = (float) vertices[0].color.r / 255.0f;
            tint_color[1] = (float) vertices[0].color.g / 255.0f;
            tint_color[2] = (float) vertices[0].color.b / 255.0f;
            tint_color[3] = (float) vertices[0].color.a / 255.0f;
            sceGxmSetUniformDataF(color_buffer, _vita2d_textureTintColorParam, 0, 4, tint_color);
        }

        auto v2d_vertices = (vita2d_texture_vertex *)
                vita2d_pool_memalign(vertexCount * sizeof(vita2d_texture_vertex), sizeof(vita2d_texture_vertex));
        if (!v2d_vertices) {
            printf("PSP2Render::draw: vita2d_pool_memalign failed (v2d_vertices)\n");
            return;
        }

        for (unsigned int i = 0; i < vertexCount; i++) {
            Vector2f v = transform.transformPoint(vertices[i].position);
            v2d_vertices[i].x = v.x;
            v2d_vertices[i].y = v.y;
            v2d_vertices[i].z = 0.5f;
            v2d_vertices[i].u = vertices[i].texCoords.x;
            v2d_vertices[i].v = vertices[i].texCoords.y;
            v2d_indices[i] = (uint16_t) i;
        }

        void *vertexDefaultBuffer;
        sceGxmReserveVertexDefaultUniformBuffer(vita2d_get_context(), &vertexDefaultBuffer);
        sceGxmSetUniformDataF(vertexDefaultBuffer, _vita2d_textureWvpParam, 0, 16, _vita2d_ortho_matrix);
        sceGxmSetFragmentTexture(vita2d_get_context(), 0, &tex->m_tex->gxm_tex);
        sceGxmSetVertexStream(vita2d_get_context(), 0, v2d_vertices);
        sceGxmDraw(vita2d_get_context(), type, SCE_GXM_INDEX_FORMAT_U16, v2d_indices, vertexCount);
    } else {
        auto v2d_vertices = (vita2d_color_vertex *)
                vita2d_pool_memalign(vertexCount * sizeof(vita2d_color_vertex), sizeof(vita2d_color_vertex));
        if (!v2d_vertices) {
            printf("PSP2Render::draw: vita2d_pool_memalign failed (v2d_vertices)\n");
            return;
        }

        for (unsigned int i = 0; i < vertexCount; i++) {
            Vector2f v = transform.transformPoint(vertices[i].position);
            v2d_vertices[i].x = v.x;
            v2d_vertices[i].y = v.y;
            v2d_vertices[i].z = +0.5f;
            v2d_vertices[i].color =
                    RGBA8((unsigned int) vertices[i].color.r,
                          (unsigned int) vertices[i].color.g,
                          (unsigned int) vertices[i].color.b,
                          (unsigned int) vertices[i].color.a);
            v2d_indices[i] = (uint16_t) i;
        }

        sceGxmSetVertexProgram(vita2d_get_context(), _vita2d_colorVertexProgram);
        sceGxmSetFragmentProgram(vita2d_get_context(), _vita2d_colorFragmentProgram);

        void *vertexDefaultBuffer;
        sceGxmReserveVertexDefaultUniformBuffer(vita2d_get_context(), &vertexDefaultBuffer);
        sceGxmSetUniformDataF(vertexDefaultBuffer, _vita2d_colorWvpParam, 0, 16, _vita2d_ortho_matrix);
        sceGxmSetVertexStream(vita2d_get_context(), 0, v2d_vertices);
        sceGxmDraw(vita2d_get_context(), type, SCE_GXM_INDEX_FORMAT_U16, v2d_indices, vertexCount);
    }
}

void PSP2Renderer::setClearColor(const Color &color) {
    vita2d_set_clear_color(
            RGBA8((unsigned int) color.r,
                  (unsigned int) color.g,
                  (unsigned int) color.b,
                  (unsigned int) color.a));
    Renderer::setClearColor(color);
}

void PSP2Renderer::flip(bool draw, bool inputs) {
    m_process_inputs = inputs;
    onUpdate();

    if (draw) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        // call base class (draw childs)
        Transform trans = Transform::Identity;
        Rectangle::onDraw(trans, draw);

        vita2d_end_drawing();
        if (m_wait_rendering) {
            vita2d_wait_rendering_done();
        }
        vita2d_swap_buffers();
    }
}

void PSP2Renderer::delay(unsigned int ms) {
    const Uint32 max_delay = 0xffffffffUL / 1000;
    if (ms > max_delay) {
        ms = max_delay;
    }
    sceKernelDelayThreadCB(ms * 1000);
}

void PSP2Renderer::delayUs(unsigned int micros) {
    sceKernelDelayThreadCB(micros);
}

void PSP2Renderer::setWaitRendering(bool wait) {
    m_wait_rendering = wait;
}

PSP2Renderer::~PSP2Renderer() {
    vita2d_wait_rendering_done();
    vita2d_fini();

    scePowerSetArmClockFrequency(266);
    scePowerSetBusClockFrequency(166);
    scePowerSetGpuClockFrequency(166);
    scePowerSetGpuXbarClockFrequency(111);
}
