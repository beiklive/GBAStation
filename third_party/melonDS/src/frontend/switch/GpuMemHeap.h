#ifndef GPUMEMHEAP_H
#define GPUMEMHEAP_H

#include "types.h"

#include <deko3d.hpp>

class GpuMemHeap
{
    struct Block
    {
        bool Free;
        u32 Offset, Size;
        // it would probably be smarter to make those indices because that's smaller
        Block* SiblingLeft, *SiblingRight;
        Block* Next, *Prev;
    };

    // this is a home made memory allocator based on TLSF (http://www.gii.upv.es/tlsf/)
    // I hope it doesn't have to many bugs

    // Free List
    u32 FirstFreeList = 0;
    u32* SecondFreeListBits;
    Block** SecondFreeList;

    Block* BlockPool;
    Block* BlockPoolUnused = nullptr;

    u32 Used;

    void BlockListPushFront(Block*& head, Block* block);
    Block* BlockListPopFront(Block*& head);
    void BlockListRemove(Block*& head, Block* block);
    void MapSizeToSecondLevel(u32 size, u32& fl, u32& sl);
    void MarkFree(Block* block);
    void UnmarkFree(Block* block);
    Block* SplitBlockRight(Block* block, u32 offset);
    Block* MergeBlocksLeft(Block* block, Block* other);
public:
    dk::MemBlock MemBlock;

    struct Allocation
    {
        u32 BlockIdx;
        u32 Offset, Size;
    };

    GpuMemHeap(dk::Device device, u32 size, u32 flags, u32 blockPoolSize);
    ~GpuMemHeap();

    Allocation Alloc(u32 size, u32 align);
    void Free(Allocation allocation);

    DkGpuAddr GpuAddr(Allocation allocation)
    {
        return MemBlock.getGpuAddr() + allocation.Offset;
    }

    template <typename T>
    T* CpuAddr(Allocation allocation)
    {
        return (T*)((u8*)MemBlock.getCpuAddr() + allocation.Offset);
    }
};

#endif