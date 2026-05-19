#ifndef GFX_H
#define GFX_H

#include <switch.h>
#include <deko3d.hpp>

#include <vector>
#include <algorithm>
#include <optional>

#include "GpuMemHeap.h"

namespace Gfx
{

union Vector2f
{
    Vector2f(float x = 0.f, float y = 0.f)
        : X(x), Y(y)
    {}

    struct
    {
        float X, Y;
    };
    float Components[2];

    bool operator==(const Vector2f& other) const
    {
        return X == other.X && Y == other.Y;
    }

    bool operator!=(const Vector2f& other) const
    {
        return *this != other;
    }

    Vector2f& operator+=(const Vector2f& other)
    {
        X += other.X;
        Y += other.Y;
        return *this;
    }
    Vector2f& operator-=(const Vector2f& other)
    {
        X -= other.X;
        Y -= other.Y;
        return *this;
    }
    Vector2f& operator*=(float scale)
    {
        X *= scale;
        Y *= scale;
        return *this;
    }
    Vector2f& operator*=(const Vector2f& other)
    {
        X *= other.X;
        Y *= other.Y;
        return *this;
    }

    Vector2f operator+(const Vector2f& other) const
    {
        return {X + other.X, Y + other.Y};
    }
    Vector2f operator-(const Vector2f& other) const
    {
        return {X - other.X, Y - other.Y};
    }
    Vector2f operator*(float scale) const
    {
        return {X * scale, Y * scale};
    }
    Vector2f operator*(const Vector2f& other) const
    {
        return {X * other.X, Y * other.Y};
    }

    float Dot(const Vector2f& other) const
    {
        return X * other.X + Y * other.Y;
    }

    float& operator[](int idx)
    {
        return Components[idx];
    }

    float operator[](int idx) const
    {
        return Components[idx];
    }

    Vector2f Min(const Vector2f& other) const
    {
        return {std::min(X, other.X), std::min(Y, other.Y)};
    }

    Vector2f Max(const Vector2f& other) const
    {
        return {std::max(X, other.X), std::max(Y, other.Y)};
    }

    Vector2f Clamp(const Vector2f& min, const Vector2f& max) const
    {
        return Min(max).Max(min);
    }

    float LengthSqr()
    {
        return Dot(*this);
    }
};

union Color
{
public:
    Color(u32 packed)
    {
        R = ((packed >> 24) & 0xFF) / 255.f;
        G = ((packed >> 16) & 0xFF) / 255.f;
        B = ((packed >> 8) & 0xFF) / 255.f;
        A = (packed & 0xFF) / 255.f;
    }

    Color(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.f)
        : R(r), G(g), B(b), A(a)
    {}

    Color WithAlpha(float alpha) const
    {
        return {R, G, B, alpha};
    }

    struct
    {
        float R, G, B, A;
    };
    float Components[4];
};

extern dk::Device Device;
extern dk::Queue PresentQueue, EmuQueue;
extern dk::CmdBuf PresentCmdBuf, EmuCmdBuf;
extern dk::Swapchain Swapchain;

extern std::optional<GpuMemHeap> TextureHeap;
extern std::optional<GpuMemHeap> ShaderCodeHeap;
extern std::optional<GpuMemHeap> DataHeap;

void Init();
void DeInit();

u32 TextureCreate(u32 width, u32 height, DkImageFormat format);
u32 TextureCreateExternal(u32 width, u32 height, dk::Image& image);
void TextureDelete(u32 idx);
// this is meant to be used sparsely
void TextureUpload(u32 idx, u32 x, u32 y, u32 width, u32 height, void* data, u32 dataStrideBytes);

void TextureSetSwizzle(u32 idx, DkSwizzle red, DkSwizzle green, DkSwizzle blue, DkSwizzle alpha);

u32 FontLoad(u8* data);
void FontDelete(u32 idx);

extern u32 SystemFontStandard;
extern u32 SystemFontNintendoExt;

extern u32 WhiteTexture;

enum
{
    sampler_FilterMask = 0x1,
    sampler_Nearest = 0 << 0, 
    sampler_Linear = 1 << 0,

    sampler_WrapMask = 0x2,
    sampler_Repeat = 0 << 1,
    sampler_ClampToEdge = 1 << 1,
};

void LoadShader(const char* path, dk::Shader& out);

void StartFrame();
void EndFrame(Color clearColor, int rotation);
void SkipTimestep();

void SetSampler(u32 sampler);

void PushScissor(u32 x, u32 y, u32 w, u32 h);
void PopScissor();

void WaitForFenceReady(dk::Fence& fence);
void SignalFence(dk::Fence& fence);

void DrawRectangle(Vector2f position, Vector2f size, Color tint, bool coolTransparency = false);
void DrawRectangle(u32 texIdx,
    Vector2f position, Vector2f size,
    Vector2f subPosition,
    Color tint,
    bool coolTransparency = false);
void DrawRectangle(u32 texIdx,
    Vector2f position, Vector2f size,
    Vector2f subPosition, Vector2f subSize,
    Color tint,
    bool coolTransparency = false);
void DrawRectangle(u32 texIdx,
    Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3,
    Vector2f subPosition, Vector2f subSize);

enum
{
    align_Left,
    align_Right,
    align_Center
};

#define GFX_NINTENDOFONT_CHECKMARK "\uE14B"
#define GFX_NINTENDOFONT_CROSS "\uE14C"
#define GFX_NINTENDOFONT_A_BUTTON "\uE0E0"
#define GFX_NINTENDOFONT_B_BUTTON "\uE0E1"
#define GFX_NINTENDOFONT_X_BUTTON "\uE0E2"
#define GFX_NINTENDOFONT_Y_BUTTON "\uE0E3"
#define GFX_NINTENDOFONT_PLUS_BUTTON "\uE0F1"
#define GFX_NINTENDOFONT_MINUS_BUTTON "\uE0F2"
#define GFX_NINTENDOFONT_BACK "\uE072"
#define GFX_NINTENDOFONT_WII_HAND "\uE062"
#define GFX_NINTENDOFONT_WII_HAND_HOLD "\uE05D"

Vector2f MeasureText(u32 fontIdx, float size, const char* text);

Vector2f DrawText(u32 fontIdx, Vector2f position, float size, Color color, int horizontalAlign, int verticalAlign, const char* text);
Vector2f DrawText(u32 fontIdx, Vector2f position, float size, Color color, const char* format, ...);

static constexpr int AtlasSize = 512;

struct AtlasTexture
{
    u32 Texture;
    u8* ClientImage;

    int DirtyX1 = AtlasSize, DirtyY1 = AtlasSize, DirtyX2 = 0, DirtyY2 = 0;
};

struct PackedQuad
{
    u32 AtlasTexture;
    int PackX, PackY;
};

struct Atlas
{
    DkImageFormat TexFmt;
    u32 BytesPerPixel;
    DkImageSwizzle Swizzle[4] = {DkImageSwizzle_Red, DkImageSwizzle_Green, DkImageSwizzle_Blue, DkImageSwizzle_Alpha};
    std::vector<AtlasTexture> Atlases;

    struct Shelf
    {
        int Atlas, Y, Height, Used;
    };
    std::vector<Shelf> Shelves;

    u8* Pack(int width, int height, PackedQuad& quad);

    void IssueUpload();

    void Destroy();

    inline u32 PackStride()
    {
        return AtlasSize * BytesPerPixel;
    }
};

extern double AnimationTimestamp;
extern double AnimationTimestep;

void Rotate90Deg(u32& outX, u32& outY, u32 inX, u32 inY, int rotation);

}

#endif