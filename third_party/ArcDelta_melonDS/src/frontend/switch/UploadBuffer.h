#ifndef UPLOADBUFFER_H
#define UPLOADBUFFER_H

#include <deko3d.hpp>

#include "GpuMemHeap.h"

class UploadBuffer
{
public:
    UploadBuffer();

    DkGpuAddr UploadData(dk::CmdBuf cmdbuf, u32 size, u8* data);
    void UploadAndCopyData(dk::CmdBuf cmdbuf, DkGpuAddr dst, u8* data, u32 size);
    void UploadAndCopyTexture(dk::CmdBuf cmdbuf, dk::Image& image, u8* data, u32 x, u32 y, u32 width, u32 height, u32 pitch, u32 layer = 0);

    u32 LastFlushBuffer = 0;
private:
    static const u32 SegmentSize = 16*1024*1024;
    GpuMemHeap::Allocation Buffers[3];
    dk::Fence Fences[3] = {};
    u32 CurBuffer = 0, CurOffset = 0;
};

#endif