//
// Created by cpasjuste on 21/11/16.
//

#include "cross2d/c2d.h"
#include <sysutil/sysutil.h>
#include <sysmodule/sysmodule.h>
#include <sys/unistd.h>

using namespace c2d;

static void quit() {
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysModuleUnload(SYSMODULE_PNGDEC);
}

static void eventHandle(u64 status, u64 param, void *userdata) {
    (void) param;
    (void) userdata;
    if (status == SYSUTIL_EXIT_GAME || status == SYSUTIL_MENU_OPEN) {
        exit(0);
    }
}

TINY3DRenderer::TINY3DRenderer(const Vector2f &size) : Renderer(size) {

    atexit(quit);

    sysModuleLoad(SYSMODULE_PNGDEC);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, eventHandle, nullptr);

    tiny3d_Init(1024 * 1024);
    TINY3DRenderer::setSize(Vector2f(848, 512));

    m_shaderList = (ShaderList *) new ShaderList("");
}

void TINY3DRenderer::draw(VertexArray *vertexArray, const Transform &transform, Texture *texture, Sprite *sprite) {

    Vertex *vertices;
    size_t vertexCount;

    if (!vertexArray || vertexArray->getVertexCount() < 1) {
        printf("TINY3DRenderer::draw: no vertices\n");
        return;
    }

    vertices = vertexArray->getVertices()->data();
    vertexCount = vertexArray->getVertexCount();

    switch (vertexArray->getPrimitiveType()) {
        case PrimitiveType::Points:
            tiny3d_SetPolygon(TINY3D_POINTS);
            break;
        case PrimitiveType::Lines:
            tiny3d_SetPolygon(TINY3D_LINES);
            break;
        case PrimitiveType::LineStrip:
            tiny3d_SetPolygon(TINY3D_LINE_STRIP);
            break;
        case PrimitiveType::Triangles:
            tiny3d_SetPolygon(TINY3D_TRIANGLES);
            break;
        case PrimitiveType::TriangleStrip:
            tiny3d_SetPolygon(TINY3D_TRIANGLE_STRIP);
            break;
        case PrimitiveType::TriangleFan:
            tiny3d_SetPolygon(TINY3D_TRIANGLE_FAN);
            break;
        default:
            printf("TINY3DRenderer::draw: unsupported primitive type\n");
            return;
    }

    TINY3DTexture *tex = ((TINY3DTexture *) texture);
    if (tex) {
        int filter = tex->filter == Texture::Filter::Linear ? TEXTURE_LINEAR : TEXTURE_NEAREST;
        tiny3d_SetTexture(0, tex->offset,
                          (u32) tex->getSize().x, (u32) tex->getSize().y,
                          (u32) tex->pitch, tex->fmt, filter);
    }

    for (unsigned int i = 0; i < vertexCount; i++) {
        Vector2f v = transform.transformPoint(vertices[i].position);
        tiny3d_VertexPos(v.x, v.y, 0);
        tiny3d_VertexColor(vertices[i].color.toRGBA());
        if (tex) {
            tiny3d_VertexTexture(
                    vertices[i].texCoords.x / texture->getSize().x,
                    vertices[i].texCoords.y / texture->getSize().y);
        }
    }

    tiny3d_End();
}

void TINY3DRenderer::flip(bool draw, bool inputs) {

    if (draw) {
        // clear
        tiny3d_Clear(getFillColor().toARGB(), TINY3D_CLEAR_ALL);
        // Enable alpha Test
        tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);
        // Enable alpha blending.
        tiny3d_BlendFunc(1,
                         (blend_src_func) (TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA),
                         (blend_dst_func) (TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA |
                                           TINY3D_BLEND_FUNC_DST_ALPHA_ZERO),
                         (blend_func) (TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD));
        tiny3d_Project2D();
    }

    // call base class (draw childs)
    Renderer::flip(draw, inputs);

    if (draw) {
        // flip
        tiny3d_Flip();
    }

    sysUtilCheckCallback();
}

void TINY3DRenderer::delay(unsigned int ms) {
    usleep(ms * 1000);
}

TINY3DRenderer::~TINY3DRenderer() {
    //sysModuleUnload(SYSMODULE_PNGDEC);
}
