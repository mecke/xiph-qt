/*
 *  stream_types_vorbis.h
 *
 *    Definition of Vorbis specific data structures.
 *
 *
 *  Copyright (c) 2005-2006  Arek Korbik
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
 *  Last modified: $Id: stream_types_vorbis.h 12754 2007-03-14 03:51:23Z arek $
 *
 */


#if !defined(__stream_types_vorbis_h__)
#define __stream_types_vorbis_h__


#if !defined(_NO_VORBIS_SUPPORT)
#include <Vorbis/codec.h>

typedef enum VorbisImportStates {
    kVStateInitial,
    kVStateReadingComments,
    kVStateReadingCodebooks,
    kVStateReadingFirstPacket,
    //kVStateSeekingLastPacket,
    kVStateReadingPackets
} VorbisImportStates;

enum {
    kVSRefsInitial = 2048,
    kVSRefsIncrement = 2048
};

typedef struct {
    VorbisImportStates state;

    vorbis_info vi;
    vorbis_comment vc;
} StreamInfo__vorbis;


#define _HAVE__VORBIS_SUPPORT 1
#endif

#endif /* __stream_types_vorbis_h__ */
