#version 460

layout (binding = 0) uniform usamplerBuffer OBJVRAM8;
layout (binding = 1) uniform usamplerBuffer OBJVRAM16;

#cmakedefine OBJ4bpp
#cmakedefine OBJ8bpp
#cmakedefine OBJBitmap
#cmakedefine OBJWindow

layout (location = 0) in vec2 InInSpritePosition;
layout (location = 1) flat in uvec3 InAddrInfo;
layout (location = 2) flat in uvec2 InSize;

layout (location = 0) out uint FinalColor;

layout (std140, binding = 0) uniform OBJUniform
{
    int OBJVRAMMask;
    vec4 AffineTransforms[32];
};

void main()
{
    ivec2 inSpritePosition = ivec2(InInSpritePosition);

    if (uint(inSpritePosition.x) >= InSize.x
        || uint(inSpritePosition.y) >= InSize.y)
        discard;

    int basepixeladdr = int(InAddrInfo.x);
    int yshift = int(InAddrInfo.y);
    uint meta = InAddrInfo.z;

    ivec2 position = inSpritePosition >> 11;
    ivec2 tile = (inSpritePosition >> 8) & ivec2(7);

#ifdef OBJ4bpp
    int pixeladdr = (tile.y << 2) + (position.x << 5) + (position.y << yshift) + (tile.x >> 1);

    uint pixel = texelFetch(OBJVRAM8, (basepixeladdr + pixeladdr) & OBJVRAMMask).r;
    if ((tile.x & 1) == 0)
        pixel &= 0xFU;
    else
        pixel >>= 4U;

#ifdef OBJWindow
    FinalColor = uint(pixel != 0U);
#else
    if (pixel == 0)
        discard;
    FinalColor = meta | pixel;
#endif
#endif
#ifdef OBJ8bpp
    int pixeladdr = (tile.y << 3) + (position.x << 6) + (position.y << yshift) + tile.x;

    uint pixel = texelFetch(OBJVRAM8, (basepixeladdr + pixeladdr) & OBJVRAMMask).r;

#ifdef OBJWindow
    FinalColor = uint(pixel != 0U);
#else
    if (pixel == 0)
        discard;
    FinalColor = meta | pixel;
#endif
#endif
#ifdef OBJBitmap
    int pixeladdr = ((inSpritePosition.y >> 8) << yshift) + ((inSpritePosition.x >> 8) << 1);

    uint pixel = texelFetch(OBJVRAM16, ((basepixeladdr + pixeladdr) & OBJVRAMMask) >> 1).r;

    if ((pixel & 0x8000U) == 0)
        discard;

    FinalColor = meta
        | ((pixel & 0x1FU) << 1)
        | ((pixel & 0x3E0U) << 4)
        | ((pixel & 0x7C00U) << 7);
#endif
}