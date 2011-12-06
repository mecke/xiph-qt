/*
 *  data_types.h
 *
 *    Definitions of common data structures shared between different
 *    components.
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
 *  Last modified: $Id: data_types.h 11324 2006-04-30 22:24:28Z arek $
 *
 */


#if !defined(__data_types_h__)
#define __data_types_h__


enum {
    kCookieTypeOggSerialNo = 'oCtN'
};

// format specific cookie types

enum {
    kCookieTypeVorbisHeader = 'vCtH',
    kCookieTypeVorbisComments = 'vCt#',
    kCookieTypeVorbisCodebooks = 'vCtC',
    kCookieTypeVorbisFirstPageNo = 'vCtN'
};

enum {
    kCookieTypeSpeexHeader = 'sCtH',
    kCookieTypeSpeexComments = 'sCt#',
    kCookieTypeSpeexExtraHeader	= 'sCtX'
};

enum {
    kCookieTypeTheoraHeader = 'tCtH',
    kCookieTypeTheoraComments = 'tCt#',
    kCookieTypeTheoraCodebooks = 'tCtC'
};

enum {
    kCookieTypeFLACStreaminfo = 'fCtS',
    kCookieTypeFLACMetadata = 'fCtM'
};


enum {
    kSampleDescriptionExtensionTheora = 'XdxT',
};


struct OggSerialNoAtom {
    long size;			// = sizeof(OggSerialNoAtom)
    long type;			// = kOggCookieSerialNoType
    long serialno;
};
typedef struct OggSerialNoAtom OggSerialNoAtom;

struct CookieAtomHeader {
    long           size;
    long           type;
    unsigned char  data[1];
};
typedef struct CookieAtomHeader CookieAtomHeader;


#endif /* __data_types_h__ */
