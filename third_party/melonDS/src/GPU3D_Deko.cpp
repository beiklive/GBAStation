#include "GPU3D_Deko.h"

#include "GPU2D_Deko.h"

#include "frontend/switch/Profiler.h"
#include "frontend/switch/Gfx.h"

#include <assert.h>
#include <switch.h>

#define XXH_STATIC_LINKING_ONLY
#include "xxhash/xxhash.h"

#include <arm_neon.h>

using Gfx::EmuCmdBuf;
using Gfx::EmuQueue;

int visiblePolygon;

u32 stupidTextureNum = 0;

namespace GPU3D
{

DekoRenderer::DekoRenderer()
    : Renderer3D(false),
    CmdMem(*Gfx::DataHeap, 1024*128)
{}

DekoRenderer::~DekoRenderer()
{}

bool DekoRenderer::Init()
{
    for (int i = 0; i < 2; i++)
    {
        YSpanSetupMemory[i] = Gfx::DataHeap->Alloc(sizeof(SpanSetupY)*MaxYSpanSetups, 4);

        RenderPolygonMemory[i] = Gfx::DataHeap->Alloc(sizeof(RenderPolygon)*2048, 4);
    }

    TileMemory = Gfx::DataHeap->Alloc(sizeof(Tiles), alignof(Tiles));

    XSpanSetupMemory = Gfx::DataHeap->Alloc(sizeof(SpanSetupX)*MaxYSpanIndices, alignof(SpanSetupX));

    BinResultMemory = Gfx::DataHeap->Alloc(sizeof(BinResult), alignof(BinResult));
    memset(Gfx::DataHeap->CpuAddr<void>(BinResultMemory), 0, sizeof(BinResult));

    FinalTileMemory = Gfx::DataHeap->Alloc(sizeof(FinalTiles), alignof(FinalTiles));

    dk::ImageLayout yspanIndicesLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setType(DkImageType_Buffer)
        .setDimensions(MaxYSpanIndices)
        .setFormat(DkImageFormat_RGBA16_Uint)
        .initialize(yspanIndicesLayout);
    YSpanIndicesTextureMemory = Gfx::TextureHeap->Alloc(yspanIndicesLayout.getSize(), yspanIndicesLayout.getAlignment());
    YSpanIndicesTexture.initialize(yspanIndicesLayout, Gfx::TextureHeap->MemBlock, YSpanIndicesTextureMemory.Offset);

    Gfx::LoadShader("romfs:/shaders/InterpXSpansZBuffer.dksh", ShaderInterpXSpans[0]);
    Gfx::LoadShader("romfs:/shaders/InterpXSpansWBuffer.dksh", ShaderInterpXSpans[1]);
    Gfx::LoadShader("romfs:/shaders/BinCombined.dksh", ShaderBinCombined);
    Gfx::LoadShader("romfs:/shaders/DepthBlendZBuffer.dksh", ShaderDepthBlend[0]);
    Gfx::LoadShader("romfs:/shaders/DepthBlendWBuffer.dksh", ShaderDepthBlend[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseNoTextureZBuffer.dksh", ShaderRasteriseNoTexture[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseNoTextureZBufferToon.dksh", ShaderRasteriseNoTextureToon[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseNoTextureZBufferHighlight.dksh", ShaderRasteriseNoTextureHighlight[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureDecalZBuffer.dksh", ShaderRasteriseUseTextureDecal[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureModulateZBuffer.dksh", ShaderRasteriseUseTextureModulate[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureToonZBuffer.dksh", ShaderRasteriseUseTextureToon[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureHighlightZBuffer.dksh", ShaderRasteriseUseTextureHighlight[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseShadowMaskZBuffer.dksh", ShaderRasteriseShadowMask[0]);
    Gfx::LoadShader("romfs:/shaders/RasteriseNoTextureWBuffer.dksh", ShaderRasteriseNoTexture[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseNoTextureWBufferToon.dksh", ShaderRasteriseNoTextureToon[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseNoTextureWBufferHighlight.dksh", ShaderRasteriseNoTextureHighlight[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureDecalWBuffer.dksh", ShaderRasteriseUseTextureDecal[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureModulateWBuffer.dksh", ShaderRasteriseUseTextureModulate[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureToonWBuffer.dksh", ShaderRasteriseUseTextureToon[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseUseTextureHighlightWBuffer.dksh", ShaderRasteriseUseTextureHighlight[1]);
    Gfx::LoadShader("romfs:/shaders/RasteriseShadowMaskWBuffer.dksh", ShaderRasteriseShadowMask[1]);
    Gfx::LoadShader("romfs:/shaders/ClearCoarseBinMask.dksh", ShaderClearCoarseBinMask);
    Gfx::LoadShader("romfs:/shaders/ClearIndirectWorkCount.dksh", ShaderClearIndirectWorkCount);
    Gfx::LoadShader("romfs:/shaders/CalculateWorkOffsets.dksh", ShaderCalculateWorkListOffset);
    Gfx::LoadShader("romfs:/shaders/SortWork.dksh", ShaderSortWork);
    Gfx::LoadShader("romfs:/shaders/FinalPass.dksh", ShaderFinalPass[0]);
    Gfx::LoadShader("romfs:/shaders/FinalPassEdge.dksh", ShaderFinalPass[1]);
    Gfx::LoadShader("romfs:/shaders/FinalPassFog.dksh", ShaderFinalPass[2]);
    Gfx::LoadShader("romfs:/shaders/FinalPassEdgeFog.dksh", ShaderFinalPass[3]);
    Gfx::LoadShader("romfs:/shaders/FinalPassAA.dksh", ShaderFinalPass[4]);
    Gfx::LoadShader("romfs:/shaders/FinalPassEdgeAA.dksh", ShaderFinalPass[5]);
    Gfx::LoadShader("romfs:/shaders/FinalPassFogAA.dksh", ShaderFinalPass[6]);
    Gfx::LoadShader("romfs:/shaders/FinalPassEdgeFogAA.dksh", ShaderFinalPass[7]);

    {
        ImageDescriptors = Gfx::DataHeap->Alloc(sizeof(dk::ImageDescriptor)*descriptorOffset_Count, DK_IMAGE_DESCRIPTOR_ALIGNMENT);
        dk::ImageDescriptor* descriptors = Gfx::DataHeap->CpuAddr<dk::ImageDescriptor>(ImageDescriptors);
        descriptors[descriptorOffset_YSpanIndices].initialize(YSpanIndicesTexture, true);
        descriptors[descriptorOffset_FinalFB].initialize(((GPU2D::DekoRenderer*)GPU::GPU2D_Renderer.get())->Get3DFramebuffer(), true);
    }

    {
        SamplerDescriptors = Gfx::DataHeap->Alloc(sizeof(dk::SamplerDescriptor)*9, DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
        dk::SamplerDescriptor* descriptors = Gfx::DataHeap->CpuAddr<dk::SamplerDescriptor>(SamplerDescriptors);
        for (u32 j = 0; j < 3; j++)
        {
            for (u32 i = 0; i < 3; i++)
            {
                const DkWrapMode translateWrapMode[3] = {DkWrapMode_ClampToEdge, DkWrapMode_Repeat, DkWrapMode_MirroredRepeat};
                descriptors[i+j*3].initialize(dk::Sampler{}.setWrapMode(translateWrapMode[i], translateWrapMode[j]));
            }
        }
    }

    MetaUniformMemory = Gfx::DataHeap->Alloc(MetaUniformSize, DK_UNIFORM_BUF_ALIGNMENT);

    return true;
}

void DekoRenderer::DeInit()
{

}

void DekoRenderer::Reset()
{
    for (u32 i = 0; i < 8; i++)
    {
        for (u32 j = 0; j < 8; j++)
        {
            for (u32 k = 0; k < TexArrays[i][j].size(); k++)
                Gfx::TextureHeap->Free(TexArrays[i][j][k].Memory);
            TexArrays[i][j].clear();
            FreeTextures[i][j].clear();
        }
    }
    TexCache.clear();

    FreeImageDescriptorsCount = TexCacheMaxImages;
    for (int i = 0; i < TexCacheMaxImages; i++)
    {
        FreeImageDescriptors[i] = i;
    }
}

void DekoRenderer::SetRenderSettings(GPU::RenderSettings& settings)
{

}

void DekoRenderer::VCount144()
{

}

void DekoRenderer::SetupAttrs(SpanSetupY* span, Polygon* poly, int from, int to)
{
    span->Z0 = poly->FinalZ[from];
    span->W0 = poly->FinalW[from];
    span->Z1 = poly->FinalZ[to];
    span->W1 = poly->FinalW[to];
    span->ColorR0 = poly->Vertices[from]->FinalColor[0];
    span->ColorG0 = poly->Vertices[from]->FinalColor[1];
    span->ColorB0 = poly->Vertices[from]->FinalColor[2];
    span->ColorR1 = poly->Vertices[to]->FinalColor[0];
    span->ColorG1 = poly->Vertices[to]->FinalColor[1];
    span->ColorB1 = poly->Vertices[to]->FinalColor[2];
    span->TexcoordU0 = poly->Vertices[from]->TexCoords[0];
    span->TexcoordV0 = poly->Vertices[from]->TexCoords[1];
    span->TexcoordU1 = poly->Vertices[to]->TexCoords[0];
    span->TexcoordV1 = poly->Vertices[to]->TexCoords[1];
}

void DekoRenderer::SetupYSpanDummy(SpanSetupY* span, Polygon* poly, int vertex, int side)
{
    s32 x0 = poly->Vertices[vertex]->FinalPosition[0];
    if (side)
    {
        span->DxInitial = -0x40000;
        x0--;
    }
    else
    {
        span->DxInitial = 0;
    }

    span->X0 = span->X1 = x0;
    span->XMin = x0;
    span->XMax = x0;
    span->Y0 = span->Y1 = poly->Vertices[vertex]->FinalPosition[1];

    span->Increment = 0;

    span->I0 = span->I1 = span->IRecip = 0;
    span->Linear = true;

    span->XCovIncr = 0;

    span->IsDummy = true;

    SetupAttrs(span, poly, vertex, vertex);
}

void DekoRenderer::SetupYSpan(int polynum, SpanSetupY* span, Polygon* poly, int from, int to, u32 y, int side)
{
    span->X0 = poly->Vertices[from]->FinalPosition[0];
    span->X1 = poly->Vertices[to]->FinalPosition[0];
    span->Y0 = poly->Vertices[from]->FinalPosition[1];
    span->Y1 = poly->Vertices[to]->FinalPosition[1];

    SetupAttrs(span, poly, from, to);

    bool negative = false;
    if (span->X1 > span->X0)
    {
        span->XMin = span->X0;
        span->XMax = span->X1-1;
    }
    else if (span->X1 < span->X0)
    {
        span->XMin = span->X1;
        span->XMax = span->X0-1;
        negative = true;
    }
    else
    {
        span->XMin = span->X0;
        if (side) span->XMin--;
        span->XMax = span->XMin;
    }

    span->IsDummy = false;

    s32 xlen = span->XMax+1 - span->XMin;
    s32 ylen = span->Y1 - span->Y0;

    // slope increment has a 18-bit fractional part
    // note: for some reason, x/y isn't calculated directly,
    // instead, 1/y is calculated and then multiplied by x
    // TODO: this is still not perfect (see for example x=169 y=33)
    if (ylen == 0)
    {
        span->Increment = 0;
    }
    else if (ylen == xlen)
    {
        span->Increment = 0x40000;
    }
    else
    {
        s32 yrecip = (1<<18) / ylen;
        span->Increment = (span->X1-span->X0) * yrecip;
        if (span->Increment < 0) span->Increment = -span->Increment;
    }

    bool xMajor = (span->Increment > 0x40000);

    if (side)
    {
        // right

        if (xMajor)
            span->DxInitial = negative ? (0x20000 + 0x40000) : (span->Increment - 0x20000);
        else if (span->Increment != 0)
            span->DxInitial = negative ? 0x40000 : 0;
        else
            span->DxInitial = -0x40000;
    }
    else
    {
        // left

        if (xMajor)
            span->DxInitial = negative ? ((span->Increment - 0x20000) + 0x40000) : 0x20000;
        else if (span->Increment != 0)
            span->DxInitial = negative ? 0x40000 : 0;
        else
            span->DxInitial = 0;
    }

    if (xMajor)
    {
        if (side)
        {
            span->I0 = span->X0 - 1;
            span->I1 = span->X1 - 1;
        }
        else
        {
            span->I0 = span->X0;
            span->I1 = span->X1;
        }

        // used for calculating AA coverage
        span->XCovIncr = (ylen << 10) / xlen;
    }
    else
    {
        span->I0 = span->Y0;
        span->I1 = span->Y1;
    }

    //if (span->I1 < span->I0)
    //    std::swap(span->I0, span->I1);

    if (span->I0 != span->I1)
        span->IRecip = (1<<30) / (span->I1 - span->I0);
    else
        span->IRecip = 0;

    span->Linear = (span->W0 == span->W1) && !(span->W0 & 0x7E) && !(span->W1 & 0x7E);

    if ((span->W0 & 0x1) && !(span->W1 & 0x1))
    {
        span->W0n = (span->W0 - 1) >> 1;
        span->W0d = (span->W0 + 1) >> 1;
        span->W1d = span->W1 >> 1;
    }
    else
    {
        span->W0n = span->W0 >> 1;
        span->W0d = span->W0 >> 1;
        span->W1d = span->W1 >> 1;
    }
}

inline u32 TextureWidth(u32 texparam)
{
    return 8 << ((texparam >> 20) & 0x7);
}

inline u32 TextureHeight(u32 texparam)
{
    return 8 << ((texparam >> 23) & 0x7);
}

inline u16 ColorAvg(u16 color0, u16 color1)
{
    u32 r0 = color0 & 0x001F;
    u32 g0 = color0 & 0x03E0;
    u32 b0 = color0 & 0x7C00;
    u32 r1 = color1 & 0x001F;
    u32 g1 = color1 & 0x03E0;
    u32 b1 = color1 & 0x7C00;

    u32 r = (r0 + r1) >> 1;
    u32 g = ((g0 + g1) >> 1) & 0x03E0;
    u32 b = ((b0 + b1) >> 1) & 0x7C00;

    return r | g | b;
}

inline u16 Color5of3(u16 color0, u16 color1)
{
    u32 r0 = color0 & 0x001F;
    u32 g0 = color0 & 0x03E0;
    u32 b0 = color0 & 0x7C00;
    u32 r1 = color1 & 0x001F;
    u32 g1 = color1 & 0x03E0;
    u32 b1 = color1 & 0x7C00;

    u32 r = (r0*5 + r1*3) >> 3;
    u32 g = ((g0*5 + g1*3) >> 3) & 0x03E0;
    u32 b = ((b0*5 + b1*3) >> 3) & 0x7C00;

    return r | g | b;
}

inline u16 Color3of5(u16 color0, u16 color1)
{
    u32 r0 = color0 & 0x001F;
    u32 g0 = color0 & 0x03E0;
    u32 b0 = color0 & 0x7C00;
    u32 r1 = color1 & 0x001F;
    u32 g1 = color1 & 0x03E0;
    u32 b1 = color1 & 0x7C00;

    u32 r = (r0*3 + r1*5) >> 3;
    u32 g = ((g0*3 + g1*5) >> 3) & 0x03E0;
    u32 b = ((b0*3 + b1*5) >> 3) & 0x7C00;

    return r | g | b;
}

inline void RGB5ToRGB6(uint8x16_t lo, uint8x16_t hi, uint8x16_t& red, uint8x16_t& green, uint8x16_t& blue)
{
    red = vandq_u8(vshlq_n_u8(lo, 1), vdupq_n_u8(0x3E));
    green = vbslq_u8(vdupq_n_u8(0xCE), vshrq_n_u8(lo, 4), vshlq_n_u8(hi, 4));
    blue = vandq_u8(vshrq_n_u8(hi, 1), vdupq_n_u8(0x3E));
    red = vandq_u8(vtstq_u8(red, red), vaddq_u8(red, vdupq_n_u8(1)));
    green = vandq_u8(vtstq_u8(green, green), vaddq_u8(green, vdupq_n_u8(1)));
    blue =  vandq_u8(vtstq_u8(blue, blue), vaddq_u8(blue, vdupq_n_u8(1)));
}

inline void RGB5ToRGB6(uint8x8_t lo, uint8x8_t hi, uint8x8_t& red, uint8x8_t& green, uint8x8_t& blue)
{
    red   = vand_u8(vshl_n_u8(lo, 1), vdup_n_u8(0x3E));
    green = vbsl_u8(vdup_n_u8(0xCE), vshr_n_u8(lo, 4), vshl_n_u8(hi, 4));
    blue  = vand_u8(vshr_n_u8(hi, 1), vdup_n_u8(0x3E));

    red   = vand_u8(vtst_u8(red, red), vadd_u8(red, vdup_n_u8(1)));
    green = vand_u8(vtst_u8(green, green), vadd_u8(green, vdup_n_u8(1)));
    blue  = vand_u8(vtst_u8(blue, blue), vadd_u8(blue, vdup_n_u8(1)));
}

inline u32 ConvertRGB5ToRGB8(u16 val)
{
    return (((u32)val & 0x1F) << 3)
        | (((u32)val & 0x3E0) << 6)
        | (((u32)val & 0x7C00) << 9);
}
inline u32 ConvertRGB5ToBGR8(u16 val)
{
    return (((u32)val & 0x1F) << 9)
        | (((u32)val & 0x3E0) << 6)
        | (((u32)val & 0x7C00) << 3);
}
inline u32 ConvertRGB5ToRGB6(u16 val)
{
    u8 r = (val & 0x1F) << 1;
    u8 g = (val & 0x3E0) >> 4;
    u8 b = (val & 0x7C00) >> 9;
    if (r) r++;
    if (g) g++;
    if (b) b++;
    return (u32)r | ((u32)g << 8) | ((u32)b << 16);
}

enum
{
    outputFmt_RGB6A5,
    outputFmt_RGBA8,
    outputFmt_BGRA8
};

template <int outputFmt>
void ConvertCompressedTexture(u32 width, u32 height, u32* output, u8* texData, u8* texAuxData, u16* palData)
{
    // we process a whole block at the time
    for (int y = 0; y < height / 4; y++)
    {
        for (int x = 0; x < width / 4; x++)
        {
            u32 data = ((u32*)texData)[x + y * (width / 4)];
            u16 auxData = ((u16*)texAuxData)[x + y * (width / 4)];

            u32 paletteOffset = auxData & 0x3FFF;
            u16 color0 = palData[paletteOffset*2] | 0x8000;
            u16 color1 = palData[paletteOffset*2+1] | 0x8000;
            u16 color2, color3;

            switch ((auxData >> 14) & 0x3)
            {
            case 0:
                color2 = palData[paletteOffset*2+2] | 0x8000;
                color3 = 0;
                break;
            case 1:
                {
                    u32 r0 = color0 & 0x001F;
                    u32 g0 = color0 & 0x03E0;
                    u32 b0 = color0 & 0x7C00;
                    u32 r1 = color1 & 0x001F;
                    u32 g1 = color1 & 0x03E0;
                    u32 b1 = color1 & 0x7C00;

                    u32 r = (r0 + r1) >> 1;
                    u32 g = ((g0 + g1) >> 1) & 0x03E0;
                    u32 b = ((b0 + b1) >> 1) & 0x7C00;
                    color2 = r | g | b | 0x8000;
                }
                color3 = 0;
                break;
            case 2:
                color2 = palData[paletteOffset*2+2] | 0x8000;
                color3 = palData[paletteOffset*2+3] | 0x8000;
                break;
            case 3:
                {
                    u32 r0 = color0 & 0x001F;
                    u32 g0 = color0 & 0x03E0;
                    u32 b0 = color0 & 0x7C00;
                    u32 r1 = color1 & 0x001F;
                    u32 g1 = color1 & 0x03E0;
                    u32 b1 = color1 & 0x7C00;

                    u32 r = (r0*5 + r1*3) >> 3;
                    u32 g = ((g0*5 + g1*3) >> 3) & 0x03E0;
                    u32 b = ((b0*5 + b1*3) >> 3) & 0x7C00;

                    color2 = r | g | b | 0x8000;
                }
                {
                    u32 r0 = color0 & 0x001F;
                    u32 g0 = color0 & 0x03E0;
                    u32 b0 = color0 & 0x7C00;
                    u32 r1 = color1 & 0x001F;
                    u32 g1 = color1 & 0x03E0;
                    u32 b1 = color1 & 0x7C00;

                    u32 r = (r0*3 + r1*5) >> 3;
                    u32 g = ((g0*3 + g1*5) >> 3) & 0x03E0;
                    u32 b = ((b0*3 + b1*5) >> 3) & 0x7C00;

                    color3 = r | g | b | 0x8000;
                }
                break;
            }

            // in 2020 our default data types are big enough to be used as lookup tables...
            u64 packed = color0 | ((u64)color1 << 16) | ((u64)color2 << 32) | ((u64)color3 << 48);

            for (int j = 0; j < 4; j++)
            {
                for (int i = 0; i < 4; i++)
                {
                    u16 color = (packed >> 16 * (data >> 2 * (i + j * 4))) & 0xFFFF;
                    u32 res;
                    switch (outputFmt)
                    {
                    case outputFmt_RGB6A5: res = ConvertRGB5ToRGB6(color)
                        | ((color & 0x8000) ? 0x1F000000 : 0); break;
                    case outputFmt_RGBA8: res = ConvertRGB5ToRGB8(color)
                        | ((color & 0x8000) ? 0xFF000000 : 0); break;
                    case outputFmt_BGRA8: res = ConvertRGB5ToBGR8(color)
                        | ((color & 0x8000) ? 0xFF000000 : 0); break;
                    }
                    output[x * 4 + i + (y * 4 + j) * width] = res;
                }
            }
        }
    }
}

template <int outputFmt, int X, int Y>
void ConvertAXIYTexture(u32 width, u32 height, u32* output, u8* texData, u16* palData)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            u8 val = texData[x + y * width];

            u32 idx = val & ((1 << Y) - 1);

            u16 color = palData[idx];
            u32 alpha = (val >> Y) & ((1 << X) - 1);
            if (X != 5)
                alpha = alpha * 4 + alpha / 2;

            u32 res;
            switch (outputFmt)
            {
            case outputFmt_RGB6A5: res = ConvertRGB5ToRGB6(color) | alpha << 24; break;
            // make sure full alpha == 255
            case outputFmt_RGBA8: res = ConvertRGB5ToRGB8(color) | (alpha << 27 | (alpha & 0x1C) << 22); break;
            case outputFmt_BGRA8: res = ConvertRGB5ToBGR8(color) | (alpha << 27 | (alpha & 0x1C) << 22); break;
            }
            output[x + y * width] = res;
        }
    }
}

void Convert16ColorsTexture(u32 width, u32 height, u32* output, u8* texData, u16* palData, bool color0Transparent)
{
    uint8x16x2_t palette = vld2q_u8((u8*)palData);

    uint8x16_t paletteR, paletteG, paletteB;
    RGB5ToRGB6(palette.val[0], palette.val[1], paletteR, paletteG, paletteB);

    uint8x16_t firstEntryAlpha = vdupq_n_u8(color0Transparent ? 0 : 0x1F);

    for (int i = 0; i < width*height/2; i += 16)
    {
        uint8x16_t packedIndices = vld1q_u8(&texData[i]);

        // unpack indices
        uint8x16_t oddIndices = vandq_u8(packedIndices, vdupq_n_u8(0xF));
        uint8x16_t evenIndices = vshrq_n_u8(packedIndices, 4);

        uint8x16_t indices0 = vzip1q_u8(oddIndices, evenIndices);
        uint8x16_t indices1 = vzip2q_u8(oddIndices, evenIndices);

        // palettise
        uint8x16x4_t finalPixels0, finalPixels1;
        finalPixels0.val[0] = vqtbl1q_u8(paletteR, indices0);
        finalPixels0.val[1] = vqtbl1q_u8(paletteG, indices0);
        finalPixels0.val[2] = vqtbl1q_u8(paletteB, indices0);
        finalPixels0.val[3] = vbslq_u8(vceqzq_u8(indices0), firstEntryAlpha, vdupq_n_u8(0x1F));
        finalPixels1.val[0] = vqtbl1q_u8(paletteR, indices1);
        finalPixels1.val[1] = vqtbl1q_u8(paletteG, indices1);
        finalPixels1.val[2] = vqtbl1q_u8(paletteB, indices1);
        finalPixels1.val[3] = vbslq_u8(vceqzq_u8(indices1), firstEntryAlpha, vdupq_n_u8(0x1F));

        vst4q_u8((u8*)&output[i*2], finalPixels0);
        vst4q_u8((u8*)&output[i*2+16], finalPixels1);
    }
}

template <int outputFmt, int colorBits>
void ConvertNColorsTexture(u32 width, u32 height, u32* output, u8* texData, u16* palData, bool color0Transparent)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width / (8 / colorBits); x++)
        {
            u8 val = texData[x + y * (width / (8 / colorBits))];

            for (int i = 0; i < 8 / colorBits; i++)
            {
                u32 index = (val >> (i * colorBits)) & ((1 << colorBits) - 1);
                u16 color = palData[index];

                bool transparent = color0Transparent && index == 0;
                u32 res;
                switch (outputFmt)
                {
                case outputFmt_RGB6A5: res = ConvertRGB5ToRGB6(color)
                    | (transparent ? 0 : 0x1F000000); break;
                case outputFmt_RGBA8: res = ConvertRGB5ToRGB8(color)
                    | (transparent ? 0 : 0xFF000000); break;
                case outputFmt_BGRA8: res = ConvertRGB5ToBGR8(color)
                    | (transparent ? 0 : 0xFF000000); break;
                }
                output[x * (8 / colorBits) + y * width + i] = res;
            }
        }
    }
}

DekoRenderer::TexCacheEntry& DekoRenderer::GetTexture(u32 texParam, u32 palBase)
{
    // remove sampling and texcoord gen params
    texParam &= ~0xC00F0000;

    u32 fmt = (texParam >> 26) & 0x7;
    u64 key = texParam;
    if (fmt != 7)
    {
        key |= (u64)palBase << 32;
        if (fmt == 5)
            key &= ~((u64)1 << 29);
    }
    //printf("%" PRIx64 " %" PRIx32 " %" PRIx32 "\n", key, texParam, palBase);

    assert(fmt != 0 && "no texture is not a texture format!");

    auto it = TexCache.find(key);

    if (it != TexCache.end())
        return it->second;

    u32 widthLog2 = (texParam >> 20) & 0x7;
    u32 heightLog2 = (texParam >> 23) & 0x7;
    u32 width = 8 << widthLog2;
    u32 height = 8 << heightLog2;

    u32 addr = (texParam & 0xFFFF) * 8;

    TexCacheEntry entry = {0};

    entry.TextureRAMStart[0] = addr;
    entry.WidthLog2 = widthLog2;
    entry.HeightLog2 = heightLog2;

    // apparently a new texture
    if (fmt == 7)
    {
        entry.TextureRAMSize[0] = width*height*2;

        for (u32 i = 0; i < width*height; i += 16)
        {
            uint8x16x2_t pixels = vld2q_u8(&GPU::VRAMFlat_Texture[addr + i * 2]);

            uint8x16_t red, green, blue;
            RGB5ToRGB6(pixels.val[0], pixels.val[1], red, green, blue);
            uint8x16_t alpha = vbslq_u8(vtstq_u8(pixels.val[1], vdupq_n_u8(0x80)), vdupq_n_u8(0x1F), vdupq_n_u8(0));

            vst4q_u8((u8*)&TextureDecodingBuffer[i],
            {
                red,
                green,
                blue,
                alpha
            });
        }

    }
    else if (fmt == 5)
    {
        u8* texData = &GPU::VRAMFlat_Texture[addr];
        u32 slot1addr = 0x20000 + ((addr & 0x1FFFC) >> 1);
        if (addr >= 0x40000)
            slot1addr += 0x10000;
        u8* texAuxData = &GPU::VRAMFlat_Texture[slot1addr];

        u16* palData = (u16*)(GPU::VRAMFlat_TexPal + palBase*16);

        entry.TextureRAMSize[0] = width*height/16*4;
        entry.TextureRAMStart[1] = slot1addr;
        entry.TextureRAMSize[1] = width*height/16*2;
        entry.TexPalStart = palBase*16;
        entry.TexPalSize = 0x10000;

        ConvertCompressedTexture<outputFmt_RGB6A5>(width, height, TextureDecodingBuffer, texData, texAuxData, palData);
    }
    else
    {
        u32 texSize, palAddr = palBase*16, numPalEntries;
        switch (fmt)
        {
        case 1: texSize = width*height; numPalEntries = 32; break;
        case 6: texSize = width*height; numPalEntries = 8; break;
        case 2: texSize = width*height/4; numPalEntries = 4; palAddr >>= 1; break;
        case 3: texSize = width*height/2; numPalEntries = 16; break;
        case 4: texSize = width*height; numPalEntries = 256; break;
        }

        palAddr &= 0x1FFFF;

        /*printf("creating texture | fmt: %d | %dx%d | %08x | %08x\n", fmt, width, height, addr, palAddr);
        svcSleepThread(1000*1000);*/

        entry.TextureRAMSize[0] = texSize;
        entry.TexPalStart = palAddr;
        entry.TexPalSize = numPalEntries*2;

        u8* texData = &GPU::VRAMFlat_Texture[addr];
        u16* palData = (u16*)(GPU::VRAMFlat_TexPal + palAddr);

        //assert(entry.TexPalStart+entry.TexPalSize <= 128*1024*1024);

        bool color0Transparent = texParam & (1 << 29);

        switch (fmt)
        {
        case 1: ConvertAXIYTexture<outputFmt_RGB6A5, 3, 5>(width, height, TextureDecodingBuffer, texData, palData); break;
        case 6: ConvertAXIYTexture<outputFmt_RGB6A5, 5, 3>(width, height, TextureDecodingBuffer, texData, palData); break;
        case 2: ConvertNColorsTexture<outputFmt_RGB6A5, 2>(width, height, TextureDecodingBuffer, texData, palData, color0Transparent); break;
        case 3: Convert16ColorsTexture(width, height, TextureDecodingBuffer, texData, palData, color0Transparent); break;
        case 4: ConvertNColorsTexture<outputFmt_RGB6A5, 8>(width, height, TextureDecodingBuffer, texData, palData, color0Transparent); break;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (entry.TextureRAMSize[i])
            entry.TextureHash[i] = XXH3_64bits(&GPU::VRAMFlat_Texture[entry.TextureRAMStart[i]], entry.TextureRAMSize[i]);
    }
    if (entry.TexPalSize)
        entry.TexPalHash = XXH3_64bits(&GPU::VRAMFlat_TexPal[entry.TexPalStart], entry.TexPalSize);

    auto& texArrays = TexArrays[widthLog2][heightLog2];
    auto& freeTextures = FreeTextures[widthLog2][heightLog2];

    if (freeTextures.size() == 0)
    {
        texArrays.resize(texArrays.size()+1);
        TexArray& array = texArrays[texArrays.size()-1];

        u32 layers = std::min<u32>((8*1024*1024) / (width*height*4), 64);

        // allocate new array texture
        dk::ImageLayout imageLayout;
        dk::ImageLayoutMaker{Gfx::Device}
            .setType(DkImageType_2DArray)
            .setFormat(DkImageFormat_RGBA8_Uint)
            .setDimensions(width, height, layers)
            .initialize(imageLayout);

        assert(FreeImageDescriptorsCount > 0);
        array.ImageDescriptor = FreeImageDescriptors[--FreeImageDescriptorsCount];

        array.Memory = Gfx::TextureHeap->Alloc(imageLayout.getSize(), imageLayout.getAlignment());
        array.Image.initialize(imageLayout, Gfx::TextureHeap->MemBlock, array.Memory.Offset);

        dk::ImageDescriptor descriptor;
        descriptor.initialize(array.Image);
        DkGpuAddr descriptors = Gfx::DataHeap->GpuAddr(ImageDescriptors);
        EmuCmdBuf.pushData(descriptors + (descriptorOffset_TexcacheStart + array.ImageDescriptor) * sizeof(DkImageDescriptor),
            &descriptor,
            sizeof(DkImageDescriptor));

        //printf("allocating new layer set for %d %d %d %d\n", width, height, texArrays.size()-1, array.ImageDescriptor);

        for (u16 i = 0; i < layers; i++)
        {
            freeTextures.push_back(TexArrayEntry{(u16)(texArrays.size()-1), i});
        }
    }

    TexArrayEntry storagePlace = freeTextures[freeTextures.size()-1];
    freeTextures.pop_back();

    TexArray& array = texArrays[storagePlace.TexArrayIdx];
    //printf("using storage place %d %d | %d %d (%d)\n", width, height, storagePlace.TexArrayIdx, storagePlace.LayerIdx, array.ImageDescriptor);

    UploadBuf.UploadAndCopyTexture(Gfx::EmuCmdBuf, array.Image,
        (u8*)TextureDecodingBuffer,
        0, 0, width, height,
        width*4,
        storagePlace.LayerIdx);

    entry.DescriptorIdx = array.ImageDescriptor;
    entry.Texture = storagePlace;

    return TexCache.emplace(std::make_pair(key, entry)).first->second;
}

struct Variant
{
    s16 Texture, Sampler;
    u16 Width, Height;
    u8 BlendMode;

    bool operator==(const Variant& other)
    {
        return Texture == other.Texture && Sampler == other.Sampler && BlendMode == other.BlendMode;
    }
};

/*
    Antialiasing
    W-Buffer
    Mit Textur
    0
    1, 3
    2
    Ohne Textur
    2
    0, 1, 3

    => 20 Shader + 1x Shadow Mask
*/

void DekoRenderer::RenderFrame()
{
    //printf("render frame\n");
    auto textureDirty = GPU::VRAMDirty_Texture.DeriveState(GPU::VRAMMap_Texture);
    auto texPalDirty = GPU::VRAMDirty_TexPal.DeriveState(GPU::VRAMMap_TexPal);

    bool textureChanged = GPU::MakeVRAMFlat_TextureCoherent(textureDirty);
    bool texPalChanged = GPU::MakeVRAMFlat_TexPalCoherent(texPalDirty);

    if (textureChanged || texPalChanged)
    {
        //printf("check invalidation %d\n", TexCache.size());
        for (auto it = TexCache.begin(); it != TexCache.end();)
        {
            TexCacheEntry& entry = it->second;
            if (textureChanged)
            {
                for (u32 i = 0; i < 2; i++)
                {
                    u32 startBit = entry.TextureRAMStart[i] / GPU::VRAMDirtyGranularity;
                    u32 bitsCount = ((entry.TextureRAMStart[i] + entry.TextureRAMSize[i] + GPU::VRAMDirtyGranularity - 1) / GPU::VRAMDirtyGranularity) - startBit;

                    u32 startEntry = startBit >> 6;
                    u64 entriesCount = ((startBit + bitsCount + 0x3F) >> 6) - startEntry;
                    for (u32 j = startEntry; j < startEntry + entriesCount; j++)
                    {
                        if (GetRangedBitMask(j, startBit, bitsCount) & textureDirty.Data[j])
                        {
                            u64 newTexHash = XXH3_64bits(&GPU::VRAMFlat_Texture[entry.TextureRAMStart[i]], entry.TextureRAMSize[i]);

                            if (newTexHash != entry.TextureHash[i])
                                goto invalidate;
                        }
                    }
                }
            }

            if (texPalChanged && entry.TexPalSize > 0)
            {
                u32 startBit = entry.TexPalStart / GPU::VRAMDirtyGranularity;
                u32 bitsCount = ((entry.TexPalStart + entry.TexPalSize + GPU::VRAMDirtyGranularity - 1) / GPU::VRAMDirtyGranularity) - startBit;

                u32 startEntry = startBit >> 6;
                u64 entriesCount = ((startBit + bitsCount + 0x3F) >> 6) - startEntry;
                for (u32 j = startEntry; j < startEntry + entriesCount; j++)
                {
                    if (GetRangedBitMask(j, startBit, bitsCount) & texPalDirty.Data[j])
                    {
                        u64 newPalHash = XXH3_64bits(&GPU::VRAMFlat_TexPal[entry.TexPalStart], entry.TexPalSize);
                        if (newPalHash != entry.TexPalHash)
                            goto invalidate;
                    }
                }
            }

            it++;
            continue;
        invalidate:
            FreeTextures[entry.WidthLog2][entry.HeightLog2].push_back(entry.Texture);

            //printf("invalidating texture %d\n", entry.ImageDescriptor);

            it = TexCache.erase(it);
        }
    }
    else if (RenderFrameIdentical)
    {
        return;
    }

    int numYSpans = 0;
    int numSetupIndices = 0;

    u32 curSlice = CmdMem.Begin(EmuCmdBuf);

    u32 numVariants = 0, prevVariant, prevTexLayer;
    Variant variants[MaxVariants];

    int foundviatexcache = 0, foundviaprev = 0, numslow = 0;

    bool enableTextureMaps = RenderDispCnt & (1<<0);

    for (int i = 0; i < RenderNumPolygons; i++)
    {
        Polygon* polygon = RenderPolygonRAM[i];

        u32 nverts = polygon->NumVertices;
        u32 vtop = polygon->VTop, vbot = polygon->VBottom;
        s32 ytop = polygon->YTop, ybot = polygon->YBottom;

        u32 curVL = vtop, curVR = vtop;
        u32 nextVL, nextVR;

        RenderPolygons[i].FirstXSpan = numSetupIndices;
        RenderPolygons[i].YTop = ytop;
        RenderPolygons[i].YBot = ybot;
        RenderPolygons[i].Attr = polygon->Attr;

        bool foundVariant = false;
        if (i > 0)
        {
            Polygon* prevPolygon = RenderPolygonRAM[i - 1];
            foundVariant = prevPolygon->TexParam == polygon->TexParam
                && prevPolygon->TexPalette == polygon->TexPalette
                && (prevPolygon->Attr & 0x30) == (polygon->Attr & 0x30)
                && prevPolygon->IsShadowMask == polygon->IsShadowMask;
            if (foundVariant)
                foundviaprev++;
        }

        if (!foundVariant)
        {
            Variant variant;
            variant.BlendMode = polygon->IsShadowMask ? 4 : ((polygon->Attr >> 4) & 0x3);
            variant.Texture = -1;
            variant.Sampler = -1;
            TexCacheEntry* texcacheEntry = nullptr;
            if (enableTextureMaps && (polygon->TexParam >> 26) & 0x7)
            {
                texcacheEntry = &GetTexture(polygon->TexParam, polygon->TexPalette);
                bool wrapS = (polygon->TexParam >> 16) & 1;
                bool wrapT = (polygon->TexParam >> 17) & 1;
                bool mirrorS = (polygon->TexParam >> 18) & 1;
                bool mirrorT = (polygon->TexParam >> 19) & 1;
                variant.Sampler = (wrapS ? (mirrorS ? 2 : 1) : 0) + (wrapT ? (mirrorT ? 2 : 1) : 0) * 3;
                variant.Texture = texcacheEntry->DescriptorIdx;
                prevTexLayer = texcacheEntry->Texture.LayerIdx;
                if (texcacheEntry->LastVariant < numVariants && variants[texcacheEntry->LastVariant] == variant)
                {
                    foundVariant = true;
                    prevVariant = texcacheEntry->LastVariant;
                    foundviatexcache++;
                }
            }

            if (!foundVariant)
            {
                numslow++;
                for (int j = numVariants - 1; j >= 0; j--)
                {
                    if (variants[j] == variant)
                    {
                        foundVariant = true;
                        prevVariant = j;
                        goto foundVariant;
                    }
                }

                prevVariant = numVariants;
                variants[numVariants] = variant;
                variants[numVariants].Width = TextureWidth(polygon->TexParam);
                variants[numVariants].Height = TextureHeight(polygon->TexParam);
                numVariants++;
                assert(numVariants <= MaxVariants);
            foundVariant:;

                if (texcacheEntry)
                    texcacheEntry->LastVariant = prevVariant;
            }
        }
        RenderPolygons[i].Variant = prevVariant;
        RenderPolygons[i].TextureLayer = (float)prevTexLayer;

        if (polygon->FacingView)
        {
            nextVL = curVL + 1;
            if (nextVL >= nverts) nextVL = 0;
            nextVR = curVR - 1;
            if ((s32)nextVR < 0) nextVR = nverts - 1;
        }
        else
        {
            nextVL = curVL - 1;
            if ((s32)nextVL < 0) nextVL = nverts - 1;
            nextVR = curVR + 1;
            if (nextVR >= nverts) nextVR = 0;
        }

        s32 minX = polygon->Vertices[vtop]->FinalPosition[0];
        s32 minXY = polygon->Vertices[vtop]->FinalPosition[1];
        s32 maxX = polygon->Vertices[vtop]->FinalPosition[0];
        s32 maxXY = polygon->Vertices[vtop]->FinalPosition[1];

        if (ybot == ytop)
        {
            vtop = 0; vbot = 0;

            RenderPolygons[i].YBot++;

            int j = 1;
            if (polygon->Vertices[j]->FinalPosition[0] < polygon->Vertices[vtop]->FinalPosition[0]) vtop = j;
            if (polygon->Vertices[j]->FinalPosition[0] > polygon->Vertices[vbot]->FinalPosition[0]) vbot = j;

            j = nverts - 1;
            if (polygon->Vertices[j]->FinalPosition[0] < polygon->Vertices[vtop]->FinalPosition[0]) vtop = j;
            if (polygon->Vertices[j]->FinalPosition[0] > polygon->Vertices[vbot]->FinalPosition[0]) vbot = j;

            assert(numYSpans < MaxYSpanSetups);
            u32 curSpanL = numYSpans;
            SetupYSpanDummy(&YSpanSetups[numYSpans++], polygon, vtop, 0);
            assert(numYSpans < MaxYSpanSetups);
            u32 curSpanR = numYSpans;
            SetupYSpanDummy(&YSpanSetups[numYSpans++], polygon, vbot, 1);

            minX = YSpanSetups[curSpanL].X0;
            minXY = YSpanSetups[curSpanL].Y0;
            maxX = YSpanSetups[curSpanR].X0;
            maxXY = YSpanSetups[curSpanR].Y0;
            if (maxX < minX)
            {
                std::swap(minX, maxX);
                std::swap(minXY, maxXY);
            }

            assert(numSetupIndices < MaxYSpanIndices);
            YSpanIndices[numSetupIndices].PolyIdx = i;
            YSpanIndices[numSetupIndices].SpanIdxL = curSpanL;
            YSpanIndices[numSetupIndices].SpanIdxR = curSpanR;
            YSpanIndices[numSetupIndices].Y = ytop;
            numSetupIndices++;
        }
        else
        {
            u32 curSpanL = numYSpans;
            assert(numYSpans < MaxYSpanSetups);
            SetupYSpan(i, &YSpanSetups[numYSpans++], polygon, curVL, nextVL, ytop, 0);
            u32 curSpanR = numYSpans;
            assert(numYSpans < MaxYSpanSetups);
            SetupYSpan(i, &YSpanSetups[numYSpans++], polygon, curVR, nextVR, ytop, 1);

            for (u32 y = ytop; y < ybot; y++)
            {
                if (y >= polygon->Vertices[nextVL]->FinalPosition[1] && curVL != polygon->VBottom)
                {
                    while (y >= polygon->Vertices[nextVL]->FinalPosition[1] && curVL != polygon->VBottom)
                    {
                        curVL = nextVL;
                        if (polygon->FacingView)
                        {
                            nextVL = curVL + 1;
                            if (nextVL >= nverts)
                                nextVL = 0;
                        }
                        else
                        {
                            nextVL = curVL - 1;
                            if ((s32)nextVL < 0)
                                nextVL = nverts - 1;
                        }
                    }

                    if (polygon->Vertices[curVL]->FinalPosition[0] < minX)
                    {
                        minX = polygon->Vertices[curVL]->FinalPosition[0];
                        minXY = polygon->Vertices[curVL]->FinalPosition[1];
                    }
                    if (polygon->Vertices[curVL]->FinalPosition[0] > maxX)
                    {
                        maxX = polygon->Vertices[curVL]->FinalPosition[0];
                        maxXY = polygon->Vertices[curVL]->FinalPosition[1];
                    }

                    assert(numYSpans < MaxYSpanSetups);
                    curSpanL = numYSpans;
                    SetupYSpan(i,&YSpanSetups[numYSpans++], polygon, curVL, nextVL, y, 0);
                }
                if (y >= polygon->Vertices[nextVR]->FinalPosition[1] && curVR != polygon->VBottom)
                {
                    while (y >= polygon->Vertices[nextVR]->FinalPosition[1] && curVR != polygon->VBottom)
                    {
                        curVR = nextVR;
                        if (polygon->FacingView)
                        {
                            nextVR = curVR - 1;
                            if ((s32)nextVR < 0)
                                nextVR = nverts - 1;
                        }
                        else
                        {
                            nextVR = curVR + 1;
                            if (nextVR >= nverts)
                                nextVR = 0;
                        }
                    }

                    if (polygon->Vertices[curVR]->FinalPosition[0] < minX)
                    {
                        minX = polygon->Vertices[curVR]->FinalPosition[0];
                        minXY = polygon->Vertices[curVR]->FinalPosition[1];
                    }
                    if (polygon->Vertices[curVR]->FinalPosition[0] > maxX)
                    {
                        maxX = polygon->Vertices[curVR]->FinalPosition[0];
                        maxXY = polygon->Vertices[curVR]->FinalPosition[1];
                    }

                    assert(numYSpans < MaxYSpanSetups);
                    curSpanR = numYSpans;
                    SetupYSpan(i,&YSpanSetups[numYSpans++], polygon, curVR, nextVR, y, 1);
                }

                assert(numSetupIndices < MaxYSpanIndices);
                YSpanIndices[numSetupIndices].PolyIdx = i;
                YSpanIndices[numSetupIndices].SpanIdxL = curSpanL;
                YSpanIndices[numSetupIndices].SpanIdxR = curSpanR;
                YSpanIndices[numSetupIndices].Y = y;
                numSetupIndices++;
            }
        }

        if (polygon->Vertices[nextVL]->FinalPosition[0] < minX)
        {
            minX = polygon->Vertices[nextVL]->FinalPosition[0];
            minXY = polygon->Vertices[nextVL]->FinalPosition[1];
        }
        if (polygon->Vertices[nextVL]->FinalPosition[0] > maxX)
        {
            maxX = polygon->Vertices[nextVL]->FinalPosition[0];
            maxXY = polygon->Vertices[nextVL]->FinalPosition[1];
        }
        if (polygon->Vertices[nextVR]->FinalPosition[0] < minX)
        {
            minX = polygon->Vertices[nextVR]->FinalPosition[0];
            minXY = polygon->Vertices[nextVR]->FinalPosition[1];
        }
        if (polygon->Vertices[nextVR]->FinalPosition[0] > maxX)
        {
            maxX = polygon->Vertices[nextVR]->FinalPosition[0];
            maxXY = polygon->Vertices[nextVR]->FinalPosition[1];
        }

        RenderPolygons[i].XMin = minX;
        RenderPolygons[i].XMinY = minXY;
        RenderPolygons[i].XMax = maxX;
        RenderPolygons[i].XMaxY = maxXY;

        //printf("polygon min max %d %d | %d %d\n", RenderPolygons[i].XMin, RenderPolygons[i].XMinY, RenderPolygons[i].XMax, RenderPolygons[i].XMaxY);
    }

    /*for (u32 i = 0; i < RenderNumPolygons; i++)
    {
        if (RenderPolygons[i].Variant >= numVariants)
        {
            printf("blarb2 %d %d %d\n", RenderPolygons[i].Variant, i, RenderNumPolygons);
        }
        //assert(RenderPolygons[i].Variant < numVariants);
    }*/
    DkGpuAddr gpuAddrBinResult = Gfx::DataHeap->GpuAddr(BinResultMemory);
    DkGpuAddr gpuAddrMetaUniform = Gfx::DataHeap->GpuAddr(MetaUniformMemory);

    if (numYSpans > 0)
    {
        SpanSetupY* yspans = Gfx::DataHeap->CpuAddr<SpanSetupY>(YSpanSetupMemory[curSlice]);
        memcpy(yspans, YSpanSetups, sizeof(SpanSetupY)*numYSpans);
        UploadBuf.UploadAndCopyData(EmuCmdBuf, Gfx::TextureHeap->GpuAddr(YSpanIndicesTextureMemory), (u8*)YSpanIndices, numSetupIndices*4*2);

        memcpy(Gfx::DataHeap->CpuAddr<void>(RenderPolygonMemory[curSlice]), RenderPolygons, RenderNumPolygons*sizeof(RenderPolygon));

        // we haven't accessed image data yet, so we don't need to invalidate anything
        EmuCmdBuf.barrier(DkBarrier_Full, DkInvalidateFlags_Image|DkInvalidateFlags_Descriptors|DkInvalidateFlags_L2Cache);
    }

    //printf("found via %d %d %d of %d\n", foundviatexcache, foundviaprev, numslow, RenderNumPolygons);

    // bind everything
    EmuCmdBuf.bindImageDescriptorSet(Gfx::DataHeap->GpuAddr(ImageDescriptors), descriptorOffset_Count);
    EmuCmdBuf.bindSamplerDescriptorSet(Gfx::DataHeap->GpuAddr(SamplerDescriptors), 9);
    EmuCmdBuf.bindStorageBuffers(DkStage_Compute, 0,
    {
        {Gfx::DataHeap->GpuAddr(YSpanSetupMemory[curSlice]), YSpanSetupMemory[curSlice].Size},
        {Gfx::DataHeap->GpuAddr(XSpanSetupMemory), XSpanSetupMemory.Size},
        {Gfx::DataHeap->GpuAddr(RenderPolygonMemory[curSlice]), RenderPolygonMemory[curSlice].Size},
        {gpuAddrBinResult, BinResultMemory.Size},
        {Gfx::DataHeap->GpuAddr(TileMemory), TileMemory.Size},
        {Gfx::DataHeap->GpuAddr(FinalTileMemory), FinalTileMemory.Size}
    });

    MetaUniform meta;
    meta.DispCnt = RenderDispCnt;
    meta.NumPolygons = RenderNumPolygons;
    meta.NumVariants = numVariants;
    meta.AlphaRef = RenderAlphaRef;
    {
        u32 r = (RenderClearAttr1 << 1) & 0x3E; if (r) r++;
        u32 g = (RenderClearAttr1 >> 4) & 0x3E; if (g) g++;
        u32 b = (RenderClearAttr1 >> 9) & 0x3E; if (b) b++;
        u32 a = (RenderClearAttr1 >> 16) & 0x1F;
        meta.ClearColor = r | (g << 8) | (b << 16) | (a << 24);
        meta.ClearDepth = ((RenderClearAttr2 & 0x7FFF) * 0x200) + 0x1FF;
        meta.ClearAttr = RenderClearAttr1 & 0x3F008000;
    }
    for (u32 i = 0; i < 32; i++)
    {
        u32 color = RenderToonTable[i];
        u32 r = (color << 1) & 0x3E;
        u32 g = (color >> 4) & 0x3E;
        u32 b = (color >> 9) & 0x3E;
        if (r) r++;
        if (g) g++;
        if (b) b++;

        meta.ToonTable[i*4+0] = r | (g << 8) | (b << 16);
    }
    for (u32 i = 0; i < 34; i++)
    {
        meta.ToonTable[i*4+1] = RenderFogDensityTable[i];
    }
    for (u32 i = 0; i < 8; i++)
    {
        u32 color = RenderEdgeTable[i];
        u32 r = (color << 1) & 0x3E;
        u32 g = (color >> 4) & 0x3E;
        u32 b = (color >> 9) & 0x3E;
        if (r) r++;
        if (g) g++;
        if (b) b++;

        meta.ToonTable[i*4+2] = r | (g << 8) | (b << 16);
    }
    meta.FogOffset = RenderFogOffset;
    meta.FogShift = RenderFogShift;
    {
        u32 fogR = (RenderFogColor << 1) & 0x3E; if (fogR) fogR++;
        u32 fogG = (RenderFogColor >> 4) & 0x3E; if (fogG) fogG++;
        u32 fogB = (RenderFogColor >> 9) & 0x3E; if (fogB) fogB++;
        u32 fogA = (RenderFogColor >> 16) & 0x1F;
        meta.FogColor = fogR | (fogG << 8) | (fogB << 16) | (fogA << 24);
    }
    meta.XScroll = RenderXPos;
    EmuCmdBuf.bindUniformBuffer(DkStage_Compute, 0, Gfx::DataHeap->GpuAddr(MetaUniformMemory), MetaUniformSize);
    EmuCmdBuf.pushConstants(gpuAddrMetaUniform, MetaUniformSize, 0, sizeof(MetaUniform), &meta);

    EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderClearCoarseBinMask});
    EmuCmdBuf.dispatchCompute(TilesPerLine*TileLines/32, 1, 1);

    bool wbuffer = false;
    if (numYSpans > 0)
    {
        wbuffer = RenderPolygonRAM[0]->WBuffer;

        EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderClearIndirectWorkCount});
        EmuCmdBuf.dispatchCompute((numVariants+31)/32, 1, 1);

        // calculate x-spans
        EmuCmdBuf.bindImages(DkStage_Compute, 0, {dkMakeImageHandle(descriptorOffset_YSpanIndices)});
        EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderInterpXSpans[wbuffer]});
        EmuCmdBuf.dispatchCompute((numSetupIndices + 31) / 32, 1, 1);
        EmuCmdBuf.barrier(DkBarrier_Primitives, 0);

        // bin polygons
        EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderBinCombined});
        EmuCmdBuf.dispatchCompute(((RenderNumPolygons + 31) / 32), 256/CoarseTileW, 192/CoarseTileH);
        EmuCmdBuf.barrier(DkBarrier_Primitives, 0);

        // calculate list offsets
        EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderCalculateWorkListOffset});
        EmuCmdBuf.dispatchCompute((numVariants + 31) / 32, 1, 1);
        EmuCmdBuf.barrier(DkBarrier_Primitives, 0);

        // sort shader work
        EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderSortWork});
        EmuCmdBuf.dispatchComputeIndirect(gpuAddrBinResult + offsetof(BinResult, SortWorkWorkCount));
        EmuCmdBuf.barrier(DkBarrier_Primitives, 0);

        // rasterise
        {
            bool highLightMode = RenderDispCnt & (1<<1);

            dk::Shader* shadersNoTexture[] =
            {
                &ShaderRasteriseNoTexture[wbuffer],
                &ShaderRasteriseNoTexture[wbuffer],
                highLightMode
                    ? &ShaderRasteriseNoTextureHighlight[wbuffer]
                    : &ShaderRasteriseNoTextureToon[wbuffer],
                &ShaderRasteriseNoTexture[wbuffer],
                &ShaderRasteriseShadowMask[wbuffer]
            };
            dk::Shader* shadersUseTexture[] =
            {
                &ShaderRasteriseUseTextureModulate[wbuffer],
                &ShaderRasteriseUseTextureDecal[wbuffer],
                highLightMode
                    ? &ShaderRasteriseUseTextureHighlight[wbuffer]
                    : &ShaderRasteriseUseTextureToon[wbuffer],
                &ShaderRasteriseUseTextureDecal[wbuffer],
                &ShaderRasteriseShadowMask[wbuffer]
            };

            dk::Shader* prevShader = NULL;
            s32 prevTexture = -1, prevSampler = -1;
            for (int i = 0; i < numVariants; i++)
            {
                dk::Shader* shader = NULL;
                if (variants[i].Texture == -1)
                {
                    shader = shadersNoTexture[variants[i].BlendMode];
                }
                else
                {
                    shader = shadersUseTexture[variants[i].BlendMode];
                    if (variants[i].Texture != prevTexture || variants[i].Sampler != prevSampler)
                    {
                        assert(variants[i].Sampler < 9);
                        EmuCmdBuf.bindTextures(DkStage_Compute, 0,
                        {
                            dkMakeTextureHandle(descriptorOffset_TexcacheStart + variants[i].Texture, variants[i].Sampler)
                        });
                        prevTexture = variants[i].Texture;
                        prevSampler = variants[i].Sampler;
                        meta.InvTextureSize[0] = 1.f / variants[i].Width;
                        meta.InvTextureSize[1] = 1.f / variants[i].Height;
                    }
                }
                assert(shader != NULL);
                if (shader != prevShader)
                {
                    EmuCmdBuf.bindShaders(DkStageFlag_Compute, {shader});
                    prevShader = shader;
                }
                meta.CurVariant = i;
                // not pretty, but alignment shouldn't matter as we only have 4 byte values
                EmuCmdBuf.pushConstants(gpuAddrMetaUniform, MetaUniformSize, offsetof(MetaUniform, CurVariant), 4*3, &meta.CurVariant);
                EmuCmdBuf.dispatchComputeIndirect(gpuAddrBinResult + offsetof(BinResult, VariantWorkCount) + i*4*4);
            }
        }
        EmuCmdBuf.barrier(DkBarrier_Primitives, 0);
    }
    else
    {
        EmuCmdBuf.barrier(DkBarrier_Primitives, 0);
    }

    // compose final image
    EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderDepthBlend[wbuffer]});
    EmuCmdBuf.dispatchCompute(256/8, 192/8, 1);
    EmuCmdBuf.barrier(DkBarrier_Primitives, 0);

    EmuCmdBuf.bindImages(DkStage_Compute, 0, {dkMakeImageHandle(descriptorOffset_FinalFB)});
    u32 finalPassShader = 0;
    if (RenderDispCnt & (1<<4))
        finalPassShader |= 0x4;
    if (RenderDispCnt & (1<<7))
        finalPassShader |= 0x2;
    if (RenderDispCnt & (1<<5))
        finalPassShader |= 0x1;
    EmuCmdBuf.bindShaders(DkStageFlag_Compute, {&ShaderFinalPass[finalPassShader]});
    EmuCmdBuf.dispatchCompute(256/32, 192, 1);
    EmuCmdBuf.barrier(DkBarrier_Primitives, 0);

    DkCmdList cmdlist = CmdMem.End(EmuCmdBuf);
    EmuQueue.submitCommands(cmdlist);
    EmuQueue.flush();
    UploadBuf.LastFlushBuffer = 0;

    /*u64 starttime = armGetSystemTick();
    EmuQueue.waitIdle();
    printf("total time %f\n", armTicksToNs(armGetSystemTick()-starttime)*0.000001f);*/

    /*for (u32 i = 0; i < RenderNumPolygons; i++)
    {
        if (RenderPolygons[i].Variant >= numVariants)
        {
            printf("blarb %d %d %d\n", RenderPolygons[i].Variant, i, RenderNumPolygons);
        }
        //assert(RenderPolygons[i].Variant < numVariants);
    }*/

    /*for (int i = 0; i < binresult->SortWorkWorkCount[0]*32; i++)
    {
        printf("sorted %x %x\n", binresult->SortedWork[i*2+0], binresult->SortedWork[i*2+1]);
    }*/
/*    if (polygonvisible != -1)
    {
        SpanSetupX* xspans = Gfx::DataHeap->CpuAddr<SpanSetupX>(XSpanSetupMemory);
        printf("span result\n");
        Polygon* poly = RenderPolygonRAM[polygonvisible];
        u32 xspanoffset = RenderPolygons[polygonvisible].FirstXSpan;
        for (u32 i = 0; i < (poly->YBottom - poly->YTop); i++)
        {
            printf("%d: %d - %d | %d %d | %d %d\n", i + poly->YTop, xspans[xspanoffset + i].X0, xspans[xspanoffset + i].X1, xspans[xspanoffset + i].__pad0, xspans[xspanoffset + i].__pad1, RenderPolygons[polygonvisible].YTop, RenderPolygons[polygonvisible].YBot);
        }
    }*/
/*
    printf("xspans: %d\n", numSetupIndices);
    SpanSetupX* xspans = Gfx::DataHeap->CpuAddr<SpanSetupX>(XSpanSetupMemory[curSlice]);
    for (int i = 0; i < numSetupIndices; i++)
    {
        printf("poly %d %d %d | line %d | %d to %d\n", YSpanIndices[i].PolyIdx, YSpanIndices[i].SpanIdxL, YSpanIndices[i].SpanIdxR, YSpanIndices[i].Y, xspans[i].X0, xspans[i].X1);
    }
    printf("bin result\n");
    BinResult* binresult = Gfx::DataHeap->CpuAddr<BinResult>(BinResultMemory);
    for (u32 y = 0; y < 192/8; y++)
    {
        for (u32 x = 0; x < 256/8; x++)
        {
            printf("%08x ", binresult->BinnedMaskCoarse[(x + y * (256/8)) * 2]);
        }
        printf("\n");
    }*/
}

void DekoRenderer::RestartFrame()
{

}

u32* DekoRenderer::GetLine(int line)
{
    return DummyLine;
}

}