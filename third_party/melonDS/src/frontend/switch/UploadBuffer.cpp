#include "UploadBuffer.h"
#include "Gfx.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

UploadBuffer::UploadBuffer()
{
    for (u32 i = 0; i < 3; i++)
        Buffers[i] = Gfx::DataHeap->Alloc(SegmentSize, 64);
}

DkGpuAddr UploadBuffer::UploadData(dk::CmdBuf cmdbuf, u32 size, u8* data)
{
    if (CurOffset + size > SegmentSize)
    {
        cmdbuf.signalFence(Fences[CurBuffer]);
        CurOffset = 0;
        CurBuffer++;
        if (CurBuffer == 3) CurBuffer = 0;
        LastFlushBuffer++;
        assert(LastFlushBuffer <= 3);
        Fences[CurBuffer].wait();
    }

    u8* srcCpu = Gfx::DataHeap->CpuAddr<u8>(Buffers[CurBuffer]) + CurOffset;
    DkGpuAddr srcGpu = Gfx::DataHeap->GpuAddr(Buffers[CurBuffer]) + CurOffset;
    memcpy(srcCpu, data, size);

    CurOffset += size + 63;
    CurOffset &= ~63;
    return srcGpu;
}

void UploadBuffer::UploadAndCopyData(dk::CmdBuf cmdbuf, DkGpuAddr dst, u8* data, u32 size)
{
    cmdbuf.copyBuffer(UploadData(cmdbuf, size, data), dst, size);
}

void UploadBuffer::UploadAndCopyTexture(dk::CmdBuf cmdbuf, dk::Image& image,
    u8* data,
    u32 x, u32 y, u32 width, u32 height, u32 pitch, u32 layer)
{
    DkCopyBuf src {UploadData(cmdbuf, pitch*height, data)};
    dk::ImageView dst{image};
    cmdbuf.copyBufferToImage(src, dst, {0, 0, layer, width, height, 1});
}