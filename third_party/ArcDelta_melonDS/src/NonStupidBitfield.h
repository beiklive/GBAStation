/*
    Copyright 2016-2021 Arisotura, RSDuck

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef NONSTUPIDBITFIELD_H
#define NONSTUPIDBITFIELD_H

#include "types.h"

#include <memory.h>

#include <initializer_list>
#include <algorithm>

// like std::bitset but less stupid and optimised for 
// our use case (keeping track of memory invalidations)

inline u64 GetRangedBitMask(u32 idx, u32 startBit, u32 bitsCount)
{
    if (bitsCount == 0)
        return 0;

    const u64 rangeStart = startBit;
    const u64 rangeEnd = rangeStart + bitsCount;
    const u64 entryStart = (u64)idx * 64;
    const u64 entryEnd = entryStart + 64;
    if (rangeStart >= entryEnd || rangeEnd <= entryStart)
        return 0;

    const u32 firstBit = (u32)(std::max(rangeStart, entryStart) - entryStart);
    const u32 endBit = (u32)(std::min(rangeEnd, entryEnd) - entryStart);
    const u64 lowMask = firstBit == 0 ? 0xFFFFFFFFFFFFFFFFULL
                                     : 0xFFFFFFFFFFFFFFFFULL << firstBit;
    const u64 highMask = endBit == 64 ? 0xFFFFFFFFFFFFFFFFULL
                                     : (1ULL << endBit) - 1;
    return lowMask & highMask;
}

template <u32 Size>
struct NonStupidBitField
{
    static constexpr u32 DataLength = (Size + 0x3F) >> 6;
    u64 Data[DataLength];

    struct Ref
    {
        NonStupidBitField<Size>& BitField;
        u32 Idx;

        operator bool()
        {
            return BitField.Data[Idx >> 6] & (1ULL << (Idx & 0x3F));
        }

        Ref& operator=(bool set)
        {
            BitField.Data[Idx >> 6] &= ~(1ULL << (Idx & 0x3F));
            BitField.Data[Idx >> 6] |= ((u64)set << (Idx & 0x3F));
            return *this;
        }
    };

    struct Iterator
    {
        NonStupidBitField<Size>& BitField;
        u32 DataIdx;
        u32 BitIdx;
        u64 RemainingBits;

        u32 operator*() { return DataIdx * 64 + BitIdx; }

        bool operator==(const Iterator& other)
        {
            return other.DataIdx == DataIdx;
        }
        bool operator!=(const Iterator& other)
        {
            return other.DataIdx != DataIdx;
        }

        void Next()
        {
            if (RemainingBits == 0)
            {
                for (u32 i = DataIdx + 1; i < DataLength; i++)
                {
                    if (BitField.Data[i])
                    {
                        DataIdx = i;
                        RemainingBits = BitField.Data[i];
                        goto done;
                    }
                }
                DataIdx = DataLength;
                return;
            done:;
            }

            BitIdx = __builtin_ctzll(RemainingBits);
            RemainingBits &= ~(1ULL << BitIdx);

            if ((Size & 0x3F) && DataIdx == DataLength - 1 && BitIdx >= (Size & 0x3F))
                DataIdx = DataLength;
        }

        Iterator operator++(int)
        {
            Iterator prev(*this);
            ++*this;
            return prev;
        }

        Iterator& operator++()
        {
            Next();
            return *this;
        }
    };

    NonStupidBitField(u32 startBit, u32 bitsCount)
    {
        Clear();

        if (bitsCount == 0)
            return;

        SetRange(startBit, bitsCount);
        /*for (int i = 0; i < Size; i++)
        {
            bool state = (*this)[i];
            if (state != (i >= startBit && i < startBit + bitsCount))
            {
                for (u32 j = 0; j < DataLength; j++)
                    printf("data %016lx\n", Data[j]);
                printf("blarg %d %d %d %d\n", i, startBit, bitsCount, Size);
                abort();
            }
        }*/
    }

    NonStupidBitField()
    {
        Clear();
    }

    Iterator End()
    {
        return Iterator{*this, DataLength, 0, 0};
    }
    Iterator Begin()
    {
        for (u32 i = 0; i < DataLength; i++)
        {
            u32 idx = __builtin_ctzll(Data[i]);
            if (Data[i] && idx + i * 64 < Size)
            {
                return {*this, i, idx, Data[i] & ~(1ULL << idx)};
            }
        }
        return End();
    }

    void Clear()
    {
        memset(Data, 0, sizeof(Data));
    }

    Ref operator[](u32 idx)
    {
        return Ref{*this, idx};
    }

    bool operator[](u32 idx) const
    {
        return Data[idx >> 6] & (1ULL << (idx & 0x3F));
    }

    void SetRange(u32 startBit, u32 bitsCount)
    {
        if (bitsCount == 0 || startBit >= Size)
            return;

        bitsCount = std::min(bitsCount, Size - startBit);
        const u32 firstEntry = startBit >> 6;
        const u32 lastEntry = (startBit + bitsCount - 1) >> 6;
        for (u32 i = firstEntry; i <= lastEntry; i++)
            Data[i] |= GetRangedBitMask(i, startBit, bitsCount);
    }

    int Min() const
    {
        for (int i = 0; i < DataLength; i++)
        {
            if (Data[i])
                return i * 64 + __builtin_ctzll(Data[i]);
        }
        return -1;
    }

    int Max() const
    {
        for (int i = DataLength - 1; i >= 0; i--)
        {
            if (Data[i])
                return i * 64 + (63 - __builtin_clzll(Data[i]));
        }
        return -1;
    }

    NonStupidBitField& operator|=(const NonStupidBitField<Size>& other)
    {
        for (u32 i = 0; i < DataLength; i++)
        {
            Data[i] |= other.Data[i];
        }
        return *this;
    }
    NonStupidBitField& operator&=(const NonStupidBitField<Size>& other)
    {
        for (u32 i = 0; i < DataLength; i++)
        {
            Data[i] &= other.Data[i];
        }
        return *this;
    }

    template<u32 OtherSize>
    NonStupidBitField& operator=(const NonStupidBitField<OtherSize>& other)
    {
        memcpy(Data, other.Data, std::min(other.DataLength, DataLength) * sizeof(u64));
        if (Size > OtherSize)
            memset(Data + other.DataLength, 0, (DataLength - other.DataLength) * sizeof(u64));
        return *this;
    }

    operator bool()
    {
        for (int i = 0; i < DataLength - 1; i++)
        {
            if (Data[i])
                return true;
        }
        if (Data[DataLength-1] & ((Size&0x3F) ? ~(0xFFFFFFFFFFFFFFFF << (Size&0x3F)) : 0xFFFFFFFFFFFFFFFF))
        {
            return true;
        }
        return false;
    }
};


#endif
