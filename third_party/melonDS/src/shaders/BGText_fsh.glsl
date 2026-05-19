#version 460

#cmakedefine Text8bpp
#cmakedefine Text4bpp
#cmakedefine Mosaic

layout (binding = 0) uniform usamplerBuffer BGVRAM8;
layout (binding = 1) uniform usamplerBuffer BGVRAM16;
layout (binding = 2) uniform usampler2D MosaicTable;

layout (location = 0) out uint FinalColor;

layout (std140, binding = 0) uniform BGUniform
{
    uint TilesetAddr, WideXMask, BGVRAMMask, BasePalette;
    uint MetaMask, ExtPalMask, pad0, pad1;
    uint pad2, pad3, pad4, MosaicLevel;
    uvec4 PerLineData[192]; // xoff, yoff, tilemapaddr
};

void main()
{
    uvec2 position = uvec2(gl_FragCoord.xy);
#ifdef Mosaic
    position.x = int(texelFetch(MosaicTable, ivec2(position.x, int(MosaicLevel)), 0).x);
#endif

    uvec4 perLineData = PerLineData[position.y];

    uint xoff = perLineData.x + position.x;
    uint yoff = perLineData.y;
    uint tilemapaddr = perLineData.z;

    uint tile = texelFetch(BGVRAM16, int((tilemapaddr + ((xoff & 0xF8U) >> 2) + ((xoff & WideXMask) << 3)) & BGVRAMMask) >> 1).x;

#ifdef Text8bpp
    uint pixelsaddr = TilesetAddr + ((tile & 0x03FFU) << 6) + (((tile & 0x0800U) != 0 ? (7-yoff) : yoff) << 3);

    pixelsaddr += (tile & 0x0400) != 0 ? (7-(xoff&0x7U)) : (xoff&0x7U);

    uint pixel = texelFetch(BGVRAM8, int(pixelsaddr & BGVRAMMask)).r;

    FinalColor = pixel == 0 ? 0 : ((pixel | MetaMask) + (((tile >> 12) << 8) & ExtPalMask));
#endif
#ifdef Text4bpp
    uint pixelsaddr = TilesetAddr + ((tile & 0x03FFU) << 5) + (((tile & 0x0800U) != 0 ? (7-yoff) : yoff) << 2);

    uint innerx = (tile & 0x0400U) != 0 ? (7-(xoff&0x7U)) : (xoff&0x7U);
    pixelsaddr += innerx >> 1;

    uint pixel = texelFetch(BGVRAM8, int(pixelsaddr & BGVRAMMask)).r;

    if ((innerx & 1) != 0)
        pixel >>= 4;
    else
        pixel &= 0xF;

    FinalColor = pixel == 0 ? 0 : (pixel | ((tile >> 12) << 4) | MetaMask);
#endif
}
