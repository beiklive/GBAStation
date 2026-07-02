#ifndef GPU3D_DEKO
#define GPU3D_DEKO

#include "GPU3D.h"

#include <deko3d.hpp>

#include "frontend/switch/CmdMemRing.h"
#include "frontend/switch/GpuMemHeap.h"
#include "frontend/switch/UploadBuffer.h"

#include "NonStupidBitfield.h"

#include <unordered_map>

namespace GPU3D
{

class DekoRenderer : public Renderer3D
{
public:
    DekoRenderer();
    ~DekoRenderer() override;

    bool Init() override;
    void DeInit() override;
    void Reset() override;

    void SetRenderSettings(GPU::RenderSettings& settings) override;

    void VCount144() override;

    void RenderFrame() override;
    void RestartFrame() override;
    u32* GetLine(int line) override;

    //dk::Fence FrameReady = {};
    //dk::Fence FrameReserveFence = {};
private:
    dk::Shader ShaderInterpXSpans[2];
    dk::Shader ShaderBinCombined;
    dk::Shader ShaderDepthBlend[2];
    dk::Shader ShaderRasteriseNoTexture[2];
    dk::Shader ShaderRasteriseNoTextureToon[2];
    dk::Shader ShaderRasteriseNoTextureHighlight[2];
    dk::Shader ShaderRasteriseUseTextureDecal[2];
    dk::Shader ShaderRasteriseUseTextureModulate[2];
    dk::Shader ShaderRasteriseUseTextureToon[2];
    dk::Shader ShaderRasteriseUseTextureHighlight[2];
    dk::Shader ShaderRasteriseShadowMask[2];
    dk::Shader ShaderClearCoarseBinMask;
    dk::Shader ShaderClearIndirectWorkCount;
    dk::Shader ShaderCalculateWorkListOffset;
    dk::Shader ShaderSortWork;
    dk::Shader ShaderFinalPass[8];

    CmdMemRing<2> CmdMem;
    GpuMemHeap::Allocation YSpanIndicesTextureMemory;
    dk::Image YSpanIndicesTexture;
    GpuMemHeap::Allocation YSpanSetupMemory[2];
    GpuMemHeap::Allocation XSpanSetupMemory;
    GpuMemHeap::Allocation BinResultMemory;
    GpuMemHeap::Allocation RenderPolygonMemory[2];
    GpuMemHeap::Allocation TileMemory;
    GpuMemHeap::Allocation FinalTileMemory;

    GpuMemHeap::Allocation ImageDescriptors;
    GpuMemHeap::Allocation SamplerDescriptors;

    struct MetaUniform
    {
        u32 NumPolygons;
        u32 NumVariants;

        u32 AlphaRef;
        u32 DispCnt;

        u32 ToonTable[4*34];

        u32 ClearColor, ClearDepth, ClearAttr;

        u32 FogOffset, FogShift, FogColor;

        u32 XScroll;
        // only used/updated for rasteriation
        u32 CurVariant;
        float InvTextureSize[2];
    };
    GpuMemHeap::Allocation MetaUniformMemory;
    const int MetaUniformSize = (sizeof(MetaUniform) + DK_UNIFORM_BUF_ALIGNMENT - 1) & ~(DK_UNIFORM_BUF_ALIGNMENT - 1);

    UploadBuffer UploadBuf;

    static const u32 TexCacheMaxImages = 4096;

    enum
    {
        descriptorOffset_YSpanIndices,
        descriptorOffset_FinalFB,
        descriptorOffset_WhiteTexture,
        descriptorOffset_TexcacheStart,
        descriptorOffset_Count = descriptorOffset_TexcacheStart + TexCacheMaxImages
    };

    u32 DummyLine[256] = {};

    struct SpanSetupY
    {
        // Attributes
        s32 Z0, Z1, W0, W1;
        s32 ColorR0, ColorG0, ColorB0;
        s32 ColorR1, ColorG1, ColorB1;
        s32 TexcoordU0, TexcoordV0;
        s32 TexcoordU1, TexcoordV1;

        // Interpolator
        s32 I0, I1;
        s32 Linear;
        s32 IRecip;
        s32 W0n, W0d, W1d;

        // Slope
        s32 Increment;

        s32 X0, X1, Y0, Y1;
        s32 XMin, XMax;
        s32 DxInitial;

        s32 XCovIncr;
        u32 IsDummy, __pad1;
    };
    struct SpanSetupX
    {
        s32 X0, X1;

        s32 EdgeLenL, EdgeLenR, EdgeCovL, EdgeCovR;

        s32 XRecip;

        u32 Flags;

        s32 Z0, Z1, W0, W1;
        s32 ColorR0, ColorG0, ColorB0;
        s32 ColorR1, ColorG1, ColorB1;
        s32 TexcoordU0, TexcoordV0;
        s32 TexcoordU1, TexcoordV1;

        s32 CovLInitial, CovRInitial;
    };
    struct SetupIndices
    {
        u16 PolyIdx, SpanIdxL, SpanIdxR, Y;
    };
    struct RenderPolygon
    {
        u32 FirstXSpan;
        s32 YTop, YBot;

        s32 XMin, XMax;
        s32 XMinY, XMaxY;

        u32 Variant;
        u32 Attr;

        float TextureLayer;
        u32 __pad0, __pad1;
    };

    static const int TileSize = 8;
    static const int CoarseTileCountX = 8;
    static const int CoarseTileCountY = 4;
    static const int CoarseTileW = CoarseTileCountX * TileSize;
    static const int CoarseTileH = CoarseTileCountY * TileSize;

    static const int TilesPerLine = 256/TileSize;
    static const int TileLines = 192/TileSize;

    static const int BinStride = 2048/32;
    static const int CoarseBinStride = BinStride/32;

    static const int MaxWorkTiles = TilesPerLine*TileLines*48;
    static const int MaxVariants = 256;

    struct BinResult
    {
        u32 VariantWorkCount[MaxVariants*4];
        u32 SortedWorkOffset[MaxVariants];

        u32 SortWorkWorkCount[4];
        u32 UnsortedWorkDescs[MaxWorkTiles*2];
        u32 SortedWork[MaxWorkTiles*2];

        u32 BinnedMaskCoarse[TilesPerLine*TileLines*CoarseBinStride];
        u32 BinnedMask[TilesPerLine*TileLines*BinStride];
        u32 WorkOffsets[TilesPerLine*TileLines*BinStride];
    };

    struct Tiles
    {
        u32 ColorTiles[MaxWorkTiles*TileSize*TileSize];
        u32 DepthTiles[MaxWorkTiles*TileSize*TileSize];
        u32 AttrStencilTiles[MaxWorkTiles*TileSize*TileSize];
    };

    struct FinalTiles
    {
        u32 ColorResult[256*192*2];
        u32 DepthResult[256*192*2];
        u32 AttrResult[256*192*2];
    };

    // eh those are pretty bad guesses
    // though real hw shouldn't be eable to render all 2048 polygons on every line either
    static const int MaxYSpanIndices = 64*2048;
    static const int MaxYSpanSetups = 6144*2;
    SetupIndices YSpanIndices[MaxYSpanIndices];
    SpanSetupY YSpanSetups[MaxYSpanSetups];
    RenderPolygon RenderPolygons[2048];

    struct TexArrayEntry
    {
        u16 TexArrayIdx;
        u16 LayerIdx;
    };
    struct TexArray
    {
        GpuMemHeap::Allocation Memory;
        dk::Image Image;
        u32 ImageDescriptor;
    };

    struct TexCacheEntry
    {
        u32 DescriptorIdx;
        u32 LastVariant; // very cheap way to make variant lookup faster

        u32 TextureRAMStart[2], TextureRAMSize[2];
        u32 TexPalStart, TexPalSize;
        u8 WidthLog2, HeightLog2;
        TexArrayEntry Texture;

        u64 TextureHash[2];
        u64 TexPalHash;
    };
    std::unordered_map<u64, TexCacheEntry> TexCache;

    u32 FreeImageDescriptorsCount = 0;
    u32 FreeImageDescriptors[TexCacheMaxImages];

    std::vector<TexArrayEntry> FreeTextures[8][8];
    std::vector<TexArray> TexArrays[8][8];

    u32 TextureDecodingBuffer[1024*1024];

    TexCacheEntry& GetTexture(u32 textureParam, u32 paletteParam);

    void SetupAttrs(SpanSetupY* span, Polygon* poly, int from, int to);
    void SetupYSpan(int polynum, SpanSetupY* span, Polygon* poly, int from, int to, u32 y, int side);
    void SetupYSpanDummy(SpanSetupY* span, Polygon* poly, int vertex, int side);
};

}

#endif