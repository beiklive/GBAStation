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

#include <stdint.h>

#include "libavutil/avassert.h"
#include "libavutil/buffer.h"
#include "libavutil/common.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/mem.h"
#include "libavutil/pixdesc.h"

#include "framepool.h"

void ff_frame_pool_uninit(FFFramePool *pool)
{
    int i;
    for (i = 0; i < FF_ARRAY_ELEMS(pool->pools); i++)
        av_buffer_pool_uninit(&pool->pools[i]);

    pool->format = -1;
    pool->width  = 0;
    pool->height = 0;
}

static AVBufferRef *pool_alloc_buffer(void *opaque, size_t size)
{
    AVBufferRef *buf;
    uint8_t *data;

    data = av_malloc(size);
    if (!data)
        return NULL;

    buf = av_buffer_create(data, size, av_buffer_default_free, NULL, 0);
    if (!buf)
        av_freep(&data);

    return buf;
}

int ff_frame_pool_video_reinit(FFFramePool *pool, int w, int h,
                               enum AVPixelFormat format, int align)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(format);
    int i, ret;

    if (!desc || w <= 0 || h <= 0 || align <= 0)
        return AVERROR(EINVAL);

    if (pool->format == format && pool->width == w && pool->height == h &&
        pool->stride_align == align)
        return 0;

    ff_frame_pool_uninit(pool);

    pool->format       = format;
    pool->width        = w;
    pool->height       = h;
    pool->stride_align = align;

    ret = av_image_fill_linesizes(pool->linesize, format, w);
    if (ret < 0)
        goto fail;

    for (i = 0; i < FF_ARRAY_ELEMS(pool->pools); i++) {
        int h2, size;

        if (!pool->linesize[i])
            continue;

        pool->linesize[i] = FFALIGN(pool->linesize[i], align);
        h2 = h;
        if (i == 1 || i == 2)
            h2 = AV_CEIL_RSHIFT(h, desc->log2_chroma_h);

        size = pool->linesize[i] * h2;
        pool->pools[i] = av_buffer_pool_init2(size, pool, pool_alloc_buffer, NULL);
        if (!pool->pools[i]) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
    }

    return 0;
fail:
    ff_frame_pool_uninit(pool);
    return ret;
}
