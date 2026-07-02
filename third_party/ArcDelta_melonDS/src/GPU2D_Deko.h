#include "GPU2D.h"

#include <deko3d.hpp>

#include <vector>

#include "frontend/switch/GpuMemHeap.h"
#include "frontend/switch/CmdMemRing.h"
#include "frontend/switch/UploadBuffer.h"

namespace GPU2D
{

class DekoRenderer : public Renderer2D
{
public:
    DekoRenderer();
    ~DekoRenderer() override;

    void Reset() override;

    void DrawScanline(u32 line, Unit* unit) override;
    void DrawSprites(u32 line, Unit* unit) override;

    void VBlankEnd(Unit* unitA, Unit* unitB) override;

    dk::Image& GetFramebuffer(u32 front, u32 num)
    {
        return FinalFramebuffers[front][num];
    }
    dk::Image& Get3DFramebuffer()
    {
        return _3DFramebuffer;
    }

    dk::Fence FramebufferReady[2] = {};
    dk::Fence FramebufferPresented[2] = {};
private:
    u16 DispFIFOFramebuffer[256*192];

    dk::Image FinalFramebuffers[2][2];
    GpuMemHeap::Allocation FinalFramebufferMemory;

    dk::Image _3DFramebuffer;
    GpuMemHeap::Allocation _3DFramebufferMemory;

    dk::Image BGOBJTexture;
    GpuMemHeap::Allocation BGOBJTextureMemory;

    GpuMemHeap::Allocation DisplayCaptureMemory;
    dk::Fence DisplayCaptureFence;

    enum
    {
        fb_BG0,
        fb_BG1,
        fb_BG2,
        fb_BG3,
        fb_OBJ,
        fb_Count
    };
    dk::Image IntermedFramebuffers[fb_Count*2];
    GpuMemHeap::Allocation IntermedFramebufferMemory;
    dk::Image OBJDepth;
    GpuMemHeap::Allocation OBJDepthMemory;
    dk::Image OBJWindow[2];
    GpuMemHeap::Allocation OBJWindowMemory;

    dk::Image DisabledBG;
    GpuMemHeap::Allocation DisabledBGMemory;

    dk::Image MosaicTable;
    GpuMemHeap::Allocation MosaicTableMemory;

    enum
    {
        textureVRAM_ABG,
        textureVRAM_BBG,
        textureVRAM_AOBJ,
        textureVRAM_BOBJ,
        textureVRAM_Count
    };

    enum
    {
        descriptorOffset_IntermedFb = 0,
        descriptorOffset_VRAM8 = descriptorOffset_IntermedFb+fb_Count*2,
        descriptorOffset_VRAM16 = descriptorOffset_VRAM8+textureVRAM_Count,
        descriptorOffset_Palettes = descriptorOffset_VRAM16+textureVRAM_Count,
        descriptorOffset_DirectBitmap = descriptorOffset_Palettes+2,
        descriptorOffset_3DFramebuffer = descriptorOffset_DirectBitmap+2,
        descriptorOffset_DisabledBG,
        descriptorOffset_MosaicTable,
        descriptorOffset_OBJWindow,
        descriptorOffset_Count = descriptorOffset_OBJWindow+2
    };
    GpuMemHeap::Allocation ImageDescriptors;
    GpuMemHeap::Allocation SamplerDescriptors;

    GpuMemHeap::Allocation VRAMTextureMemory[textureVRAM_Count];
    dk::Image VRAM8Texture[textureVRAM_Count];
    dk::Image VRAM16Texture[textureVRAM_Count];
    GpuMemHeap::Allocation PaletteTextureMemory;
    enum
    {
        paletteMemory_BGExtPalOffset = 2*512,
        paletteMemory_OBJExtPalOffset = paletteMemory_BGExtPalOffset+64*512,
        paletteMemory_UnitSize = paletteMemory_OBJExtPalOffset+16*512
    };
    dk::Image PaletteTextures[2];
    GpuMemHeap::Allocation DirectBitmapTextureMemory;
    dk::Image DirectBitmapTexture[2];

    UploadBuffer UploadBuf;

    GpuMemHeap::Allocation BGUniformMemory;
    GpuMemHeap::Allocation ComposeUniformMemory;
    GpuMemHeap::Allocation OBJUniformMemory;

    dk::Shader ShaderFullscreenQuad;
    dk::Shader ShaderBGText4Bpp[2];
    dk::Shader ShaderBGText8Bpp[2];
    dk::Shader ShaderBGAffine[2];
    dk::Shader ShaderBGExtendedBitmap8pp[2];
    dk::Shader ShaderBGExtendedBitmapDirect[2];
    dk::Shader ShaderBGExtendedMixed[2];
    dk::Shader ShaderComposeBGOBJ;
    dk::Shader ShaderComposeBGOBJDirectBitmapOnly;
    dk::Shader ShaderComposeBGOBJShowBitmap;
    dk::Shader ShaderOBJRegular;
    dk::Shader ShaderOBJAffine;
    dk::Shader ShaderOBJ4bpp;
    dk::Shader ShaderOBJ8bpp;
    dk::Shader ShaderOBJBitmap;
    dk::Shader ShaderOBJWindow4bpp;
    dk::Shader ShaderOBJWindow8bpp;

    CmdMemRing<8> CmdMem;

    enum
    {
        bgState_Disable,
        bgState_3D,
        bgState_Text4bpp,
        bgState_Text8bpp,
        bgState_Affine,
        bgState_ExtendedBitmap8bpp,
        bgState_ExtendedBitmapDirect,
        bgState_ExtendedMixed
    };

    int BGState[2][4];

    struct OBJUniform
    {
        u32 VRAMMask, __pad0, __pad1, __pad2;
        float RotScaleParams[4*32];
    };
    const u32 OBJUniformSize = (sizeof(OBJUniform) + DK_UNIFORM_BUF_ALIGNMENT - 1) & ~(DK_UNIFORM_BUF_ALIGNMENT - 1);

    struct ComposeUniform
    {
        u32 BGSpriteMasks[5];
        u32 MasterBrightnessMode, MasterBrightnessFactor;
        u32 BlendCnt, StandardColorEffect;
        u32 EVA, EVB, EVY;
        u32 BGNumMask[4];
        u32 Window[192*4];
    };
    const u32 ComposeUniformSize = (sizeof(ComposeUniform) + DK_UNIFORM_BUF_ALIGNMENT - 1) & ~(DK_UNIFORM_BUF_ALIGNMENT - 1);

    ComposeUniform ComposeUniforms[2];

    struct BGUniform
    {
        union
        {
            struct
            {
                u32 TilesetAddr, WideXMask, BGVRAMMask, BasePalette;
                u32 MetaMask, ExtpalMask, __pad1, __pad2;
                u32 __pad3, __pad4, __pad5, MosaicLevel;
            } Text;
            struct
            {
                u32 TilesetAddr, TilemapAddr, BGVRAMMask, YShift;
                u32 XMask, YMask, OfxMask, OfyMask;
                u32 MetaMask, ExtPalMask, __pad2, MosaicLevel;
            } Affine;
        };

        u32 PerLineData[4*192];
    };

    BGUniform BGTextUniforms[2][4];
    const u32 BGUniformSize = (sizeof(BGUniform) + DK_UNIFORM_BUF_ALIGNMENT - 1) & ~(DK_UNIFORM_BUF_ALIGNMENT - 1);

    struct ComposeRegion
    {
        u32 StdPalDst, BGExtPalDst, OBJExtPalDst;
        DkGpuAddr StdPalSrc, BGExtPalSrc, OBJExtPalSrc;
        u32 StdPalSize, BGExtPalSize, OBJExtPalSize;

        u32 LinesCount;

        bool ForceBlank;
        u32 DispCnt;
        u16 BGCnt[4];

        u16 BlendCnt;
        u16 MasterBrightness;
        u8 EVA, EVB, EVY;
    };

    std::vector<ComposeRegion> ComposeRegions[2];

    void DoCapture();

    template <u32 Size>
    void UploadVRAM(const NonStupidBitField<Size>& dirty, u8* src, DkGpuAddr dst);
    template <u32 Size>
    void UploadPalVRAM(const NonStupidBitField<Size>& dirty, u8* dataSrc, u32& dst, DkGpuAddr& src, u32& size);

    void FlushBGDraw(u32 curline, u32 bgmask);
    void FlushOBJDraw(u32 curline);
    void FillinCurComposeRegion(ComposeRegion& out);
    void ComposeBGOBJ();


    bool CmdBufOpen = false;

    void OpenCmdBuf();

    u8 BGOBJRedrawn[2] = {0};

    bool OBJWindowEmpty[2];

    bool CaptureLatch;
    u32 CaptureCnt, DispCnt;

    s32 OBJBatchFirstLine[2], OBJBatchLinesCount[2];
    s32 BGBatchFirstLine[2][4], BGBatchLinesCount[2][4];

    u16 LastBGXPos[2][4];
    u16 LastBGYPos[2][4];
    s32 LastBGXRef[2][2];
    s32 LastBGYRef[2][2];
    s16 LastBGRotA[2][2];
    s16 LastBGRotB[2][2];
    s16 LastBGRotC[2][2];
    s16 LastBGRotD[2][2];
    u32 LastDispCnt[2];
    u16 LastBGCnt[2][4];
    u8 LastBGMosaicSizeX[2];
    u8 LastBGMosaicYMax[2];

    bool AffineChangedMidframe[2][2];

    u16 LastBlendCnt[2];
    u16 LastMasterBrightness[2];
    u8 LastEVA[2], LastEVB[2], LastEVY[2];

    bool LastForceBlank[2];

    bool DirectBitmapNeeded[2];

    u16 DirectBitmap[2][256*192];

    u8 OAMShadow[2*1024];

    template<u32 bgmode> void DrawScanlineBGMode(u32 line);
    void DrawScanlineBGMode6(u32 line);
    void DrawScanlineBGMode7(u32 line);
    void DrawScanline_BGOBJ(u32 line);

    void DrawBG_3D(u32 line);
    void DrawBG_Text(u32 line, u32 bgnum);
    void DrawBG_Affine(u32 line, u32 bgnum);
    void DrawBG_Extended(u32 line, u32 bgnum);
    void DrawBG_Large(u32 line);
};

}