/*
 *  decoder_types.h
 *
 *    Definitions of TheoraDecoder data structures.
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
 *  Last modified: $Id: decoder_types.h 12346 2007-01-18 13:45:42Z arek $
 *
 */


#if !defined(__decoder_types_h__)
#define __decoder_types_h__

#if defined(__APPLE_CC__)
#include <QuickTime/QuickTime.h>
#else
#include <QuickTimeComponents.h>

#if defined(TARGET_OS_WIN32)
#define _WINIOCTL_
#include <windows.h>
#endif

#if defined(__DO_WE_NEED_ALL_THOSE_P__)
#include <MacTypes.h>
#include <MacErrors.h>
#include <Endian.h>
#include <MacMemory.h>
#include <Resources.h>
#include <Components.h>
#include <Sound.h>
#include <QuickTimeComponents.h>
#include <FixMath.h>
#include <Math64.h>
#include <IntlResources.h>
#include <MoviesFormat.h>
#include <Gestalt.h>
#include <TextEncodingConverter.h>
#endif /* __DO_WE_NEED_ALL_THOSE_P__ */
#endif /* __APPLE_CC__ */

#if defined(__APPLE_CC__) && defined(XIPHQT_USE_FRAMEWORKS)
#include <TheoraExp/theoradec.h>
#else
#include <theora/theoradec.h>
#endif /* __APPLE_CC__ */


// Constants
const UInt8 kNumPixelFormatsSupported = 2;
const UInt32 kPacketBufferAllocIncrement = 64 * 1024;

// Data structures
typedef struct	{
    ComponentInstance       self;
    ComponentInstance       delegateComponent;
    ComponentInstance       target;
    OSType**                wantedDestinationPixelTypeH;
    ImageCodecMPDrawBandUPP drawBandUPP;

    th_info ti;
    //theora_comment tc;
    th_setup_info *ts;
    th_dec_ctx *td;
    Boolean info_initialised;
    long last_frame;

    /* packet buffer information */
    UInt8* p_buffer;
    UInt32 p_buffer_len;
    UInt32 p_buffer_used;
} Theora_GlobalsRecord, *Theora_Globals;

typedef struct {
    long width;
    long height;
    long depth;
    long dataSize;
    long frameNumber;
    long draw;
    long decoded;
    OSType pixelFormat;
} Theora_DecompressRecord;

#endif /* __decoder_types_h__ */
