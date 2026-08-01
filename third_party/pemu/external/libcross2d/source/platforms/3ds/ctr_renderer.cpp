//
// Created by cpasjuste on 21/11/16.
//

#include "cross2d/platforms/3ds/ctr_texture.h"
#include "cross2d/platforms/3ds/ctr_renderer.h"
#include "vshader.pica.shbin.h"
#include "ctr_sys.h"

#ifndef NDEBUG
#include <malloc.h>
#include <sys/unistd.h>
#define SOC_ALIGN       0x1000
#define SOC_BUFFER_SIZE 0x100000
#endif

using namespace c2d;

CTRRenderer::CTRRenderer(const Vector2f &size) : Renderer({400, 240}) {
    osSetSpeedupEnable(true);
    gfxInitDefault();
    gfxSet3D(false);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    /// from citro2d (init)
    ctx.vtxBufSize = 64 * 1024;
    ctx.vtxBuf = (C2Di_Vertex *) linearAlloc(ctx.vtxBufSize * sizeof(C2Di_Vertex));
    if (!ctx.vtxBuf) {
        return;
    }

    ctx.shader = DVLB_ParseFile((u32 *) cross2d_pica_shbin, cross2d_pica_shbin_size);
    if (!ctx.shader) {
        linearFree(ctx.vtxBuf);
        return;
    }

    shaderProgramInit(&ctx.program);
    shaderProgramSetVsh(&ctx.program, &ctx.shader->DVLE[0]);

    AttrInfo_Init(&ctx.attrInfo);
    AttrInfo_AddLoader(&ctx.attrInfo, 0, GPU_FLOAT, 3);         // v0=position
    AttrInfo_AddLoader(&ctx.attrInfo, 1, GPU_FLOAT, 2);         // v1=texcoord
    AttrInfo_AddLoader(&ctx.attrInfo, 2, GPU_FLOAT, 2);         // v2=blend
    AttrInfo_AddLoader(&ctx.attrInfo, 3, GPU_UNSIGNED_BYTE, 4); // v3=color

    BufInfo_Init(&ctx.bufInfo);
    BufInfo_Add(&ctx.bufInfo, ctx.vtxBuf, sizeof(C2Di_Vertex), 4, 0x3210);

    // Cache these common projection matrices
    Mtx_OrthoTilt(&s_projTop, 0.0f, m_size.x, m_size.y, 0.0f, 1.0f, -1.0f, true);

    // Prepare proctex
    C3D_ProcTexInit(&ctx.ptBlend, 0, 1);
    C3D_ProcTexClamp(&ctx.ptBlend, GPU_PT_CLAMP_TO_EDGE, GPU_PT_CLAMP_TO_EDGE);
    C3D_ProcTexCombiner(&ctx.ptBlend, true, GPU_PT_U, GPU_PT_V);
    C3D_ProcTexFilter(&ctx.ptBlend, GPU_PT_LINEAR);

    // Prepare proctex lut
    float data[129];
    for (int i = 0; i <= 128; i++) {
        data[i] = (float) i / 128.0f;
    }
    ProcTexLut_FromArray(&ctx.ptBlendLut, data);

    /// from citro2d (prepare)
    C3D_BindProgram(&ctx.program);
    C3D_SetAttrInfo(&ctx.attrInfo);
    C3D_SetBufInfo(&ctx.bufInfo);
    C3D_ProcTexBind(1, &ctx.ptBlend);
    C3D_ProcTexLutBind(GPU_LUT_ALPHAMAP, &ctx.ptBlendLut);

    C3D_TexEnv *env;
    // Set texenv0 to retrieve the texture color (or white if disabled)
    env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    // Set texenv1 to blend the output of texenv0 with the primary color
    env = C3D_GetTexEnv(1);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_PREVIOUS, GPU_PRIMARY_COLOR, GPU_TEXTURE3);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PREVIOUS, GPU_PRIMARY_COLOR, (GPU_TEVSRC) 0);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);

    // Reset unused texenv stages
    C3D_TexEnvInit(C3D_GetTexEnv(2));
    C3D_TexEnvInit(C3D_GetTexEnv(3));

    // Configure depth test to overwrite pixels with the same depth (needed to draw overlapping sprites)
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    // Don't cull anything
    C3D_CullFace(GPU_CULL_NONE);

    m_top = C3D_RenderTargetCreate((int) m_size.y, (int) m_size.x, GPU_RB_RGBA8, GPU_RB_DEPTH16);
    if (m_top) {
        C3D_RenderTargetSetOutput(m_top, GFX_TOP, GFX_LEFT,
                                  GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
                                  GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
                                  GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
                                  GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    }

    ctx.vtxBufPos = 0;
    vtxBufLastPos = 0;
    Mtx_Identity(&ctx.projMtx);
    Mtx_Identity(&ctx.mdlvMtx);

    // uniforms
    uLoc_mdlvMtx = (u8) shaderInstanceGetUniformLocation(ctx.program.vertexShader, "mdlvMtx");
    uLoc_projMtx = (u8) shaderInstanceGetUniformLocation(ctx.program.vertexShader, "projMtx");
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projMtx, &s_projTop);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_mdlvMtx, &ctx.mdlvMtx);

#ifndef NDEBUG
    //consoleInit(GFX_BOTTOM, nullptr);
    consoleDebugInit(debugDevice_SVC);
    stdout = stderr;
    //u32 *buffer = (u32 *) memalign(SOC_ALIGN, SOC_BUFFER_SIZE);
    //socInit(buffer, SOC_BUFFER_SIZE);
    //link_sock = link3dsStdio();

    printf(">> heap: %lu\n", ctr_get_heap_space());
#endif

    printf("CTRRenderer(%ix%i)\n", (int) m_size.x, (int) m_size.y);

    available = true;
}

void CTRRenderer::draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) {
    if (vertexArray == nullptr || vertexArray->getVertexCount() < 1) {
        //printf("gl_render::draw: no vertices\n");
        return;
    }

    Vertex *vertices = vertexArray->getVertices()->data();
    size_t vertexCount = vertexArray->getVertexCount();
    auto *tex = (CTRTexture *) texture;
    float blend;

    GPU_Primitive_t type;
    switch (vertexArray->getPrimitiveType()) {
        case PrimitiveType::Triangles:
            type = GPU_TRIANGLES;
            break;
        case PrimitiveType::TriangleStrip:
            type = GPU_TRIANGLE_STRIP;
            break;
        case PrimitiveType::TriangleFan:
            type = GPU_TRIANGLE_FAN;
            break;
        default:
            printf("CTRRenderer::draw: unsupported primitive type\n");
            return;
    }

    if (tex) {
        C3D_TexBind(0, tex->m_tex);
    }

    C3D_TexEnvSrc(C3D_GetTexEnv(0), C3D_Both,
                  tex ? GPU_TEXTURE0 : GPU_CONSTANT, (GPU_TEVSRC) 0, (GPU_TEVSRC) 0);

    for (unsigned int i = 0; i < vertexCount; i++) {
        Vertex v = vertices[i];
        Vector2f pos = transform.transformPoint(v.position);

        C2Di_Vertex *vtx = &ctx.vtxBuf[ctx.vtxBufPos++];
        vtx->pos[0] = pos.x;
        vtx->pos[1] = pos.y;
        vtx->pos[2] = 0.5f;
        vtx->texcoord[0] = v.texCoords.x;
        vtx->texcoord[1] = v.texCoords.y;
        blend = v.color.r == 255 && v.color.g == 255 && v.color.b == 255 ? 0 : 1;
        vtx->ptcoord[0] = 0;
        vtx->ptcoord[1] = tex ? blend : 1.0f;
        vtx->color = v.color.toABGR();
    }

    size_t len = ctx.vtxBufPos - vtxBufLastPos;
    if (len > 0) {
        C3D_DrawArrays(type, (int) vtxBufLastPos, (int) len);
        vtxBufLastPos = ctx.vtxBufPos;
    }
}

void CTRRenderer::flip(bool draw, bool inputs) {
    m_process_inputs = inputs;
    onUpdate();

    if (draw) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_FrameDrawOn(m_top);
        C3D_RenderTargetClear(m_top, C3D_CLEAR_ALL, m_clearColor.toRGBA(), 0);

        // call base class (draw childs)
        Transform trans = Transform::Identity;
        Renderer::onDraw(trans, draw);

        ctx.vtxBufPos = 0;
        vtxBufLastPos = 0;
        C3D_FrameEnd(0);
    }
}

void CTRRenderer::delay(unsigned int ms) {
    s64 nano = ms * 1000000;
    svcSleepThread(nano);
}

CTRRenderer::~CTRRenderer() {
    C3D_RenderTargetDelete(m_top);
    shaderProgramFree(&ctx.program);
    DVLB_Free(ctx.shader);
    linearFree(ctx.vtxBuf);
    C3D_Fini();
    gfxExit();
    romfsExit();
#ifndef NDEBUG
    if (link_sock) {
        close(link_sock);
    }
    socExit();
#endif
}
