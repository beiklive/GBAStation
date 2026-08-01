//
// Created by cpasjuste on 21/11/16.
//

#ifndef C2D_CTR_RENDERER_H_
#define C2D_CTR_RENDERER_H_

#include "cross2d/skeleton/renderer.h"
#include "citro3d.h"

namespace c2d {

    class CTRRenderer : public Renderer {

    public:
        explicit CTRRenderer(const Vector2f &size = Vector2f(0, 0));

        ~CTRRenderer() override;

        void draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) override;

        void flip(bool draw = true, bool inputs = true) override;

        void delay(unsigned int ms) override;

        C3D_RenderTarget *m_top;

    private:
        typedef struct {
            float pos[3];
            float texcoord[2];
            float ptcoord[2];
            u32 color;
        } C2Di_Vertex;

        typedef struct {
            DVLB_s *shader;
            shaderProgram_s program;
            C3D_AttrInfo attrInfo;
            C3D_BufInfo bufInfo;
            C3D_ProcTex ptBlend;
            C3D_ProcTex ptCircle;
            C3D_ProcTexLut ptBlendLut;
            C3D_ProcTexLut ptCircleLut;
            u32 sceneW, sceneH;

            C2Di_Vertex *vtxBuf;
            u16 *idxBuf;

            size_t vtxBufSize;
            size_t vtxBufPos;

            size_t idxBufSize;
            size_t idxBufPos;
            size_t idxBufLastPos;

            u32 flags;
            C3D_Mtx projMtx;
            C3D_Mtx mdlvMtx;
            C3D_Tex *curTex;
            u32 fadeClr;
        } C2Di_Context;

        C2Di_Context ctx;

        C3D_Mtx s_projTop;
        int uLoc_mdlvMtx, uLoc_projMtx;
        size_t vtxBufLastPos = 0;
#ifndef NDEBUG
        int link_sock = -1;
#endif
    };
}

#endif //C2D_CTR_RENDERER_H_