#version 460

#cmakedefine ShowBitmap
#cmakedefine ComposeBGOBJ

layout (binding = 0) uniform usampler2D BGLayer0;
layout (binding = 1) uniform usampler2D BGLayer1;
layout (binding = 2) uniform usampler2D BGLayer2;
layout (binding = 3) uniform usampler2D BGLayer3;

layout (binding = 4) uniform usampler2D SpriteLayer;

layout (binding = 5) uniform usamplerBuffer Palette;

layout (binding = 6) uniform usampler2D DirectBitmap;

layout (binding = 7) uniform usampler2D OBJWindow;

layout (location = 0) out vec4 FinalColor;
layout (location = 1) out uint DisplayCapture;

layout (std140, binding = 0) uniform ComposeUniform
{
    uint BGSpriteMask0, BGSpriteMask1, BGSpriteMask2, BGSpriteMask3, BGSpriteMask4;
    uint MasterBrightnessMode, MasterBrightnessFactor;
    uint BlendCnt, StandardColorEffect;
    uint EVA, EVB, EVY;

    uint BGNumMask0, BGNumMask1, BGNumMask2, BGNumMask3;
    uvec4 Window[192];
};

void ColorBrightnessUp(inout uint rb, inout uint g, uint factor)
{
    rb += ((((0x3F003FU - rb) * factor) >> 4));
    g += ((((0x3FU - g) * factor) >> 4));
}

void ColorBrightnessDown(inout uint rb, inout uint g, uint factor)
{
    rb -= (((rb * factor) >> 4));
    g -= (((g * factor) >> 4));
}

void ColorBlend4(inout uint rb0, inout uint g0, uint rb1, uint g1, uint eva, uint evb)
{
    rb0 = ((rb0 * eva) + (rb1 * evb)) >> 4;
    g0 = ((g0 * eva) + (g1 * evb)) >> 4;

    if ((rb0 & 0x7FU) > 0x3FU) rb0 |= 0x3FU;
    if ((rb0 & 0x7F0000U) > 0x3F0000U) rb0 |= 0x3F0000U;
    if (g0 > 0x3FU) g0 = 0x3FU;
}

void ColorBlend5(inout uint rb0, inout uint g0, uint rb1, uint g1, uint eva)
{
    eva = (eva & 0x1FU) + 1U;
    uint evb = 32 - eva;

    if (eva == 32) return;

    rb0 = ((rb0 * eva) + (rb1 * evb)) >> 5;
    g0 = ((g0 * eva) + (g1 * evb)) >> 5;

    if (eva <= 16)
    {
        rb0 += 0x010001;
        g0 += 0x0001;
    }

    if ((rb0 & 0x7FU) > 0x3FU) rb0 |= 0x3FU;
    if ((rb0 & 0x7F0000U) > 0x3F0000U) rb0 |= 0x3F0000U;
    if (g0 > 0x3FU) g0 = 0x3FU;
}

void PalettisePixel(uint indexed, out uint rb, out uint g)
{
    if ((indexed & 0x800000U) != 0)
    {
        uint palettisedColor = texelFetch(Palette, int(indexed & 0xFFFFU)).r;

        rb = ((palettisedColor & 0x1FU) << 1) | ((palettisedColor & 0x7C00U) << 7);
        g = (palettisedColor & 0x3E0U) >> 4;
    }
    else
    {
        // already bitmap
        rb = indexed & 0x3F003FU;
        g = (indexed & 0x3F00U) >> 8;
    }
}

void main()
{
    ivec2 position = ivec2(gl_FragCoord.xy);
    uint finalColorRB, finalColorG;
#ifdef ComposeBGOBJ
    // find out the two top most layers

    uvec4 window = Window[position.y];

    uint objWindow = texelFetch(OBJWindow, position, 0).r;

    uint winAttr = window.x;
    if ((window.y & 0x80000000U) != 0U && objWindow != 0)
        winAttr = window.y >> 16;
    if (uint(position.x) >= (window.z & 0xFFFFU) && uint(position.x) < (window.z >> 16))
        winAttr = window.x >> 16;
    if (uint(position.x) >= (window.w & 0xFFFFU) && uint(position.x) < (window.w >> 16))
        winAttr = window.y;

    uint bgLayer0 = texelFetch(BGLayer0, position, 0).r;
    uint bgLayer1 = texelFetch(BGLayer1, position, 0).r;
    uint bgLayer2 = texelFetch(BGLayer2, position, 0).r;
    uint bgLayer3 = texelFetch(BGLayer3, position, 0).r;

    uint spriteLayer = texelFetch(SpriteLayer, position, 0).r;

    if ((winAttr & BGNumMask0) == 0U)
        bgLayer0 = 0U;
    if ((winAttr & BGNumMask1) == 0U)
        bgLayer1 = 0U;
    if ((winAttr & BGNumMask2) == 0U)
        bgLayer2 = 0U;
    if ((winAttr & BGNumMask3) == 0U)
        bgLayer3 = 0U;
    if ((winAttr & (1U<<4)) == 0U)
        spriteLayer = 0U;

    uint spritePrioMask = 1U << ((spriteLayer >> 29) & 0x3U);
    if ((spriteLayer & 0xFF000000U) == 0)
        spritePrioMask = 0;
    spriteLayer &= ~0x60000000U;

    uint layer0 = 0x20800000U, layer1 = 0x20800000U;

    // welcome to the biggest conditional select circus in the world
    // this could probably be done smarter with findLSB, but we're targetting a lower GLSL version

    // find the topmost bg layer
    if ((spritePrioMask & BGSpriteMask0) != 0)
    {
        layer1 = layer0;
        layer0 = spriteLayer;
    }
    if ((bgLayer0 & 0xFF000000U) != 0)
    {
        layer1 = layer0;
        layer0 = bgLayer0;
    }
    if ((spritePrioMask & BGSpriteMask1) != 0)
    {
        layer1 = layer0;
        layer0 = spriteLayer;
    }
    if ((bgLayer1 & 0xFF000000U) != 0)
    {
        layer1 = layer0;
        layer0 = bgLayer1;
    }
    if ((spritePrioMask & BGSpriteMask2) != 0)
    {
        layer1 = layer0;
        layer0 = spriteLayer;
    }
    if ((bgLayer2 & 0xFF000000U) != 0)
    {
        layer1 = layer0;
        layer0 = bgLayer2;
    }
    if ((spritePrioMask & BGSpriteMask3) != 0)
    {
        layer1 = layer0;
        layer0 = spriteLayer;
    }
    if ((bgLayer3 & 0xFF000000U) != 0)
    {
        layer1 = layer0;
        layer0 = bgLayer3;
    }
    if ((spritePrioMask & BGSpriteMask4) != 0)
    {
        layer1 = layer0;
        layer0 = spriteLayer;
    }

    uint layer0RB, layer0G;
    PalettisePixel(layer0, layer0RB, layer0G);
    uint layer1RB, layer1G;
    PalettisePixel(layer1, layer1RB, layer1G);

    uint flag0 = layer0 >> 24;
    uint flag1 = layer1 >> 24;

    uint target1;
    if      ((flag1 & 0x80U) != 0) target1 = 0x1000U;
    else if ((flag1 & 0x40U) != 0) target1 = 0x0100U;
    else                   target1 = flag1 << 8;

    if ((flag0 & 0x80U) != 0 && (BlendCnt & target1) != 0)
    {
        uint eva, evb;
        // sprite blending
        if ((layer0 & 0x800000U) == 0)
        {
            // bitmap sprite
            eva = flag0 & 0x1FU;
            evb = 16U - eva;
        }
        else
        {
            eva = EVA;
            evb = EVB;
        }

        ColorBlend4(layer0RB, layer0G, layer1RB, layer1G, eva, evb);
    }
    else if ((flag0 & 0x40U) != 0 && (BlendCnt & target1) != 0)
    {
        // 3D blending
        ColorBlend5(layer0RB, layer0G, layer1RB, layer1G, flag0);
    }
    else
    {
        if      ((flag0 & 0x80U) != 0) flag0 = 0x10U;
        else if ((flag0 & 0x40U) != 0) flag0 = 0x01U;

        if ((BlendCnt & flag0) != 0 && (winAttr & 0x20U) != 0U)
        {
            if (StandardColorEffect == 1)
            {
                if ((BlendCnt & target1) != 0)
                {
                    ColorBlend4(layer0RB, layer0G, layer1RB, layer1G, EVA, EVB);
                }
            }
            else if (StandardColorEffect == 2)
            {
                ColorBrightnessUp(layer0RB, layer0G, EVY);
            }
            else if (StandardColorEffect == 3)
            {
                ColorBrightnessDown(layer0RB, layer0G, EVY);
            }
        }
    }

    layer0RB &= 0x3F003FU;
    layer0G &= 0x3FU;

    uint displayCaptureHigh = 0xFF000000U & layer0;
    DisplayCapture = layer0RB | (layer0G << 8) | displayCaptureHigh;

    finalColorRB = layer0RB;
    finalColorG = layer0G;
#endif
#ifdef ShowBitmap
    uint color = texelFetch(DirectBitmap, position, 0).r;

    finalColorRB = ((color & 0x1FU) << 1) | ((color & 0x7C00) << 7);
    finalColorG = (color & 0x3E0U) >> 4;
#endif

    if (MasterBrightnessMode == 1)
    {
        ColorBrightnessUp(finalColorRB, finalColorG, MasterBrightnessFactor);
    }
    else if (MasterBrightnessMode == 2)
    {
        ColorBrightnessDown(finalColorRB, finalColorG, MasterBrightnessFactor);
    }

    FinalColor.r = float(finalColorRB & 0x3FU) / 63.0;
    FinalColor.g = float(finalColorG) / 63.0;
    FinalColor.b = float((finalColorRB >> 16) & 0x3FU) / 63.0;
    FinalColor.a = 1.0;
}