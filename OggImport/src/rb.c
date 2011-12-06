/*
 *  rb.c
 *
 *    Simple, boring ring buffer implementation.
 *
 *
 *  Copyright (c) 2005  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: rb.c 10575 2005-12-10 19:33:16Z arek $
 *
 */


#include "config.h"
#include "rb.h"


OSErr rb_init(ring_buffer *rb, long size) {
    OSErr ret = noErr;

    if (!rb->buffer) {
        rb->buffer = (unsigned char *) NewPtr(2 * size);
        ret = MemError();

        if (ret == noErr) {
            rb->b_start = rb->buffer;
            rb->b_end = rb->buffer;
            rb->b_reserved = rb->buffer;
            rb->b_size = size;
            rb->b_real_size = 2 * size;
        }
    }

    return ret;
}

void rb_free(ring_buffer *rb) {
    DisposePtr((Ptr) rb->buffer);
    rb->buffer = NULL;

    rb->b_size = 0;
    rb->b_real_size = 0;
    rb->b_start = NULL;
    rb->b_end = NULL;
    rb->b_reserved = NULL;
}

void rb_reset(ring_buffer *rb) {
    rb->b_start = rb->buffer;
    rb->b_end = rb->buffer;
    rb->b_reserved = rb->buffer;
}

long rb_data_available(ring_buffer *rb) {
    long ret = 0;

    if (rb->b_start < rb->b_end)
        ret = (long) (rb->b_end - rb->b_start);
    else if (rb->b_end < rb->b_start)
        ret = (long) (rb->b_end) + rb->b_size - (long) (rb->b_start);

    return ret;
}

long rb_space_available(ring_buffer *rb) {
    long ret = rb->b_size;

    if (rb->b_end < rb->b_start)
        ret = (long) (rb->b_start - rb->b_end);
    else if (rb->b_start < rb->b_end)
        ret = (long) (rb->b_start) + rb->b_size - (long) (rb->b_end);

    return ret;
}

int rb_in(ring_buffer *rb, unsigned char *src, long size) {
    if (size > rb->b_size - rb_data_available(rb))
        return 0;

    if ((long) rb->b_end - (long) rb->buffer + size <= rb->b_size) {
        BlockMoveData(src, rb->b_end, size);
        rb->b_end += size;
    } else {
        long wrapped_size = rb->b_size - ((long) rb->b_end - (long) rb->buffer);
        BlockMoveData(src, rb->b_end, wrapped_size);

        BlockMoveData(src + wrapped_size, rb->buffer, size - wrapped_size);
        rb->b_end = rb->buffer + (size - wrapped_size);
    }

    rb->b_reserved = rb->b_end;
    return 1;
}

void rb_zap(ring_buffer *rb, long size) {
    if (size >= rb_data_available(rb))
        //rb_reset(rb);
        rb->b_start = rb->b_end;
    else {
        if (rb->b_start < rb->b_end || rb->b_start + size < rb->buffer + rb->b_size)
            rb->b_start = rb->b_start + size;
        else
            rb->b_start = rb->b_start + size - rb->b_size;
    }
}

void* rb_data(ring_buffer *rb) {
    long available = rb_data_available(rb);

    if (available == 0)
        return rb->buffer;
    else {
        if (rb->b_start > rb->b_end) {
            // OK, here we're taking advange of the facts that the real buffer size
            // is twice as long as the official size; so we can copy wrapped tail of
            // the buffer at the end of the (official size) buffer, and return pointer
            // to a continuous block of data... is that OK?
            BlockMoveData(rb->buffer, rb->buffer + rb->b_size, (long) (rb->b_end - rb->buffer));
        }
        return rb->b_start;
    }

}

void* rb_reserve(ring_buffer *rb, long size) {
    long available = rb_space_available(rb);

    if (size == 0) {
        rb->b_reserved = rb->b_end;
        return NULL;
    }

    if (available == 0 || size > available)
        return NULL;

    if (rb->b_end <= rb->b_start) {
        // size is smaller then available space, so reserved end should be still before the start
        rb->b_reserved = rb->b_end + size;
        return rb->b_end;
    } else if (rb->b_end + size <= rb->buffer + rb->b_size) {
        rb->b_reserved = rb->b_end + size;
        return rb->b_end;
    } else {
        rb->b_reserved = rb->b_end + size;
        return rb->b_end; // :D
    }
}

void rb_sync_reserved(ring_buffer *rb) {
    if (rb->b_reserved > rb->buffer + rb->b_size) {
        BlockMoveData(rb->buffer + rb->b_size, rb->buffer, rb->b_reserved - rb->buffer - rb->b_size);
        rb->b_reserved = rb->b_reserved - rb->b_size;
    }

    rb->b_end = rb->b_reserved;
}
