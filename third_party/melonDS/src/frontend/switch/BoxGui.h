#ifndef BOXGUI_H
#define BOXGUI_H

#include <switch.h>

#include <algorithm>
#include <functional>

#include <math.h>

#include "Gfx.h"

namespace BoxGui
{

enum
{
    direction_Horizontal,
    direction_Vertical,
};

struct Rect
{
    Gfx::Vector2f Position, Size;

    bool Intersects(const Rect& other) const
    {
        return Position.X < other.Position.X + other.Size.X
            && Position.Y < other.Position.Y + other.Size.Y
            && Position.X + Size.X > other.Position.X
            && Position.Y + Size.Y > other.Position.Y;
    }

    bool Contains(const Rect& other) const
    {
        return other.Position.X >= Position.X
            && other.Position.Y >= Position.Y
            && other.Position.X + other.Size.X <= Position.X + Size.X
            && other.Position.Y + other.Size.Y <= Position.Y + Size.Y;
    }

    Rect SafeIntegerise()
    {
        return {{floorf(Position.X), floorf(Position.Y)}, {ceilf(Size.X), ceilf(Size.Y)}};
    }

    Rect SharedBounds(const Rect& other) const
    {
        Rect result;
        result.Position = Position.Min(other.Position);
        result.Size = (Position+Size).Max(other.Position+other.Size)-result.Position;
        return result;
    }

    Rect CenteredChild(const Gfx::Vector2f size) const
    {
        Rect result;
        result.Position = Position + Size * 0.5f - size * 0.5f;
        result.Size = size;
        return result;
    }
};

struct Frame
{
    Frame(Gfx::Vector2f size);
    Frame(Frame& parent, Rect bounds,
        Gfx::Vector2f lowerMargin = {0.f, 0.f}, Gfx::Vector2f upperMargin = {0.f, 0.f},
        int scrollAxis = -1, u64 scrollId = UINT64_MAX, bool scrollClampMin = true, bool scrollClampMax = true);

    bool IsVisible()
    {
        return Parent ? Parent->Area.Intersects({Area.Position - LowerMargin, Area.Size + LowerMargin + UpperMargin}) : true;
    }

    Rect UnpaddedArea()
    {
        return {Area.Position - LowerMargin, Area.Size + LowerMargin + UpperMargin};
    }

    // lower as in lower in the coordinates, not on screen
    Rect Area;
    Frame* Parent;
    int ScrollAxis;
    float ScrollOffset;
    u64 ScrollId;
    Gfx::Vector2f LowerMargin, UpperMargin;
};

struct Skewer
{
    Skewer(Frame& parent, float oppositeAxisOffset, int axis);

    void AlignLeft(float initialOffset);
    void AlignRight(float initialOffset);

    Rect Spit(Gfx::Vector2f size, int alignment = Gfx::align_Center);
    void Advance(float offset);

    Gfx::Vector2f CurrentPosition()
    {
        return Parent.Area.Position + Offset;
    }

    float RemainingLength();

    Frame& Parent;
    Gfx::Vector2f Offset;
    int Axis;
    bool Forward = true;
};

bool ConfirmPressed();
bool CancelPressed();
bool SearchPressed();
bool DetailsPressed();

bool LeftPressed();
bool RightPressed();

void OpenModalDialog(std::function<bool(Frame&)> modalFunc);
bool HasModalDialog();

enum
{
    direction_Left,
    direction_Right,
    direction_Up,
    direction_Down,
};

bool InputElement(Frame& frame, u64 uniqueName, bool first = false);
void ForceSelecton(u64 uniqueName, bool skipAnimation = true, int level = -1);

u64 MakeUniqueName(const char* name, int salt);

void Update(Frame& rootFrame, u64 keysDown, u64 keysUp);

}

#endif