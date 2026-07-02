#ifndef CMDMEMRING_H
#define CMDMEMRING_H

#include <deko3d.hpp>

#include "GpuMemHeap.h"

// adapted from https://github.com/switchbrew/switch-examples/blob/master/graphics/deko3d/deko_examples/source/SampleFramework/CCmdMemRing.h

template <u32 NumSlices>
class CmdMemRing
{
    static_assert(NumSlices > 0, "Need a non-zero number of slices...");
    GpuMemHeap::Allocation Mem;
    GpuMemHeap& Heap;
    u32 CurSlice = 0;
    dk::Fence Fences[NumSlices] = {};

public:
    CmdMemRing(GpuMemHeap& heap, u32 sliceSize)
        : Heap(heap)
    {
        sliceSize = (sliceSize + DK_CMDMEM_ALIGNMENT - 1) &~ (DK_CMDMEM_ALIGNMENT - 1);
        Mem = heap.Alloc(NumSlices*sliceSize, DK_CMDMEM_ALIGNMENT);
    }

    ~CmdMemRing()
    {
        Heap.Free(Mem);
    }

    u32 Begin(dk::CmdBuf cmdbuf)
    {
        // Clear/reset the command buffer, which also destroys all command list handles
        // (but remember: it does *not* in fact destroy the command data)
        cmdbuf.clear();

        // Wait for the current slice of memory to be available, and feed it to the command buffer
        u32 sliceSize = Mem.Size / NumSlices;
        Fences[CurSlice].wait();

        // Feed the memory to the command buffer
        cmdbuf.addMemory(Heap.MemBlock, Mem.Offset + CurSlice * sliceSize, sliceSize);

        return CurSlice;
    }

    DkCmdList End(dk::CmdBuf cmdbuf)
    {
        // Signal the fence corresponding to the current slice; so that in the future when we want
        // to use it again, we can wait for the completion of the commands we've just submitted
        // (and as such we don't overwrite in-flight command data with new one)
        cmdbuf.signalFence(Fences[CurSlice]);

        // Advance the current slice counter; wrapping around when we reach the end
        CurSlice = (CurSlice + 1) % NumSlices;

        // Finish off the command list, returning it to the caller
        return cmdbuf.finishList();
    }
};

#endif