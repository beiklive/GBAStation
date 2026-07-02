#include "types.h"

#include "GPU3D.h"

#include <arm_neon.h>

namespace GPU3D
{

extern u32 CurPolygonAttr;

extern s32 ClipMatrix[16];

template<int comp, s32 plane, bool attribs>
void ClipSegment(Vertex* outbuf, Vertex* vin, Vertex* vout)
{
    s64 factor_num = vin->Position[3] - (plane*vin->Position[comp]);
    s32 factor_den = factor_num - (vout->Position[3] - (plane*vout->Position[comp]));

#define INTERPOLATE(var)  { outbuf->var = (vin->var + ((vout->var - vin->var) * factor_num) / factor_den); }

    if (comp != 0) INTERPOLATE(Position[0]);
    if (comp != 1) INTERPOLATE(Position[1]);
    if (comp != 2) INTERPOLATE(Position[2]);
    INTERPOLATE(Position[3]);
    outbuf->Position[comp] = plane*outbuf->Position[3];

    if (attribs)
    {
        INTERPOLATE(Color[0]);
        INTERPOLATE(Color[1]);
        INTERPOLATE(Color[2]);

        INTERPOLATE(TexCoords[0]);
        INTERPOLATE(TexCoords[1]);
    }

    outbuf->Clipped = true;

#undef INTERPOLATE
}

template<int comp, bool attribs>
int ClipAgainstPlane(Vertex* vertices, int nverts, int clipstart)
{
    Vertex temp[10];
    int prev, next;
    int c = clipstart;

    if (clipstart == 2)
    {
        temp[0] = vertices[0];
        temp[1] = vertices[1];
    }

    for (int i = clipstart; i < nverts; i++)
    {
        prev = i-1; if (prev < 0) prev = nverts-1;
        next = i+1; if (next >= nverts) next = 0;

        Vertex vtx = vertices[i];
        if (vtx.Position[comp] > vtx.Position[3])
        {
            if ((comp == 2) && (!(CurPolygonAttr & (1<<12)))) return 0;

            Vertex* vprev = &vertices[prev];
            if (vprev->Position[comp] <= vprev->Position[3])
            {
                ClipSegment<comp, 1, attribs>(&temp[c], &vtx, vprev);
                c++;
            }

            Vertex* vnext = &vertices[next];
            if (vnext->Position[comp] <= vnext->Position[3])
            {
                ClipSegment<comp, 1, attribs>(&temp[c], &vtx, vnext);
                c++;
            }
        }
        else
            temp[c++] = vtx;
    }

    nverts = c; c = clipstart;
    for (int i = clipstart; i < nverts; i++)
    {
        prev = i-1; if (prev < 0) prev = nverts-1;
        next = i+1; if (next >= nverts) next = 0;

        Vertex vtx = temp[i];
        if (vtx.Position[comp] < -vtx.Position[3])
        {
            Vertex* vprev = &temp[prev];
            if (vprev->Position[comp] >= -vprev->Position[3])
            {
                ClipSegment<comp, -1, attribs>(&vertices[c], &vtx, vprev);
                c++;
            }

            Vertex* vnext = &temp[next];
            if (vnext->Position[comp] >= -vnext->Position[3])
            {
                ClipSegment<comp, -1, attribs>(&vertices[c], &vtx, vnext);
                c++;
            }
        }
        else
            vertices[c++] = vtx;
    }

    // checkme
    for (int i = 0; i < c; i++)
    {
        Vertex* vtx = &vertices[i];

        vtx->Color[0] &= ~0xFFF; vtx->Color[0] += 0xFFF;
        vtx->Color[1] &= ~0xFFF; vtx->Color[1] += 0xFFF;
        vtx->Color[2] &= ~0xFFF; vtx->Color[2] += 0xFFF;
    }

    return c;
}

#ifndef NEONSOFTGPU_ENABLED

void MatrixMult4x4(s32* m, s32* s)
{
    s32 tmp[16];
    memcpy(tmp, m, 16*4);

    // m = s*m
    m[0] = ((s64)s[0]*tmp[0] + (s64)s[1]*tmp[4] + (s64)s[2]*tmp[8] + (s64)s[3]*tmp[12]) >> 12;
    m[1] = ((s64)s[0]*tmp[1] + (s64)s[1]*tmp[5] + (s64)s[2]*tmp[9] + (s64)s[3]*tmp[13]) >> 12;
    m[2] = ((s64)s[0]*tmp[2] + (s64)s[1]*tmp[6] + (s64)s[2]*tmp[10] + (s64)s[3]*tmp[14]) >> 12;
    m[3] = ((s64)s[0]*tmp[3] + (s64)s[1]*tmp[7] + (s64)s[2]*tmp[11] + (s64)s[3]*tmp[15]) >> 12;

    m[4] = ((s64)s[4]*tmp[0] + (s64)s[5]*tmp[4] + (s64)s[6]*tmp[8] + (s64)s[7]*tmp[12]) >> 12;
    m[5] = ((s64)s[4]*tmp[1] + (s64)s[5]*tmp[5] + (s64)s[6]*tmp[9] + (s64)s[7]*tmp[13]) >> 12;
    m[6] = ((s64)s[4]*tmp[2] + (s64)s[5]*tmp[6] + (s64)s[6]*tmp[10] + (s64)s[7]*tmp[14]) >> 12;
    m[7] = ((s64)s[4]*tmp[3] + (s64)s[5]*tmp[7] + (s64)s[6]*tmp[11] + (s64)s[7]*tmp[15]) >> 12;

    m[8] = ((s64)s[8]*tmp[0] + (s64)s[9]*tmp[4] + (s64)s[10]*tmp[8] + (s64)s[11]*tmp[12]) >> 12;
    m[9] = ((s64)s[8]*tmp[1] + (s64)s[9]*tmp[5] + (s64)s[10]*tmp[9] + (s64)s[11]*tmp[13]) >> 12;
    m[10] = ((s64)s[8]*tmp[2] + (s64)s[9]*tmp[6] + (s64)s[10]*tmp[10] + (s64)s[11]*tmp[14]) >> 12;
    m[11] = ((s64)s[8]*tmp[3] + (s64)s[9]*tmp[7] + (s64)s[10]*tmp[11] + (s64)s[11]*tmp[15]) >> 12;

    m[12] = ((s64)s[12]*tmp[0] + (s64)s[13]*tmp[4] + (s64)s[14]*tmp[8] + (s64)s[15]*tmp[12]) >> 12;
    m[13] = ((s64)s[12]*tmp[1] + (s64)s[13]*tmp[5] + (s64)s[14]*tmp[9] + (s64)s[15]*tmp[13]) >> 12;
    m[14] = ((s64)s[12]*tmp[2] + (s64)s[13]*tmp[6] + (s64)s[14]*tmp[10] + (s64)s[15]*tmp[14]) >> 12;
    m[15] = ((s64)s[12]*tmp[3] + (s64)s[13]*tmp[7] + (s64)s[14]*tmp[11] + (s64)s[15]*tmp[15]) >> 12;
}

void MatrixMult4x3(s32* m, s32* s)
{
    s32 tmp[16];
    memcpy(tmp, m, 16*4);

    // m = s*m
    m[0] = ((s64)s[0]*tmp[0] + (s64)s[1]*tmp[4] + (s64)s[2]*tmp[8]) >> 12;
    m[1] = ((s64)s[0]*tmp[1] + (s64)s[1]*tmp[5] + (s64)s[2]*tmp[9]) >> 12;
    m[2] = ((s64)s[0]*tmp[2] + (s64)s[1]*tmp[6] + (s64)s[2]*tmp[10]) >> 12;
    m[3] = ((s64)s[0]*tmp[3] + (s64)s[1]*tmp[7] + (s64)s[2]*tmp[11]) >> 12;

    m[4] = ((s64)s[3]*tmp[0] + (s64)s[4]*tmp[4] + (s64)s[5]*tmp[8]) >> 12;
    m[5] = ((s64)s[3]*tmp[1] + (s64)s[4]*tmp[5] + (s64)s[5]*tmp[9]) >> 12;
    m[6] = ((s64)s[3]*tmp[2] + (s64)s[4]*tmp[6] + (s64)s[5]*tmp[10]) >> 12;
    m[7] = ((s64)s[3]*tmp[3] + (s64)s[4]*tmp[7] + (s64)s[5]*tmp[11]) >> 12;

    m[8] = ((s64)s[6]*tmp[0] + (s64)s[7]*tmp[4] + (s64)s[8]*tmp[8]) >> 12;
    m[9] = ((s64)s[6]*tmp[1] + (s64)s[7]*tmp[5] + (s64)s[8]*tmp[9]) >> 12;
    m[10] = ((s64)s[6]*tmp[2] + (s64)s[7]*tmp[6] + (s64)s[8]*tmp[10]) >> 12;
    m[11] = ((s64)s[6]*tmp[3] + (s64)s[7]*tmp[7] + (s64)s[8]*tmp[11]) >> 12;

    m[12] = ((s64)s[9]*tmp[0] + (s64)s[10]*tmp[4] + (s64)s[11]*tmp[8] + (s64)0x1000*tmp[12]) >> 12;
    m[13] = ((s64)s[9]*tmp[1] + (s64)s[10]*tmp[5] + (s64)s[11]*tmp[9] + (s64)0x1000*tmp[13]) >> 12;
    m[14] = ((s64)s[9]*tmp[2] + (s64)s[10]*tmp[6] + (s64)s[11]*tmp[10] + (s64)0x1000*tmp[14]) >> 12;
    m[15] = ((s64)s[9]*tmp[3] + (s64)s[10]*tmp[7] + (s64)s[11]*tmp[11] + (s64)0x1000*tmp[15]) >> 12;
}

void MatrixMult3x3(s32* m, s32* s)
{
    s32 tmp[12];
    memcpy(tmp, m, 12*4);

    // m = s*m
    m[0] = ((s64)s[0]*tmp[0] + (s64)s[1]*tmp[4] + (s64)s[2]*tmp[8]) >> 12;
    m[1] = ((s64)s[0]*tmp[1] + (s64)s[1]*tmp[5] + (s64)s[2]*tmp[9]) >> 12;
    m[2] = ((s64)s[0]*tmp[2] + (s64)s[1]*tmp[6] + (s64)s[2]*tmp[10]) >> 12;
    m[3] = ((s64)s[0]*tmp[3] + (s64)s[1]*tmp[7] + (s64)s[2]*tmp[11]) >> 12;

    m[4] = ((s64)s[3]*tmp[0] + (s64)s[4]*tmp[4] + (s64)s[5]*tmp[8]) >> 12;
    m[5] = ((s64)s[3]*tmp[1] + (s64)s[4]*tmp[5] + (s64)s[5]*tmp[9]) >> 12;
    m[6] = ((s64)s[3]*tmp[2] + (s64)s[4]*tmp[6] + (s64)s[5]*tmp[10]) >> 12;
    m[7] = ((s64)s[3]*tmp[3] + (s64)s[4]*tmp[7] + (s64)s[5]*tmp[11]) >> 12;

    m[8] = ((s64)s[6]*tmp[0] + (s64)s[7]*tmp[4] + (s64)s[8]*tmp[8]) >> 12;
    m[9] = ((s64)s[6]*tmp[1] + (s64)s[7]*tmp[5] + (s64)s[8]*tmp[9]) >> 12;
    m[10] = ((s64)s[6]*tmp[2] + (s64)s[7]*tmp[6] + (s64)s[8]*tmp[10]) >> 12;
    m[11] = ((s64)s[6]*tmp[3] + (s64)s[7]*tmp[7] + (s64)s[8]*tmp[11]) >> 12;
}

void MatrixScale(s32* m, s32* s)
{
    m[0] = ((s64)s[0]*m[0]) >> 12;
    m[1] = ((s64)s[0]*m[1]) >> 12;
    m[2] = ((s64)s[0]*m[2]) >> 12;
    m[3] = ((s64)s[0]*m[3]) >> 12;

    m[4] = ((s64)s[1]*m[4]) >> 12;
    m[5] = ((s64)s[1]*m[5]) >> 12;
    m[6] = ((s64)s[1]*m[6]) >> 12;
    m[7] = ((s64)s[1]*m[7]) >> 12;

    m[8] = ((s64)s[2]*m[8]) >> 12;
    m[9] = ((s64)s[2]*m[9]) >> 12;
    m[10] = ((s64)s[2]*m[10]) >> 12;
    m[11] = ((s64)s[2]*m[11]) >> 12;
}

void MatrixTranslate(s32* m, s32* s)
{
    m[12] += ((s64)s[0]*m[0] + (s64)s[1]*m[4] + (s64)s[2]*m[8]) >> 12;
    m[13] += ((s64)s[0]*m[1] + (s64)s[1]*m[5] + (s64)s[2]*m[9]) >> 12;
    m[14] += ((s64)s[0]*m[2] + (s64)s[1]*m[6] + (s64)s[2]*m[10]) >> 12;
    m[15] += ((s64)s[0]*m[3] + (s64)s[1]*m[7] + (s64)s[2]*m[11]) >> 12;
}

template<bool attribs>
int ClipPolygon(Vertex* vertices, int nverts, int clipstart)
{
    // clip.
    // for each vertex:
    // if it's outside, check if the previous and next vertices are inside
    // if so, place a new vertex at the edge of the view volume

    // TODO: check for 1-dot polygons
    // TODO: the hardware seems to use a different algorithm. it reacts differently to vertices with W=0
    // some vertices that should get Y=-0x1000 get Y=0x1000 for some reason on hardware. it doesn't make sense.
    // clipping seems to process the Y plane before the X plane.

    // Z clipping
    nverts = ClipAgainstPlane<2, attribs>(vertices, nverts, clipstart);

    // Y clipping
    nverts = ClipAgainstPlane<1, attribs>(vertices, nverts, clipstart);

    // X clipping
    nverts = ClipAgainstPlane<0, attribs>(vertices, nverts, clipstart);

    return nverts;
}

void TransformVertex(s16* inVertex, s32* outVertex)
{
    s64 x = input[0];
    s64 y = input[1];
    s64 z = input[2];
    outVertex[0] = ((x*ClipMatrix[0] + y*ClipMatrix[4] + z*ClipMatrix[8]) >> 12) + ClipMatrix[12];
    outVertex[1] = ((x*ClipMatrix[1] + y*ClipMatrix[5] + z*ClipMatrix[9]) >> 12) + ClipMatrix[13];
    outVertex[2] = ((x*ClipMatrix[2] + y*ClipMatrix[6] + z*ClipMatrix[10]) >> 12) + ClipMatrix[14];
    outVertex[3] = ((x*ClipMatrix[3] + y*ClipMatrix[7] + z*ClipMatrix[11]) >> 12) + ClipMatrix[15];
}

#else

void MatrixMult4x4(s32* m, s32* s)
{
    int32x4x4_t matM = vld1q_s32_x4(m);
    int32x4x4_t matS = vld1q_s32_x4(s);

    int64x2_t accum0 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[0], 0);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[1]), matS.val[0], 1);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[2]), matS.val[0], 2);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[3]), matS.val[0], 3);

    int64x2_t accum1 = vmull_high_laneq_s32(matM.val[0], matS.val[0], 0);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[1], matS.val[0], 1);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[2], matS.val[0], 2);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[3], matS.val[0], 3);

    int64x2_t accum2 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[1], 0);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[1]), matS.val[1], 1);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[2]), matS.val[1], 2);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[3]), matS.val[1], 3);

    int64x2_t accum3 = vmull_high_laneq_s32(matM.val[0], matS.val[1], 0);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[1], matS.val[1], 1);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[2], matS.val[1], 2);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[3], matS.val[1], 3);

    int64x2_t accum4 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[2], 0);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[1]), matS.val[2], 1);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[2]), matS.val[2], 2);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[3]), matS.val[2], 3);

    int64x2_t accum5 = vmull_high_laneq_s32(matM.val[0], matS.val[2], 0);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[1], matS.val[2], 1);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[2], matS.val[2], 2);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[3], matS.val[2], 3);

    int64x2_t accum6 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[3], 0);
    accum6 = vmlal_laneq_s32(accum6, vget_low_s32(matM.val[1]), matS.val[3], 1);
    accum6 = vmlal_laneq_s32(accum6, vget_low_s32(matM.val[2]), matS.val[3], 2);
    accum6 = vmlal_laneq_s32(accum6, vget_low_s32(matM.val[3]), matS.val[3], 3);

    int64x2_t accum7 = vmull_high_laneq_s32(matM.val[0], matS.val[3], 0);
    accum7 = vmlal_high_laneq_s32(accum7, matM.val[1], matS.val[3], 1);
    accum7 = vmlal_high_laneq_s32(accum7, matM.val[2], matS.val[3], 2);
    accum7 = vmlal_high_laneq_s32(accum7, matM.val[3], matS.val[3], 3);

    int32x4x4_t result;
    result.val[0] = vshrn_high_n_s64(vshrn_n_s64(accum0, 12), accum1, 12);
    result.val[1] = vshrn_high_n_s64(vshrn_n_s64(accum2, 12), accum3, 12);
    result.val[2] = vshrn_high_n_s64(vshrn_n_s64(accum4, 12), accum5, 12);
    result.val[3] = vshrn_high_n_s64(vshrn_n_s64(accum6, 12), accum7, 12);

    vst1q_s32_x4(m, result);
}

void MatrixMult4x3(s32* m, s32* s)
{
    int32x4x4_t matM = vld1q_s32_x4(m);
    int32x4x3_t matS = vld1q_s32_x3(s);

    int64x2_t accum0 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[0], 0);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[1]), matS.val[0], 1);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[2]), matS.val[0], 2);

    int64x2_t accum1 = vmull_high_laneq_s32(matM.val[0], matS.val[0], 0);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[1], matS.val[0], 1);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[2], matS.val[0], 2);

    int64x2_t accum2 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[0], 3);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[1]), matS.val[1], 0);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[2]), matS.val[1], 1);

    int64x2_t accum3 = vmull_high_laneq_s32(matM.val[0], matS.val[0], 3);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[1], matS.val[1], 0);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[2], matS.val[1], 1);

    int64x2_t accum4 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[1], 2);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[1]), matS.val[1], 3);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[2]), matS.val[2], 0);

    int64x2_t accum5 = vmull_high_laneq_s32(matM.val[0], matS.val[1], 2);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[1], matS.val[1], 3);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[2], matS.val[2], 0);

    int64x2_t accum6 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[2], 1);
    accum6 = vmlal_laneq_s32(accum6, vget_low_s32(matM.val[1]), matS.val[2], 2);
    accum6 = vmlal_laneq_s32(accum6, vget_low_s32(matM.val[2]), matS.val[2], 3);
    accum6 = vmlal_laneq_s32(accum6, vget_low_s32(matM.val[3]), vdupq_n_s32(0x1000), 3);

    int64x2_t accum7 = vmull_high_laneq_s32(matM.val[0], matS.val[2], 1);
    accum7 = vmlal_high_laneq_s32(accum7, matM.val[1], matS.val[2], 2);
    accum7 = vmlal_high_laneq_s32(accum7, matM.val[2], matS.val[2], 3);
    accum7 = vmlal_high_laneq_s32(accum7, matM.val[3], vdupq_n_s32(0x1000), 3);

    int32x4x4_t result;
    result.val[0] = vshrn_high_n_s64(vshrn_n_s64(accum0, 12), accum1, 12);
    result.val[1] = vshrn_high_n_s64(vshrn_n_s64(accum2, 12), accum3, 12);
    result.val[2] = vshrn_high_n_s64(vshrn_n_s64(accum4, 12), accum5, 12);
    result.val[3] = vshrn_high_n_s64(vshrn_n_s64(accum6, 12), accum7, 12);

    vst1q_s32_x4(m, result);
}

void MatrixMult3x3(s32* m, s32* s)
{
    int32x4x3_t matM = vld1q_s32_x3(m);
    int32x4x3_t matS = vld1q_s32_x3(s);

    int64x2_t accum0 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[0], 0);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[1]), matS.val[0], 1);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(matM.val[2]), matS.val[0], 2);

    int64x2_t accum1 = vmull_high_laneq_s32(matM.val[0], matS.val[0], 0);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[1], matS.val[0], 1);
    accum1 = vmlal_high_laneq_s32(accum1, matM.val[2], matS.val[0], 2);

    int64x2_t accum2 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[0], 3);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[1]), matS.val[1], 0);
    accum2 = vmlal_laneq_s32(accum2, vget_low_s32(matM.val[2]), matS.val[1], 1);

    int64x2_t accum3 = vmull_high_laneq_s32(matM.val[0], matS.val[0], 3);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[1], matS.val[1], 0);
    accum3 = vmlal_high_laneq_s32(accum3, matM.val[2], matS.val[1], 1);

    int64x2_t accum4 = vmull_laneq_s32(vget_low_s32(matM.val[0]), matS.val[1], 2);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[1]), matS.val[1], 3);
    accum4 = vmlal_laneq_s32(accum4, vget_low_s32(matM.val[2]), matS.val[2], 0);

    int64x2_t accum5 = vmull_high_laneq_s32(matM.val[0], matS.val[1], 2);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[1], matS.val[1], 3);
    accum5 = vmlal_high_laneq_s32(accum5, matM.val[2], matS.val[2], 0);

    int32x4x3_t result;
    result.val[0] = vshrn_high_n_s64(vshrn_n_s64(accum0, 12), accum1, 12);
    result.val[1] = vshrn_high_n_s64(vshrn_n_s64(accum2, 12), accum3, 12);
    result.val[2] = vshrn_high_n_s64(vshrn_n_s64(accum4, 12), accum5, 12);

    vst1q_s32_x3(m, result);
}

void MatrixScale(s32* m, s32* s)
{
    // assumes s has a length of atleast 4

    int32x4_t scale = vld1q_s32(s);
    int32x4x3_t mat = vld1q_s32_x3(m);

    mat.val[0] = vshrn_high_n_s64(vshrn_n_s64(
        vmull_laneq_s32(vget_low_s32(mat.val[0]), scale, 0), 12),
        vmull_high_laneq_s32(mat.val[0], scale, 0), 12);
    mat.val[1] = vshrn_high_n_s64(vshrn_n_s64(
        vmull_laneq_s32(vget_low_s32(mat.val[1]), scale, 1), 12),
        vmull_high_laneq_s32(mat.val[1], scale, 1), 12);
    mat.val[2] = vshrn_high_n_s64(vshrn_n_s64(
        vmull_laneq_s32(vget_low_s32(mat.val[2]), scale, 2), 12),
        vmull_high_laneq_s32(mat.val[2], scale, 2), 12);

    vst1q_s32_x3(m, mat);
}

void MatrixTranslate(s32* m, s32* s)
{
    // assumes s has a length of atleast 4
    int32x4_t translate = vld1q_s32(s);
    int32x4x4_t mat = vld1q_s32_x4(m);

    int64x2_t accum0 = vmull_laneq_s32(vget_low_s32(mat.val[0]), translate, 0);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(mat.val[1]), translate, 1);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(mat.val[2]), translate, 2);
    int64x2_t accum1 = vmull_high_laneq_s32(mat.val[0], translate, 0);
    accum1 = vmlal_high_laneq_s32(accum1, mat.val[1], translate, 1);
    accum1 = vmlal_high_laneq_s32(accum1, mat.val[2], translate, 2);

    vst1q_s32(&m[12], vaddq_s32(mat.val[3], vshrn_high_n_s64(vshrn_n_s64(accum0, 12), accum1, 12)));
}

template<bool attribs>
int ClipPolygon(Vertex* vertices, int nverts, int clipstart)
{
    // fast early out for the cases of:
    // - all vertices are inside
    // - all vertices are outside of one clip plane

    uint32x4_t allInside = vdupq_n_u32(0xFFFFFFFF);
    uint32x4_t allOutsidePositive = vdupq_n_u32(0xFFFFFFFF);
    uint32x4_t allOutsideNegative = vdupq_n_u32(0xFFFFFFFF);

    for (int i = 0; i < nverts; i++)
    {
        int32x4_t position = vld1q_s32(vertices[i].Position);

        uint32x4_t positiveClip = vcgtq_s32(position, vdupq_n_s32(position[3]));
        uint32x4_t negativeClip = vcltq_s32(position, vnegq_s32(vdupq_n_s32(position[3])));

        allOutsidePositive = vandq_u32(positiveClip, allOutsidePositive);
        allOutsideNegative = vandq_u32(negativeClip, allOutsideNegative);

        allInside = vbicq_u32(allInside, vorrq_u32(positiveClip, negativeClip));
    }

    if (vreinterpret_u64_u16(vmovn_u32(allOutsidePositive))[0]
        || vreinterpret_u64_u16(vmovn_u32(allOutsideNegative))[0])
    {
        // all vertices are outside
        for (int i = clipstart; i < nverts; i++)
            vertices[i].Clipped = true;
        return 0;
    }

    u64 allInside64 = vreinterpret_u64_u16(vmovn_u32(allInside))[0];
    if ((allInside64 & 0xFFFFFFFFFFFF) == 0xFFFFFFFFFFFF)
    {
        // all are inside
        for (int i = clipstart; i < nverts; i++)
        {
            Vertex* vtx = &vertices[i];
            vtx->Color[0] |= 0xFFF;
            vtx->Color[1] |= 0xFFF;
            vtx->Color[2] |= 0xFFF;
        }
        return nverts;
    }

    // Z clipping
    nverts = ClipAgainstPlane<2, attribs>(vertices, nverts, clipstart);

    // Y clipping
    nverts = ClipAgainstPlane<1, attribs>(vertices, nverts, clipstart);

    // X clipping
    nverts = ClipAgainstPlane<0, attribs>(vertices, nverts, clipstart);

    return nverts;
}

void TransformVertex(s16* inVertex, s32* outVertex)
{
    int32x4x4_t clipmat = vld1q_s32_x4(ClipMatrix);

    int32x4_t vertex = vmovl_s16(vld1_s16(inVertex));

    int64x2_t accum0 = vmull_laneq_s32(vget_low_s32(clipmat.val[0]), vertex, 0);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(clipmat.val[1]), vertex, 1);
    accum0 = vmlal_laneq_s32(accum0, vget_low_s32(clipmat.val[2]), vertex, 2);
    int64x2_t accum1 = vmull_high_laneq_s32(clipmat.val[0], vertex, 0);
    accum1 = vmlal_high_laneq_s32(accum1, clipmat.val[1], vertex, 1);
    accum1 = vmlal_high_laneq_s32(accum1, clipmat.val[2], vertex, 2);

    vst1q_s32(outVertex, vaddq_s32(vshrn_high_n_s64(vshrn_n_s64(accum0, 12), accum1, 12), clipmat.val[3]));
}

#endif

template int ClipPolygon<false>(Vertex*, int, int);
template int ClipPolygon<true>(Vertex*, int, int);

}