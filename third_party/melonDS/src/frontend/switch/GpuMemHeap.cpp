#include "GpuMemHeap.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

void GpuMemHeap::BlockListPushFront(Block*& head, Block* block)
{
    if (head)
    {
        assert(head->Prev == nullptr);
        head->Prev = block;
    }

    block->Prev = nullptr;
    block->Next = head;
    head = block;
}

GpuMemHeap::Block* GpuMemHeap::BlockListPopFront(Block*& head)
{
    Block* result = head;
    assert(result && "popping from empty block list");

    head = result->Next;
    if (head)
        head->Prev = nullptr;

    return result;
}

void GpuMemHeap::BlockListRemove(Block*& head, Block* block)
{
    assert((head == block) == !block->Prev);
    if (!block->Prev)
        head = block->Next;

    if (block->Prev)
        block->Prev->Next = block->Next;
    if (block->Next)
        block->Next->Prev = block->Prev;
}

void GpuMemHeap::MapSizeToSecondLevel(u32 size, u32& fl, u32& sl)
{
    assert(size >= 32);
    fl = 31 - __builtin_clz(size);
    sl = (size - (1 << fl)) >> (fl - 5);
}

void GpuMemHeap::MarkFree(Block* block)
{
    assert(!block->Free);
    block->Free = true;
    u32 fl, sl;
    MapSizeToSecondLevel(block->Size, fl, sl);

    BlockListPushFront(SecondFreeList[(fl - 5) * 32 + sl], block);

    FirstFreeList |= 1 << (fl - 5);
    SecondFreeListBits[fl - 5] |= 1 << sl;
}

void GpuMemHeap::UnmarkFree(Block* block)
{
    assert(block->Free);
    block->Free = false;
    u32 fl, sl;
    MapSizeToSecondLevel(block->Size, fl, sl);

    BlockListRemove(SecondFreeList[(fl - 5) * 32 + sl], block);

    if (!SecondFreeList[(fl - 5) * 32 + sl])
    {
        SecondFreeListBits[fl - 5] &= ~(1 << sl);

        if (SecondFreeListBits[fl - 5] == 0)
            FirstFreeList &= ~(1 << (fl - 5));
    }
}

// makes a new block to the right and returns it
GpuMemHeap::Block* GpuMemHeap::SplitBlockRight(Block* block, u32 offset)
{
    assert(!block->Free);
    assert(offset < block->Size);
    Block* newBlock = BlockListPopFront(BlockPoolUnused);

    newBlock->Offset = block->Offset + offset;
    newBlock->Size = block->Size - offset;
    newBlock->SiblingLeft = block;
    newBlock->SiblingRight = block->SiblingRight;
    newBlock->Free = false;
    if (newBlock->SiblingRight)
    {
        assert(newBlock->SiblingRight->SiblingLeft == block);
        newBlock->SiblingRight->SiblingLeft = newBlock;
    }

    block->Size -= newBlock->Size;
    block->SiblingRight = newBlock;

    return newBlock;
}

GpuMemHeap::Block* GpuMemHeap::MergeBlocksLeft(Block* block, Block* other)
{
    assert(block->SiblingRight == other);
    assert(other->SiblingLeft == block);
    assert(!block->Free);
    assert(!other->Free);
    assert(block->Offset + block->Size == other->Offset);
    block->Size += other->Size;
    block->SiblingRight = other->SiblingRight;
    if (block->SiblingRight)
    {
        assert(block->SiblingRight->SiblingLeft == other);
        block->SiblingRight->SiblingLeft = block;
    }

    BlockListPushFront(BlockPoolUnused, other);

    return block;
}

GpuMemHeap::GpuMemHeap(dk::Device device, u32 size, u32 flags, u32 blockPoolSize)
{
    assert((size & (DK_MEMBLOCK_ALIGNMENT - 1)) == 0 && "block size not properly aligned");
    u32 sizeLog2 = 31 - __builtin_clz(size);
    if (((u32)1 << sizeLog2) > size)
        sizeLog2++; // round up to the next power of two
    assert(sizeLog2 >= 5);

    u32 rows = sizeLog2 - 4; // remove rows below 32 bytes and round up to next

    SecondFreeListBits = new u32[rows];
    memset(SecondFreeListBits, 0, rows*4);
    SecondFreeList = new Block*[rows * 32];
    memset(SecondFreeList, 0, rows*32*8);

    BlockPool = new Block[blockPoolSize];
    memset(BlockPool, 0, sizeof(Block)*blockPoolSize);
    for (u32 i = 0; i < blockPoolSize; i++)
        BlockListPushFront(BlockPoolUnused, &BlockPool[i]);

    // insert heap into the free list
    Block* heap = BlockListPopFront(BlockPoolUnused);
    heap->Offset = 0;
    heap->Size = size - (flags & DkMemBlockFlags_Code ? DK_SHADER_CODE_UNUSABLE_SIZE : 0);
    heap->SiblingLeft = nullptr;
    heap->SiblingRight = nullptr;
    heap->Free = false;
    MarkFree(heap);

    Used = 0;

    MemBlock = dk::MemBlockMaker{device, size}
        .setFlags(flags).create();
}

GpuMemHeap::~GpuMemHeap()
{
    MemBlock.destroy();

    delete[] BlockPool;
    delete[] SecondFreeList;
    delete[] SecondFreeListBits;
}

GpuMemHeap::Allocation GpuMemHeap::Alloc(u32 size, u32 align)
{
    assert(size > 0);
    assert((align & (align - 1)) == 0 && "alignment must be a power of two");
    // minimum alignment (and thus size) is 32 bytes
    align = std::max((u32)32, align);
    size = (size + align - 1) & ~(align - 1);

    //printf("allocating %f MB on heap %p (used %f%%)\n", (float)size/(1024.f*1024.f), MemBlock.getCpuAddr(), (float)Used/MemBlock.getSize());

    u32 fl, sl;
    MapSizeToSecondLevel(size + (align > 32 ? align : 0), fl, sl);

    u32 secondFreeListBits = SecondFreeListBits[fl - 5] & (0xFFFFFFFF << (sl + 1));

    if (sl == 31 || secondFreeListBits == 0)
    {
        u32 firstFreeListBits = FirstFreeList & (0xFFFFFFFF << (fl - 5 + 1));
        assert(firstFreeListBits && "out of memory :(");

        fl = __builtin_ctz(firstFreeListBits) + 5;
        secondFreeListBits = SecondFreeListBits[fl - 5];
        assert(secondFreeListBits);
        sl = __builtin_ctz(secondFreeListBits);
    }
    assert(secondFreeListBits && "out of memory :(");

    sl = __builtin_ctz(secondFreeListBits);

    Block* block = SecondFreeList[(fl - 5) * 32 + sl];
    UnmarkFree(block);

    // align within the block
    if ((block->Offset & (align - 1)) > 0)
    {
        assert(align > 32);
        Block* newBlock = SplitBlockRight(block, ((block->Offset + align - 1) & ~(align - 1)) - block->Offset);
        MarkFree(block);
        block = newBlock;
    }
    // put remaining data back
    if (block->Size > size)
    {
        MarkFree(SplitBlockRight(block, size));
    }

    assert((block->Offset & (align - 1)) == 0);
    assert(block->Size == size);
    return {(u32)(block - BlockPool), block->Offset, block->Size};
}

void GpuMemHeap::Free(Allocation allocation)
{
    Block* block = &BlockPool[allocation.BlockIdx];
    assert(!block->Free);

    if (block->SiblingLeft && block->SiblingLeft->Free)
    {
        UnmarkFree(block->SiblingLeft);
        block = MergeBlocksLeft(block->SiblingLeft, block);
    }
    if (block->SiblingRight && block->SiblingRight->Free)
    {
        UnmarkFree(block->SiblingRight);
        block = MergeBlocksLeft(block, block->SiblingRight);
    }

    MarkFree(block);
}
