#ifndef GPU3D_DEKO
#define GPU3D_DEKO

#include "GPU3D.h"

#include <deko3d.hpp>

#include "frontend/switch/CmdMemRing.h"
#include "frontend/switch/GpuMemHeap.h"
#include "frontend/switch/UploadBuffer.h"

#include "NonStupidBitfield.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

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
    static constexpr int MaxScaleFactor = 4;
    dk::Shader ShaderInterpXSpans[MaxScaleFactor][2];
    dk::Shader ShaderBinCombined[MaxScaleFactor];
    dk::Shader ShaderDepthBlend[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseNoTexture[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseNoTextureToon[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseNoTextureHighlight[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseUseTextureDecal[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseUseTextureModulate[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseUseTextureToon[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseUseTextureHighlight[MaxScaleFactor][2];
    dk::Shader ShaderRasteriseShadowMask[MaxScaleFactor][2];
    dk::Shader ShaderClearCoarseBinMask[MaxScaleFactor];
    dk::Shader ShaderClearIndirectWorkCount[MaxScaleFactor];
    dk::Shader ShaderCalculateWorkListOffset[MaxScaleFactor];
    dk::Shader ShaderSortWork[MaxScaleFactor];
    dk::Shader ShaderFinalPass[MaxScaleFactor][8];
    bool ShaderScaleLoaded[MaxScaleFactor] = {};

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
    bool DescriptorsInitialized = false;

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
        descriptorOffset_LowResFB,
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

    static const int BinStride = 2048/32;
    static const int CoarseBinStride = BinStride/32;
    static const int MaxVariants = 256;

    int ScaleFactor = 1;
    int ScreenWidth = 256;
    int ScreenHeight = 192;
    int TilesPerLine = 256/TileSize;
    int TileLines = 192/TileSize;
    int MaxWorkTiles = (256/TileSize)*(192/TileSize)*48;
    int MaxYSpanIndices = 64*2048;
    int MaxYSpanSetups = 6144*2;
    bool ScaleResourcesAllocated = false;

    bool BetterPolygons = false;

    std::vector<SetupIndices> YSpanIndices;
    std::vector<SpanSetupY> YSpanSetups;
    std::vector<RenderPolygon> RenderPolygons;

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

    void LoadShaders(int scale);
    void AllocateScaleResources(int scale);
    void FreeScaleResources();
    std::size_t BinResultSize() const;
    std::size_t TilesSize() const;
    std::size_t FinalTilesSize() const;
    std::size_t VariantWorkCountOffset() const { return 0; }
    std::size_t SortWorkWorkCountOffset() const;

    void SetupAttrs(SpanSetupY* span, Polygon* poly, int from, int to);
    void SetupYSpan(int polynum, SpanSetupY* span, Polygon* poly, int from, int to, u32 y, int side, const s32 scaledPositions[][2]);
    void SetupYSpanDummy(SpanSetupY* span, Polygon* poly, int vertex, int side, const s32 scaledPositions[][2]);
};

}

#endif
