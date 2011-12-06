/*
 *  encoder_types.h
 *
 *    Definitions of TheoraEncoder data structures.
 *
 *
 *  Copyright (c) 2006  Arek Korbik
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
 *  Last modified: $Id: encoder_types.h 12346 2007-01-18 13:45:42Z arek $
 *
 */


#if !defined(__encoder_types_h__)
#define __encoder_types_h__

#if defined(__APPLE_CC__)
#include <QuickTime/QuickTime.h>
#else
#include <QuickTimeComponents.h>

#if defined(TARGET_OS_WIN32)
#define _WINIOCTL_
#include <windows.h>
#endif
#endif /* __APPLE_CC__ */

#if defined(__APPLE_CC__)
#include <Theora/theora.h>
#else
#include <theora.h>
#endif /* __APPLE_CC__ */


// Constants
//const UInt8 kNumPixelFormatsSupported = 2;
//const UInt32 kPacketBufferAllocIncrement = 64 * 1024;

// Data structures
typedef struct	{
    ComponentInstance       self;
    ComponentInstance       target;

    ICMCompressorSessionRef session;
    ICMCompressionSessionOptionsRef sessionOptions;

    theora_info ti;
    theora_comment tc;
    theora_state ts;

    Boolean info_initialised;
    long last_frame;

    /* packet buffer information */
    UInt8* p_buffer;
    UInt32 p_buffer_len;
    UInt32 p_buffer_used;

    long width;
    long height;
    size_t maxEncodedDataSize;
    int nextDecodeNumber;

    //struct InternalPixelBuffer currentFrame;
    yuv_buffer yuv;
    UInt8     *yuv_buffer;
    UInt32     yuv_buffer_size;

    /* ========================= */
    int keyFrameCountDown;

    /* ======= settings ======== */
    CodecQ set_quality;
    UInt32 set_fps_numer;
    UInt32 set_fps_denom;
    UInt32 set_bitrate;
    UInt32 set_keyrate;

    UInt16 set_sharp;
    UInt16 set_quick;
} TheoraGlobals, *TheoraGlobalsPtr;

#endif /* __decoder_types_h__ */
