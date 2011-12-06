/*
 *  stream_types_flac.h
 *
 *    Definition of FLAC specific data structures.
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
 *  Last modified: $Id: stream_types_flac.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__stream_types_flac_h__)
#define __stream_types_flac_h__


#if !defined(_NO_FLAC_SUPPORT)
#include <Vorbis/codec.h>

typedef enum FLACImportStates {
    kFStateInitial,
    kFStateReadingComments,
    kFStateReadingAdditionalMDBlocks,
    kFStateReadingFirstPacket,
    kFStateReadingPackets
} FLACImportStates;

enum {
    kFSRefsInitial = 4096,
    kFSRefsIncrement = 4096
};

typedef struct {
    FLACImportStates state;

    // TODO: add needed variables
    vorbis_comment vc;
    SInt32 metablocks;
    SInt32 skipped;
    UInt32 bps;
} StreamInfo__flac;


#define _HAVE__FLAC_SUPPORT 1
#endif

#endif /* __stream_types_flac_h__ */
