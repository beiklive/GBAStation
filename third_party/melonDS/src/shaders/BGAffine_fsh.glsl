#version 460

#cmakedefine Affine
#cmakedefine ExtendedBitmap8bpp
#cmakedefine ExtendedBitmapDirectColor
#cmakedefine ExtendedMixed
#cmakedefine Mosaic

layout (binding = 0) uniform usamplerBuffer BGVRAM8;
layout (binding = 1) uniform usamplerBuffer BGVRAM16;
layout (binding = 2) uniform usampler2D MosaicTable;

layout (location = 0) out uint FinalColor;

layout (std140, binding = 0) uniform BGUniform
{
    int TilesetAddr, TilemapAddr, BGVRAMMask, YShift;
    int XMask, YMask, OfxMask, OfyMask;
    uint MetaMask, ExtPalMask, pad0, MosaicLevel;
    ivec4 PerLineData[192]; // rotX, rotY, rotA, rotC
};

void main()
{
    ivec2 position = ivec2(gl_FragCoord.xy);

#ifdef Mosaic
    position.x = int(texelFetch(MosaicTable, ivec2(position.x, int(MosaicLevel)), 0).x);
#endif

    ivec4 perLineData = PerLineData[position.y];

    ivec2 rot = perLineData.xy + int(position).x * perLineData.zw;

#if defined(ExtendedMixed) || defined(Affine)
    if (((rot.x | rot.y) & OfxMask) == 0)
    {
#ifdef Affine
        int tileoffset = (((rot.y & XMask) >> 11) << (YShift & 0x1F)) + ((rot.x & XMask) >> 11);

        uint tile = texelFetch(BGVRAM8, (TilemapAddr + tileoffset) & BGVRAMMask).r;

        int tilexoff = (rot.x >> 8) & 0x7;
        int tileyoff = (rot.y >> 8) & 0x7;

        int coloroffset = (int(tile & 0x03FF) << 6) + (tileyoff << 3) + tilexoff;

        uint color = texelFetch(BGVRAM8, (TilesetAddr + coloroffset) & BGVRAMMask).r;

        FinalColor = color == 0 ? 0 : (color | MetaMask);
#endif
#ifdef ExtendedMixed
        int tileoffset = ((((rot.y & XMask) >> 11) << (YShift & 0x1F)) + ((rot.x & XMask) >> 11)) << 1;

        uint tile = texelFetch(BGVRAM16, ((TilemapAddr + tileoffset) & BGVRAMMask) >> 1).r;

        int tilexoff = (rot.x >> 8) & 0x7;
        int tileyoff = (rot.y >> 8) & 0x7;

        if ((tile & 0x0400U) != 0) tilexoff = 7-tilexoff;
        if ((tile & 0x0800U) != 0) tileyoff = 7-tileyoff;

        int coloroffset = (int(tile & 0x03FF) << 6) + (tileyoff << 3) + tilexoff;

        uint color = texelFetch(BGVRAM8, (TilesetAddr + coloroffset) & BGVRAMMask).r;

        FinalColor = color == 0 ? 0 : (color | MetaMask) + (((tile >> 12) << 8) & ExtPalMask);
#endif
    }
#else
    if ((rot.x & OfxMask) == 0 && (rot.y & OfyMask) == 0)
    {
#ifdef ExtendedBitmap8bpp
        int addrOffset = (((rot.y & YMask) >> 8) << YShift) + ((rot.x & XMask) >> 8);

        uint color = texelFetch(BGVRAM8, (TilemapAddr + addrOffset) & BGVRAMMask).r;
        FinalColor = color == 0 ? 0 : color | MetaMask;
#endif
#ifdef ExtendedBitmapDirectColor
        int addrOffset = ((((rot.y & YMask) >> 8) << YShift) + ((rot.x & XMask) >> 8)) << 1;

        uint color = texelFetch(BGVRAM16, ((TilemapAddr + addrOffset) & BGVRAMMask) >> 1).r;

        if ((color & 0x8000) == 0)
        {
            FinalColor = 0;
        }
        else
        {
            FinalColor = MetaMask
                | ((color & 0x1FU) << 1)
                | ((color & 0x3E0U) << 4)
                | ((color & 0x7C00U) << 7);
        }
#endif
    }
#endif
    else
    {
        FinalColor = 0;
    }
}