#include "BoxGui.h"

#include <unordered_map>
#include <vector>
#include <optional>

#include <math.h>

#include <assert.h>
#include <stdio.h>

namespace BoxGui
{

std::optional<std::function<bool(Frame&)>> CurrentModalDialog;

int ModalLevel()
{
    return CurrentModalDialog ? 1 : 0;
}

struct ScrollFrame
{
    int ModalLevel;
    float ScrollMin = 0.f, ScrollMax = 0.f;
    float Scroll = 0.f;
    int Axis;
    Rect Area;
    bool StillUsed;
};
std::unordered_map<u64, ScrollFrame> ScrollFrames;
float ScrollTime = 0.f;
float ScrollInitialDistance = 0.f;

Frame::Frame(Gfx::Vector2f size)
{
    Area.Size = size;
    Parent = nullptr;

    ScrollAxis = -1;
    ScrollOffset = 0.f;
}
    
Frame::Frame(Frame& parent, Rect area,
    Gfx::Vector2f lowerMargin, Gfx::Vector2f upperMargin,
    int scrollAxis, u64 scrollId,
    bool scrollClampMin , bool scrollClampMax)
{
    Area.Position = parent.Area.Position + area.Position + lowerMargin;
    Area.Size = area.Size - lowerMargin - upperMargin;
    Parent = &parent;
    LowerMargin = lowerMargin;
    UpperMargin = upperMargin;

    ScrollAxis = scrollAxis;
    if (ScrollAxis != -1)
    {
        assert(scrollId != UINT64_MAX);
        ScrollFrame& scrollFrame = ScrollFrames[scrollId];
        scrollFrame.Area = Area;
        scrollFrame.Axis = scrollAxis;
        scrollFrame.StillUsed = true;
        scrollFrame.ModalLevel = ModalLevel();
        ScrollOffset = scrollFrame.Scroll;
        ScrollId = scrollId;

        if (scrollClampMin)
            scrollFrame.ScrollMin = -INFINITY;
        else
            scrollFrame.ScrollMax = INFINITY;
    }

    // apply scroll
    if (parent.ScrollAxis != -1)
    {
        ScrollFrame& scrollFrame = ScrollFrames[parent.ScrollId];
        scrollFrame.ScrollMin = std::min(scrollFrame.ScrollMin, Area.Position[parent.ScrollAxis]);
        scrollFrame.ScrollMax = std::max(scrollFrame.ScrollMax,
            Area.Position[parent.ScrollAxis] + Area.Size[parent.ScrollAxis] - parent.Area.Size[parent.ScrollAxis]);

        Area.Position.Components[parent.ScrollAxis] -= parent.ScrollOffset;
    }
}

Skewer::Skewer(Frame& parent, float oppositeAxisOffset, int axis)
    : Parent(parent), Axis(axis)
{
    Offset.Components[axis ^ 1] = oppositeAxisOffset;
}

void Skewer::AlignLeft(float initialOffset)
{
    Offset.Components[Axis] = initialOffset;
    Forward = true;
}

void Skewer::AlignRight(float initialOffset)
{
    Offset.Components[Axis] = Parent.Area.Size.Components[Axis] - initialOffset;
    Forward = false;
}

Rect Skewer::Spit(Gfx::Vector2f size, int alignment)
{
    Gfx::Vector2f position;

    position.Components[Axis] = Offset.Components[Axis];
    if (!Forward)
        position.Components[Axis] -= size.Components[Axis];
    Offset.Components[Axis] += size.Components[Axis] * (Forward ? 1.f : -1.f);

    switch (alignment)
    {
    case Gfx::align_Left:
        position.Components[Axis ^ 1] = Offset.Components[Axis ^ 1] - size.Components[Axis ^ 1];
        break;
    case Gfx::align_Right:
        position.Components[Axis ^ 1] = Offset.Components[Axis ^ 1];
        break;
    case Gfx::align_Center:
        position.Components[Axis ^ 1] = Offset.Components[Axis ^ 1] - size.Components[Axis ^ 1] / 2.f;
        break;
    }

    return {position, size};
}

void Skewer::Advance(float offset)
{
    Offset.Components[Axis] += offset * (Forward ? 1.f : -1.f);
}

float Skewer::RemainingLength()
{
    return Parent.Area.Size.Components[Axis] - Offset.Components[Axis];
}

struct InputFrame
{
    Rect Area;
    uintptr_t Parents[8]; // pointer is only used as an ID
    u64 ParentScrollId;
};
std::unordered_map<u64, InputFrame> InputFrames;
u64 CurrentSelections[2] = {UINT64_MAX, UINT64_MAX};
u64 FirstElement = UINT64_MAX;

u64 NextSelection[2];

bool InputElement(Frame& frame, u64 uniqueName, bool first)
{
    InputFrame inputFrame;
    inputFrame.Area = frame.Area;
    inputFrame.ParentScrollId = frame.Parent ? frame.Parent->ScrollId : UINT64_MAX;
    inputFrame.Parents[0] = (uintptr_t)frame.Parent;
    for (int i = 1; i < 8; i++)
    {
        if (inputFrame.Parents[i - 1])
            inputFrame.Parents[i] = (uintptr_t)((Frame*)inputFrame.Parents[i - 1])->Parent;
        else
            inputFrame.Parents[i] = 0;
    }
    InputFrames[uniqueName] = inputFrame;

    // this is a dirty trick
    if (FirstElement == UINT64_MAX || first)
        FirstElement = uniqueName;
    return CurrentSelections[0] == uniqueName || CurrentSelections[1] == uniqueName;
}

bool SkipScrollAnimation = false;

void ForceSelecton(u64 uniqueName, bool skipAnimation, int level)
{
    NextSelection[level != - 1 ? level : ModalLevel()] = uniqueName;
    SkipScrollAnimation = skipAnimation;
}

inline u32 ROR(u32 x, u32 n)
{
    return (x >> (n&0x1F)) | (x << ((32-n)&0x1F));
}

u64 MakeUniqueName(const char* name, int subentry)
{
    u64 a = 43278943898;
    u64 b = 87345982394 ^ ROR(subentry, 13);

    while (*name)
    {
        a = ROR(a, 7);
        a ^= *name;
        a += b;
        b += ROR(subentry, 16);
        name++;
    }

    return a;
}

u64 KeysDown = 0;

float KeyRepeatTime = 0.f;
u64 KeysToRepeat = 0;

bool ConfirmPressed()
{
    return KeysDown & HidNpadButton_A;
}

bool CancelPressed()
{
    return KeysDown & HidNpadButton_B;
}

bool SearchPressed()
{
    return KeysDown & HidNpadButton_Y;
}

bool DetailsPressed()
{
    return KeysDown & HidNpadButton_Plus;
}

u32 DirectionsCaptured = 0;

bool LeftPressed()
{
    DirectionsCaptured |= 1;
    return KeysDown & HidNpadButton_Left;
}

bool RightPressed()
{
    DirectionsCaptured |= 1<<1;
    return KeysDown & HidNpadButton_Right;
}

void OpenModalDialog(std::function<bool(Frame&)> modalFunc)
{
    assert(!CurrentModalDialog);

    CurrentModalDialog = modalFunc;
}
bool HasModalDialog()
{
    return CurrentModalDialog.has_value();
}

void ResetSelection()
{
    if (InputFrames.count(CurrentSelections[ModalLevel()]) == 0)
    {
        SkipScrollAnimation = true;
        NextSelection[ModalLevel()] = FirstElement;
    }
    FirstElement = UINT64_MAX;
}

void Update(Frame& rootFrame, u64 keysDown, u64 keysUp)
{
    KeysDown = keysDown;
    if (KeysToRepeat == 0)
    {
        KeyRepeatTime = 0.22f;
        KeysToRepeat = keysDown & (HidNpadButton_AnyUp|HidNpadButton_AnyDown|HidNpadButton_AnyLeft|HidNpadButton_AnyRight);
    }
    if (keysUp & KeysToRepeat)
    {
        KeysToRepeat = 0;
        KeyRepeatTime = 0.f;
    }

    if (KeysToRepeat)
    {
        KeyRepeatTime -= Gfx::AnimationTimestep;
        if (KeyRepeatTime <= 0.f)
        {
            KeysDown |= KeysToRepeat;
            KeyRepeatTime = 0.08f;
        }
    }

    keysDown = KeysDown;

    if (CurrentModalDialog)
    {
        DirectionsCaptured = 0;
        FirstElement = UINT64_MAX;
        InputFrames.clear();

        if (!(*CurrentModalDialog)(rootFrame))
            CurrentModalDialog.reset();
        else
            ResetSelection();

        // yes a bit strange that the normal GUI
        // gets input with one frame delay
        // while modal dialogs don't
        KeysDown = 0;
    }
    else
    {
        ResetSelection();
    }

    int axis = -1;
    int direction = 0;

    if (keysDown & HidNpadButton_AnyUp)
    {
        axis = 1;
        direction = -1;
    }
    else if (keysDown & HidNpadButton_AnyDown)
    {
        axis = 1;
        direction = 1;
    }
    else if (keysDown & HidNpadButton_AnyLeft && !(DirectionsCaptured & 1))
    {
        axis = 0;
        direction = -1;
    }
    else if (keysDown & HidNpadButton_AnyRight && !(DirectionsCaptured & (1<<1)))
    {
        axis = 0;
        direction = 1;
    }
    DirectionsCaptured = 0;

    if (axis != -1)
    {
        InputFrame currentInput = InputFrames[CurrentSelections[ModalLevel()]];
        for (int i = 0; i < 8; i++)
        {
            uintptr_t parent = currentInput.Parents[i];
            if (!parent)
                break;

            u64 bestCandidate = UINT64_MAX;
            float bestCandidateDistance = infinityf();
            float bestCandidateDistanceSecondary = infinityf();

            float offset = currentInput.Area.Position.Components[axis];
            if (direction == 1)
                offset += currentInput.Area.Size.Components[axis];
            float offsetSecondary = currentInput.Area.Position.Components[axis ^ 1] + currentInput.Area.Size.Components[axis ^ 1] * 0.5f;

            for (auto& it : InputFrames)
            {
                if (it.first == CurrentSelections[ModalLevel()])
                    continue;

                bool sharesParent = false;
                for (int i = 0; i < 8; i++)
                {
                    if (it.second.Parents[i] == parent)
                    {
                        sharesParent = true;
                        break;
                    }
                }
                if (!sharesParent)
                    continue;

                float itCompValue = it.second.Area.Position.Components[axis];
                if (direction == -1)
                    itCompValue += it.second.Area.Size.Components[axis];
                float itCompValueSecondary = it.second.Area.Position.Components[axis ^ 1] + it.second.Area.Size.Components[axis ^ 1] * 0.5f;

                float distance = (itCompValue - offset) * direction;
                float distanceSecondary = fabsf(itCompValueSecondary - offsetSecondary);

                if (distance > 0.f && distance < bestCandidateDistance)
                {
                    bestCandidate = it.first;
                    bestCandidateDistance = distance;
                    bestCandidateDistanceSecondary = distanceSecondary;
                }
                if (fabsf(distance - bestCandidateDistance) < 0.01f && distanceSecondary < bestCandidateDistanceSecondary)
                {
                    bestCandidate = it.first;
                    bestCandidateDistance = distance;
                    bestCandidateDistanceSecondary = distanceSecondary;
                }
            }

            if (bestCandidate != UINT64_MAX)
            {
                NextSelection[ModalLevel()] = bestCandidate;
                break;
            }
        }
        /*u64 bestCandidate = UINT64_MAX;
        bool bestCandidateIsSibling = false;
        float bestCandidateDistance = infinityf();
        float bestCandidateDistanceSecondary = infinityf();

        float offset = currentInput.Area.Position.Components[axis];
        if (direction == 1)
            offset += currentInput.Area.Size.Components[axis];
        float offsetSecondary = currentInput.Area.Position.Components[axis ^ 1];

        for (auto& it : InputFrames)
        {
            if (it.first == CurrentSelections[ModalLevel()])
                continue;

            float itCompValue = it.second.Area.Position.Components[axis];
            if (direction == -1)
                itCompValue += it.second.Area.Size.Components[axis];

            float distance = (itCompValue - offset) * direction;
            bool isSibling = it.second.Parent == currentInput.Parent;

            if (bestCandidateIsSibling && !isSibling)
                continue;
            float itCompValueSecondary = it.second.Area.Position.Components[axis ^ 1];
            float distanceSecondary = fabsf(itCompValueSecondary - offsetSecondary);

            if (distance > 0.f && (distance < bestCandidateDistance || (!bestCandidateIsSibling && isSibling)))
            {
                bestCandidate = it.first;
                bestCandidateDistance = distance;
                bestCandidateDistanceSecondary = distanceSecondary;
                bestCandidateIsSibling = isSibling;
            }
            if (distanceSecondary > 0.f && fabsf(distance - bestCandidateDistance) < 0.01f && distanceSecondary < bestCandidateDistanceSecondary)
            {
                bestCandidate = it.first;
                bestCandidateDistance = distance;
                bestCandidateDistanceSecondary = distanceSecondary;
                bestCandidateIsSibling = isSibling;
            }
        }

        if (bestCandidate != UINT64_MAX)
        {
            NextSelection[ModalLevel()] = bestCandidate;
        }*/
    }

    {
        InputFrame& curSelection = InputFrames[CurrentSelections[ModalLevel()]];

        auto it = ScrollFrames.find(curSelection.ParentScrollId);
        if (it != ScrollFrames.end())
        {
            int axis = it->second.Axis;

            float distance = (curSelection.Area.Position[axis] + curSelection.Area.Size[axis] * 0.5f)
                - (it->second.Area.Position[axis] + it->second.Area.Size[axis] * 0.5f);
            float target = it->second.Scroll + distance;

            if (!SkipScrollAnimation)
            {
                if (ScrollTime == 0.f)
                    ScrollInitialDistance = distance;

                if (fabsf(distance) > 0.001f)
                {
                    // try to center it
                    ScrollTime += Gfx::AnimationTimestep;

                    // don't ask me where this formula came from
                    float velocity = expf(ScrollTime) * 100.f * sqrtf(fabsf(ScrollInitialDistance));
                    if (distance < 0.f)
                        velocity = -velocity;

                    it->second.Scroll += velocity * Gfx::AnimationTimestep;

                    if (distance < 0.f && it->second.Scroll < target)
                        it->second.Scroll = target;
                    else if (distance > 0.f && it->second.Scroll > target)
                        it->second.Scroll = target;

                    if (it->second.Scroll < it->second.ScrollMin)
                    {
                        it->second.Scroll = it->second.ScrollMin;
                        ScrollTime = 0.f;
                    }
                    if (it->second.Scroll > it->second.ScrollMax)
                    {
                        it->second.Scroll = it->second.ScrollMax;
                        ScrollTime = 0.f;
                    }
                }
                else
                {
                    ScrollTime = 0.f;
                }
            }
            else
            {
                SkipScrollAnimation = false;
                it->second.Scroll = std::clamp(target, it->second.ScrollMin, it->second.ScrollMax);
                ScrollTime = 0.f;
            }
        }
    }

    {
        std::vector<u64> unusedScrollFrames;
        for (auto& it : ScrollFrames)
        {
            if (it.second.ModalLevel == ModalLevel() && !it.second.StillUsed)
                unusedScrollFrames.push_back(it.first);
            else
                it.second.StillUsed = false;
        }
        for (u32 i = 0; i < unusedScrollFrames.size(); i++)
            ScrollFrames.erase(unusedScrollFrames[i]);
    }

    InputFrames.clear();

    for (int i = 0; i < 2; i++)
    {
        if (NextSelection[i] != UINT64_MAX)
        {
            CurrentSelections[i] = NextSelection[i];
            NextSelection[i] = UINT64_MAX;
        }
    }
}

}