#ifndef GPU2D_NEONSOFT
#define GPU2D_NEONSOFT

#include "GPU2D.h"

class GPU2D_NeonSoft : public GPU2D
{
public:
    GPU2D_NeonSoft(u32 num);
    ~GPU2D_NeonSoft() override {}

    void DrawScanline(u32 line) override;
    void DrawSprites(u32 line) override;

protected:
    void MosaicXSizeChanged() {}

private:
    u32 BGOBJLine[272*2] __attribute__((aligned (16)));
    u32* _3DLine;

    u32 OBJLine[272*2] __attribute__((aligned (16)));
    u8 OBJIndex[256];

    u8 WindowMask[272] __attribute__((aligned (16)));
    u8 OBJWindow[272] __attribute__((aligned (16)));

    u8 MosaicTable[16][256];
    u8* CurBGXMosaicTable;
    u8* CurOBJXMosaicTable;

    u32 NumSprites[4];
    u32 NumSpritesPerLayer[4];
    u8 SpriteCache[4][128];

    bool SkipRendering;
    bool _3DSemiTransparencies;
    bool SemiTransBitmapSprites;
    bool SemiTransTileSprites;

    template <bool Enable3DBlend, int SecondSrcBlend>
    void ApplyColorEffect();

    void PalettiseRange(u32 start);

    void InterleaveSprites(u32 prio);

    void DoCapture(u32 line, u32 width);

    template<u32 bgmode>
    void DrawScanlineBGMode(u32 line);
    void DrawScanlineBGMode6(u32 line);
    void DrawScanlineBGMode7(u32 line);
    void DrawScanline_BGOBJ(u32 line);

    void DrawBG_3D();
    template <bool mosaic>
    void DrawBG_Text(u32 line, u32 bgnum);
    template <bool mosaic>
    void DrawBG_Affine(u32 line, u32 bgnum);
    template <bool mosaic>
    void DrawBG_Extended(u32 line, u32 bgnum);
    template <bool mosaic>
    void DrawBG_Large(u32 line);

    template <bool window>
    void DrawSprite_Normal(u32 num, u32 width, u32 height, s32 xpos, s32 ypos);
    template <bool window>
    void DrawSprite_Rotscale(u32 num, u32 boundwidth, u32 boundheight, u32 width, u32 height, s32 xpos, s32 ypos);
};

#endif