/*
 *  rb.h
 *
 *    Simple ring buffer implementation - header file.
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
 *  Last modified: $Id: rb.h 10575 2005-12-10 19:33:16Z arek $
 *
 */


#if !defined(__rb_h__)
#define __rb_h__


#include "config.h"

typedef struct {
    unsigned char   *buffer;
    unsigned char   *b_start;
    unsigned char   *b_end;
    unsigned char   *b_reserved;

    long             b_size;
    long             b_real_size;
} ring_buffer;


extern OSErr rb_init(ring_buffer *rb, long size);

extern void  rb_free(ring_buffer *rb);

extern void  rb_reset(ring_buffer *rb);

extern long  rb_data_available(ring_buffer *rb);

extern long  rb_space_available(ring_buffer *rb);

extern int   rb_in(ring_buffer *rb, unsigned char *src, long size);

extern void  rb_zap(ring_buffer *rb, long size);

extern void* rb_data(ring_buffer *rb);

extern void* rb_reserve(ring_buffer *rb, long size);

extern void  rb_sync_reserved(ring_buffer *rb);


#endif /* __rb_h__ */
