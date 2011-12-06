/*
 *  CAVorbisDecoderPublic.r
 *
 *    Information bit definitions for the 'thng' resource.
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
 *  Last modified: $Id: CAVorbisDecoderPublic.r 10574 2005-12-10 16:22:13Z arek $
 *
 */


#include "vorbis_versions.h"
#include "fccs.h"



#define RES_ID			-17110
#define COMP_TYPE		'adec'
#define COMP_SUBTYPE	kAudioFormatXiphOggFramedVorbis
#define COMP_MANUF		'Xiph'
#define VERSION			kCAVorbis_adec_Version
#define NAME			"Xiph Vorbis (Ogg-framed) Decoder"
#define DESCRIPTION		"An AudioCodec that decodes Xiph Vorbis (Ogg-framed) into linear PCM data"
#define ENTRY_POINT		"CAOggVorbisDecoderEntry"

#define kPrimaryResourceID               -17110
#define kComponentType                   'adec'
#define kComponentSubtype                kAudioFormatXiphOggFramedVorbis
#define kComponentManufacturer           'Xiph'
#define	kComponentFlags                  0
#define kComponentVersion                kCAVorbis_adec_Version
#define kComponentName                   "Xiph (Ogg-framed) Vorbis"
#define kComponentInfo                   "An AudioCodec that decodes Xiph (Ogg-framed) Vorbis into linear PCM data"
#define kComponentEntryPoint             "CAOggVorbisDecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

//#include "ACComponentResources.r"
//#include "AUResources.r"
#include "XCAResources.r"


#define RES_ID			-17114
#define COMP_TYPE		'adec'
#define COMP_SUBTYPE	kAudioFormatXiphVorbis
#define COMP_MANUF		'Xiph'
#define VERSION			kCAVorbis_adec_Version
#define NAME			"Xiph Vorbis Decoder"
#define DESCRIPTION		"An AudioCodec that decodes Xiph Vorbis into linear PCM data"
#define ENTRY_POINT		"CAVorbisDecoderEntry"

#define kPrimaryResourceID               -17114
#define kComponentType                   'adec'
#define kComponentSubtype                kAudioFormatXiphVorbis
#define kComponentManufacturer           'Xiph'
#define	kComponentFlags                  0
#define kComponentVersion                kCAVorbis_adec_Version
#define kComponentName                   "Xiph Vorbis"
#define kComponentInfo                   "An AudioCodec that decodes Xiph Vorbis into linear PCM data"
#define kComponentEntryPoint             "CAVorbisDecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

//#include "ACComponentResources.r"
//#include "AUResources.r"
#include "XCAResources.r"
