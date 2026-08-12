/*
 * Copyright (c) 2011 Stefano Sabatini
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFILTER_FRAMEPOOL_H
#define AVFILTER_FRAMEPOOL_H

#include <stdint.h>

#include "libavutil/buffer.h"
#include "libavutil/frame.h"
#include "libavutil/pixfmt.h"

/**
 * A pool of reusable buffers to use for frame allocations.
 */
typedef struct FFFramePool {
    /**
     * The pools to allocate from.
     * There may be up to 4 pools for each plane of a frame.
     */
    AVBufferPool *pools[4];

    /**
     * Pool parameters.
     */
    int format;
    int width, height;
    int stride_align;
    int linesize[4];
} FFFramePool;

/**
 * Allocates a new frame buffer/pool.
 *
 * This function may free the old pool contained in the frame pool.
 *
 * @param pool pointer to an existing frame pool
 * @param w width of the frame
 * @param h height of the frame
 * @param format format of the frame
 * @param align alignment required for each row of the data
 * @return 0 on success, a negative AVERROR on error
 */
int ff_frame_pool_video_reinit(FFFramePool *pool, int w, int h,
                               enum AVPixelFormat format, int align);

/**
 * Free the frame pool.
 *
 * @param pool frame pool to free
 */
void ff_frame_pool_uninit(FFFramePool *pool);

#endif /* AVFILTER_FRAMEPOOL_H */
