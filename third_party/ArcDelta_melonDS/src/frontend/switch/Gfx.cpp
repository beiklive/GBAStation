#define _USE_MATH_DEFINES
#include <math.h>

#include "Gfx.h"

#include <deko3d.hpp>

#include <assert.h>
#include <string.h>

#include <array>
#include <unordered_map>
#include <algorithm>
#include <cmath>

#include <stdio.h>

#include <stdarg.h>

#include "stb_truetype/stb_truetype.h"

#include "mm_vec/mm_vec.h"

#include "CmdMemRing.h"

extern "C" void GBAStationNDSStubLogLine(const char* line) __attribute__((weak));

namespace Gfx
{

namespace
{

void GfxLog(const char* format, ...)
{
    char line[768] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (GBAStationNDSStubLogLine)
        GBAStationNDSStubLogLine(line);
    else
        printf("%s\n", line);
}

}

dk::Device Device;
dk::Queue PresentQueue, EmuQueue;
dk::CmdBuf PresentCmdBuf, EmuCmdBuf;
dk::Swapchain Swapchain;

struct Vertex
{
    float Position[2];
    float UV[2];
    u8 Color[4];
    float CoolTransparency[2];
};

struct Transformation
{
    float Projection[4*4];
    float InvHeight;
    float Pad[3];
};

struct NdsShaderUniform
{
    float Param0[4];
    float Param1[4];
    float Runtime[4];
};

int SwapchainSlot = 0;

std::optional<GpuMemHeap> TextureHeap;
std::optional<GpuMemHeap> ShaderCodeHeap;
std::optional<GpuMemHeap> DataHeap;

std::optional<CmdMemRing<2>> CmdMem;

dk::Image Framebuffers[2];

const u32 MaxVertices = 1024*64;
const u32 MaxIndices = MaxVertices * 6;

Vertex VertexDataClient[MaxVertices];
u16 IndexDataClient[MaxIndices];

GpuMemHeap::Allocation ImageDescriptors[2];
GpuMemHeap::Allocation SamplerDescriptor;

std::vector<u64> ImageDescriptorsDirty;
u32 ImageDescriptorsAllocated;

u32 CurClientVertex = 0;
u32 CurClientIndex = 0;

u32 CurSampler = 0;
ShaderMode CurShaderMode = shaderMode_Default;
std::array<float, 8> CurNdsShaderParams {2.4f, 0.05f, 0.65f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float CurNdsSourceScale = 1.0f;

enum
{
    drawCallDirty_Texture = 1 << 0,
    drawCallDirty_Sampler = 1 << 1,
    drawCallDirty_Scissor = 1 << 2,
    drawCallDirty_Shader = 1 << 3,
    drawCallDirty_WaitFence = 1 << 4,
    drawCallDirty_SignalFence = 1 << 5,
};

enum class DrawCallKind
{
    Draw,
    WaitFence,
    SignalFence,
    NdsMultiPass,
};

struct NdsMultiPassDraw
{
    u32 SourceTexture = 0;
    u32 TempTextureA = 0;
    u32 TempTextureB = 0;
    u32 TempWidth = 0;
    u32 TempHeight = 0;
    u32 InitialIndexOffset = 0;
    u32 IntermediateIndexOffset = 0;
    u32 FinalIndexOffset = 0;
    int PassCount = 0;
    float SourceScale = 1.0f;
    std::array<NdsFilterPass, 4> Passes {};
    DkScissor Scissor {};
};

struct DrawCall
{
    DrawCallKind Kind;
    u32 Dirty;
    u32 TextureIdx, Sampler;
    ShaderMode Shader;
    u32 IndexOffset;
    u32 Count;
    DkScissor Scissor;
    dk::Fence* Fence;
    NdsMultiPassDraw MultiPass;
    float NdsSourceScale = 1.0f;
};
std::vector<DrawCall> DrawCalls;

GpuMemHeap::Allocation VertexData[2];
GpuMemHeap::Allocation IndexData[2];
GpuMemHeap::Allocation UniformBuffer;
GpuMemHeap::Allocation NdsShaderUniformBuffer;
bool NdsExtensionsInitialized = false;

GpuMemHeap::Allocation TextureStagingBuffer[2];
u32 TextureStagingBufferOffset;

dk::Shader VertexShader, FragmentShaders[shaderMode_Count];

NWindow* Window;

template <typename T>
struct Registry
{
    std::vector<T> Items;
    std::vector<u32> FreeItems;

    u32 Alloc()
    {
        u32 result;
        if (FreeItems.size() > 0)
        {
            result = FreeItems[FreeItems.size() - 1];
            FreeItems.pop_back();
        }
        else
        {
            result = Items.size();
            Items.push_back(T());
        }
        return result;
    }

    void Free(u32 index)
    {
        FreeItems.push_back(index);
    }

    T& operator[](u32 index)
    {
        assert(index < Items.size());
        return Items[index];
    }
};

struct Texture
{
    bool External;
    u32 Width, Height;
    DkImageFormat Format;
    GpuMemHeap::Allocation GpuMem;
    dk::Image Image;
    DkImageSwizzle ComponentSwizzle[4];
    int ImageDescriptorIdx = -1;
};

struct PendingTextureUpload
{
    u32 TextureIdx;
    u32 X, Y, Width, Height;
    u32 DataStrideBytes;
    u32 StagingSlot;
    u32 StagingOffsetBytes;
};

Registry<Texture> Textures;

std::vector<u32> UsedTextures;
std::vector<PendingTextureUpload> TextureUploadsPending;
bool FrameActive = false;

void UseTexture(u32 textureIdx)
{
    if (Textures[textureIdx].ImageDescriptorIdx == -1)
    {
        Textures[textureIdx].ImageDescriptorIdx = UsedTextures.size();
        UsedTextures.push_back(textureIdx);
    }
}

u32 TextureCreateWithFlags(u32 width, u32 height, DkImageFormat format, int flags)
{
    u32 idx = Textures.Alloc();

    Texture& texture = Textures[idx];
    texture.External = false;
    texture.Width = width;
    texture.Height = height;
    texture.Format = format;

    dk::ImageLayout layout;
    dk::ImageLayoutMaker{Device}
        .setFlags(flags)
        .setFormat(format)
        .setDimensions(width, height)
        .initialize(layout);
    texture.ComponentSwizzle[0] = DkImageSwizzle_Red;
    texture.ComponentSwizzle[1] = DkImageSwizzle_Green;
    texture.ComponentSwizzle[2] = DkImageSwizzle_Blue;
    texture.ComponentSwizzle[3] = DkImageSwizzle_Alpha;
    texture.ImageDescriptorIdx = -1;
    texture.GpuMem = TextureHeap->Alloc(layout.getSize(), layout.getAlignment());

    texture.Image.initialize(layout, TextureHeap->MemBlock, texture.GpuMem.Offset);

    return idx;
}

u32 TextureCreate(u32 width, u32 height, DkImageFormat format)
{
    return TextureCreateWithFlags(width, height, format, 0);
}

u32 TextureCreateRenderTarget(u32 width, u32 height, DkImageFormat format)
{
    return TextureCreateWithFlags(width, height, format, DkImageFlags_UsageRender);
}

u32 TextureCreateExternal(u32 width, u32 height, dk::Image& image)
{
    u32 idx = Textures.Alloc();
    Texture& texture = Textures[idx];
    texture.External = true;
    texture.Image = image;
    texture.Width = width;
    texture.Height = height;
    texture.ComponentSwizzle[0] = DkImageSwizzle_Red;
    texture.ComponentSwizzle[1] = DkImageSwizzle_Green;
    texture.ComponentSwizzle[2] = DkImageSwizzle_Blue;
    texture.ComponentSwizzle[3] = DkImageSwizzle_Alpha;
    texture.ImageDescriptorIdx = -1;

    return idx;
}

void TextureDelete(u32 idx)
{
    Texture& texture = Textures[idx];

    TextureUploadsPending.erase(
        std::remove_if(TextureUploadsPending.begin(),
                       TextureUploadsPending.end(),
                       [idx](const PendingTextureUpload& upload) {
                           return upload.TextureIdx == idx;
                       }),
        TextureUploadsPending.end());
    texture.ImageDescriptorIdx = -1;

    if (!texture.External)
        TextureHeap->Free(texture.GpuMem);

    Textures.Free(idx);
}

void TextureUpload(u32 index, u32 x, u32 y, u32 width, u32 height, void* data, u32 dataStride)
{
    if (!FrameActive)
        PresentQueue.waitIdle();

    const u32 uploadSize = dataStride * height;
    const u32 alignedUploadSize = (uploadSize + DK_IMAGE_LINEAR_STRIDE_ALIGNMENT - 1) &
                                  ~(DK_IMAGE_LINEAR_STRIDE_ALIGNMENT - 1);
    assert(TextureStagingBufferOffset + alignedUploadSize <= TextureStagingBuffer[0].Size);
    assert(!Textures[index].External);

    const u32 stagingSlot = static_cast<u32>(SwapchainSlot);
    const u32 stagingOffset = TextureStagingBufferOffset;
    TextureUploadsPending.push_back({index, x, y, width, height, dataStride, stagingSlot, stagingOffset});
    u8* stagingBufferCpuAddr = DataHeap->CpuAddr<u8>(TextureStagingBuffer[SwapchainSlot]) + TextureStagingBufferOffset;
    memcpy(stagingBufferCpuAddr, data, uploadSize);
    TextureStagingBufferOffset += alignedUploadSize;
}

void TextureSetSwizzle(u32 idx, DkImageSwizzle red, DkImageSwizzle green, DkImageSwizzle blue, DkImageSwizzle alpha)
{
    Texture& texture = Textures[idx];
    texture.ComponentSwizzle[0] = red;
    texture.ComponentSwizzle[1] = green;
    texture.ComponentSwizzle[2] = blue;
    texture.ComponentSwizzle[3] = alpha;
}

const int MaximumWaste = 4;

u8* Atlas::Pack(int width, int height, PackedQuad& quad)
{
    // this packing algorithm isn't the best one in the world
    // but it works well enough for fonts because they're
    // relatively homogenous in their size
    // and it's definitely better than the old one
    const int Seam = 3;

    int paddedWidth = width + Seam;
    int paddedHeight = height + Seam;

    int minDiff = INT32_MAX;
    int minIdx = -1;
    for (u32 i = 0; i < Shelves.size(); i++)
    {
        int heightDiff = Shelves[i].Height - paddedHeight;
        if (AtlasSize - Shelves[i].Used >= paddedWidth
            && heightDiff >= 0 
            && heightDiff < minDiff)
        {
            minDiff = heightDiff;
            minIdx = i;
        }
    }

    Shelf* shelf;
    if (minIdx == -1 || minDiff > MaximumWaste)
    {
        bool useNewAtlas = Shelves.size() == 0;

        if (!useNewAtlas)
        {
            shelf = &Shelves[Shelves.size() - 1];
            useNewAtlas = shelf->Y + shelf->Height + paddedHeight > AtlasSize;
        }

        if (useNewAtlas)
        {
            AtlasTexture newAtlas;
            newAtlas.Texture = TextureCreate(AtlasSize, AtlasSize, TexFmt);
            TextureSetSwizzle(newAtlas.Texture, Swizzle[0], Swizzle[1], Swizzle[2], Swizzle[3]);
            newAtlas.ClientImage = new u8[AtlasSize * AtlasSize * BytesPerPixel];
            memset(newAtlas.ClientImage, 0, AtlasSize * AtlasSize * BytesPerPixel);

            Atlases.push_back(newAtlas);
            Shelves.push_back({(int)Atlases.size() - 1, 0, paddedHeight, 0});

            shelf = &Shelves[Shelves.size() - 1];
        }
        else
        {
            Shelves.push_back({shelf->Atlas, shelf->Y+shelf->Height, paddedHeight, 0});
            shelf = &Shelves[Shelves.size() - 1];
        }
    }
    else
    {
        shelf = &Shelves[minIdx];
    }
    AtlasTexture* atlas = &Atlases[shelf->Atlas];

    quad.AtlasTexture = atlas->Texture;

    quad.PackX = shelf->Used;
    quad.PackY = shelf->Y;

    atlas->DirtyX1 = std::min(atlas->DirtyX1, quad.PackX);
    atlas->DirtyY1 = std::min(atlas->DirtyY1, quad.PackY);
    // add some extra padding to avoid seems
    atlas->DirtyX2 = std::clamp(atlas->DirtyX2, std::min(quad.PackX + paddedWidth, AtlasSize), AtlasSize);
    atlas->DirtyY2 = std::clamp(atlas->DirtyY2, std::min(quad.PackY + paddedHeight, AtlasSize), AtlasSize);

    shelf->Used += width + Seam;

    return atlas->ClientImage + quad.PackX * BytesPerPixel + quad.PackY * PackStride();
}

void Atlas::IssueUpload()
{
    for (u32 i = 0; i < Atlases.size(); i++)
    {
        AtlasTexture& atlas = Atlases[i];

        int dirtyWidth = atlas.DirtyX2 - atlas.DirtyX1;
        int dirtyHeight = atlas.DirtyY2 - atlas.DirtyY1;

        if (dirtyWidth > 0 && dirtyHeight > 0)
        {
            TextureUpload(atlas.Texture,
                atlas.DirtyX1, atlas.DirtyY1,
                dirtyWidth, dirtyHeight,
                &atlas.ClientImage[atlas.DirtyX1 * BytesPerPixel + atlas.DirtyY1 * PackStride()],
                PackStride());

            atlas.DirtyX1 = atlas.DirtyY1 = AtlasSize;
            atlas.DirtyX2 = atlas.DirtyY2 = 0;
        }
    }
}

void Atlas::Destroy()
{
    for (int i = 0; i < Atlases.size(); i++)
    {
        delete[] Atlases[i].ClientImage;
        TextureDelete(Atlases[i].Texture);
    }
}

struct PackedGlyph
{
    PackedQuad Quad;
    int BoxX1, BoxY1, BoxX2, BoxY2;
    float AdvanceWidth, LeftSideBearing;
};

struct GlyphKey
{
    u32 Codepoint;
    float Scale;

    bool operator==(const Gfx::GlyphKey& other) const
    {
        return Codepoint == other.Codepoint && (int)(Scale * (1<<12)) == (int)(other.Scale * (1<<12));
    }
};

struct GlyphKeyHash
{
    std::size_t operator()(const Gfx::GlyphKey& key) const
    {
        // float is converted to fixpoint so that small differences won't end in a glyph rendered twice
        return (std::size_t)key.Codepoint ^ ((std::size_t)(key.Scale * (1<<12)) << 4);
    }
};

struct Font
{
    stbtt_fontinfo Info;
    int Ascent, Descent, LineGap;

    std::unordered_map<GlyphKey, PackedGlyph, GlyphKeyHash> PackedGlyphs;
};

Registry<Font> Fonts;
Atlas FontAtlas{DkImageFormat_R8_Unorm, 1, {DkImageSwizzle_One, DkImageSwizzle_One, DkImageSwizzle_One, DkImageSwizzle_Red}};

u32 FontLoad(u8* data)
{
    u32 idx = Fonts.Alloc();
    Font& font = Fonts[idx];

    stbtt_InitFont(&font.Info, data, 0);
    stbtt_GetFontVMetrics(&font.Info, &font.Ascent, &font.Descent, &font.LineGap);

    return idx;
}

void FontDelete(u32 idx)
{
    Font& font = Fonts[idx];
    font.PackedGlyphs.clear();

    Fonts.Free(idx);
}

float FontGetScale(u32 idx, float pixelHeight)
{
    return stbtt_ScaleForPixelHeight(&Fonts[idx].Info, pixelHeight);
}

float FontGetAscent(u32 idx, float scale)
{
    return (float)Fonts[idx].Ascent * scale;
}

float FontGetDescent(u32 idx, float scale)
{
    return (float)Fonts[idx].Descent * scale;
}

float FontGetLineGap(u32 idx, float scale)
{
    return (float)Fonts[idx].LineGap * scale;
}

PackedGlyph& FontGetGlyph(u32 idx, u32 codepoint, float scale)
{
    Font& font = Fonts[idx];

    auto existingRender = font.PackedGlyphs.find({codepoint, scale});
    if (existingRender == font.PackedGlyphs.end())
    {
        PackedGlyph result;

        int glyphIndex = stbtt_FindGlyphIndex(&font.Info, codepoint);

        stbtt_GetGlyphBitmapBox(&font.Info,
            glyphIndex,
            scale, scale,
            &result.BoxX1, &result.BoxY1,
            &result.BoxX2, &result.BoxY2);
        int advanceWidth, leftSideBearing;
        stbtt_GetGlyphHMetrics(&font.Info, glyphIndex, &advanceWidth, &leftSideBearing);
        result.AdvanceWidth = advanceWidth * scale;
        result.LeftSideBearing = leftSideBearing * scale;

        // terrible packing algorithm ahead
        int glyphBoxW = result.BoxX2 - result.BoxX1;
        int glyphBoxH = result.BoxY2 - result.BoxY1;

        u8* outPtr = FontAtlas.Pack(glyphBoxW, glyphBoxH, result.Quad);

        stbtt_MakeGlyphBitmap(&font.Info,
            outPtr, 
            glyphBoxW, glyphBoxH,
            FontAtlas.PackStride(),
            scale, scale,
            glyphIndex);

        font.PackedGlyphs[{codepoint, scale}] = result;
        return font.PackedGlyphs[{codepoint, scale}];
    }
    else
    {
        return existingRender->second;
    }
}

u32 SystemFontStandard;
u32 SystemFontNintendoExt;
u32 SystemFontChinese;

u8* SystemFontStandardData;
u8* SystemFontNintendoExtData;
u8* SystemFontChineseData;

u32 WhiteTexture;

u8* CopySharedFont(PlSharedFontType type)
{
    PlFontData font {};
    if (R_FAILED(plGetSharedFontByType(&font, type)) || !font.address || font.size == 0)
        return nullptr;

    u8* data = new u8[font.size];
    memcpy(data, font.address, font.size);
    return data;
}

struct DkshHeader
{
    uint32_t magic; // DKSH_MAGIC
    uint32_t header_sz; // sizeof(DkshHeader)
    uint32_t control_sz;
    uint32_t code_sz;
    uint32_t programs_off;
    uint32_t num_programs;
};

bool LoadShader(const char* path, dk::Shader& out)
{
    GfxLog("GBAStationNDSStub: Gfx LoadShader begin path=%s", path ? path : "(null)");
    FILE* f = fopen(path, "rb");
    if (f)
    {
        DkshHeader header;
        size_t read = fread(&header, sizeof(DkshHeader), 1, f);
        if (!read)
        {
            GfxLog("GBAStationNDSStub: Gfx LoadShader header read failed path=%s", path);
            printf("couldn't read shader header %s\n", path);
            fclose(f);
            return false;
        }

        GfxLog("GBAStationNDSStub: Gfx LoadShader header path=%s control=%u code=%u programs=%u",
               path, header.control_sz, header.code_sz, header.num_programs);

        rewind(f);
        u8* ctrlmem = new u8[header.control_sz];
        read = fread(ctrlmem, header.control_sz, 1, f);
        if (!read)
        {
            GfxLog("GBAStationNDSStub: Gfx LoadShader control read failed path=%s", path);
            printf("couldn't read shader control %s\n", path);
            delete[] ctrlmem;
            fclose(f);
            return false;
        }

        GfxLog("GBAStationNDSStub: Gfx LoadShader code alloc begin path=%s bytes=%u", path, header.code_sz);
        GpuMemHeap::Allocation data = ShaderCodeHeap->Alloc(header.code_sz, DK_SHADER_CODE_ALIGNMENT);
        GfxLog("GBAStationNDSStub: Gfx LoadShader code alloc ok path=%s offset=%u size=%u",
               path, data.Offset, data.Size);
        read = fread(ShaderCodeHeap->CpuAddr<void>(data), header.code_sz, 1, f);
        if (!read)
        {
            GfxLog("GBAStationNDSStub: Gfx LoadShader code read failed path=%s", path);
            printf("couldn't read shader code %s\n", path);
            delete[] ctrlmem;
            fclose(f);
            return false;
        }

        dk::ShaderMaker{ShaderCodeHeap->MemBlock, data.Offset}
            .setControl(ctrlmem)
            .setProgramId(0)
            .initialize(out);

        GfxLog("GBAStationNDSStub: Gfx LoadShader ok path=%s", path);

        delete[] ctrlmem;
        fclose(f);
        return true;
    }
    else
    {
        GfxLog("GBAStationNDSStub: Gfx LoadShader open failed path=%s", path ? path : "(null)");
        printf("couldn't open shader file %s\n", path);
        return false;
    }
}

std::vector<DkScissor> ScissorStack;

struct DrawTransform
{
    float M00 = 1.0f;
    float M01 = 0.0f;
    float M02 = 0.0f;
    float M10 = 0.0f;
    float M11 = 1.0f;
    float M12 = 0.0f;
};

std::vector<DrawTransform> DrawTransformStack;

DrawTransform CurrentDrawTransform()
{
    if (DrawTransformStack.empty())
        return {};
    return DrawTransformStack.back();
}

Vector2f TransformPoint(Vector2f point)
{
    const DrawTransform transform = CurrentDrawTransform();
    return {
        transform.M00 * point.X + transform.M01 * point.Y + transform.M02,
        transform.M10 * point.X + transform.M11 * point.Y + transform.M12,
    };
}

bool HasDrawTransform()
{
    const DrawTransform transform = CurrentDrawTransform();
    return transform.M00 != 1.0f || transform.M01 != 0.0f || transform.M02 != 0.0f ||
           transform.M10 != 0.0f || transform.M11 != 1.0f || transform.M12 != 0.0f;
}

void PushDrawTransform(float m00, float m01, float m02, float m10, float m11, float m12)
{
    const DrawTransform parent = CurrentDrawTransform();
    DrawTransform next {};
    next.M00 = m00 * parent.M00 + m01 * parent.M10;
    next.M01 = m00 * parent.M01 + m01 * parent.M11;
    next.M02 = m00 * parent.M02 + m01 * parent.M12 + m02;
    next.M10 = m10 * parent.M00 + m11 * parent.M10;
    next.M11 = m10 * parent.M01 + m11 * parent.M11;
    next.M12 = m10 * parent.M02 + m11 * parent.M12 + m12;
    DrawTransformStack.push_back(next);
}

void PopDrawTransform()
{
    if (!DrawTransformStack.empty())
        DrawTransformStack.pop_back();
}

void PushScissor(u32 x, u32 y, u32 w, u32 h)
{
    if (HasDrawTransform())
    {
        const Vector2f p0 = TransformPoint({static_cast<float>(x), static_cast<float>(y)});
        const Vector2f p1 = TransformPoint({static_cast<float>(x + w), static_cast<float>(y)});
        const Vector2f p2 = TransformPoint({static_cast<float>(x), static_cast<float>(y + h)});
        const Vector2f p3 = TransformPoint({static_cast<float>(x + w), static_cast<float>(y + h)});
        const float minX = std::max(0.0f, std::floor(std::min({p0.X, p1.X, p2.X, p3.X})));
        const float minY = std::max(0.0f, std::floor(std::min({p0.Y, p1.Y, p2.Y, p3.Y})));
        const float maxX = std::min(1280.0f, std::ceil(std::max({p0.X, p1.X, p2.X, p3.X})));
        const float maxY = std::min(720.0f, std::ceil(std::max({p0.Y, p1.Y, p2.Y, p3.Y})));
        x = static_cast<u32>(minX);
        y = static_cast<u32>(minY);
        w = static_cast<u32>(std::max(1.0f, maxX - minX));
        h = static_cast<u32>(std::max(1.0f, maxY - minY));
    }
    ScissorStack.push_back({x, y, w, h});
}

void PopScissor()
{
    ScissorStack.pop_back();
}

void DebugOutput(void* userData, const char* context, DkResult result, const char* message)
{
    printf("deko debug %d %s\n", result, message);
}

void InitNdsExtensions()
{
    if (NdsExtensionsInitialized)
        return;

    auto loadFragmentOrFallback = [](const char* path, ShaderMode mode, ShaderMode fallback) {
        if (!LoadShader(path, FragmentShaders[mode]))
        {
            printf("falling back shader %s to mode %d\n", path, fallback);
            FragmentShaders[mode] = FragmentShaders[fallback];
        }
    };
    loadFragmentOrFallback("romfs:/shaders/NdsDot_fsh.dksh", shaderMode_NdsDot, shaderMode_Default);
    loadFragmentOrFallback("romfs:/shaders/NdsDotClear_fsh.dksh", shaderMode_NdsDotClear, shaderMode_NdsDot);
    loadFragmentOrFallback("romfs:/shaders/NdsXbrzFreescale_fsh.dksh", shaderMode_NdsXbrzFreescale, shaderMode_NdsDot);
    loadFragmentOrFallback("romfs:/shaders/NdsLcdGridNdsColor_fsh.dksh", shaderMode_NdsLcdGridNdsColor, shaderMode_NdsDot);
    loadFragmentOrFallback("romfs:/shaders/NdsDrasticSimple_fsh.dksh", shaderMode_NdsDrasticSimple, shaderMode_NdsDot);

    NdsShaderUniformBuffer = DataHeap->Alloc(sizeof(NdsShaderUniform), DK_UNIFORM_BUF_ALIGNMENT);
    NdsExtensionsInitialized = true;
}

void Init()
{
    GfxLog("GBAStationNDSStub: Gfx Init enter textureHeapMB=120 shaderHeapMB=48 dataHeapMB=896");
    Window = nwindowGetDefault();
    GfxLog("GBAStationNDSStub: Gfx nwindowGetDefault ok window=%p", Window);
    nwindowSetDimensions(Window, 1920, 1080);

    GfxLog("GBAStationNDSStub: Gfx device create begin");
    Device = dk::DeviceMaker{}.setCbDebug(DebugOutput).create();
    GfxLog("GBAStationNDSStub: Gfx device create ok");
    GfxLog("GBAStationNDSStub: Gfx present queue create begin");
    PresentQueue = dk::QueueMaker{Device}
        .setFlags(DkQueueFlags_Graphics|DkQueueFlags_DisableZcull)
        .setCommandMemorySize(DK_QUEUE_MIN_CMDMEM_SIZE*4)
        .setFlushThreshold(DK_QUEUE_MIN_CMDMEM_SIZE)
        .create();
    GfxLog("GBAStationNDSStub: Gfx present queue create ok");
    GfxLog("GBAStationNDSStub: Gfx emu queue create begin");
    EmuQueue = dk::QueueMaker{Device}
        .setFlags(DkQueueFlags_HighPrio|DkQueueFlags_Graphics|DkQueueFlags_Compute|DkQueueFlags_DisableZcull)
        .setCommandMemorySize(DK_QUEUE_MIN_CMDMEM_SIZE*4)
        .setFlushThreshold(DK_QUEUE_MIN_CMDMEM_SIZE)
        .create();
    GfxLog("GBAStationNDSStub: Gfx emu queue create ok");

    GfxLog("GBAStationNDSStub: Gfx TextureHeap create begin bytes=%u", 1024U*1024U*120U);
    TextureHeap.emplace(Device, 1024*1024*120, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 1024*8);
    GfxLog("GBAStationNDSStub: Gfx TextureHeap create ok");
    GfxLog("GBAStationNDSStub: Gfx ShaderCodeHeap create begin bytes=%u", 1024U*1024U*48U);
    ShaderCodeHeap.emplace(Device, 1024*1024*48,
        DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code, 512);
    GfxLog("GBAStationNDSStub: Gfx ShaderCodeHeap create ok");
    GfxLog("GBAStationNDSStub: Gfx DataHeap create begin bytes=%u", 1024U*1024U*896U);
    DataHeap.emplace(Device, 1024*1024*896, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 256);
    GfxLog("GBAStationNDSStub: Gfx DataHeap create ok");

    GfxLog("GBAStationNDSStub: Gfx command buffers create begin");
    PresentCmdBuf = dk::CmdBufMaker{Device}.create();
    EmuCmdBuf = dk::CmdBufMaker{Device}.create();
    CmdMem.emplace(*DataHeap, 0x10000*2);
    GfxLog("GBAStationNDSStub: Gfx command buffers create ok");

    dk::ImageLayout fbLayout;
    dk::ImageLayoutMaker{Device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(1920, 1080)
        .initialize(fbLayout);
    std::array<DkImage const*, 2> fbArray;
    for (int i = 0; i < 2; i++)
    {
        GfxLog("GBAStationNDSStub: Gfx swapchain framebuffer alloc begin index=%d bytes=%llu",
               i, static_cast<unsigned long long>(fbLayout.getSize()));
        GpuMemHeap::Allocation block = TextureHeap->Alloc(fbLayout.getSize(), fbLayout.getAlignment());
        Framebuffers[i].initialize(fbLayout, TextureHeap->MemBlock, block.Offset);
        fbArray[i] = &Framebuffers[i];
        GfxLog("GBAStationNDSStub: Gfx swapchain framebuffer alloc ok index=%d offset=%u size=%u",
               i, block.Offset, block.Size);
    }

    GfxLog("GBAStationNDSStub: Gfx swapchain create begin");
    Swapchain = dk::SwapchainMaker{Device, Window, fbArray}.create();
    GfxLog("GBAStationNDSStub: Gfx swapchain create ok");

    const bool defaultVertexLoaded = LoadShader("romfs:/shaders/Default_vsh.dksh", VertexShader);
    const bool defaultFragmentLoaded = LoadShader("romfs:/shaders/Default_fsh.dksh", FragmentShaders[shaderMode_Default]);
    assert(defaultVertexLoaded);
    assert(defaultFragmentLoaded);
    GfxLog("GBAStationNDSStub: Gfx default shaders ok");

    for (int i = 0; i < 2; i++)
    {
        GfxLog("GBAStationNDSStub: Gfx frame resources alloc begin slot=%d", i);
        VertexData[i] = DataHeap->Alloc(MaxVertices * sizeof(Vertex), alignof(Vertex));
        IndexData[i] = DataHeap->Alloc(MaxIndices * sizeof(u16), alignof(u16));

        TextureStagingBuffer[i] = DataHeap->Alloc(1024*1024*8, DK_MEMBLOCK_ALIGNMENT);

        ImageDescriptors[i] = DataHeap->Alloc(sizeof(dk::ImageDescriptor) * 1024, DK_IMAGE_DESCRIPTOR_ALIGNMENT);
        GfxLog("GBAStationNDSStub: Gfx frame resources alloc ok slot=%d", i);
    }
    UniformBuffer = DataHeap->Alloc(sizeof(Transformation), DK_UNIFORM_BUF_ALIGNMENT);

    {
        SamplerDescriptor = DataHeap->Alloc(sizeof(dk::SamplerDescriptor) * 4, DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
        dk::SamplerDescriptor* descriptors = DataHeap->CpuAddr<dk::SamplerDescriptor>(SamplerDescriptor);

        for (int i = 0; i < 4; i++)
        {
            DkFilter filter = (i & sampler_FilterMask) == sampler_Linear ? DkFilter_Linear : DkFilter_Nearest;
            DkWrapMode wrapMode = (i & sampler_WrapMask) == sampler_Repeat ? DkWrapMode_Repeat : DkWrapMode_ClampToEdge;
            descriptors[i].initialize(dk::Sampler{}
                .setFilter(DkFilter_Linear, filter)
                .setWrapMode(wrapMode, wrapMode));
        }
    }

    GfxLog("GBAStationNDSStub: Gfx system fonts begin");
    plInitialize(PlServiceType_User);
    SystemFontStandardData = CopySharedFont(PlSharedFontType_Standard);
    SystemFontStandard = FontLoad(SystemFontStandardData);

    SystemFontNintendoExtData = CopySharedFont(PlSharedFontType_NintendoExt);
    SystemFontNintendoExt = FontLoad(SystemFontNintendoExtData);

    SystemFontChineseData = CopySharedFont(PlSharedFontType_ChineseSimplified);
    if (!SystemFontChineseData)
        SystemFontChineseData = CopySharedFont(PlSharedFontType_ExtChineseSimplified);
    if (!SystemFontChineseData)
        SystemFontChineseData = SystemFontStandardData;
    SystemFontChinese = FontLoad(SystemFontChineseData);
    plExit();
    GfxLog("GBAStationNDSStub: Gfx system fonts ok standard=%p ext=%p chinese=%p",
           SystemFontStandardData, SystemFontNintendoExtData, SystemFontChineseData);

    GfxLog("GBAStationNDSStub: Gfx white texture create begin");
    WhiteTexture = TextureCreate(8, 8, DkImageFormat_R8_Unorm);
    TextureSetSwizzle(WhiteTexture, DkImageSwizzle_One, DkImageSwizzle_One, DkImageSwizzle_One, DkImageSwizzle_One);
    GfxLog("GBAStationNDSStub: Gfx Init ok whiteTexture=%u", WhiteTexture);
}

void DeInit()
{
    NdsExtensionsInitialized = false;
    FontDelete(SystemFontNintendoExt);
    FontDelete(SystemFontStandard);
    FontDelete(SystemFontChinese);
    delete[] SystemFontStandardData;
    delete[] SystemFontNintendoExtData;
    if (SystemFontChineseData != SystemFontStandardData)
        delete[] SystemFontChineseData;

    FontAtlas.Destroy();

    PresentQueue.waitIdle();
    EmuQueue.waitIdle();

    Swapchain.destroy();

    PresentCmdBuf.destroy();
    EmuCmdBuf.destroy();
    PresentQueue.destroy();
    EmuQueue.destroy();

    CmdMem.reset();

    TextureHeap.reset();
    ShaderCodeHeap.reset();
    DataHeap.reset();

    Device.destroy();
}

double AnimationTimestamp;
double AnimationTimestep, AnimationLastTimestamp;
bool DoSkipTimestep = false;

void SkipTimestep()
{
    DoSkipTimestep = true;
}

void Rotate90DegInv(u32& outX, u32& outY, u32 inX, u32 inY, int rotation)
{
    u32 tmpX = inX, tmpY = inY;
    switch (rotation)
    {
    case 0: outX = tmpX; outY = tmpY; break;
    case 1: outX = 1280 - tmpY; outY = tmpX; break;
    case 2: outX = 1280 - tmpX; outY = 720 - tmpY; break;
    case 3: outX = tmpY; outY = 720 - tmpX; break;
    }
}

void Rotate90Deg(u32& outX, u32& outY, u32 inX, u32 inY, int rotation)
{
    u32 tmpX = inX, tmpY = inY;
    switch (rotation)
    {
    case 0: outX = tmpX; outY = tmpY; break;
    case 1: outX = tmpY; outY = 1280 - tmpX; break;
    case 2: outX = 1280 - tmpX; outY = 720 - tmpY; break;
    case 3: outX = 720 - tmpY; outY = tmpX; break;
    }
}

void StartFrame()
{
    SwapchainSlot = PresentQueue.acquireImage(Swapchain);
    FrameActive = true;
    DrawTransformStack.clear();
    CurShaderMode = shaderMode_Default;
    CurSampler = sampler_Nearest | sampler_ClampToEdge;

    AnimationTimestamp = armTicksToNs(armGetSystemTick()) * 0.000000001;
    if (!DoSkipTimestep)
    {
        AnimationTimestep = AnimationTimestamp - AnimationLastTimestamp;
    }
    else
    {
        DoSkipTimestep = false;
    }
    AnimationLastTimestamp = AnimationTimestamp;
}

void EndFrame(Color clearColor, int rotation)
{
    assert(ScissorStack.size() == 0);

    u32 fbPixelWidth, fbPixelHeight;
    if (appletGetOperationMode() == AppletOperationMode_Console)
    {
        fbPixelWidth = 1920;
        fbPixelHeight = 1080;
    }
    else
    {
        fbPixelWidth = 1280;
        fbPixelHeight = 720;
    }
    Swapchain.setCrop(0, 0, fbPixelWidth, fbPixelHeight);


    FontAtlas.IssueUpload();

    CmdMem->Begin(PresentCmdBuf);

    for (u32 i = 0; i < TextureUploadsPending.size(); i++)
    {
        PendingTextureUpload& upload = TextureUploadsPending[i];
        Texture& texture = Textures[upload.TextureIdx];
        dk::ImageView view{texture.Image};
        DkGpuAddr stagingBufferGpuAddr = DataHeap->GpuAddr(TextureStagingBuffer[upload.StagingSlot]) + upload.StagingOffsetBytes;
        assert((stagingBufferGpuAddr & (DK_IMAGE_LINEAR_STRIDE_ALIGNMENT - 1)) == 0);
        PresentCmdBuf.copyBufferToImage({stagingBufferGpuAddr, upload.DataStrideBytes, upload.Height}, view, {upload.X, upload.Y, 0, upload.Width, upload.Height, 1});
    }
    if (TextureUploadsPending.size() > 0)
    {
        TextureStagingBufferOffset = 0;
        TextureUploadsPending.clear();

        PresentCmdBuf.barrier(DkBarrier_Full, 0);
    }

    dk::ImageView colorTarget{Framebuffers[SwapchainSlot]};
    PresentCmdBuf.bindRenderTargets(&colorTarget);

    PresentCmdBuf.setScissors(0, {{0, 0, 1920, 1080}});
    PresentCmdBuf.setViewports(0, {{0, 0, 1920.0f, 1080.0f}});
    PresentCmdBuf.clearColor(0, DkColorMask_RGBA, clearColor.R, clearColor.G, clearColor.B, clearColor.A);
    PresentCmdBuf.setScissors(0, {{0, 0, fbPixelWidth, fbPixelHeight}});
    PresentCmdBuf.setViewports(0, {{0, 0, (float)fbPixelWidth, (float)fbPixelHeight}});

    PresentCmdBuf.bindDepthStencilState(dk::DepthStencilState{}
        .setDepthWriteEnable(false)
        .setDepthTestEnable(false));

    PresentCmdBuf.bindColorState(dk::ColorState{}.setBlendEnable(0, true));
    PresentCmdBuf.bindBlendStates(0, dk::BlendState{}
            .setColorBlendOp(DkBlendOp_Add)
            .setSrcColorBlendFactor(DkBlendFactor_SrcAlpha)
            .setDstColorBlendFactor(DkBlendFactor_InvSrcAlpha));

    PresentCmdBuf.bindVtxBuffer(0, DataHeap->GpuAddr(VertexData[SwapchainSlot]), VertexData[SwapchainSlot].Size);
    PresentCmdBuf.bindVtxAttribState(
    {
        DkVtxAttribState{0, 0, offsetof(Vertex, Position), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
        DkVtxAttribState{0, 0, offsetof(Vertex, UV), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
        DkVtxAttribState{0, 0, offsetof(Vertex, Color), DkVtxAttribSize_4x8, DkVtxAttribType_Unorm, 0},
        DkVtxAttribState{0, 0, offsetof(Vertex, CoolTransparency), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
    });
    PresentCmdBuf.bindVtxBufferState({{sizeof(Vertex), 0}});
    PresentCmdBuf.bindIdxBuffer(DkIdxFormat_Uint16, DataHeap->GpuAddr(IndexData[SwapchainSlot]));

    auto bindScreenProjection = [&]() {
        Transformation transformation;
        xm4_orthographic(transformation.Projection, -1280.f/2, 1280.f/2, 720.f/2.f, -720.f/2, -1.f, 1.f);
        float rot[16];
        float trans[16];
        int screenWidth = 1280, screenHeight = 720;
        if ((rotation % 2) == 1)
            std::swap(screenWidth, screenHeight);
        xm4_translatev(trans, -screenWidth/2, -screenHeight/2, 0.f);
        xm4_rotatef(rot, -M_PI_2 * rotation, 0.f, 0.f, 1.f);
        xm4_mul(rot, trans, rot);
        xm4_mul(transformation.Projection, rot, transformation.Projection);
        transformation.InvHeight = 1.f / screenHeight;
        PresentCmdBuf.pushConstants(DataHeap->GpuAddr(UniformBuffer), UniformBuffer.Size, 0, sizeof(Transformation), &transformation);
        PresentCmdBuf.bindUniformBuffer(DkStage_Vertex, 0, DataHeap->GpuAddr(UniformBuffer), UniformBuffer.Size);
    };
    auto bindOffscreenProjection = [&](u32 width, u32 height) {
        Transformation transformation;
        xm4_orthographic(transformation.Projection,
                         -static_cast<float>(width) / 2.0f,
                          static_cast<float>(width) / 2.0f,
                          static_cast<float>(height) / 2.0f,
                         -static_cast<float>(height) / 2.0f,
                         -1.0f,
                          1.0f);
        float trans[16];
        xm4_translatev(trans, -static_cast<float>(width) / 2.0f, -static_cast<float>(height) / 2.0f, 0.0f);
        xm4_mul(transformation.Projection, trans, transformation.Projection);
        transformation.InvHeight = 1.0f / static_cast<float>(std::max<u32>(height, 1));
        PresentCmdBuf.pushConstants(DataHeap->GpuAddr(UniformBuffer), UniformBuffer.Size, 0, sizeof(Transformation), &transformation);
        PresentCmdBuf.bindUniformBuffer(DkStage_Vertex, 0, DataHeap->GpuAddr(UniformBuffer), UniformBuffer.Size);
    };
    auto bindNdsParams = [&](const std::array<float, 8>& values, float sourceScale) {
        NdsShaderUniform params {};
        for (int i = 0; i < 4; ++i)
        {
            params.Param0[i] = values[i];
            params.Param1[i] = values[i + 4];
        }
        params.Runtime[0] = std::max(sourceScale, 1.0f);
        PresentCmdBuf.pushConstants(DataHeap->GpuAddr(NdsShaderUniformBuffer),
                                    NdsShaderUniformBuffer.Size,
                                    0,
                                    sizeof(NdsShaderUniform),
                                    &params);
        PresentCmdBuf.bindUniformBuffer(DkStage_Fragment,
                                        1,
                                        DataHeap->GpuAddr(NdsShaderUniformBuffer),
                                        NdsShaderUniformBuffer.Size);
    };
    bindScreenProjection();
    bindNdsParams(CurNdsShaderParams, 1.0f);

    memcpy(DataHeap->CpuAddr<void>(VertexData[SwapchainSlot]), VertexDataClient, sizeof(Vertex)*CurClientVertex);
    memcpy(DataHeap->CpuAddr<void>(IndexData[SwapchainSlot]), IndexDataClient, sizeof(u16)*CurClientIndex);
    CurClientVertex = CurClientIndex = 0;

    assert(UsedTextures.size() <= 1024 && "use less textures at once!");
    dk::ImageDescriptor* imageDescriptors = DataHeap->CpuAddr<dk::ImageDescriptor>(ImageDescriptors[SwapchainSlot]);
    for (u32 i = 0; i < UsedTextures.size(); i++)
    {
        Texture& texture = Textures[UsedTextures[i]];

        dk::ImageView view{texture.Image};
        view.setSwizzle(texture.ComponentSwizzle[0], texture.ComponentSwizzle[1], texture.ComponentSwizzle[2], texture.ComponentSwizzle[3]);
        imageDescriptors[i].initialize(view);
    }

    if (UsedTextures.size() > 0)
    {
        PresentCmdBuf.bindSamplerDescriptorSet(DataHeap->GpuAddr(SamplerDescriptor), 4);
        PresentCmdBuf.bindImageDescriptorSet(DataHeap->GpuAddr(ImageDescriptors[SwapchainSlot]), UsedTextures.size());
    }

    bool gpuStateDirty = true;

    for (u32 i = 0; i < DrawCalls.size(); i++)
    {
        DrawCall& drawCall = DrawCalls[i];
        if (drawCall.Kind == DrawCallKind::WaitFence)
        {
            assert(drawCall.Fence);
            PresentCmdBuf.waitFence(*drawCall.Fence);
            gpuStateDirty = true;
        }
        else if (drawCall.Kind == DrawCallKind::SignalFence)
        {
            assert(drawCall.Fence);
            PresentCmdBuf.signalFence(*drawCall.Fence);
            gpuStateDirty = true;
        }
        else if (drawCall.Kind == DrawCallKind::NdsMultiPass)
        {
            NdsMultiPassDraw& multi = drawCall.MultiPass;
            if (multi.PassCount < 2)
            {
                gpuStateDirty = true;
                continue;
            }

            auto bindTexture = [&](u32 texture, u32 sampler) {
                assert(Textures[texture].ImageDescriptorIdx != -1);
                assert(Textures[texture].ImageDescriptorIdx < UsedTextures.size());
                PresentCmdBuf.bindTextures(DkStage_Fragment, 0,
                    dkMakeTextureHandle(Textures[texture].ImageDescriptorIdx, sampler));
            };
            auto bindPassParams = [&](const NdsFilterPass& pass, float sourceScale) {
                std::array<float, 8> values = CurNdsShaderParams;
                values[7] = static_cast<float>(pass.Code);
                bindNdsParams(values, sourceScale);
            };

            u32 sourceTexture = multi.SourceTexture;
            float sourceScale = std::max(multi.SourceScale, 1.0f);
            for (int passIndex = 0; passIndex < multi.PassCount - 1; ++passIndex)
            {
                const NdsFilterPass& pass = multi.Passes[passIndex];
                const u32 targetTexture = (passIndex % 2 == 0) ? multi.TempTextureA : multi.TempTextureB;
                Texture& target = Textures[targetTexture];
                dk::ImageView targetView{target.Image};
                PresentCmdBuf.bindRenderTargets(&targetView);
                PresentCmdBuf.setViewports(0, {{0.0f, 0.0f, static_cast<float>(multi.TempWidth), static_cast<float>(multi.TempHeight)}});
                PresentCmdBuf.setScissors(0, {{0, 0, multi.TempWidth, multi.TempHeight}});
                PresentCmdBuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);
                bindOffscreenProjection(multi.TempWidth, multi.TempHeight);
                bindPassParams(pass, sourceScale);
                bindTexture(sourceTexture, pass.Sampler);
                PresentCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {&VertexShader, &FragmentShaders[pass.Shader]});
                const u32 indexOffset = (passIndex == 0) ? multi.InitialIndexOffset : multi.IntermediateIndexOffset;
                PresentCmdBuf.drawIndexed(DkPrimitive_Triangles, 6, 1, indexOffset, 0, 0);
                PresentCmdBuf.barrier(DkBarrier_Full, 0);
                sourceTexture = targetTexture;
                sourceScale *= std::max(pass.OutputScale, 1);
            }

            dk::ImageView mainTarget{Framebuffers[SwapchainSlot]};
            PresentCmdBuf.bindRenderTargets(&mainTarget);
            PresentCmdBuf.setViewports(0, {{0.0f, 0.0f, static_cast<float>(fbPixelWidth), static_cast<float>(fbPixelHeight)}});
            DkScissor scissor = multi.Scissor;
            u32 x1, y1, x2, y2;
            Rotate90DegInv(x1, y1, scissor.x, scissor.y, rotation);
            Rotate90DegInv(x2, y2, scissor.x + scissor.width, scissor.y + scissor.height, rotation);
            scissor.x = std::min(x1, x2);
            scissor.y = std::min(y1, y2);
            scissor.width = abs((s32)x1 - (s32)x2);
            scissor.height = abs((s32)y1 - (s32)y2);
            if (fbPixelHeight == 1080)
            {
                scissor.x = scissor.x * 3 / 2;
                scissor.y = scissor.y * 3 / 2;
                scissor.width = scissor.width * 3 / 2;
                scissor.height = scissor.height * 3 / 2;
            }
            PresentCmdBuf.setScissors(0, {scissor});
            bindScreenProjection();

            const NdsFilterPass& finalPass = multi.Passes[multi.PassCount - 1];
            bindPassParams(finalPass, sourceScale);
            bindTexture(sourceTexture, finalPass.Sampler);
            PresentCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {&VertexShader, &FragmentShaders[finalPass.Shader]});
            PresentCmdBuf.drawIndexed(DkPrimitive_Triangles, 6, 1, multi.FinalIndexOffset, 0, 0);
            bindNdsParams(CurNdsShaderParams, 1.0f);
            gpuStateDirty = true;
        }
        else
        {
            if ((drawCall.Dirty & drawCallDirty_Scissor) || gpuStateDirty)
            {
                // hacky
                DkScissor scissor = drawCall.Scissor;
                u32 x1, y1, x2, y2;
                Rotate90DegInv(x1, y1, scissor.x, scissor.y, rotation);
                Rotate90DegInv(x2, y2, scissor.x + scissor.width, scissor.y + scissor.height, rotation);

                scissor.x = std::min(x1, x2);
                scissor.y = std::min(y1, y2);
                scissor.width = abs((s32)x1 - (s32)x2);
                scissor.height = abs((s32)y1 - (s32)y2);
                if (fbPixelHeight == 1080)
                {
                    scissor.x = scissor.x * 3 / 2;
                    scissor.y = scissor.y * 3 / 2;
                    scissor.width = scissor.width * 3 / 2;
                    scissor.height = scissor.height * 3 / 2;
                }
                PresentCmdBuf.setScissors(0, {scissor});
            }
            if ((drawCall.Dirty & (drawCallDirty_Texture|drawCallDirty_Sampler)) || gpuStateDirty)
            {
                assert(Textures[drawCall.TextureIdx].ImageDescriptorIdx != -1);
                assert(Textures[drawCall.TextureIdx].ImageDescriptorIdx < UsedTextures.size());
                PresentCmdBuf.bindTextures(DkStage_Fragment, 0,
                    dkMakeTextureHandle(Textures[drawCall.TextureIdx].ImageDescriptorIdx, drawCall.Sampler));
            }
            if ((drawCall.Dirty & drawCallDirty_Shader) || gpuStateDirty)
            {
                PresentCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {&VertexShader, &FragmentShaders[drawCall.Shader]});
            }

            if (drawCall.Shader != shaderMode_Default)
                bindNdsParams(CurNdsShaderParams, drawCall.NdsSourceScale);

            PresentCmdBuf.drawIndexed(DkPrimitive_Triangles, drawCall.Count, 1, drawCall.IndexOffset, 0, 0);
            gpuStateDirty = false;
        }
    }

    PresentQueue.submitCommands(CmdMem->End(PresentCmdBuf));
    PresentQueue.presentImage(Swapchain, SwapchainSlot);
    FrameActive = false;

    DrawCalls.clear();
    for (u32 i = 0; i < UsedTextures.size(); i++)
        Textures[UsedTextures[i]].ImageDescriptorIdx = -1;
    UsedTextures.clear();
}

void WaitForFenceReady(dk::Fence& fence)
{
    DrawCalls.push_back({DrawCallKind::WaitFence, drawCallDirty_WaitFence, 0, 0, shaderMode_Default, 0, 0, DkScissor(), &fence, {}});
}

void SignalFence(dk::Fence& fence)
{
    DrawCalls.push_back({DrawCallKind::SignalFence, drawCallDirty_SignalFence, 0, 0, shaderMode_Default, 0, 0, DkScissor(), &fence, {}});
}

void IssueDrawCall(u32 texture, u32 count, u32 indexOffset)
{
    u32 dirty = 0;
    DkScissor& curScissor = ScissorStack[ScissorStack.size() - 1];
    bool lastWasFence = false;
    if (DrawCalls.size() > 0)
    {
        DrawCall& prevDrawCall = DrawCalls[DrawCalls.size() - 1];
        if (prevDrawCall.Kind != DrawCallKind::Draw)
        {
            dirty = ~(drawCallDirty_WaitFence|drawCallDirty_SignalFence);
            lastWasFence = true;
        }
        else if (prevDrawCall.TextureIdx != texture)
            dirty |= drawCallDirty_Texture;
        if (prevDrawCall.Kind == DrawCallKind::Draw)
        {
            if (prevDrawCall.Sampler != CurSampler)
                dirty |= drawCallDirty_Sampler;
            if (prevDrawCall.Shader != CurShaderMode)
                dirty |= drawCallDirty_Shader;
            if (prevDrawCall.NdsSourceScale != CurNdsSourceScale)
                dirty |= drawCallDirty_Shader;

            if (prevDrawCall.Scissor.x != curScissor.x
                || prevDrawCall.Scissor.y != curScissor.y
                || prevDrawCall.Scissor.width != curScissor.width
                || prevDrawCall.Scissor.height != curScissor.height)
            {
                dirty |= drawCallDirty_Scissor;
            }
        }
    }
    else
    {
        dirty = ~(drawCallDirty_WaitFence|drawCallDirty_SignalFence);
    }

    UseTexture(texture);

    if (dirty || lastWasFence)
        DrawCalls.push_back({DrawCallKind::Draw, dirty, texture, CurSampler, CurShaderMode, indexOffset, count, curScissor, nullptr, {}, CurNdsSourceScale});
    else
        DrawCalls[DrawCalls.size() - 1].Count += count;
}

void SetShaderMode(ShaderMode mode)
{
    CurShaderMode = (mode >= shaderMode_Default && mode < shaderMode_Count) ? mode : shaderMode_Default;
}

void SetNdsShaderParams(const std::array<float, 8>& params)
{
    CurNdsShaderParams = params;
}

void SetNdsSourceScale(float scale)
{
    CurNdsSourceScale = std::max(scale, 1.0f);
}

void SetSampler(u32 sampler)
{
    CurSampler = sampler;
}

void DrawRectangle(u32 texIdx, 
    Vector2f position, Vector2f size,
    Vector2f subPosition, Vector2f subSize,
    Color tint,
    bool coolTransparency)
{
    Texture& texture = Textures[texIdx];

    Vector2f rcpTexSize{1.f / texture.Width, 1.f / texture.Height};
    Vector2f uvMin = subPosition * rcpTexSize;
    Vector2f uvMax = uvMin + subSize * rcpTexSize;

    u8 tintR8 = tint.R * 255;
    u8 tintG8 = tint.G * 255;
    u8 tintB8 = tint.B * 255;
    u8 tintA8 = tint.A * 255;

    float coolTransparencyMin = coolTransparency ? 0.6f : 1.f;
    float coolTransparencyMax = coolTransparency ? 0.9f : 1.f;

    Vector2f outerBounds = position + size;
    const Vector2f p0 = TransformPoint(position);
    const Vector2f p1 = TransformPoint({outerBounds.X, position.Y});
    const Vector2f p2 = TransformPoint({position.X, outerBounds.Y});
    const Vector2f p3 = TransformPoint(outerBounds);

    assert(CurClientVertex + 4 <= MaxVertices);
    VertexDataClient[CurClientVertex + 0] = {p0.X, p0.Y,
        uvMin.X, uvMin.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};
    VertexDataClient[CurClientVertex + 1] = {p1.X, p1.Y,
        uvMax.X, uvMin.Y, tintR8,
        tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};
    VertexDataClient[CurClientVertex + 2] = {p2.X, p2.Y,
        uvMin.X, uvMax.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};
    VertexDataClient[CurClientVertex + 3] = {p3.X, p3.Y,
        uvMax.X, uvMax.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};

    assert(CurClientIndex + 6 <= MaxIndices);
    IndexDataClient[CurClientIndex + 0] = CurClientVertex;
    IndexDataClient[CurClientIndex + 1] = CurClientVertex + 2;
    IndexDataClient[CurClientIndex + 2] = CurClientVertex + 3;
    IndexDataClient[CurClientIndex + 3] = CurClientVertex;
    IndexDataClient[CurClientIndex + 4] = CurClientVertex + 3;
    IndexDataClient[CurClientIndex + 5] = CurClientVertex + 1;

    IssueDrawCall(texIdx, 6, CurClientIndex);

    CurClientVertex += 4;
    CurClientIndex += 6;
}

void DrawRectangle(Vector2f position, Vector2f size, Color tint, bool coolTransparency)
{
    DrawRectangle(WhiteTexture, position, size, Vector2f{}, Vector2f{}, tint, coolTransparency);
}

void DrawRectangle(u32 texIdx, Vector2f position, Vector2f size, Vector2f subPosition, Color tint, bool coolTransparency)
{
    DrawRectangle(texIdx, position, size, subPosition, subPosition + size, tint, coolTransparency);
}

void DrawRectangle(u32 texIdx,
    Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3,
    Vector2f subPosition, Vector2f subSize)
{
    DrawRectangle(texIdx, p0, p1, p2, p3, subPosition, subSize, {1.0f, 1.0f, 1.0f, 1.0f});
}

void DrawRectangle(u32 texIdx,
    Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3,
    Vector2f subPosition, Vector2f subSize,
    Color tint,
    bool coolTransparency)
{
    Texture& texture = Textures[texIdx];

    Vector2f rcpTexSize{1.f / texture.Width, 1.f / texture.Height};
    Vector2f uvMin = subPosition * rcpTexSize;
    Vector2f uvMax = uvMin + subSize * rcpTexSize;
    p0 = TransformPoint(p0);
    p1 = TransformPoint(p1);
    p2 = TransformPoint(p2);
    p3 = TransformPoint(p3);

    u8 tintR8 = tint.R * 255;
    u8 tintG8 = tint.G * 255;
    u8 tintB8 = tint.B * 255;
    u8 tintA8 = tint.A * 255;

    float coolTransparencyMin = coolTransparency ? 0.6f : 1.f;
    float coolTransparencyMax = coolTransparency ? 0.9f : 1.f;

    assert(CurClientVertex + 4 <= MaxVertices);
    VertexDataClient[CurClientVertex + 0] = {p0.X, p0.Y,
        uvMin.X, uvMin.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};
    VertexDataClient[CurClientVertex + 1] = {p1.X, p1.Y,
        uvMax.X, uvMin.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};
    VertexDataClient[CurClientVertex + 2] = {p2.X, p2.Y,
        uvMin.X, uvMax.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};
    VertexDataClient[CurClientVertex + 3] = {p3.X, p3.Y,
        uvMax.X, uvMax.Y,
        tintR8, tintG8, tintB8, tintA8,
        coolTransparencyMin, coolTransparencyMax};

    assert(CurClientIndex + 6 <= MaxIndices);
    IndexDataClient[CurClientIndex + 0] = CurClientVertex;
    IndexDataClient[CurClientIndex + 1] = CurClientVertex + 2;
    IndexDataClient[CurClientIndex + 2] = CurClientVertex + 3;
    IndexDataClient[CurClientIndex + 3] = CurClientVertex;
    IndexDataClient[CurClientIndex + 4] = CurClientVertex + 3;
    IndexDataClient[CurClientIndex + 5] = CurClientVertex + 1;

    IssueDrawCall(texIdx, 6, CurClientIndex);

    CurClientVertex += 4;
    CurClientIndex += 6;
}

void DrawNdsMultiPassRectangle(u32 texIdx,
    Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3,
    Vector2f subPosition, Vector2f subSize,
    const NdsFilterPass* passes,
    int passCount,
    u32 tempTextureA,
    u32 tempTextureB,
    u32 tempWidth,
    u32 tempHeight)
{
    passCount = std::clamp(passCount, 0, 4);
    if (passCount < 2 || !passes || tempTextureA == 0 || tempTextureB == 0 || tempWidth == 0 || tempHeight == 0)
    {
        DrawRectangle(texIdx, p0, p1, p2, p3, subPosition, subSize);
        return;
    }

    UseTexture(texIdx);
    UseTexture(tempTextureA);
    UseTexture(tempTextureB);

    auto appendQuad = [&](Vector2f q0, Vector2f q1, Vector2f q2, Vector2f q3,
                          Vector2f uvMin, Vector2f uvMax,
                          bool applyTransform) {
        if (applyTransform)
        {
            q0 = TransformPoint(q0);
            q1 = TransformPoint(q1);
            q2 = TransformPoint(q2);
            q3 = TransformPoint(q3);
        }
        const u32 indexOffset = CurClientIndex;
        assert(CurClientVertex + 4 <= MaxVertices);
        VertexDataClient[CurClientVertex + 0] = {q0.X, q0.Y, uvMin.X, uvMin.Y, 255, 255, 255, 255, 1.0f, 1.0f};
        VertexDataClient[CurClientVertex + 1] = {q1.X, q1.Y, uvMax.X, uvMin.Y, 255, 255, 255, 255, 1.0f, 1.0f};
        VertexDataClient[CurClientVertex + 2] = {q2.X, q2.Y, uvMin.X, uvMax.Y, 255, 255, 255, 255, 1.0f, 1.0f};
        VertexDataClient[CurClientVertex + 3] = {q3.X, q3.Y, uvMax.X, uvMax.Y, 255, 255, 255, 255, 1.0f, 1.0f};
        assert(CurClientIndex + 6 <= MaxIndices);
        IndexDataClient[CurClientIndex + 0] = CurClientVertex;
        IndexDataClient[CurClientIndex + 1] = CurClientVertex + 2;
        IndexDataClient[CurClientIndex + 2] = CurClientVertex + 3;
        IndexDataClient[CurClientIndex + 3] = CurClientVertex;
        IndexDataClient[CurClientIndex + 4] = CurClientVertex + 3;
        IndexDataClient[CurClientIndex + 5] = CurClientVertex + 1;
        CurClientVertex += 4;
        CurClientIndex += 6;
        return indexOffset;
    };

    Texture& sourceTexture = Textures[texIdx];
    const Vector2f sourceUvMin{subPosition.X / static_cast<float>(sourceTexture.Width),
                               subPosition.Y / static_cast<float>(sourceTexture.Height)};
    const Vector2f sourceUvMax{sourceUvMin.X + subSize.X / static_cast<float>(sourceTexture.Width),
                               sourceUvMin.Y + subSize.Y / static_cast<float>(sourceTexture.Height)};
    const u32 initialOffset = appendQuad({0.0f, 0.0f},
                                         {static_cast<float>(tempWidth), 0.0f},
                                         {0.0f, static_cast<float>(tempHeight)},
                                         {static_cast<float>(tempWidth), static_cast<float>(tempHeight)},
                                         sourceUvMin,
                                         sourceUvMax,
                                         false);
    const u32 intermediateOffset = appendQuad({0.0f, 0.0f},
                                              {static_cast<float>(tempWidth), 0.0f},
                                              {0.0f, static_cast<float>(tempHeight)},
                                              {static_cast<float>(tempWidth), static_cast<float>(tempHeight)},
                                              {0.0f, 0.0f},
                                              {1.0f, 1.0f},
                                              false);
    const u32 finalOffset = appendQuad(p0, p1, p2, p3, {0.0f, 0.0f}, {1.0f, 1.0f}, true);

    NdsMultiPassDraw multi {};
    multi.SourceTexture = texIdx;
    multi.TempTextureA = tempTextureA;
    multi.TempTextureB = tempTextureB;
    multi.TempWidth = tempWidth;
    multi.TempHeight = tempHeight;
    multi.InitialIndexOffset = initialOffset;
    multi.IntermediateIndexOffset = intermediateOffset;
    multi.FinalIndexOffset = finalOffset;
    multi.PassCount = passCount;
    multi.SourceScale = CurNdsSourceScale;
    for (int i = 0; i < passCount; ++i)
        multi.Passes[i] = passes[i];
    multi.Scissor = ScissorStack[ScissorStack.size() - 1];

    DrawCalls.push_back({DrawCallKind::NdsMultiPass,
                         static_cast<u32>(~(drawCallDirty_WaitFence | drawCallDirty_SignalFence)),
                         texIdx,
                         CurSampler,
                         CurShaderMode,
                         finalOffset,
                         6,
                         multi.Scissor,
                         nullptr,
                         multi,
                         CurNdsSourceScale});
}

Vector2f MeasureText(u32 fontIdx, float size, const char* text)
{
    float scale = FontGetScale(fontIdx, size);
    float lineGap = FontGetLineGap(fontIdx, scale);
    u32 lines = 1;
    float lineWidth = 0.f, textWidth = 0.f;
    const char* textPtr = text;
    while (*textPtr)
    {
        u32 codepoint;
        textPtr += decode_utf8(&codepoint, (u8*)textPtr);

        if (codepoint == '\n')
        {
            lines++;
            textWidth = std::max(textWidth, lineWidth);
            lineWidth = 0.f;
        }
        else
        {
            lineWidth += FontGetGlyph(fontIdx, codepoint, scale).AdvanceWidth;
        }
    }
    textWidth = std::max(textWidth, lineWidth);

    float textHeight = lines * size + lineGap * (lines - 1);

    return {textWidth, textHeight};
}

Vector2f DrawText(u32 fontIdx, Vector2f position, float size, Color color, int horizontalAlign, int verticalAlign, const char* text)
{
    float scale = FontGetScale(fontIdx, size);
    float ascent = FontGetAscent(fontIdx, scale);
    float lineGap = FontGetLineGap(fontIdx, scale);

    SetSampler(sampler_Linear|sampler_ClampToEdge);

    if (verticalAlign != align_Left || horizontalAlign != align_Left)
    {
        Vector2f textSize = MeasureText(fontIdx, size, text);

        if (horizontalAlign == align_Right)
            position.X -= textSize.X;
        else if (horizontalAlign == align_Center)
            position.X -= textSize.X / 2.f;

        if (verticalAlign == align_Right)
            position.Y -= textSize.Y;
        else if (verticalAlign == align_Center)
            position.Y -= textSize.Y / 2.f;
    }

    Vector2f offset;
    Vector2f bounds{0.f, size};

    while (*text)
    {
        u32 codepoint;
        text += decode_utf8(&codepoint, (u8*)text);

        if (codepoint == '\n')
        {
            bounds.X = std::max(bounds.X, offset.X);
            bounds.Y += size + lineGap;

            offset.X = 0.f;
            offset.Y += size + lineGap;
        }
        else
        {
            PackedGlyph& glyph = FontGetGlyph(fontIdx, codepoint, scale);

            if (codepoint != ' ')
            {
                int glyphWidth = glyph.BoxX2 - glyph.BoxX1;
                int glyphHeight = glyph.BoxY2 - glyph.BoxY1;

                DrawRectangle(glyph.Quad.AtlasTexture,
                    position + offset + Vector2f{(float)glyph.BoxX1, (float)glyph.BoxY1 + ascent},
                    {(float)glyphWidth, (float)glyphHeight},
                    {(float)glyph.Quad.PackX, (float)glyph.Quad.PackY},
                    {(float)glyphWidth, (float)glyphHeight},
                    color);
            }
            offset.X += glyph.AdvanceWidth;
        }
    }

    SetSampler(sampler_Nearest|sampler_ClampToEdge);
    bounds.X = std::max(bounds.X, offset.X);

    return bounds;
}

Vector2f DrawText(u32 fontIdx, Vector2f position, float size, Color color, const char* format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    int requiredLength = vsnprintf(NULL, 0, format, vargs) + 1;
    assert(requiredLength >= 1);
    char formattedText[requiredLength];
    vsnprintf(formattedText, requiredLength, format, vargs);
    va_end(vargs);

    return DrawText(fontIdx, position, size, color, align_Left, align_Left, formattedText);
}

}
