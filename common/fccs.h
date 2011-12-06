/*
 *  fccs.h
 *
 *    Four Character Code identifiers for Xiph formats, used by
 *    XiphQT for internal identification and identification when
 *    exported in QT movie container.
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
 *  Last modified: $Id: fccs.h 11324 2006-04-30 22:24:28Z arek $
 *
 */



#if !defined(__fccs_h__)
#define __fccs_h__


enum {
    kXiphComponentsManufacturer             = 'Xiph',

    kAudioFormatXiphVorbis                  = 'XiVs',
    kAudioFormatXiphOggFramedVorbis         = 'XoVs',

    kAudioFormatXiphSpeex                   = 'XiSp',
    kAudioFormatXiphOggFramedSpeex          = 'XoSp',

    kAudioFormatXiphFLAC                    = 'XiFL',
    kAudioFormatXiphOggFramedFLAC           = 'XoFL',

    kVideoFormatXiphTheora                  = 'XiTh'
};


#endif /* __fccs_h__ */
